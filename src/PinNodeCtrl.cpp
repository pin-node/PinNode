/**************************************************************************
 * PinNodeCtrl - see PinNodeCtrl.h
 **************************************************************************/
// PINNODE_HOST_BUILD: compile natively for tests, against host shims of Arduino.h, EEPROM.h and
// avr/wdt.h and a fake RPULink (see tests/native). RPULink.cpp and the transports stay out.
#if defined(ARDUINO_ARCH_AVR) || defined(PINNODE_HOST_BUILD)

#include <Arduino.h>
#include <avr/wdt.h>
#include <EEPROM.h>
#include "RPULink.h"
#include "PinNodeCtrl.h"

// the game counter sits at the top of a 4 KB EEPROM, clear of the RPU OS and game settings (< 200)
#if defined(E2END) && (E2END < PINNODE_EEPROM_GAME_COUNTER + 1)
#error "PinNode needs an AVR with 4 KB of EEPROM (ATmega2560, the Arduino Mega)"
#endif

// The game's EEPROM layout, as the RPU OS stores it: unsigned longs little-endian, erased = 0xFF.
static unsigned long EepromReadUL(unsigned short a) {
  unsigned long v = 0;
  for (byte i = 0; i < 4; i++) v |= (unsigned long)EEPROM.read(a + i) << (8 * i);
  return v == 0xFFFFFFFFUL ? 0 : v;
}
static void EepromWriteUL(unsigned short a, unsigned long v) {
  for (byte i = 0; i < 4; i++) EEPROM.update(a + i, (byte)(v >> (8 * i)));
}

void PinNodeCtrl_OnPinNodeRecord(byte type, const byte *data, byte len) __attribute__((weak));

static const char *helloVersion = "";
static const char *helloRuleset = "";
static byte helloManifest[8];
static byte helloMaxPlayers = 4, helloSettings = 0;
static uint16_t helloHwRev = 0;
static byte helloFlags = 0;                // HELLO flags (PinNodeCtrl_SetHello)
// records from the node, offered to the program first (true = it took the record)
static boolean (*recordHook)(byte type, const byte *data, byte len) = NULL;
static void (*otaHandler)(byte phase) = NULL;
static uint32_t buildId = 0;
static boolean linkWasUp = false;
static volatile boolean otaRequested = false;
static unsigned long unixTimeAtSync = 0;
static unsigned long millisAtSync = 0;


static void SendSettingsReport();
static void SendActionsReport();
static boolean firstHello = true;
static boolean auditPending = false;       // an AUDIT_SNAPSHOT is due (sent from Update)

static void SendHello() {
  // HELLO: proto u8 = 2, buildId u32, manifestHash[8], version (len-prefixed, <= 20),
  // maxPlayers u8, settingsCount u8, flags u8 (0: RPU code), hwRev u16 (the board rev in our
  // firmware stamp, so the node only flashes images this board's bootloader will start). HELLO_EXT: rulesetId (len-prefixed, <= 48), always
  // queued straight after, so the node gets both in order (SPEC-node-link §3.1).
  byte buf[1 + 48];
  byte n = 0;
  buf[n++] = 2;
  for (byte i = 0; i < 4; i++) buf[n++] = (byte)(buildId >> (8 * i));
  for (byte i = 0; i < 8; i++) buf[n++] = helloManifest[i];
  byte vl = strlen(helloVersion);
  if (vl > 20) vl = 20;
  buf[n++] = vl;
  memcpy(buf + n, helloVersion, vl); n += vl;
  buf[n++] = helloMaxPlayers;
  buf[n++] = helloSettings;
  // flags: bit1 = first HELLO since this Mega started (a real reset); a re-HELLO after the link
  // recovers from a stall (the game's own delay() calls) leaves it clear
  buf[n++] = (firstHello ? 0x02 : 0) | helloFlags;
  firstHello = false;
  buf[n++] = (byte)(helloHwRev & 0xFF);
  buf[n++] = (byte)(helloHwRev >> 8);
  RPULink_Queue(RPULINK_HELLO, buf, n);

  byte rl = strlen(helloRuleset);
  if (rl > 48) rl = 48;
  buf[0] = rl;
  memcpy(buf + 1, helloRuleset, rl);
  RPULink_Queue(RPULINK_HELLO_EXT, buf, 1 + rl);
  SendSettingsReport();                      // the node's settings view starts from this
  SendActionsReport();                       // and its action flags (local, confirm, lock…)
  auditPending = true;                       // and its audits (sent from Update, after this burst)
}

// CRC-32 (IEEE) of the program image, flash 0 .. __data_load_end, as the service computes it
// from the release .hex (SPEC-node-link §4). Nibble table: small and fast enough at boot.
extern char __data_load_end;
static const uint32_t crcNibble[16] PROGMEM = {
  0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
  0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C};

uint32_t PinNodeCtrl_BuildId() {
  uint32_t end = pgm_get_far_address(__data_load_end);
  uint32_t crc = 0xFFFFFFFFUL;
  for (uint32_t a = 0; a < end; a++) {
    crc ^= pgm_read_byte_far(a);
    crc = (crc >> 4) ^ pgm_read_dword(&crcNibble[crc & 0x0F]);
    crc = (crc >> 4) ^ pgm_read_dword(&crcNibble[crc & 0x0F]);
  }
  return ~crc;
}

// start from the app (START_REQUEST 0xAA): the game takes it like a Start press, or Update()
// answers "not allowed now" if nothing took it within 500 ms (menus, tilt, ball search...)
static uint16_t startReq = 0;
static boolean startPending = false;
static unsigned long startAtMs;

// ------------------------------------------------------------------ app settings
static const PinNodeSetting *setTable = NULL;
static byte setCount = 0;
static void (*setApplied)(void) = NULL;
static boolean gameOpen = false;           // between GameStart and GameEnd

