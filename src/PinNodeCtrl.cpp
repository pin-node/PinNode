/**************************************************************************
 * PinNodeCtrl - see PinNodeCtrl.h
 **************************************************************************/
// PINNODE_HOST_BUILD: compile natively for tests, against host shims of Arduino.h, EEPROM.h and
// avr/wdt.h and a fake RPULink (see tests/native). RPULink.cpp and the transports stay out.
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_RP2040) || defined(PINNODE_HOST_BUILD)

#include <Arduino.h>
#include <stdarg.h>
#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/watchdog.h>
#else
#include <avr/wdt.h>
#endif
#include <EEPROM.h>
#include "RPULink.h"
#include "PinNodeCtrl.h"

// the game counter sits at the top of a 4 KB EEPROM, clear of the RPU OS and game settings (< 200)
#if defined(E2END) && (E2END < PINNODE_EEPROM_GAME_COUNTER + 1)
#error "PinNode needs an AVR with 4 KB of EEPROM (ATmega2560, the Arduino Mega)"
#endif

// The RP2350's EEPROM must be opened before use. PinNode can run before RPU_InitializeMPU (which
// opens it too, only if nobody has): open it here if needed, at the Mega's 4 KB.
static void EepromReady() {
#if defined(ARDUINO_ARCH_RP2040)
  if (EEPROM.length() == 0) EEPROM.begin(4096);
#endif
}

// The game's EEPROM layout, as the RPU OS stores it: unsigned longs little-endian, erased = 0xFF.
static unsigned long EepromReadUL(unsigned short a) {
  EepromReady();
  unsigned long v = 0;
  for (byte i = 0; i < 4; i++) v |= (unsigned long)EEPROM.read(a + i) << (8 * i);
  return v == 0xFFFFFFFFUL ? 0 : v;
}
static void EepromWriteUL(unsigned short a, unsigned long v) {
  EepromReady();
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

static unsigned long initResult;
static boolean initNoted;
void PinNodeCtrl_NoteInitResult(unsigned long r) { initResult = r; initNoted = true; }

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
  if (initNoted) {                           // why this start chose this game or the original
    char t[24];
    snprintf(t, sizeof t, "rpu init 0x%04lx", initResult);
    RPULink_QueueText(RPULINK_LOG, t);
  }
  SendSettingsReport();                      // the node's settings view starts from this
  SendActionsReport();                       // and its action flags (local, confirm, lock…)
  auditPending = true;                       // and its audits (sent from Update, after this burst)
}

// Restart the board. The RP2350 (PinNode 400) keeps EEPROM as a RAM copy of a flash sector:
// save it first, or the boot choice just written is lost.
static void Restart() {
#if defined(ARDUINO_ARCH_RP2040)
  EEPROM.commit();
  watchdog_reboot(0, 0, 10);
#else
  wdt_enable(WDTO_15MS);
#endif
  for (;;);
}

// CRC-32 (IEEE) of the program image, flash 0 .. __data_load_end, as the service computes it
// from the release .hex (SPEC-node-link §4). Nibble table: small and fast enough at boot.
// On the RP2350: the image from the start of flash to __flash_binary_end, rounded up to 256 bytes.
#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
extern char __flash_binary_start, __flash_binary_end;
#else
extern char __data_load_end;
#endif
static const uint32_t crcNibble[16] PROGMEM = {
  0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
  0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C};

