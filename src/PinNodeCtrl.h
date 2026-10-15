/**************************************************************************
 * PinNodeCtrl: the Mega's side of the RPU-Link control records
 * (strangescience docs/architecture.md §6.3, §8.2).
 *   HELLO (0x42)          protocol 2; sent at start-up and again whenever the link comes back up
 *   LOG (0x30)            debug text for the S3 web UI
 *   CTRL_OTA_REQUEST      -> CTRL_OTA_READY (safe state, RST_ARM raised) or
 *   (0xC0)                   CTRL_OTA_REFUSED (reason)
 *   CTRL_TIME (0xC1)      unix time, for high-score dates
 * Records 0xA0-0xAF (PinNode) are passed to PinNodeCtrl_OnPinNodeRecord (weak).
 **************************************************************************/
#ifndef PINNODE_CTRL_H
#define PINNODE_CTRL_H

#include <Arduino.h>

#define PINNODE_OTA_REFUSED_GAME_IN_PROGRESS  1
#define PINNODE_OTA_REFUSED_NOT_SUPPORTED     2   // no safe state for this build

#define PINNODE_OTA_RESET_TIMEOUT_MS      60000UL // safe state with no reset from the S3: restart

// Mega OTA over the link needs a board-specific safe state (and a reset line the node can pull).
// Boards that support it register a handler; without one, OTA requests are refused.
//   NOTICE: show it (e.g. "UPDATING")   SAFE: coils/K1/lamps off, arm the reset line
//   ABORT:  no reset came within PINNODE_OTA_RESET_TIMEOUT_MS: disarm (the Mega then restarts)
#define PINNODE_OTA_PHASE_NOTICE  0
#define PINNODE_OTA_PHASE_SAFE    1
#define PINNODE_OTA_PHASE_ABORT   2
void PinNodeCtrl_SetOtaHandler(void (*handler)(byte phase));

// Opt-in diagnostics on a serial port of the game's choosing, e.g. PinNodeCtrl_DebugTo(&Serial):
// "node found, link up", "link down", "no node, quiet", and error counters every minute while
// linked. Off unless called; PinNode prints nothing otherwise.
void PinNodeCtrl_DebugTo(Print *out);

// HELLO, protocol 2 (pinnode-spec-docs SPEC-node-link §3.1). Strings are kept by pointer
// (use literals). manifestHash: the 8 bytes of the manifest hash (PinNodeGame.h), or NULL
// until the game has a manifest. buildId (CRC-32 of the program image) is computed here.
// hwRev: the RPU board revision (RPU_OS_HARDWARE_REV: 4, 102, 300 …), sent in HELLO so the node
// only flashes images this board's bootloader will start.
void PinNodeCtrl_Begin(const char *version, const char *rulesetId, const byte *manifestHash,
                       byte maxPlayers, byte settingsCount, uint16_t hwRev);
uint32_t PinNodeCtrl_BuildId();
// Call once per loop(). otaAllowed: true in attract / operator menus / diagnostics only.
// Doesn't return once an OTA is accepted (the S3 resets the Mega).
void PinNodeCtrl_Update(unsigned long now, boolean otaAllowed);
void PinNodeCtrl_Log(const char *text);

// Ask the PinNode network card to do something only a person at the machine should (SPEC-node-link
// 0x46). For an operator-menu entry: the RPU socket ESP32 has no BOOT button.
#define PINNODE_REQ_SETUP          1   // open its Bluetooth setup window for 10 minutes (Change WiFi)
#define PINNODE_REQ_FACTORY_RESET  2   // release it from the machine and wipe WiFi and claim (selling)
#define PINNODE_REQ_RELEASE        3   // release it from the machine (as the website's Release)
void PinNodeCtrl_NodeRequest(byte op);

// ---- PinNode adjustments: the library's own items for the game's adjustments menu. One hook in the
// game (PINNODE_ADJUSTMENTS_MENU_ITEM, or the converter places it) shows them all, and a new library
// version adds new ones to every game without changing it. Stored in EEPROM 4080-4087; saved, and
// acted on, when the operator menu closes (the mode leaves PINNODE_MODE_SERVICE).
#define PINNODE_ADJ_APP_STARTS     0   // Off/On, default On: starts from the PinNode app (PinNode_AppStartsOn)
#define PINNODE_ADJ_WIFI_RESET     1   // Off/On: when the menu closes, the card opens setup mode for
                                       // Change WiFi in the app; then back to Off