static long SettingRead(byte idx) {
  const PinNodeSetting &st = setTable[idx];
  long v = st.size == 4 ? (long)EepromReadUL(st.eeprom) : (long)EEPROM.read(st.eeprom);
  // RPU games read flag bytes as (byte & 1), so unprogrammed 0xFF means on: report what the game uses
  if (st.size == 1 && st.minValue == 0 && st.maxValue == 1) v &= 1;
  return v * (long)st.scale;
}

static void put32s(byte *p, long v) { for (byte i = 0; i < 4; i++) p[i] = (byte)((unsigned long)v >> (8 * i)); }

static void SendSettingsReport() {
  for (byte start = 0; start < setCount; start += 10) {
    byte n = setCount - start > 10 ? 10 : setCount - start;
    byte d[2 + 10 * 5];
    d[0] = start;
    d[1] = n;
    for (byte i = 0; i < n; i++) {
      d[2 + 5 * i] = start + i;
      put32s(d + 3 + 5 * i, SettingRead(start + i));
    }
    RPULink_Queue(0x2B, d, 2 + 5 * n);
  }
}

void PinNodeCtrl_Settings(const PinNodeSetting *table, byte count, void (*applied)(void)) {
  setTable = table;
  setCount = count;
  setApplied = applied;
}

void PinNodeCtrl_SettingsChanged() { SendSettingsReport(); }

// ------------------------------------------------------------------ audits

static const PinNodeAudit *audTable = NULL;
static byte audCount = 0;
static unsigned long lastAuditMs = 0;
static unsigned long auditAtMs = 0;        // 0 = none: a snapshot due at this time
#define PINNODE_AUDIT_PERIOD_MS  (24UL * 60UL * 60UL * 1000UL)

void PinNodeCtrl_Audits(const PinNodeAudit *table, byte count) {
  audTable = table;
  audCount = count;
}

void PinNodeCtrl_AuditsChanged() { auditPending = true; }

// AUDIT_SNAPSHOT (0x2D): start u8, n u8, n x (index u8, value u32). false if the queue was full.
static boolean SendAuditSnapshot() {
  for (byte start = 0; start < audCount; start += 10) {
    byte n = audCount - start > 10 ? 10 : audCount - start;
    byte d[2 + 10 * 5];
    d[0] = start;
    d[1] = n;
    for (byte i = 0; i < n; i++) {
      const PinNodeAudit &a = audTable[start + i];
      unsigned long v = a.size == 4 ? EepromReadUL(a.eeprom) : EEPROM.read(a.eeprom);
      d[2 + 5 * i] = start + i;
      put32s(d + 3 + 5 * i, (long)v);
    }
    if (!RPULink_Queue(0x2D, d, 2 + 5 * n)) return false;
  }
  return true;
}

// The phone's clear-audits (PinNode_Tests): the values go to the node first, so they're on record,
// then each registered audit goes to 0. Returns the first audit's value before (total plays).
long PinNodeCtrl_ClearAudits() {
  long before = 0;
  if (audCount) before = audTable[0].size == 4 ? (long)EepromReadUL(audTable[0].eeprom) : (long)EEPROM.read(audTable[0].eeprom);
  SendAuditSnapshot();
  for (byte i = 0; i < audCount; i++) {
    if (audTable[i].size == 4) EepromWriteUL(audTable[i].eeprom, 0);
    else EEPROM.update(audTable[i].eeprom, 0);
  }
  auditPending = true;
  return before;
}

static byte curMode = 0xFF;                 // from PinNodeCtrl_Mode
static void AdjustmentsLoad();              // the PinNode adjustments (below)

static void SettingChangedRecord(byte idx, long value, byte source, uint16_t req, byte result) {
  byte r[9];
  r[0] = idx;
  put32s(r + 1, value);
  r[5] = source;
  r[6] = (byte)req;
  r[7] = (byte)(req >> 8);
  r[8] = result;
  RPULink_Queue(0x2C, r, 9);
}

// SETTING_SET: reqId u16, index u8, value i32, [source u8] -> SETTING_CHANGED: index u8, value i32,
// source u8, reqId u16, result u8 (the value after the attempt)
static boolean settingsLocked = false;    // SETTINGS_LOCK; RAM only (the node re-sends it after a reconnect)

boolean PinNodeCtrl_SettingsLocked() { return settingsLocked; }
boolean PinNodeCtrl_SettingLocked(byte index) {
  return settingsLocked && index < setCount && setTable[index].difficulty;
}

static void SettingSet(const byte *d, byte len) {
  uint16_t req = (uint16_t)(d[0] | (d[1] << 8));
  byte idx = d[2];
  long v = (long)((unsigned long)d[3] | ((unsigned long)d[4] << 8) | ((unsigned long)d[5] << 16) | ((unsigned long)d[6] << 24));
  byte source = len >= 8 ? d[7] : 2;
  byte result = PINNODE_SETTING_OK;
  if (idx >= setCount) result = PINNODE_SETTING_UNKNOWN;
  else if (v < setTable[idx].minValue || v > setTable[idx].maxValue ||
           (setTable[idx].step > 0 && (v - setTable[idx].minValue) % setTable[idx].step != 0))
    result = PINNODE_SETTING_OUT_OF_RANGE;
  else if (settingsLocked && setTable[idx].difficulty) result = PINNODE_SETTING_LOCKED;
  else if (!setTable[idx].liveSafe && gameOpen) result = PINNODE_SETTING_NOT_LIVE;
  else {
    const PinNodeSetting &st = setTable[idx];
    unsigned long stored = (unsigned long)((v + (long)st.scale / 2) / (long)st.scale);
    if (st.size == 4) EepromWriteUL(st.eeprom, stored);
    else EEPROM.update(st.eeprom, (byte)stored);
    AdjustmentsLoad();                       // an app Settings entry may be one of ours
    if (setApplied) setApplied();
  }
  byte r[9];
  r[0] = idx;
  put32s(r + 1, idx < setCount ? SettingRead(idx) : 0);
  r[5] = source;
  r[6] = (byte)req;
  r[7] = (byte)(req >> 8);
  r[8] = result;
  RPULink_Queue(0x2C, r, 9);
}