uint32_t PinNodeCtrl_BuildId() {
  uint32_t crc = 0xFFFFFFFFUL;
#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
  // whole 256-byte pages: a UF2 carries the image that way, so the node's CRC of a .uf2 matches
  const char *end = &__flash_binary_start + ((&__flash_binary_end - &__flash_binary_start + 255) & ~255);
  for (const char *p = &__flash_binary_start; p < end; p++) {
    crc ^= (uint8_t)*p;
#else
  uint32_t end = pgm_get_far_address(__data_load_end);
  for (uint32_t a = 0; a < end; a++) {
    crc ^= pgm_read_byte_far(a);
#endif
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
// The settings and audit tables are in RAM (PinNodeCtrl_Settings / _Audits) or in flash (the _P
// calls, for PROGMEM tables: on the Mega a RAM table costs 19 bytes a setting). Read one entry at a time.
#if !defined(__AVR__) && !defined(memcpy_P)
#define memcpy_P memcpy
#endif
static const PinNodeSetting *setTable = NULL;
static byte setCount = 0;
static boolean setInFlash = false;
static void (*setApplied)(void) = NULL;
static boolean gameOpen = false;           // between GameStart and GameEnd

static PinNodeSetting SettingAt(byte idx) {
  PinNodeSetting st;
  if (setInFlash) memcpy_P(&st, &setTable[idx], sizeof st);
  else st = setTable[idx];
  return st;
}

static long SettingRead(byte idx) {
  const PinNodeSetting st = SettingAt(idx);
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
  setInFlash = false;
  setApplied = applied;
}

void PinNodeCtrl_SettingsP(const PinNodeSetting *table, byte count, void (*applied)(void)) {
  PinNodeCtrl_Settings(table, count, applied);
  setInFlash = true;
}

void PinNodeCtrl_SettingsChanged() { SendSettingsReport(); }

// ------------------------------------------------------------------ audits

static const PinNodeAudit *audTable = NULL;
static byte audCount = 0;
static boolean audInFlash = false;

static PinNodeAudit AuditAt(byte idx) {
  PinNodeAudit a;
  if (audInFlash) memcpy_P(&a, &audTable[idx], sizeof a);
  else a = audTable[idx];
  return a;
}

static unsigned long AuditRead(byte idx) {
  const PinNodeAudit a = AuditAt(idx);
  return a.size == 4 ? EepromReadUL(a.eeprom) : EEPROM.read(a.eeprom);
}
static unsigned long lastAuditMs = 0;
static unsigned long auditAtMs = 0;        // 0 = none: a snapshot due at this time
#define PINNODE_AUDIT_PERIOD_MS  (24UL * 60UL * 60UL * 1000UL)

void PinNodeCtrl_Audits(const PinNodeAudit *table, byte count) {
  audTable = table;
  audCount = count;
  audInFlash = false;
}

void PinNodeCtrl_AuditsP(const PinNodeAudit *table, byte count) {
  PinNodeCtrl_Audits(table, count);
  audInFlash = true;
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
      d[2 + 5 * i] = start + i;
      put32s(d + 3 + 5 * i, (long)AuditRead(start + i));
    }
    if (!RPULink_Queue(0x2D, d, 2 + 5 * n)) return false;
  }
  return true;
}

// The phone's clear-audits (PinNode_Tests): the values go to the node first, so they're on record,
// then each registered audit goes to 0. Returns the first audit's value before (total plays).
long PinNodeCtrl_ClearAudits() {
  long before = 0;
  if (audCount) before = (long)AuditRead(0);
  SendAuditSnapshot();
  for (byte i = 0; i < audCount; i++) {
    const PinNodeAudit a = AuditAt(i);
    if (a.size == 4) EepromWriteUL(a.eeprom, 0);
    else EEPROM.update(a.eeprom, 0);
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
  return settingsLocked && index < setCount && SettingAt(index).difficulty;
}

// A volume changed from the app: the player plays a short pip at the new level, as the operator menu's
// own sample would (RPU volume settings: live-safe, 0-10, a byte; RPU's 0-10 -> dB table). The 400's
// player makes the tone itself (weak: other boards have none, so nothing plays).
#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
extern "C" void PinNode400_Pip(int gainDb) __attribute__((weak));
static const signed char volumeGainDb[11] = {-70, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0};

void PinNode_VolumePip(byte level) {
  if (!PinNode400_Pip || level == 0 || level > 10) return;
  PinNode400_Pip(volumeGainDb[level]);
}
#else
void PinNode_VolumePip(byte level) { (void)level; }       // no player of ours on this board
#endif

// ------------------------------------------------------------------ developer log

static unsigned long logWindowMs;
static byte logInWindow;
static unsigned int logDropped;

void PinNode_Log(const char *fmt, ...) {
  unsigned long now = millis();
  if (now - logWindowMs >= 1000) { logWindowMs = now; logInWindow = 0; }
  if (logInWindow >= PINNODE_LOG_PER_S) { logDropped++; return; }
  char line[121];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line, sizeof line, fmt, ap);
  va_end(ap);
  if (logDropped) {                        // say what was lost before this line
    char d[32];
    snprintf(d, sizeof d, "(%u lines dropped)", logDropped);
    if (!RPULink_QueueText(RPULINK_LOG, d)) return;
    logDropped = 0;
    logInWindow++;
  }
  // a LOG record carries up to 54 bytes of text: longer lines go in pieces, the later ones marked
  size_t n = strlen(line), at = 0;
  char part[55];
  while (at < n || n == 0) {
    size_t k = n - at > 53 ? 53 : n - at;
    part[0] = at ? '~' : ' ';                // '~': continues the line before
    memcpy(part + 1, line + at, k);
    part[k + 1] = 0;
    if (!RPULink_QueueText(RPULINK_LOG, part)) { logDropped++; return; }
    at += k;
    if (n == 0) break;
  }
  logInWindow++;
}

static boolean IsVolumeSetting(const PinNodeSetting &st) {
  return st.liveSafe && st.size == 1 && st.minValue == 0 && st.maxValue == 10 && st.scale == 1;
}

static void SettingSet(const byte *d, byte len) {
  uint16_t req = (uint16_t)(d[0] | (d[1] << 8));
  byte idx = d[2];
  long v = (long)((unsigned long)d[3] | ((unsigned long)d[4] << 8) | ((unsigned long)d[5] << 16) | ((unsigned long)d[6] << 24));
  byte source = len >= 8 ? d[7] : 2;
  byte result = PINNODE_SETTING_OK;
  PinNodeSetting st = {};
  if (idx < setCount) st = SettingAt(idx);
  if (idx >= setCount) result = PINNODE_SETTING_UNKNOWN;
  else if (v < st.minValue || v > st.maxValue || (st.step > 0 && (v - st.minValue) % st.step != 0))
    result = PINNODE_SETTING_OUT_OF_RANGE;
  else if (settingsLocked && st.difficulty) result = PINNODE_SETTING_LOCKED;
  else if (!st.liveSafe && gameOpen) result = PINNODE_SETTING_NOT_LIVE;
  else {
    unsigned long stored = (unsigned long)((v + (long)st.scale / 2) / (long)st.scale);
    if (st.size == 4) EepromWriteUL(st.eeprom, stored);
    else EEPROM.update(st.eeprom, (byte)stored);
    AdjustmentsLoad();                       // an app Settings entry may be one of ours
    if (setApplied) setApplied();
    if (IsVolumeSetting(st)) PinNode_VolumePip((byte)v);
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
static boolean claimIsPairing;              // g = 0xFFFF: a pairing code (SPEC-dev-flash §9.2 B), shown steady

// CLAIM_DISPLAY (0xA9): g u16, n u8, n x (slot u8, code u32), secs u16
static void ClaimDisplay(const byte *d, byte len) {
  byte n = d[2];
  if (len < 3 + 5 * n + 2) return;
  claimIsPairing = d[0] == 0xFF && d[1] == 0xFF;
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

boolean PinNodeCtrl_ClaimIsPairing() { return claimIsPairing; }

// ------------------------------------------------------------------ check-in initials

static char checkinIni[4][3];               // per slot (1-based slot = index + 1); kept until the next game
static boolean checkinHave[4];

// CHECKIN (0xA4): slot u8 (0 = queued for the next slot), initials[3], flags u8 (bit0 bound, bit1 unbound)
static void Checkin(const byte *d) {
  byte slot = d[0];
  if (slot < 1 || slot > 4) return;           // queued: it comes again with its slot once it binds
  if (d[4] & 0x02) { checkinHave[slot - 1] = false; return; }
  if (!(d[4] & 0x01)) return;
  boolean any = false;
  for (byte i = 0; i < 3; i++) {
    char c = (char)d[1 + i];
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    if (c < ' ' || c > '~') c = ' ';
    checkinIni[slot - 1][i] = c;
    if (c != ' ') any = true;
  }
  checkinHave[slot - 1] = any;
}

boolean PinNodeCtrl_PlayerInitials(byte slot, char out[3]) {
  if (slot < 1 || slot > 4 || !checkinHave[slot - 1]) return false;
  memcpy(out, checkinIni[slot - 1], 3);
  return true;
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

boolean PinNodeCtrl_OriginalChosen() { EepromReady(); return EEPROM.read(PINNODE_EEPROM_BOOT_TARGET) == 'O'; }
void PinNodeCtrl_SetBootTarget(byte original) { EepromReady(); EEPROM.update(PINNODE_EEPROM_BOOT_TARGET, original ? 'O' : 0xFF); }

// SOUND_PREVIEW (0xAD): reqId u16, op u8 (1 play, 0 stop), path (no NUL): the owner plays a file from the
// card on the machine from the app (outside games). Answered at once, and again when it ends, with
// SOUND_STATE (0x3A): reqId u16, state u8 (PINNODE_PREVIEW_...). PinNode 400: RPU400Audio plays it.
#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
extern "C" int PinNode400_Preview(int op, const char *path) __attribute__((weak));
extern "C" int PinNode400_PreviewPoll() __attribute__((weak));
#endif
boolean PinNodeCtrl_BulkAllowed();           // below: attract or the operator menus
static uint16_t previewReq;
static boolean previewOn = false, previewAnswered = false;
static unsigned long previewAtMs;

static void PreviewState(uint16_t req, byte state) {
  byte r[3] = {(byte)req, (byte)(req >> 8), state};
  RPULink_Queue(0x3A, r, 3);
}

static void SoundPreview(const byte *d, byte len) {
  uint16_t req = (uint16_t)(d[0] | (d[1] << 8));
  char path[49];
  byte n = len - 3 < 48 ? len - 3 : 48;
  memcpy(path, d + 3, n);
  path[n] = 0;
#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
  if (!PinNode400_Preview) { PreviewState(req, PINNODE_PREVIEW_UNSUPPORTED); return; }
  if (d[2] && !PinNodeCtrl_BulkAllowed()) { PreviewState(req, PINNODE_PREVIEW_NOT_NOW); return; }
  int st = PinNode400_Preview(d[2] ? 1 : 0, path);
  previewOn = false;
  if (d[2] && st == PINNODE_PREVIEW_PLAYING) {   // starting: the answer waits until its file opens (or not)
    previewReq = req; previewOn = true; previewAnswered = false; previewAtMs = millis();
    return;
  }
  PreviewState(req, (byte)st);
#else
  (void)path;
  PreviewState(req, PINNODE_PREVIEW_UNSUPPORTED);
#endif
}

static void PreviewTick() {
#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
  if (!previewOn || !PinNode400_PreviewPoll) return;
  int st = PinNode400_PreviewPoll();
  if (st == 10 || (!st && !previewAnswered && millis() - previewAtMs > 2000)) {   // it plays
    if (!previewAnswered) PreviewState(previewReq, PINNODE_PREVIEW_PLAYING);
    previewAnswered = true;
    return;
  }
  if (!st) return;
  previewOn = false;                         // ended, or the file didn't open
  PreviewState(previewReq, (byte)st);
#endif
}

static void HandleRecord(byte type, const byte *data, byte len) {
  if (recordHook && recordHook(type, data, len)) return;
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
  } else if (type == 0xAD && len >= 3) {      // SOUND_PREVIEW
    SoundPreview(data, len);
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
    if (type == 0xA4 && len >= 5) Checkin(data);   // and still to the game's hook (greetings)
    if (PinNodeCtrl_OnPinNodeRecord) PinNodeCtrl_OnPinNodeRecord(type, data, len);
  }
}


// ------------------------------------------------------------------ game records

static uint16_t gameCounter;
static boolean ballOpen = false;           // a ball started and not yet ended
static byte openSlot, openBall;
static unsigned long gameStartMs, ballStartMs;
static byte lastMode = 0xFF, lastBall = 0xFF, lastSlot = 0xFF;
static byte lastCredits = 0xFF, lastFreePlay = 0xFF;

static void put16(byte *p, uint16_t v) { p[0] = (byte)v; p[1] = (byte)(v >> 8); }
static void put32(byte *p, uint32_t v) { for (byte i = 0; i < 4; i++) p[i] = (byte)(v >> (8 * i)); }

// ------------------------------------------------------------------ achievements & stats

// STAT is coalesced per (slot, stat, op) in RAM until BALL_END, GAME_END or 500 ms
// (SPEC-node-link §5), so a bumper hit costs a RAM add, not a record.
#if defined(__AVR__)
#define PINNODE_STAT_SLOTS     6     // the Mega's 8 KB: a full table sends its oldest early
#else
#define PINNODE_STAT_SLOTS     12
#endif
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

static void SidecarClear();                   // Sidecar (below)
static void SidecarResync();
static void SidecarTick(unsigned long now);

void PinNodeCtrl_GameStart(byte credits, boolean freePlay) {
  gameCounter += 1;
  EEPROM.update(PINNODE_EEPROM_GAME_COUNTER, (byte)gameCounter);
  EEPROM.update(PINNODE_EEPROM_GAME_COUNTER + 1, (byte)(gameCounter >> 8));
  gameOpen = true;
  ballOpen = false;
  openSlot = openBall = 0;                     // no ball yet (live scores wait for BallStart)
  for (byte i = 0; i < PINNODE_STAT_SLOTS; i++) pendStat[i].used = 0;
  for (byte i = 0; i < 4; i++) claimCode[i] = 0;   // a new game ends the last one's claim window
  for (byte i = 0; i < 4; i++) checkinHave[i] = false;   // and the last one's checked-in players
  SidecarClear();                              // the node clears its own at GAME_START too
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

#define PINNODE_LIVE_MIN_MS    250UL       // at most 4 a second
#define PINNODE_LIVE_EVERY_MS  2000UL      // and at least this often, so a phone that just arrived catches up
static unsigned long liveSent[4], liveMs;
static byte livePlayers, liveUp, liveBall;
static boolean liveAny;

void PinNodeCtrl_Scores(const unsigned long *scores, byte numPlayers) {
  if (!gameOpen || !scores) { liveAny = false; return; }
  if (!openSlot || !openBall) return;          // not before the first BallStart: no player up or ball yet
  unsigned long now = millis();
  byte n = numPlayers > 4 ? 4 : numPlayers;
  boolean changed = !liveAny || n != livePlayers || openSlot != liveUp || openBall != liveBall;
  for (byte i = 0; i < 4 && !changed; i++) changed = (i < n ? scores[i] : 0) != liveSent[i];
  if (liveAny && now - liveMs < (changed ? PINNODE_LIVE_MIN_MS : PINNODE_LIVE_EVERY_MS)) return;
  byte d[5 + 16];
  put16(d, gameCounter);
  d[2] = n; d[3] = openSlot; d[4] = openBall;
  for (byte i = 0; i < 4; i++) put32(d + 5 + 4 * i, i < n ? scores[i] : 0);
  if (!RPULink_Queue(0x3C, d, sizeof d)) return;
  for (byte i = 0; i < 4; i++) liveSent[i] = i < n ? scores[i] : 0;
  livePlayers = n; liveUp = openSlot; liveBall = openBall; liveMs = now; liveAny = true;
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

// ------------------------------------------------------------------ Sidecar (PinNode 400)

#if defined(ARDUINO_ARCH_RP2040) || defined(PINNODE_HOST_BUILD)
// The lowest priority on the link: a Sidecar record is only queued while this many bytes stay free for
// the game's own records. States and timers wait in their tables (newest value) until there's room;
// an event that finds no room is dropped (counted).
#ifndef PINNODE_SIDECAR_STATES
#define PINNODE_SIDECAR_STATES   256
#endif
#ifndef PINNODE_SIDECAR_TIMERS
#define PINNODE_SIDECAR_TIMERS   32
#endif
#define PINNODE_SIDECAR_RESERVE  96
#define PINNODE_SIDECAR_FIRST_RESERVED_KEY  240
#define PINNODE_SIDECAR_FIRST_RESERVED_CODE 0xFF00
#define PINNODE_SIDECAR_STATES_PER_RECORD   9
struct SidecarState { byte slot, key, used, pending; long value; };
struct SidecarTimer { byte slot, key, used, pending, op; unsigned long left, total, atMs; };
static SidecarState scState[PINNODE_SIDECAR_STATES];
static SidecarTimer scTimer[PINNODE_SIDECAR_TIMERS];
static void (*scResync)(void) = NULL;
static unsigned int scDropped = 0;
static boolean scAny = false;                 // anything in the tables (skip the scans when not)

static boolean SidecarRoom(byte len) { return RPULink_QueueFree() >= (unsigned int)(PINNODE_SIDECAR_RESERVE + 2 + len); }

static void SidecarClear() {
  for (unsigned int i = 0; i < PINNODE_SIDECAR_STATES; i++) scState[i].used = 0;
  for (byte i = 0; i < PINNODE_SIDECAR_TIMERS; i++) scTimer[i].used = 0;
  scAny = false;
}

// the node may have lost what it kept: everything shown goes again
static void SidecarResync() {
  for (unsigned int i = 0; i < PINNODE_SIDECAR_STATES; i++) if (scState[i].used) scState[i].pending = 1;
  for (byte i = 0; i < PINNODE_SIDECAR_TIMERS; i++) if (scTimer[i].used) scTimer[i].pending = 1;
  if (scResync && gameOpen) scResync();
}

void PinNodeCtrl_SetSidecarResync(void (*fn)(void)) { scResync = fn; }
unsigned int PinNodeCtrl_SidecarDropped() { return scDropped; }

void PinNodeCtrl_Sidecar(byte slot, uint16_t code, long value) {
  if (!gameOpen || code >= PINNODE_SIDECAR_FIRST_RESERVED_CODE) return;
  byte d[9];
  put16(d, gameCounter);
  d[2] = slot;
  put16(d + 3, code);
  put32(d + 5, (uint32_t)value);
  if (!SidecarRoom(sizeof d) || !RPULink_Queue(RPULINK_SIDECAR_EVENT, d, sizeof d)) scDropped++;
}

void PinNodeCtrl_SidecarState(byte slot, byte key, long value) {
  if (!gameOpen || key >= PINNODE_SIDECAR_FIRST_RESERVED_KEY) return;
  int freeIdx = -1, spare = -1;
  for (unsigned int i = 0; i < PINNODE_SIDECAR_STATES; i++) {
    SidecarState &s = scState[i];
    if (!s.used) { if (freeIdx < 0) freeIdx = i; continue; }
    if (s.slot == slot && s.key == key) {
      if (s.value == value) return;             // already sent, or already waiting to be
      s.value = value;
      s.pending = 1;
      return;
    }
    if (!s.pending && spare < 0) spare = i;     // sent and settled: can be forgotten if the table is full
  }
  if (freeIdx < 0) freeIdx = spare;             // forgetting one only means it can't be re-sent at a resync
  if (freeIdx < 0) { scDropped++; return; }     // every entry is still waiting to go
  SidecarState &s = scState[freeIdx];
  s.slot = slot; s.key = key; s.value = value; s.used = 1; s.pending = 1;
  scAny = true;
}

static void SidecarSetTimer(byte slot, byte key, byte op, unsigned long msLeft, unsigned long msTotal, boolean keepTotal) {
  if (!gameOpen || key >= PINNODE_SIDECAR_FIRST_RESERVED_KEY) return;
  int idx = -1, freeIdx = -1;
  for (byte i = 0; i < PINNODE_SIDECAR_TIMERS; i++) {
    if (scTimer[i].used && scTimer[i].slot == slot && scTimer[i].key == key) { idx = i; break; }
    if (!scTimer[i].used && freeIdx < 0) freeIdx = i;
  }
  if (idx < 0) {
    if (op == PINNODE_SC_TIMER_STOPPED) return;   // never shown: nothing to stop
    if (freeIdx < 0) { scDropped++; return; }
    idx = freeIdx;
    scTimer[idx].total = 0;
  }
  SidecarTimer &t = scTimer[idx];
  t.slot = slot; t.key = key; t.op = op; t.left = msLeft; t.atMs = millis();
  if (!keepTotal) t.total = msTotal;
  t.used = 1; t.pending = 1;
  scAny = true;
}

void PinNodeCtrl_SidecarTimer(byte slot, byte key, unsigned long msLeft, unsigned long msTotal) {
  SidecarSetTimer(slot, key, PINNODE_SC_TIMER_RUNNING, msLeft, msTotal, false);
}
void PinNodeCtrl_SidecarTimerPause(byte slot, byte key, unsigned long msLeft) {
  SidecarSetTimer(slot, key, PINNODE_SC_TIMER_PAUSED, msLeft, 0, true);
}
void PinNodeCtrl_SidecarTimerStop(byte slot, byte key) {
  SidecarSetTimer(slot, key, PINNODE_SC_TIMER_STOPPED, 0, 0, true);
}

// From Update: timers first (a countdown that starts late looks wrong), then states, a record per slot
static void SidecarTick(unsigned long now) {
  if (!scAny) return;
  for (byte i = 0; i < PINNODE_SIDECAR_TIMERS; i++) {
    SidecarTimer &t = scTimer[i];
    if (!t.used || !t.pending) continue;
    if (!SidecarRoom(13)) return;
    unsigned long left = t.left;
    if (t.op == PINNODE_SC_TIMER_RUNNING) {
      unsigned long ran = now - t.atMs;
      left = ran >= left ? 0 : left - ran;
    }
    byte d[13];
    put16(d, gameCounter);
    d[2] = t.slot; d[3] = t.key; d[4] = t.op;
    put32(d + 5, left);
    put32(d + 9, t.total);
    if (!RPULink_Queue(RPULINK_SIDECAR_TIMER, d, sizeof d)) return;
    t.pending = 0;
    if (t.op == PINNODE_SC_TIMER_STOPPED) t.used = 0;   // gone from the phone: nothing to resync
  }
  for (;;) {
    int first = -1;
    for (unsigned int i = 0; i < PINNODE_SIDECAR_STATES && first < 0; i++)
      if (scState[i].used && scState[i].pending) first = i;
    if (first < 0) return;
    byte slot = scState[first].slot;
    byte d[4 + 5 * PINNODE_SIDECAR_STATES_PER_RECORD];
    byte n = 0;
    int taken[PINNODE_SIDECAR_STATES_PER_RECORD];
    for (unsigned int i = first; i < PINNODE_SIDECAR_STATES && n < PINNODE_SIDECAR_STATES_PER_RECORD; i++) {
      SidecarState &s = scState[i];
      if (!s.used || !s.pending || s.slot != slot) continue;
      d[4 + 5 * n] = s.key;
      put32(d + 5 + 5 * n, (uint32_t)s.value);
      taken[n++] = i;
    }
    put16(d, gameCounter);
    d[2] = slot; d[3] = n;
    byte len = 4 + 5 * n;
    if (!SidecarRoom(len) || !RPULink_Queue(RPULINK_SIDECAR_STATE, d, len)) return;
    for (byte k = 0; k < n; k++) scState[taken[k]].pending = 0;
  }
}
#else
static void SidecarClear() {}
static void SidecarResync() {}
static void SidecarTick(unsigned long) {}
#endif

void PinNodeCtrl_Begin(const char *version, const char *rulesetId, const byte *manifestHash,
                       byte maxPlayers, byte settingsCount, uint16_t hwRev) {
  EepromReady();
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

#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
// PinNode 400: the node reflashes the RP over SWD, so every RPU game has the same safe state, from
// RPU's own calls (weak: without RPU OS linked in, there is no handler and updates are refused).
// The SWD flash halts the RP with the 6800 still halted: whatever the PIAs hold then stays, so coils
// and flippers go off and the displays blank first, and the interrupts get time to write that out.
void RPU_DisableSolenoidStack() __attribute__((weak));
void RPU_SetDisableFlippers(boolean disableFlippers, byte solbit) __attribute__((weak));
void RPU_TurnOffAllLamps() __attribute__((weak));
void RPU_SetDisplayBlank(int displayNumber, byte bitMask) __attribute__((weak));
extern "C" void PinNode400_AudioSilence() __attribute__((weak));   // the game's player: stop, flush to silence

static void Rev400OtaHandler(byte phase) {
  if (phase != PINNODE_OTA_PHASE_SAFE) return;
  // the halt freezes the DAC on whatever its buffers hold: silence first
  if (PinNode400_AudioSilence) PinNode400_AudioSilence();
  RPU_DisableSolenoidStack();               // no new coils; one already firing ends on its timer
  RPU_SetDisableFlippers(true, 0x40);       // CONTSOL_DISABLE_FLIPPERS
  RPU_TurnOffAllLamps();
  for (int d = 0; d < 5; d++) RPU_SetDisplayBlank(d, 0x00);
  delay(250);                                // the timer interrupts write it all out
}

static boolean Rev400HasRpu() {
  return RPU_DisableSolenoidStack && RPU_SetDisableFlippers && RPU_TurnOffAllLamps && RPU_SetDisplayBlank;
}
#endif

#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
// PinNode 400 bulk mode (files to the SD card): only in attract or the operator menus
boolean PinNodeCtrl_BulkAllowed() { return curMode == PINNODE_MODE_ATTRACT || curMode == PINNODE_MODE_SERVICE; }

// The RP's SD card (the game's player, weak): CARD_STATUS after HELLO and on a change, for the
// node's health report (the site offers "Upload sounds" only with a card)
extern "C" int PinNode400_CardStatus(unsigned int *tracks) __attribute__((weak));
// The game's sound player, a status line when it has a new one (its startup, its output): to the node
// log as a LOG record, so its state is visible without the RP's USB
extern "C" int PinNode400_AudioStatus(char *out, unsigned int size) __attribute__((weak));
static int lastCardOk = -1;
static unsigned int lastCardTracks;
static void CardStatusTick(boolean force) {
  if (!PinNode400_CardStatus) return;
  unsigned int tracks = 0;
  int ok = PinNode400_CardStatus(&tracks) ? 1 : 0;
  if (!force && ok == lastCardOk && tracks == lastCardTracks) return;
  byte r[3] = {(byte)ok, (byte)tracks, (byte)(tracks >> 8)};
  if (RPULink_Queue(RPULINK_CARD_STATUS, r, 3)) { lastCardOk = ok; lastCardTracks = tracks; }
}
#endif

static void EnterOtaSafeState() {
  otaHandler(PINNODE_OTA_PHASE_NOTICE);  // e.g. "UPDATING" on the displays
  PumpLink(300);                         // let it show, keeping the link alive
  otaHandler(PINNODE_OTA_PHASE_SAFE);    // coils, K1, lamps off; the node may now reset the Mega
  RPULink_SetFlag(RPULINK_FLAG_SAFE_STATE, true);
  RPULink_Queue(RPULINK_CTRL_OTA_READY, NULL, 0);
  PumpLink(PINNODE_OTA_RESET_TIMEOUT_MS);

  // no reset came: back to the game through the bootloader
  otaHandler(PINNODE_OTA_PHASE_ABORT);
  Restart();
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
  // Boot select needs a board that can run the original: Rev 4/10x (Mega) and 400 (RP2350)
  if (bootTarget && (helloHwRev < 4 || (helloHwRev >= 300 && helloHwRev != 400))) { BootResult(PINNODE_BOOT_UNSUPPORTED); return; }
  if ((boolean)bootTarget == original) { BootResult(PINNODE_BOOT_ALREADY); return; }
  if (!original && curMode != PINNODE_MODE_ATTRACT) { BootResult(PINNODE_BOOT_NOT_NOW); return; }
  PinNodeCtrl_SetBootTarget(bootTarget);
  BootResult(PINNODE_BOOT_OK);
  // the answer reaches the node first: pump until it's acked (300 ms wasn't always enough behind a
  // burst of records, and the owner's web switch reported "no_response", 2026-10-08), at most 2 s
  unsigned long t0 = millis();
  do RPULink_Update(millis()); while (RPULink_Pending() && millis() - t0 < 2000UL);
  PumpLink(50);
  Restart();                               // restart through the bootloader, like the OTA abort
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
    SidecarResync();                        // and everything shown on the phone
#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
    CardStatusTick(true);
#endif
  }
  if (up) SidecarTick(now);
#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
  static unsigned long lastCardMs;
  if (up && now - lastCardMs > 2000) {
    lastCardMs = now;
    CardStatusTick(false);
    char line[60];
    if (PinNode400_AudioStatus && PinNode400_AudioStatus(line, sizeof line)) RPULink_QueueText(RPULINK_LOG, line);
  }
#endif
  if (up) PreviewTick();
  DebugTick(now, up);
  linkWasUp = up;

  if (!otaRequested) return;
  otaRequested = false;

#if defined(ARDUINO_ARCH_RP2040) && !defined(PINNODE_HOST_BUILD)
  // PinNode 400: updates come over SWD, whatever the game passes (its false is for the WiFi socket);
  // only in attract or the operator menus
  if (!otaHandler && Rev400HasRpu()) otaHandler = Rev400OtaHandler;
  otaAllowed = curMode == PINNODE_MODE_ATTRACT || curMode == PINNODE_MODE_SERVICE;
#endif
  if (!otaHandler || !otaAllowed) {          // no safe state on this board, or not now
    byte reason = otaHandler ? PINNODE_OTA_REFUSED_GAME_IN_PROGRESS : PINNODE_OTA_REFUSED_NOT_SUPPORTED;
    RPULink_Queue(RPULINK_CTRL_OTA_REFUSED, &reason, 1);
    return;
  }
  EnterOtaSafeState();
  (void)now;
}

#endif