#define PINNODE_NUM_ADJUSTMENTS    2
// The item's range and value for the game's menu: values[0] = min, values[1] = max, *value points at
// the library's copy. False when index isn't one of ours.
boolean PinNode_AdjustmentRange(byte index, byte *values, byte **value);
// For a self-test that edits EEPROM bytes directly (RunExtendedSelfTest games): the item's top value
// and its EEPROM address. False when index isn't one of ours.
boolean PinNode_AdjustmentEeprom(byte index, long *maxValue, unsigned short *eeprom);
boolean PinNode_AppStartsOn();
// For an app Settings entry (PinNodeSetting.eeprom) on a library item:
#define PINNODE_EEPROM_APP_STARTS  (PINNODE_EEPROM_ADJUSTMENTS + PINNODE_ADJ_APP_STARTS)
boolean PinNodeCtrl_TimeKnown();
unsigned long PinNodeCtrl_UnixTime(unsigned long now);   // 0 if not known

void PinNodeCtrl_OnPinNodeRecord(byte type, const byte *data, byte len);   // weak, for the PinNode library

// ---- game records (SPEC-node-link §3.1). Slots and balls are 1-based.
// The game counter g (u16) lives in EEPROM and counts up at every GameStart, so a game is
// unique across Mega resets; the node widens it and adds its boot number.
#define PINNODE_EEPROM_GAME_COUNTER   4088     // 2 bytes, clear of RPU's and the game's settings
#define PINNODE_MODE_ATTRACT   0
#define PINNODE_MODE_GAME      1
#define PINNODE_MODE_SERVICE   2
#define PINNODE_MODE_OTA_SAFE  3
#define PINNODE_MODE_ORIGINAL  4   // the board's own CPU runs the original game; the Mega only keeps the link
#define PINNODE_EEPROM_BOOT_TARGET    4090     // 1 byte: 'O' = boot the original game (BOOT_SELECT)
#define PINNODE_EEPROM_ADJUSTMENTS    4080     // 8 bytes (4080-4087): the PinNode adjustments below
// CREDIT_ADD (node 0xA8 -> CREDIT_RESULT 0x38): credits added from the app or web by the owner. The game
// takes them (PinNodeCtrl_TakeCreditAdd, e.g. in attract, its own AddCredit); untaken after 3 s: UNSUPPORTED.
#define PINNODE_CREDIT_OK          0
#define PINNODE_CREDIT_NOT_NOW     1   // not in attract (a game, a test), or one already pending
#define PINNODE_CREDIT_UNSUPPORTED 2   // no verified credit write for this game
#define PINNODE_CREDIT_OVER_MAX    3
#define PINNODE_CREDIT_WRITE_FAIL  4   // the read-back didn't match
boolean PinNodeCtrl_TakeCreditAdd(uint16_t *reqId, byte *n);
// Answers it; on success also sends CREDIT (source 2, app) and the new CREDITS.
void PinNodeCtrl_CreditResult(uint16_t reqId, byte result, byte n, byte credits);
#define PINNODE_BOOT_OK          0   // restarting into the chosen game
#define PINNODE_BOOT_NOT_NOW     1   // a game is in progress (taken in attract, or with the original running)
#define PINNODE_BOOT_UNSUPPORTED 2   // this board can't boot its original CPU on command (RPU rev < 4)
#define PINNODE_BOOT_ALREADY     3
#define PINNODE_END_NORMAL     0
#define PINNODE_END_SLAM       1
#define PINNODE_END_RESET      2
#define PINNODE_TILT_WARNING   0
#define PINNODE_TILT_TILT      1
#define PINNODE_TILT_SLAM      2

void PinNodeCtrl_Mode(byte mode, byte ball, byte slot);   // call every loop: sent on change
void PinNodeCtrl_GameStart(byte credits, boolean freePlay);
void PinNodeCtrl_PlayerAdded(byte slot);
void PinNodeCtrl_BallStart(byte slot, byte ball);
void PinNodeCtrl_BallEnd(byte slot, byte ball, unsigned long score);
#define PINNODE_BALLS_SETTING  1   // ballsSource in GAME_END (optional): read from the setting
#define PINNODE_BALLS_COUNTED  2   //   counted during the game
void PinNodeCtrl_GameEnd(const unsigned long *scores, byte numPlayers, byte endedBy, byte ballsSource = 0);   // once per game
void PinNodeCtrl_Coin(byte chute, byte creditsAfter);