// ------------------------------------------------------------------ operator actions

static const PinNodeAction *actTable = NULL;
static byte actCount = 0;
static PinNodeActionRequest actReq;         // one request waiting for the game
static boolean actReqPending = false;
static unsigned long actReqAtMs;
static byte activeTest = 0xFF;              // the test the game said is running
static unsigned long activeKeepaliveMs;
static boolean phoneDiag = false;           // diagnostics entered from the phone (idle exit applies)
static unsigned long lastActionMs;
static byte exitDiagIndex = 0xFF;

byte PinNodeCtrl_ActionFlags(byte action) {
  return actTable && action < actCount ? actTable[action].flags : 0;
}

// An RPU game, before it clears its audits (clear-audits, 2.8): the values just before, on record first
boolean PinNodeCtrl_SendAuditsNow() { return SendAuditSnapshot(); }

void PinNodeCtrl_Actions(const PinNodeAction *table, byte count) {
  actTable = table;
  actCount = count;
  exitDiagIndex = 0xFF;
  for (byte i = 0; i < count && exitDiagIndex == 0xFF; i++)
    if (table[i].flags & PINNODE_ACTION_EXIT_DIAG) exitDiagIndex = i;
}

// ACTIONS_REPORT (0x34): n u8, n x (index u8, flags u8)
static void SendActionsReport() {
  for (byte start = 0; start < actCount; start += 25) {
    byte n = actCount - start > 25 ? 25 : actCount - start;
    byte d[1 + 25 * 2];
    d[0] = n;
    for (byte i = 0; i < n; i++) { d[1 + 2 * i] = start + i; d[2 + 2 * i] = actTable[start + i].flags; }
    RPULink_Queue(0x34, d, 1 + 2 * n);
  }
}

void PinNodeCtrl_ActionResult(uint16_t reqId, byte action, byte result, long value, byte reason) {
  boolean silent = result == PINNODE_ACTION_DONE && reason == PINNODE_REASON_LINK;   // the node reports it
  byte d[10];
  d[0] = (byte)reqId;
  d[1] = (byte)(reqId >> 8);
  d[2] = action;
  d[3] = result;
  put32s(d + 4, value);
  byte n = 8;
  if (result == PINNODE_ACTION_DONE) d[n++] = reason;
  if (!silent) RPULink_Queue(0x32, d, n);
  if (action < actCount && (actTable[action].flags & PINNODE_ACTION_ENTER_DIAG) &&
      (result == PINNODE_ACTION_OK || (result == PINNODE_ACTION_DONE && reason == PINNODE_REASON_FINISHED))) {
    phoneDiag = true;
    lastActionMs = millis();
  }
  if (action < actCount && (actTable[action].flags & PINNODE_ACTION_TEST)) {
    if (result == PINNODE_ACTION_OK) { activeTest = action; activeKeepaliveMs = millis(); }
    else if (result == PINNODE_ACTION_DONE && activeTest == action) activeTest = 0xFF;
  }
}

void PinNodeCtrl_SwitchEvent(byte sw, boolean closed) {
  if (activeTest == 0xFF) return;
  byte d[2] = {sw, (byte)(closed ? 1 : 0)};
  RPULink_Queue(0x33, d, 2);
}

boolean PinNodeCtrl_TakeAction(PinNodeActionRequest *req) {
  if (!actReqPending) return false;
  actReqPending = false;
  *req = actReq;
  return true;
}

// OP_ACTION (0xAB): reqId u16, action u8, op u8 (0 start, 1 stop, 2 keepalive), arg i32
static void OpAction(const byte *d, byte len) {
  uint16_t reqId = (uint16_t)(d[0] | (d[1] << 8));
  byte action = d[2], op = d[3];
  long arg = len >= 8 ? (long)((unsigned long)d[4] | ((unsigned long)d[5] << 8) |
                               ((unsigned long)d[6] << 16) | ((unsigned long)d[7] << 24)) : 0;
  lastActionMs = millis();                         // any action, keepalives too, holds off the idle exit
  if (op == 2) {                                   // keepalive: no reply
    if (action == activeTest) activeKeepaliveMs = millis();
    return;
  }
  if (action >= actCount || op > PINNODE_OP_STOP) {
    PinNodeCtrl_ActionResult(reqId, action, PINNODE_ACTION_UNKNOWN, 0);
    return;
  }
  if (actReqPending) {                             // the node sends one at a time
    PinNodeCtrl_ActionResult(reqId, action, PINNODE_ACTION_NOT_NOW, 0);
    return;
  }
  actReq.reqId = reqId;
  actReq.action = action;
  actReq.op = op;
  actReq.arg = arg;
  actReq.reason = PINNODE_REASON_STOPPED;
  actReqPending = true;
  actReqAtMs = millis();
}

// from Update: answer what the game didn't take; stop a test with no keepalive or no link
static unsigned long linkDownSinceMs = 0;
static void ActionsTick(unsigned long now, boolean linkUp) {
  if (actReqPending && (long)(now - actReqAtMs) > 500) {
    actReqPending = false;
    PinNodeCtrl_ActionResult(actReq.reqId, actReq.action, PINNODE_ACTION_NOT_NOW, 0);
  }
  if (linkUp) linkDownSinceMs = 0;
  else if (!linkDownSinceMs) linkDownSinceMs = now ? now : 1;
  if (activeTest == 0xFF && !actReqPending && phoneDiag && curMode == PINNODE_MODE_SERVICE &&
      exitDiagIndex != 0xFF && (long)(now - lastActionMs) > (long)PINNODE_DIAG_IDLE_MS) {
    actReq.reqId = 0;                              // the game's own exit-diagnostics, from the library
    actReq.action = exitDiagIndex;
    actReq.op = PINNODE_OP_START;
    actReq.arg = 0;
    actReq.reason = PINNODE_REASON_FINISHED;
    actReqPending = true;
    actReqAtMs = now;
    lastActionMs = now;                            // one try per idle period
    return;
  }
  if (activeTest == 0xFF || actReqPending) return;
  byte reason = 0xFF;
  if ((long)(now - activeKeepaliveMs) > (long)PINNODE_ACTION_KEEPALIVE_MS) reason = PINNODE_REASON_TIMEOUT;
  else if (linkDownSinceMs && (long)(now - linkDownSinceMs) > (long)PINNODE_ACTION_LINK_GRACE_MS) reason = PINNODE_REASON_LINK;
  if (reason == 0xFF) return;
  actReq.reqId = 0;                                // the library's own stop
  actReq.action = activeTest;
  actReq.op = PINNODE_OP_STOP;
  actReq.arg = 0;
  actReq.reason = reason;
  actReqPending = true;
  actReqAtMs = now;
  activeKeepaliveMs = now;                         // one stop per timeout
}

// ------------------------------------------------------------------ claim codes

static unsigned long claimCode[4];          // 0 = none; per slot (1-based slot = index + 1)
static unsigned long claimUntilMs;

// CLAIM_DISPLAY (0xA9): g u16, n u8, n x (slot u8, code u32), secs u16
static void ClaimDisplay(const byte *d, byte len) {
  byte n = d[2];
  if (len < 3 + 5 * n + 2) return;
  for (byte i = 0; i < 4; i++) claimCode[i] = 0;
  for (byte i = 0; i < n; i++) {
    const byte *e = d + 3 + 5 * i;
    if (e[0] < 1 || e[0] > 4) continue;
    claimCode[e[0] - 1] = (unsigned long)e[1] | ((unsigned long)e[2] << 8) |
                          ((unsigned long)e[3] << 16) | ((unsigned long)e[4] << 24);
  }
  unsigned int secs = d[3 + 5 * n] | (d[4 + 5 * n] << 8);
  claimUntilMs = millis() + 1000UL * secs;
}

boolean PinNodeCtrl_ClaimCode(byte slot, unsigned long *code) {
  if (slot < 1 || slot > 4 || !claimCode[slot - 1]) return false;
  if ((long)(millis() - claimUntilMs) >= 0) {   // the claim window is over
    for (byte i = 0; i < 4; i++) claimCode[i] = 0;
    return false;
  }
  *code = claimCode[slot - 1];
  return true;
}

// CREDIT_ADD (0xA8): reqId u16, n u8. Taken by the program; Update() answers what nobody takes.
static boolean creditPending = false;
static uint16_t creditReq;
static byte creditN;
static unsigned long creditAtMs;

boolean PinNodeCtrl_TakeCreditAdd(uint16_t *reqId, byte *n) {
  if (!creditPending) return false;
  creditPending = false;
  *reqId = creditReq; *n = creditN;
  return true;
}

void PinNodeCtrl_CreditResult(uint16_t reqId, byte result, byte n, byte credits) {
  byte r[4] = {(byte)reqId, (byte)(reqId >> 8), result, credits};
  RPULink_Queue(0x38, r, 4);
  if (result != PINNODE_CREDIT_OK) return;
  byte c[2] = {2, n};                        // CREDIT, source 2 (app): the credit.add event
  RPULink_Queue(0x28, c, 2);
}

// BOOT_SELECT (0xAC): reqId u16, target u8 (0 this game, 1 the original). Answered from Update().
static boolean bootPending = false;
static uint16_t bootReq;
static byte bootTarget;

boolean PinNodeCtrl_OriginalChosen() { return EEPROM.read(PINNODE_EEPROM_BOOT_TARGET) == 'O'; }
void PinNodeCtrl_SetBootTarget(byte original) { EEPROM.update(PINNODE_EEPROM_BOOT_TARGET, original ? 'O' : 0xFF); }

// ------------------------------------------------------------------ node status (for a status display)

static byte nodeWifi, nodeCloud;
static unsigned long nodeStatusMs;
static boolean nodeStatusSeen;
static char checkins[5][4];                 // [0] = waiting for the next game, [1-4] = slots; "" = none
static char nodeMsg[29];
static unsigned long nodeMsgUntil;

boolean PinNodeCtrl_NodeStatus(byte *wifi, byte *cloud) {
  if (!nodeStatusSeen || millis() - nodeStatusMs > 30000UL) return false;
  *wifi = nodeWifi; *cloud = nodeCloud;
  return true;
}
boolean PinNodeCtrl_CheckedIn(byte slot, char *initials) {
  if (slot > 4 || !checkins[slot][0]) return false;
  memcpy(initials, checkins[slot], 4);
  return true;
}
const char *PinNodeCtrl_NodeMessage() {
  if (!nodeMsg[0] || (long)(millis() - nodeMsgUntil) > 0) return NULL;
  return nodeMsg;
}

static void NodeInfoRecord(byte type, const byte *data, byte len) {
  if (type == 0xA0 && len >= 2) {             // NODE_STATUS: wifi u8, cloud u8, queued u16, ...
    nodeWifi = data[0]; nodeCloud = data[1]; nodeStatusMs = millis(); nodeStatusSeen = true;
  } else if (type == 0xA4 && len >= 5) {      // CHECKIN: slot u8, initials[3], flags u8 (1 bound, 2 waiting)
    byte slot = data[4] == 0x02 ? 0 : data[0];
    if (slot > 4) return;
    for (byte i = 0; i < 3; i++) checkins[slot][i] = (data[1 + i] >= 0x20 && data[1 + i] < 0x7F) ? (char)data[1 + i] : ' ';
    checkins[slot][3] = 0;
    if (slot) checkins[0][0] = 0;             // the waiting one was bound to this game
  } else if (type == 0xA6 && len >= 1) {      // DISPLAY_MSG: secs u8, text
    byte n = len - 1 < 28 ? len - 1 : 28;
    memcpy(nodeMsg, data + 1, n);
    nodeMsg[n] = 0;
    nodeMsgUntil = millis() + 1000UL * (data[0] ? data[0] : 10);
  }
}