// Start from the app (node START_REQUEST): take it where the game reads its Start button, run
// the same path (credits, free play, max players), and answer with PinNodeCtrl_StartResult.
// A request nobody takes within 500 ms is answered PINNODE_START_NOT_NOW by Update().
#define PINNODE_START_OK           0
#define PINNODE_START_NO_CREDITS   1
#define PINNODE_START_MAX_PLAYERS  2
#define PINNODE_START_NOT_NOW      3
#define PINNODE_START_UNSUPPORTED  4
#define PINNODE_START_APP_OFF      5   // the game's "Starts from the app" setting (app-starts) is off
#define PINNODE_SETTING_OK           0
#define PINNODE_SETTING_OUT_OF_RANGE 1
#define PINNODE_SETTING_NOT_LIVE     2
#define PINNODE_SETTING_LOCKED       3   // a difficulty setting while SETTINGS_LOCK is on (tournament)
#define PINNODE_SETTING_UNKNOWN      4
boolean PinNodeCtrl_TakeStartRequest(uint16_t *reqId);
// The same, honouring the game's app-starts setting: with it off, a request is answered
// PINNODE_START_APP_OFF here and false is returned. An app start then runs exactly the Start
// button's path: it spends a credit unless free play is on.
boolean PinNodeCtrl_TakeAppStart(uint16_t *reqId, boolean appStartsOn);
// The machine's credits for the app (CREDITS 0x35): call every loop; sent on a change and after
// each HELLO, so replays, match, coins and menu changes all show without hooking each one.
void PinNodeCtrl_Credits(byte credits, boolean freePlay);
// Boot the original game or this one (node BOOT_SELECT 0xAC -> BOOT_RESULT 0x36). The choice is
// kept in EEPROM; the library restarts the Mega to apply it. Games use the PinNode_Boot* helpers in
// PinNode.h around RPU_InitializeMPU; these are what they call.
boolean PinNodeCtrl_OriginalChosen();
void PinNodeCtrl_SetBootTarget(byte original);   // 1 = the original game, 0 = this game

// Report under another identity (ruleset, version, manifest hash, build) with the given HELLO flags; the
// HELLO is sent afresh with whatever settings, actions and audits are registered (set them first).
void PinNodeCtrl_SetHello(const char *rulesetId, const char *version, const byte *manifestHash, uint32_t build,
                          byte maxPlayers, byte flags);
// The program sees the node's records first: return true to take one (the library then ignores it)
void PinNodeCtrl_SetRecordHook(boolean (*hook)(byte type, const byte *data, byte len));

// App-adjustable settings: SETTINGS_REPORT (0x2B), SETTING_SET (0xA1) -> SETTING_CHANGED (0x2C).
// Each setting lives in EEPROM like the game's own operator adjustments; the app's value is the
// stored value x scale. A change is written to EEPROM, then applied() lets the game re-read its
// settings (as it does leaving the operator menu). Register before PinNodeCtrl_Begin: HELLO's
// settingsCount becomes count, and the full table is reported after every HELLO.
struct PinNodeSetting {
  unsigned short eeprom;      // EEPROM address
  byte size;                  // 1 = byte, 4 = unsigned long (unsigned long, little-endian, as the RPU OS stores it)
  long minValue, maxValue;    // app units
  unsigned int scale;         // app value = stored x scale (e.g. 100 for a byte counted in hundreds)
  boolean liveSafe;           // may change mid-game (volume yes, ball saves no)
  // optional (an older 6-field table leaves these 0):
  boolean difficulty;         // changes which leaderboard games count on: refused while locked
  long step;                  // app units; 0 = any value in range
};
void PinNodeCtrl_Settings(const PinNodeSetting *table, byte count, void (*applied)(void));
void PinNodeCtrl_SettingsChanged();   // call after the operator menu: re-report (the node diffs)
// SETTINGS_LOCK (0xA7, a tournament): difficulty settings can't change, from the app or from the
// operator menu. The game's menu should check PinNodeCtrl_SettingLocked before saving an edit.
boolean PinNodeCtrl_SettingsLocked();
boolean PinNodeCtrl_SettingLocked(byte index);   // locked and that setting is a difficulty one

// Audits: the game's EEPROM counters (plays, replays, coins per chute, ...), reported as
// AUDIT_SNAPSHOT (0x2D) keyed by table index = the manifest's audits[].index. Sent after every
// HELLO, after each GameEnd, every 24 h, on AUDIT_REQUEST (0xA3), and after AuditsChanged().
struct PinNodeAudit {
  unsigned short eeprom;      // EEPROM address
  byte size;                  // 1 = byte, 4 = unsigned long (unsigned long, little-endian)
};
void PinNodeCtrl_Audits(const PinNodeAudit *table, byte count);
void PinNodeCtrl_AuditsChanged();     // call after the operator menu (audits may have been reset)
long PinNodeCtrl_ClearAudits();       // send them, then zero every registered audit; the first one's value before

// Claim codes (CLAIM_DISPLAY 0xA9, SPEC-node-link §6): after a game the node sends a 6-digit code
// for each slot nobody checked in to. Show it in that slot's score window during attract,
// alternating with the score, until the window ends or the next game starts.
boolean PinNodeCtrl_ClaimCode(byte slot, unsigned long *code);   // slot 1-4; false if none