static void HandleRecord(byte type, const byte *data, byte len) {
  if (recordHook && recordHook(type, data, len)) return;
  NodeInfoRecord(type, data, len);
  if (type == 0xA1 && len >= 7) {
    SettingSet(data, len);
  } else if (type == 0xA7 && len >= 1) {      // SETTINGS_LOCK: on u8, profileHash u32 (for the node's records)
    settingsLocked = data[0] != 0;
  } else if (type == 0xAB && len >= 4) {      // OP_ACTION
    OpAction(data, len);
  } else if (type == 0xA3) {                 // AUDIT_REQUEST
    auditPending = true;
  } else if (type == 0xA9 && len >= 3) {      // CLAIM_DISPLAY
    ClaimDisplay(data, len);
  } else if (type == 0xAA && len >= 3) {
    if (startPending) {                      // one at a time; the node never sends two
      byte r[4] = {data[0], data[1], PINNODE_START_NOT_NOW, 0};
      RPULink_Queue(0x31, r, 4);
      return;
    }
    startReq = (uint16_t)(data[0] | (data[1] << 8));
    startPending = true;
    startAtMs = millis();
  } else if (type == 0xA8 && len >= 3) {      // CREDIT_ADD
    uint16_t req = (uint16_t)(data[0] | (data[1] << 8));
    if (creditPending) { PinNodeCtrl_CreditResult(req, PINNODE_CREDIT_NOT_NOW, 0, 0); return; }
    creditReq = req; creditN = data[2]; creditAtMs = millis(); creditPending = true;
  } else if (type == 0xAC && len >= 3) {      // BOOT_SELECT
    bootReq = (uint16_t)(data[0] | (data[1] << 8));
    bootTarget = data[2] ? 1 : 0;
    bootPending = true;
  } else if (type == RPULINK_CTRL_OTA_REQUEST) {
    otaRequested = true;
  } else if (type == RPULINK_CTRL_TIME && len >= 4) {
    unixTimeAtSync = (unsigned long)data[0] | ((unsigned long)data[1] << 8) |
                     ((unsigned long)data[2] << 16) | ((unsigned long)data[3] << 24);
    millisAtSync = millis();
  } else if (type >= RPULINK_PINNODE_FIRST_S2M && type <= RPULINK_PINNODE_LAST_S2M) {
    if (PinNodeCtrl_OnPinNodeRecord) PinNodeCtrl_OnPinNodeRecord(type, data, len);
  }
}


// ------------------------------------------------------------------ game records

static uint16_t gameCounter;
static boolean ballOpen = false;           // a ball started and not yet ended
static byte openSlot, openBall;
static unsigned long gameStartMs, ballStartMs;
static byte lastMode = 0xFF, lastBall = 0xFF, lastSlot = 0xFF;
byte PinNodeCtrl_LastMode(byte *ball, byte *slot) { *ball = lastBall; *slot = lastSlot; return lastMode; }
static byte lastCredits = 0xFF, lastFreePlay = 0xFF;

static void put16(byte *p, uint16_t v) { p[0] = (byte)v; p[1] = (byte)(v >> 8); }
static void put32(byte *p, uint32_t v) { for (byte i = 0; i < 4; i++) p[i] = (byte)(v >> (8 * i)); }

// ------------------------------------------------------------------ achievements & stats

// STAT is coalesced per (slot, stat, op) in RAM until BALL_END, GAME_END or 500 ms
// (SPEC-node-link §5), so a bumper hit costs a RAM add, not a record.
#define PINNODE_STAT_SLOTS     12
#define PINNODE_STAT_HOLD_MS   500
struct PendingStat { byte slot, stat, op, used; long v; unsigned long firstMs; };
static PendingStat pendStat[PINNODE_STAT_SLOTS];

static boolean SendStat(PendingStat &p) {
  byte d[9];
  put16(d, gameCounter);
  d[2] = p.slot;
  d[3] = p.stat;
  d[4] = p.op;
  put32(d + 5, (uint32_t)p.v);
  if (!RPULink_Queue(0x26, d, 9)) return false;
  p.used = 0;
  return true;
}

// all of them (ball/game end), or only those held PINNODE_STAT_HOLD_MS (from Update)
static void FlushStats(boolean all, unsigned long now) {
  for (byte i = 0; i < PINNODE_STAT_SLOTS; i++)
    if (pendStat[i].used && (all || (long)(now - pendStat[i].firstMs) >= (long)PINNODE_STAT_HOLD_MS)) SendStat(pendStat[i]);
}

void PinNodeCtrl_Achievement(byte slot, byte ach, long v) {
  byte d[8];
  put16(d, gameCounter);
  d[2] = slot;
  d[3] = ach;
  put32(d + 4, (uint32_t)v);
  RPULink_Queue(0x25, d, 8);
}

void PinNodeCtrl_Stat(byte slot, byte stat, byte op, long v) {
  if (!gameOpen) return;
  byte freeIdx = 0xFF, oldest = 0xFF;
  for (byte i = 0; i < PINNODE_STAT_SLOTS; i++) {
    PendingStat &p = pendStat[i];
    if (p.used && p.slot == slot && p.stat == stat && p.op == op) {
      if (op == PINNODE_STAT_MAX) { if (v > p.v) p.v = v; }
      else p.v += v;
      return;
    }
    if (!p.used) { if (freeIdx == 0xFF) freeIdx = i; }
    else if (oldest == 0xFF || (long)(p.firstMs - pendStat[oldest].firstMs) < 0) oldest = i;
  }
  if (freeIdx == 0xFF) {                      // table full: send the oldest to make room
    if (!SendStat(pendStat[oldest])) return;  // link queue full too: drop this one
    freeIdx = oldest;
  }
  PendingStat &p = pendStat[freeIdx];
  p.slot = slot; p.stat = stat; p.op = op; p.v = v; p.firstMs = millis(); p.used = 1;
}

void PinNodeCtrl_Mode(byte mode, byte ball, byte slot) {
  curMode = mode;
  if (mode != PINNODE_MODE_SERVICE) phoneDiag = false;
  if (mode == lastMode && ball == lastBall && slot == lastSlot) return;
  byte d[3] = {mode, ball, slot};
  if (RPULink_Queue(0x2E, d, 3)) {
    lastMode = mode; lastBall = ball; lastSlot = slot;
  }
}

// Report under another identity (another ruleset, version and build), keeping the link: the HELLO is sent
// afresh with the settings, actions and audits registered at the time (set them first). flags go in HELLO.
void PinNodeCtrl_SetHello(const char *rulesetId, const char *version, const byte *manifestHash, uint32_t build,
                          byte maxPlayers, byte flags) {
  helloRuleset = rulesetId;
  helloVersion = version;
  for (byte i = 0; i < 8; i++) helloManifest[i] = manifestHash ? manifestHash[i] : 0;
  helloMaxPlayers = maxPlayers;
  helloSettings = setCount;
  helloFlags = flags;
  buildId = build;
  RPULink_DropQueued();                      // the HELLO and reports already queued, if not yet sent
  SendHello();
}

uint16_t PinNodeCtrl_GameCounter() { return gameCounter; }

void PinNodeCtrl_SetRecordHook(boolean (*hook)(byte type, const byte *data, byte len)) { recordHook = hook; }

void PinNodeCtrl_Credits(byte credits, boolean freePlay) {
  if (credits == lastCredits && (byte)freePlay == lastFreePlay) return;
  byte d[2] = {credits, (byte)(freePlay ? 1 : 0)};
  if (RPULink_Queue(0x35, d, 2)) {
    lastCredits = credits; lastFreePlay = (byte)freePlay;
  }
}

void PinNodeCtrl_GameStart(byte credits, boolean freePlay) {
  gameCounter += 1;
  EEPROM.update(PINNODE_EEPROM_GAME_COUNTER, (byte)gameCounter);
  EEPROM.update(PINNODE_EEPROM_GAME_COUNTER + 1, (byte)(gameCounter >> 8));
  gameOpen = true;
  ballOpen = false;
  for (byte i = 1; i <= 4; i++) checkins[i][0] = 0;   // a new game: earlier check-ins are done
  for (byte i = 0; i < PINNODE_STAT_SLOTS; i++) pendStat[i].used = 0;
  for (byte i = 0; i < 4; i++) claimCode[i] = 0;   // a new game ends the last one's claim window
  gameStartMs = millis();
  byte d[4];
  put16(d, gameCounter);
  d[2] = credits;
  d[3] = freePlay ? 1 : 0;
  RPULink_Queue(0x20, d, 4);
}

void PinNodeCtrl_PlayerAdded(byte slot) {
  if (!gameOpen) return;
  byte d[3];
  put16(d, gameCounter);
  d[2] = slot;
  RPULink_Queue(0x21, d, 3);
}

void PinNodeCtrl_BallStart(byte slot, byte ball) {
  if (!gameOpen) return;
  // the same ball served again with no BallEnd (ball save, or a drain before scoring) is
  // still the same ball: one ball_start, and its time runs on
  if (ballOpen && slot == openSlot && ball == openBall) return;
  ballOpen = true;
  openSlot = slot;
  openBall = ball;
  ballStartMs = millis();
  byte d[4];
  put16(d, gameCounter);
  d[2] = slot;
  d[3] = ball;
  RPULink_Queue(0x22, d, 4);
}

void PinNodeCtrl_BallEnd(byte slot, byte ball, unsigned long score) {
  if (!gameOpen) return;
  ballOpen = false;
  FlushStats(true, 0);                        // the ball's stats go ahead of its BALL_END
  byte d[12];
  put16(d, gameCounter);
  d[2] = slot;
  d[3] = ball;
  put32(d + 4, score);
  put32(d + 8, millis() - ballStartMs);
  RPULink_Queue(0x23, d, 12);
}

void PinNodeCtrl_GameEnd(const unsigned long *scores, byte numPlayers, byte endedBy, byte ballsSource) {
  if (!gameOpen) return;                      // idempotent: a slam path calling it twice is harmless
  FlushStats(true, 0);                        // ahead of GAME_END, while the game is open
  gameOpen = false;
  ballOpen = false;
  // plays, replays and high-score beats have moved; wait for the match / high-score code to write them
  auditAtMs = millis() + 15000UL;
  if (!auditAtMs) auditAtMs = 1;
  if (numPlayers > 4) numPlayers = 4;
  byte d[4 + 16 + 4 + 1];
  put16(d, gameCounter);
  d[2] = endedBy;
  d[3] = numPlayers;
  for (byte i = 0; i < numPlayers; i++) put32(d + 4 + 4 * i, scores[i]);
  put32(d + 4 + 4 * numPlayers, millis() - gameStartMs);
  byte n = 4 + 4 * numPlayers + 4;
  if (ballsSource) d[n++] = ballsSource;      // how the balls per game are known (optional)
  RPULink_Queue(0x24, d, n);
}

void PinNodeCtrl_Coin(byte chute, byte creditsAfter) {
  byte d[2] = {chute, creditsAfter};
  RPULink_Queue(0x27, d, 2);
  // the chute counter just went up: the audits follow in 2 s (after the game has written them), so the
  // phone's audit page shows the coin at once
  if (audCount && !auditAtMs) { auditAtMs = millis() + 2000UL; if (!auditAtMs) auditAtMs = 1; }
}

void PinNodeCtrl_Tilt(byte slot, byte kind) {
  byte d[4];
  put16(d, gameCounter);
  d[2] = slot;
  d[3] = kind;
  RPULink_Queue(0x29, d, 4);
}