// Achievements (ACH 0x25) and stats (STAT 0x26), keyed by the manifest's achievements[].index and
// stats[].index (SPEC-node-link §3.1). slot is 1-4. An achievement goes at once (v = optional
// detail). Stats are coalesced in RAM per (slot, stat, op) and sent at BallEnd, GameEnd, or
// 500 ms after the first one (from Update); only counted during a game.
#define PINNODE_STAT_ADD   0
#define PINNODE_STAT_MAX   1
void PinNodeCtrl_Achievement(byte slot, byte ach, long v = 0);
void PinNodeCtrl_Stat(byte slot, byte stat, byte op, long v);

// Operator actions from the phone (SPEC-operator-actions): tests (lamp, display, solenoid, switch,
// sound…) and oneshots (clear audits, factory defaults, presets, enter/exit diagnostics). The game
// declares one PinNodeAction per manifest actions[] entry, in index order; the flags are the
// manifest's (the node enforces local, confirm, lock and busy from the ACTIONS_REPORT this sends
// after every HELLO). The game polls PinNodeCtrl_TakeAction where it can act (attract and its
// diagnostics state), runs or stops the action, and answers with PinNodeCtrl_ActionResult:
//   start of a test:   OK (it's running; the library then expects a keepalive at least every 60 s)
//                      or NOT_NOW (tests only in diagnostics; oneshots never in a game)
//   start, same test, new arg: switch in place, answer OK
//   stop / auto-stop:  stop it, answer DONE with req.reason
//   oneshot:           OK, then DONE (FINISHED) when it has run (or DONE straight away)
//   leaving diagnostics with a test running: DONE with PINNODE_REASON_DIAGNOSTICS
// The library stops a running test by itself (a request with op STOP) after 60 s without a
// keepalive (reason TIMEOUT) or 3 s without the link (reason LINK). A request the game doesn't
// take within 500 ms is answered NOT_NOW.
// Idle exit (owner, 2026-10-03): when the phone put the machine into diagnostics (an ENTER_DIAG
// action answered OK or DONE) and it stays there 5 min with no test running and no action
// received, the library hands the game its own EXIT_DIAG action (reqId 0), so the game leaves
// diagnostics the usual way. Diagnostics entered at the coin door is never timed out.
#define PINNODE_ACTION_TEST          0x01
#define PINNODE_ACTION_LOCAL         0x02
#define PINNODE_ACTION_CONFIRM       0x04
#define PINNODE_ACTION_LOCK          0x08   // refused while SETTINGS_LOCK (presets, factory defaults)
#define PINNODE_ACTION_ENTER_DIAG    0x10
#define PINNODE_ACTION_EXIT_DIAG     0x20
#define PINNODE_OP_START             0
#define PINNODE_OP_STOP              1
#define PINNODE_ACTION_OK            0
#define PINNODE_ACTION_NOT_NOW       1
#define PINNODE_ACTION_UNKNOWN       2
#define PINNODE_ACTION_LOCAL_ONLY    3
#define PINNODE_ACTION_DONE          4
#define PINNODE_REASON_FINISHED      0
#define PINNODE_REASON_STOPPED       1
#define PINNODE_REASON_TIMEOUT       2
#define PINNODE_REASON_DIAGNOSTICS   4
#define PINNODE_REASON_LINK          5   // the library's stop after 3 s without the link: the node
                                         // reports that run's done itself, so this one isn't sent
#define PINNODE_ACTION_KEEPALIVE_MS  60000UL
#define PINNODE_ACTION_LINK_GRACE_MS 3000UL
#define PINNODE_DIAG_IDLE_MS         300000UL
struct PinNodeAction { byte flags; };
struct PinNodeActionRequest {
  uint16_t reqId;
  byte action;                // manifest index
  byte op;                    // PINNODE_OP_START or PINNODE_OP_STOP
  long arg;
  byte reason;                // for a stop: answer DONE with this reason
};
void PinNodeCtrl_Actions(const PinNodeAction *table, byte count);   // register before PinNodeCtrl_Begin
// RPU games: send AUDIT_SNAPSHOT now, before clearing the audits, so the previous values are kept
boolean PinNodeCtrl_SendAuditsNow();
boolean PinNodeCtrl_TakeAction(PinNodeActionRequest *req);
byte PinNodeCtrl_ActionFlags(byte action);   // the registered PINNODE_ACTION_* flags (0 = unknown)
void PinNodeCtrl_ActionResult(uint16_t reqId, byte action, byte result, long value, byte reason = 0);
void PinNodeCtrl_SwitchEvent(byte sw, boolean closed);   // during a switch test only
void PinNodeCtrl_StartResult(uint16_t reqId, byte result, byte slot);
void PinNodeCtrl_Tilt(byte slot, byte kind);
uint16_t PinNodeCtrl_GameCounter();

#endif