void PinNodeCtrl_Begin(const char *version, const char *rulesetId, const byte *manifestHash,
                       byte maxPlayers, byte settingsCount, uint16_t hwRev) {
  AdjustmentsLoad();
  helloHwRev = hwRev;
  helloVersion = version;
  helloRuleset = rulesetId;
  for (byte i = 0; i < 8; i++) helloManifest[i] = manifestHash ? manifestHash[i] : 0;
  helloMaxPlayers = maxPlayers;
  helloSettings = setCount ? setCount : settingsCount;   // a registered table wins
  buildId = PinNodeCtrl_BuildId();
  gameCounter = (uint16_t)EEPROM.read(PINNODE_EEPROM_GAME_COUNTER) |
                ((uint16_t)EEPROM.read(PINNODE_EEPROM_GAME_COUNTER + 1) << 8);
  if (gameCounter == 0xFFFF) gameCounter = 0;   // erased EEPROM
  RPULink_SetHandler(HandleRecord);
  SendHello();
  linkWasUp = RPULink_IsUp();
}

// ------------------------------------------------------------------ PinNode adjustments
// Every item has an EEPROM byte (4080 + index). OperatorMenus and MACHINE_STATE_ADJUST_* games edit the
// library's copy (adj[]); games whose self-test writes EEPROM directly (RunExtendedSelfTest) edit the
// byte. When the menu closes, whichever side changed wins; then WiFi reset acts and goes back to 0.
static const byte adjMax[PINNODE_NUM_ADJUSTMENTS] = {1, 1};
static const byte adjDefault[PINNODE_NUM_ADJUSTMENTS] = {1, 0};
static byte adj[PINNODE_NUM_ADJUSTMENTS] = {1, 0};
static byte adjAtOpen[PINNODE_NUM_ADJUSTMENTS];

static byte AdjustmentRead(byte i) {
  byte v = EEPROM.read(PINNODE_EEPROM_ADJUSTMENTS + i);
  return v > adjMax[i] ? adjDefault[i] : v;
}

static void AdjustmentsLoad() {
  for (byte i = 0; i < PINNODE_NUM_ADJUSTMENTS; i++) adj[i] = AdjustmentRead(i);
  if (adj[PINNODE_ADJ_WIFI_RESET]) {         // left at 1 by a power-off in the menu: never act at boot
    adj[PINNODE_ADJ_WIFI_RESET] = 0;
    EEPROM.update(PINNODE_EEPROM_ADJUSTMENTS + PINNODE_ADJ_WIFI_RESET, 0);
  }
}

static void AdjustmentsMenuOpened() {
  for (byte i = 0; i < PINNODE_NUM_ADJUSTMENTS; i++) adjAtOpen[i] = adj[i];
}

static void AdjustmentsMenuClosed() {
  for (byte i = 0; i < PINNODE_NUM_ADJUSTMENTS; i++) {
    if (adj[i] != adjAtOpen[i]) EEPROM.update(PINNODE_EEPROM_ADJUSTMENTS + i, adj[i]);   // edited in our copy
    else adj[i] = AdjustmentRead(i);                                                     // or in EEPROM
  }
  if (adj[PINNODE_ADJ_WIFI_RESET]) {
    PinNodeCtrl_NodeRequest(PINNODE_REQ_SETUP);
    adj[PINNODE_ADJ_WIFI_RESET] = 0;
    EEPROM.update(PINNODE_EEPROM_ADJUSTMENTS + PINNODE_ADJ_WIFI_RESET, 0);
  }
}

boolean PinNode_AdjustmentRange(byte index, byte *values, byte **value) {
  if (index >= PINNODE_NUM_ADJUSTMENTS) return false;
  values[0] = 0;
  values[1] = adjMax[index];
  *value = &adj[index];
  return true;
}

boolean PinNode_AdjustmentEeprom(byte index, long *maxValue, unsigned short *eeprom) {
  if (index >= PINNODE_NUM_ADJUSTMENTS) return false;
  *maxValue = adjMax[index];
  *eeprom = PINNODE_EEPROM_ADJUSTMENTS + index;
  return true;
}

boolean PinNode_AppStartsOn() { return adj[PINNODE_ADJ_APP_STARTS] != 0; }

void PinNodeCtrl_NodeRequest(byte op) {
  RPULink_Queue(RPULINK_NODE_REQUEST, &op, 1);
}

void PinNodeCtrl_Log(const char *text) {
  RPULink_QueueText(RPULINK_LOG, text);
}

boolean PinNodeCtrl_TimeKnown() {
  return unixTimeAtSync != 0;
}

unsigned long PinNodeCtrl_UnixTime(unsigned long now) {
  if (!unixTimeAtSync) return 0;
  return unixTimeAtSync + (now - millisAtSync) / 1000UL;
}


// Safe state for a Mega reflash (architecture.md §8.2). The S3 then resets the Mega
// into its bootloader. If that never happens, restart through the watchdog.
void PinNodeCtrl_SetOtaHandler(void (*handler)(byte phase)) { otaHandler = handler; }

// ------------------------------------------------------------------ opt-in debug output

static Print *debugOut = NULL;
static boolean debugWasDormant = false;
static unsigned long debugStatsMs = 0;

void PinNodeCtrl_DebugTo(Print *out) { debugOut = out; }

static void DebugTick(unsigned long now, boolean up) {
  if (!debugOut) return;
  boolean dormant = RPULink_IsDormant();
  if (up && !linkWasUp) { debugOut->println(F("PinNode: node found, link up")); debugStatsMs = now; }
  if (!up && linkWasUp) debugOut->println(F("PinNode: link down"));
  if (dormant && !debugWasDormant) debugOut->println(F("PinNode: no node, quiet (probing every 2 s)"));
  debugWasDormant = dormant;
  if (up && (long)(now - debugStatsMs) >= 60000L) {
    debugStatsMs = now;
    debugOut->print(F("PinNode: bad frames "));  debugOut->print(RPULink_BadFrames());
    debugOut->print(F(", overruns "));           debugOut->print(RPULink_DataOverruns());
    debugOut->print(F(", framing "));            debugOut->print(RPULink_FramingErrors());
    debugOut->print(F(", dropped "));            debugOut->println(RPULink_DroppedRecords());
  }
}

static void PumpLink(unsigned long ms) {
  unsigned long start = millis();
  while ((millis() - start) < ms) RPULink_Update(millis());
}

static void EnterOtaSafeState() {
  otaHandler(PINNODE_OTA_PHASE_NOTICE);  // e.g. "UPDATING" on the displays
  PumpLink(300);                         // let it show, keeping the link alive
  otaHandler(PINNODE_OTA_PHASE_SAFE);    // coils, K1, lamps off; the node may now reset the Mega
  RPULink_SetFlag(RPULINK_FLAG_SAFE_STATE, true);
  RPULink_Queue(RPULINK_CTRL_OTA_READY, NULL, 0);
  PumpLink(PINNODE_OTA_RESET_TIMEOUT_MS);

  // no reset came: back to the game through the bootloader
  otaHandler(PINNODE_OTA_PHASE_ABORT);
  wdt_enable(WDTO_15MS);
  for (;;);
}

static void BootResult(byte result) {
  byte r[4] = {(byte)bootReq, (byte)(bootReq >> 8), bootTarget, result};
  RPULink_Queue(0x36, r, 4);
}

// A BOOT_SELECT, answered: switch only from attract (or with the original running), then restart
// the Mega so RPU_InitializeMPU picks the game (PinNode_BootOptions).
static void ServeBootSelect() {
  if (!bootPending) return;
  bootPending = false;
  boolean original = curMode == PINNODE_MODE_ORIGINAL;
  if (bootTarget && (helloHwRev < 4 || helloHwRev >= 300)) { BootResult(PINNODE_BOOT_UNSUPPORTED); return; }
  if ((boolean)bootTarget == original) { BootResult(PINNODE_BOOT_ALREADY); return; }
  if (!original && curMode != PINNODE_MODE_ATTRACT) { BootResult(PINNODE_BOOT_NOT_NOW); return; }
  PinNodeCtrl_SetBootTarget(bootTarget);
  BootResult(PINNODE_BOOT_OK);
  // the answer reaches the node first: pump until it's acked (300 ms wasn't always enough behind a
  // burst of records, and the owner's web switch reported "no_response", 2026-10-08), at most 2 s
  unsigned long t0 = millis();
  do RPULink_Update(millis()); while (RPULink_Pending() && millis() - t0 < 2000UL);
  PumpLink(50);
  wdt_enable(WDTO_15MS);                   // restart through the bootloader, like the OTA abort
  for (;;);
}

boolean PinNodeCtrl_TakeStartRequest(uint16_t *reqId) {
  if (!startPending) return false;
  startPending = false;
  *reqId = startReq;
  return true;
}

boolean PinNodeCtrl_TakeAppStart(uint16_t *reqId, boolean appStartsOn) {
  if (!PinNodeCtrl_TakeStartRequest(reqId)) return false;
  if (appStartsOn) return true;
  PinNodeCtrl_StartResult(*reqId, PINNODE_START_APP_OFF, 0);
  return false;
}

void PinNodeCtrl_StartResult(uint16_t reqId, byte result, byte slot) {
  byte r[4] = {(byte)reqId, (byte)(reqId >> 8), result, slot};
  RPULink_Queue(0x31, r, 4);
}

void PinNodeCtrl_Update(unsigned long now, boolean otaAllowed) {
  ServeBootSelect();
  static boolean inService;                  // the operator menu: save and act on our adjustments as it closes
  if (curMode == PINNODE_MODE_SERVICE) { if (!inService) { inService = true; AdjustmentsMenuOpened(); } }
  else if (inService) { inService = false; AdjustmentsMenuClosed(); }
  // credits from the app that the program didn't take (PinNodeCtrl_TakeCreditAdd) within 3 s
  if (creditPending && (long)(now - creditAtMs) > 3000) {
    creditPending = false;
    PinNodeCtrl_CreditResult(creditReq, PINNODE_CREDIT_UNSUPPORTED, 0, 0);
  }
  // signed: startAtMs is millis() when the link parsed the request, which can be later than the
  // loop's CurrentTime passed here, and unsigned that reads as ~49 days old
  FlushStats(false, now);
  if (startPending && (long)(now - startAtMs) > 500) {
    startPending = false;
    PinNodeCtrl_StartResult(startReq, PINNODE_START_NOT_NOW, 0);
  }
  if (audCount && (long)(now - lastAuditMs) >= (long)PINNODE_AUDIT_PERIOD_MS) auditPending = true;   // daily
  if (auditAtMs && (long)(now - auditAtMs) >= 0) { auditAtMs = 0; auditPending = true; }
  if (auditPending && audCount && RPULink_IsUp() && SendAuditSnapshot()) {
    auditPending = false;
    lastAuditMs = now;
  }
  boolean up = RPULink_IsUp();
  ActionsTick(now, up);
  if (up && !linkWasUp) {                   // the S3 may have restarted
    SendHello();
    lastMode = 0xFF;                        // and repeat the mode
    lastCredits = 0xFF;                     // and the credits
  }
  DebugTick(now, up);
  linkWasUp = up;

  if (!otaRequested) return;
  otaRequested = false;

  if (!otaHandler || !otaAllowed) {          // no safe state on this board, or not now
    byte reason = otaHandler ? PINNODE_OTA_REFUSED_GAME_IN_PROGRESS : PINNODE_OTA_REFUSED_NOT_SUPPORTED;
    RPULink_Queue(RPULINK_CTRL_OTA_REFUSED, &reason, 1);
    return;
  }
  EnterOtaSafeState();
  (void)now;
}

#endif
