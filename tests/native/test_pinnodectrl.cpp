// Native tests for PinNodeCtrl (the Mega side of the PinNode link), built against host shims
// (shim/) and a fake RPULink that records what the library queues. Run by tests/test_native.py.
#include <vector>
#include <string>
#include <Arduino.h>
#include <EEPROM.h>
#include "RPULink.h"
#include "PinNodeCtrl.h"

unsigned long fake_millis = 1000;
const uint8_t *fake_flash = (const uint8_t *)"";
uint32_t fake_flash_len = 0;
FakeEEPROM EEPROM;
uint8_t SREG;

// ---- fake link
struct Rec { byte type; std::vector<byte> d; };
static std::vector<Rec> sent;
static RPULinkRecordHandler handler;
static bool linkUp = true, linkDormant = false;
boolean RPULink_Queue(byte type, const byte *data, byte len) { sent.push_back({type, std::vector<byte>(data, data + len)}); return true; }
boolean RPULink_QueueText(byte type, const char *t) { return RPULink_Queue(type, (const byte *)t, (byte)strlen(t)); }
boolean RPULink_IsUp() { return linkUp; }
boolean RPULink_IsDormant() { return linkDormant; }
boolean RPULink_Pending() { return false; }
unsigned int RPULink_BadFrames() { return 3; }
unsigned int RPULink_DataOverruns() { return 2; }
unsigned int RPULink_FramingErrors() { return 0; }
unsigned int RPULink_DroppedRecords() { return 1; }
struct CapturePrint : Print { std::string out; size_t write(uint8_t c) override { out += (char)c; return 1; } };
void RPULink_SetFlag(byte, boolean) {}
void RPULink_SetHandler(RPULinkRecordHandler h) { handler = h; }
void RPULink_Update(unsigned long) {}
void RPULink_DropQueued() {}
void PinNodeCtrl_OnPinNodeRecord(byte, const byte *, byte) {}   // the game's optional hook (weak on AVR)

static int fails = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static int count(byte type) { int n = 0; for (auto &r : sent) n += r.type == type; return n; }
static const Rec *last(byte type) { const Rec *p = nullptr; for (auto &r : sent) if (r.type == type) p = &r; return p; }
static long i32(const std::vector<byte> &d, int at) { return (long)(int32_t)((uint32_t)d[at] | (uint32_t)d[at + 1] << 8 | (uint32_t)d[at + 2] << 16 | (uint32_t)d[at + 3] << 24); }
static void inject(byte type, std::vector<byte> d) { handler(type, d.data(), (byte)d.size()); }
static void tick(unsigned long ms) { fake_millis += ms; PinNodeCtrl_Update(fake_millis, false); }

static const PinNodeSetting settings[] = {
  {100, 1, 0, 1, 1, true},                       // 0: bool (free play), live
  {14, 4, 0, 9999990, 1, false, true, 10},       // 1: score, difficulty, step 10
  {101, 1, 0, 100, 1, true, false, 5},           // 2: volume, step 5
};
static const PinNodeAudit audits[] = {{10, 4}, {30, 4}};
static const PinNodeAction actions[] = {
  {PINNODE_ACTION_TEST},                                     // 0 lamp test
  {PINNODE_ACTION_TEST | PINNODE_ACTION_LOCAL},              // 1 solenoid test
  {PINNODE_ACTION_CONFIRM},                                  // 2 clear audits
  {PINNODE_ACTION_CONFIRM | PINNODE_ACTION_LOCK},            // 3 factory defaults
  {PINNODE_ACTION_LOCAL | PINNODE_ACTION_ENTER_DIAG},        // 4 enter diagnostics
  {PINNODE_ACTION_LOCAL | PINNODE_ACTION_EXIT_DIAG},         // 5 exit diagnostics
};

int main() {
  memset(EEPROM.mem, 0xFF, sizeof EEPROM.mem);

  // build id: CRC-32/IEEE, the service's (check value of "123456789")
  fake_flash = (const uint8_t *)"123456789";
  fake_flash_len = 9;
  CHECK(PinNodeCtrl_BuildId() == 0xCBF43926UL);

  // HELLO carries hwRev; the reports follow it
  static const byte mh[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  PinNodeCtrl_Settings(settings, 3, nullptr);
  PinNodeCtrl_Audits(audits, 2);
  PinNodeCtrl_Actions(actions, 6);
  PinNodeCtrl_Begin("1.2.3", "rs", mh, 4, 0, 300);
  const Rec *h = last(0x42);
  CHECK(h && h->d[0] == 2);
  CHECK(h && h->d[h->d.size() - 2] == (300 & 0xFF) && h->d[h->d.size() - 1] == (300 >> 8));
  CHECK(h && h->d[1 + 4 + 8 + 1 + 5 + 1] == 3);                 // settingsCount from the table
  CHECK(count(0x43) == 1 && count(0x2B) == 1);
  const Rec *ar = last(0x34);
  CHECK(ar && ar->d[0] == 6 && ar->d[1] == 0 && ar->d[2] == PINNODE_ACTION_TEST && ar->d[8] == (PINNODE_ACTION_CONFIRM | PINNODE_ACTION_LOCK));
  // an erased bool byte (0xFF) reports as the game reads it: 1
  const Rec *sr = last(0x2B);
  CHECK(sr && i32(sr->d, 3) == 1);
  // the audit snapshot goes from Update, after the HELLO burst; erased counters read 0
  CHECK(count(0x2D) == 0);
  tick(10);
  CHECK(count(0x2D) == 1 && i32(last(0x2D)->d, 3) == 0);

  // settings: range, step, lock
  inject(0xA1, {1, 0, 2, 55, 0, 0, 0});                         // volume 55: on step
  CHECK(last(0x2C)->d[8] == PINNODE_SETTING_OK && EEPROM.mem[101] == 55);
  inject(0xA1, {2, 0, 2, 57, 0, 0, 0});                         // 57: off step
  CHECK(last(0x2C)->d[8] == PINNODE_SETTING_OUT_OF_RANGE);
  inject(0xA7, {1, 0, 0, 0, 0});                                // tournament lock on
  CHECK(PinNodeCtrl_SettingsLocked() && PinNodeCtrl_SettingLocked(1) && !PinNodeCtrl_SettingLocked(2));
  inject(0xA1, {3, 0, 1, 0x40, 0x42, 0x0F, 0});                 // award 1,000,000: difficulty
  CHECK(last(0x2C)->d[8] == PINNODE_SETTING_LOCKED);
  inject(0xA1, {4, 0, 2, 60, 0, 0, 0});                         // volume still fine
  CHECK(last(0x2C)->d[8] == PINNODE_SETTING_OK);
  inject(0xA7, {0, 0, 0, 0, 0});
  inject(0xA1, {5, 0, 1, 0x40, 0x42, 0x0F, 0});
  CHECK(last(0x2C)->d[8] == PINNODE_SETTING_OK && i32(last(0x2C)->d, 1) == 1000000);

  // a game: stats coalesce until BallEnd and go ahead of BALL_END
  PinNodeCtrl_GameStart(1, false);
  PinNodeCtrl_BallStart(1, 1);
  PinNodeCtrl_BallStart(1, 1);                                  // ball save re-serve: one ball_start
  CHECK(count(0x22) == 1);
  size_t mark = sent.size();
  PinNodeCtrl_Stat(1, 0, PINNODE_STAT_ADD, 1);
  PinNodeCtrl_Stat(1, 0, PINNODE_STAT_ADD, 1);
  PinNodeCtrl_Stat(1, 0, PINNODE_STAT_ADD, 1);
  PinNodeCtrl_Stat(1, 1, PINNODE_STAT_MAX, 5);
  PinNodeCtrl_Stat(1, 1, PINNODE_STAT_MAX, 9);
  PinNodeCtrl_Stat(1, 1, PINNODE_STAT_MAX, 7);
  CHECK(count(0x26) == 0);
  PinNodeCtrl_Achievement(1, 3, 42);
  CHECK(last(0x25) && last(0x25)->d[2] == 1 && last(0x25)->d[3] == 3 && i32(last(0x25)->d, 4) == 42);
  PinNodeCtrl_BallEnd(1, 1, 5000);
  int stats = 0; bool statsBeforeEnd = true, seenEnd = false;
  for (size_t i = mark; i < sent.size(); i++) {
    if (sent[i].type == 0x23) seenEnd = true;
    if (sent[i].type == 0x26) {
      stats++;
      if (seenEnd) statsBeforeEnd = false;
      if (sent[i].d[3] == 0) CHECK(sent[i].d[4] == PINNODE_STAT_ADD && i32(sent[i].d, 5) == 3);
      if (sent[i].d[3] == 1) CHECK(sent[i].d[4] == PINNODE_STAT_MAX && i32(sent[i].d, 5) == 9);
    }
  }
  CHECK(stats == 2 && statsBeforeEnd && seenEnd);
  // a stat held 500 ms goes on its own
  PinNodeCtrl_BallStart(1, 2);
  PinNodeCtrl_Stat(1, 0, PINNODE_STAT_ADD, 2);
  tick(100);
  CHECK(count(0x26) == 2);
  tick(450);
  CHECK(count(0x26) == 3);

  // start from the app: CurrentTime from the loop's top can be earlier than the request's stamp
  inject(0xAA, {9, 0, 0});
  PinNodeCtrl_Update(fake_millis - 3, false);                   // same loop: must not time out
  CHECK(count(0x31) == 0);
  uint16_t sreq;
  CHECK(PinNodeCtrl_TakeStartRequest(&sreq) && sreq == 9);
  inject(0xAA, {10, 0, 0});
  tick(600);                                                    // nobody took it
  CHECK(last(0x31) && last(0x31)->d[0] == 10 && last(0x31)->d[2] == PINNODE_START_NOT_NOW);

  // credits from the app: the game takes them (TakeCreditAdd); untaken after 3 s they're unsupported
  inject(0xA8, {11, 0, 2});
  uint16_t creq; byte cn;
  CHECK(PinNodeCtrl_TakeCreditAdd(&creq, &cn) && creq == 11 && cn == 2);
  inject(0xA8, {12, 0, 1});
  tick(10);
  CHECK(!last(0x38) || last(0x38)->d[0] != 12);                 // not yet: the game may still take it
  tick(3100);
  CHECK(last(0x38) && last(0x38)->d[0] == 12 && last(0x38)->d[2] == PINNODE_CREDIT_UNSUPPORTED);
  CHECK(count(0x28) == 0);                                      // no CREDIT record for a refusal
  PinNodeCtrl_CreditResult(13, PINNODE_CREDIT_OK, 2, 17);       // a success: result + CREDIT app
  CHECK(last(0x38)->d[0] == 13 && last(0x38)->d[2] == PINNODE_CREDIT_OK && last(0x38)->d[3] == 17);
  CHECK(last(0x28) && last(0x28)->d[0] == 2 && last(0x28)->d[1] == 2);

  // claim codes: shown until the window ends or a game starts
  unsigned long code = 0;
  unsigned long scores[1] = {5000};
  PinNodeCtrl_GameEnd(scores, 1, PINNODE_END_NORMAL);
  inject(0xA9, {0, 0, 1, 1, 0x40, 0xE2, 0x01, 0x00, 60, 0});    // slot 1, code 123456, 60 s
  CHECK(PinNodeCtrl_ClaimCode(1, &code) && code == 123456);
  CHECK(!PinNodeCtrl_ClaimCode(2, &code));
  fake_millis += 61000;
  CHECK(!PinNodeCtrl_ClaimCode(1, &code));
  // the post-game audit snapshot waits 15 s (it's past that now)
  int audits0 = count(0x2D);
  tick(1);
  CHECK(count(0x2D) == audits0 + 1);
  // a coin: the audits follow 2 s later (the chute counter shows on the phone at once)
  PinNodeCtrl_Coin(3, 4);
  CHECK(last(0x27) && last(0x27)->d[0] == 3);
  int audits1 = count(0x2D);
  tick(1);
  CHECK(count(0x2D) == audits1);
  fake_millis += 2100;
  tick(1);
  CHECK(count(0x2D) == audits1 + 1);

  // operator actions
  PinNodeActionRequest r;
  inject(0xAB, {1, 0, 0, 0, 5, 0, 0, 0});                       // start lamp test, lamp 5
  CHECK(PinNodeCtrl_TakeAction(&r) && r.reqId == 1 && r.action == 0 && r.op == PINNODE_OP_START && r.arg == 5);
  PinNodeCtrl_ActionResult(r.reqId, r.action, PINNODE_ACTION_OK, 5);
  CHECK(last(0x32)->d[3] == PINNODE_ACTION_OK && last(0x32)->d.size() == 8);
  PinNodeCtrl_SwitchEvent(7, true);                             // a test runs: switch events go
  CHECK(last(0x33) && last(0x33)->d[0] == 7 && last(0x33)->d[1] == 1);
  for (int i = 0; i < 5; i++) { tick(50000); inject(0xAB, {0, 0, 0, 2}); }   // keepalives: still running
  CHECK(!PinNodeCtrl_TakeAction(&r));
  tick(61000);                                                  // no keepalive for 60 s: the library stops it
  CHECK(PinNodeCtrl_TakeAction(&r) && r.op == PINNODE_OP_STOP && r.reqId == 0 && r.action == 0 && r.reason == PINNODE_REASON_TIMEOUT);
  PinNodeCtrl_ActionResult(r.reqId, r.action, PINNODE_ACTION_DONE, 0, r.reason);
  CHECK(last(0x32)->d[3] == PINNODE_ACTION_DONE && last(0x32)->d.size() == 9 && last(0x32)->d[8] == PINNODE_REASON_TIMEOUT);
  int sw0 = count(0x33);
  PinNodeCtrl_SwitchEvent(7, false);                            // no test: nothing
  CHECK(count(0x33) == sw0);
  // link down 3 s stops a running test
  inject(0xAB, {2, 0, 1, 0, 0, 0, 0, 0});
  CHECK(PinNodeCtrl_TakeAction(&r));
  PinNodeCtrl_ActionResult(r.reqId, r.action, PINNODE_ACTION_OK, 0);
  linkUp = false;
  tick(10);                                                     // Update runs every loop: the drop is seen at once
  tick(2000);
  CHECK(!PinNodeCtrl_TakeAction(&r));                           // under 3 s: a blip, keep going
  tick(1100);
  CHECK(PinNodeCtrl_TakeAction(&r) && r.op == PINNODE_OP_STOP && r.action == 1 && r.reason == PINNODE_REASON_LINK);
  int results0 = count(0x32);
  PinNodeCtrl_ActionResult(r.reqId, r.action, PINNODE_ACTION_DONE, 0, r.reason);
  CHECK(count(0x32) == results0);                               // the node reports a link-loss done, not the Mega
  int sw1 = count(0x33);
  PinNodeCtrl_SwitchEvent(1, true);                             // and the test is over
  CHECK(count(0x33) == sw1);
  linkUp = true;
  // unknown index; a request the game doesn't take is answered not now
  inject(0xAB, {3, 0, 9, 0, 0, 0, 0, 0});
  CHECK(last(0x32)->d[0] == 3 && last(0x32)->d[3] == PINNODE_ACTION_UNKNOWN);
  inject(0xAB, {4, 0, 2, 0, 0, 0, 0, 0});
  tick(600);
  CHECK(last(0x32)->d[0] == 4 && last(0x32)->d[3] == PINNODE_ACTION_NOT_NOW);

  // idle exit: diagnostics entered from the phone, 5 min with nothing: the game gets its exit action
  inject(0xAB, {5, 0, 4, 0, 0, 0, 0, 0});                       // enter diagnostics
  CHECK(PinNodeCtrl_TakeAction(&r) && r.action == 4);
  PinNodeCtrl_ActionResult(r.reqId, r.action, PINNODE_ACTION_DONE, 0, PINNODE_REASON_FINISHED);
  PinNodeCtrl_Mode(PINNODE_MODE_SERVICE, 0, 0);
  tick(200000);
  CHECK(!PinNodeCtrl_TakeAction(&r));
  inject(0xAB, {0, 0, 0, 2});                                   // any action resets the idle clock
  tick(200000);
  CHECK(!PinNodeCtrl_TakeAction(&r));
  tick(101000);
  CHECK(PinNodeCtrl_TakeAction(&r) && r.reqId == 0 && r.action == 5 && r.op == PINNODE_OP_START);
  PinNodeCtrl_ActionResult(r.reqId, r.action, PINNODE_ACTION_DONE, 0, PINNODE_REASON_FINISHED);
  PinNodeCtrl_Mode(PINNODE_MODE_ATTRACT, 0, 0);
  // diagnostics from the coin door: never timed out
  PinNodeCtrl_Mode(PINNODE_MODE_SERVICE, 0, 0);
  tick(400000);
  CHECK(!PinNodeCtrl_TakeAction(&r));
  PinNodeCtrl_Mode(PINNODE_MODE_ATTRACT, 0, 0);

  // opt-in debug output on the game's serial port
  CapturePrint dbg;
  PinNodeCtrl_DebugTo(&dbg);
  linkUp = false; tick(10);
  CHECK(dbg.out.find("link down") != std::string::npos);
  linkDormant = true; tick(10);
  CHECK(dbg.out.find("no node, quiet") != std::string::npos);
  linkDormant = false; linkUp = true; tick(10);
  CHECK(dbg.out.find("node found, link up") != std::string::npos);
  tick(61000);
  CHECK(dbg.out.find("bad frames 3, overruns 2, framing 0, dropped 1") != std::string::npos);
  PinNodeCtrl_DebugTo(NULL);

  // app starts (app-starts setting): Off answers APP_OFF here; On hands the request to the game
  inject(0xAA, {20, 0, 0});
  uint16_t areq = 0;
  CHECK(!PinNodeCtrl_TakeAppStart(&areq, false));
  CHECK(last(0x31) && last(0x31)->d[0] == 20 && last(0x31)->d[2] == PINNODE_START_APP_OFF);
  inject(0xAA, {21, 0, 0});
  CHECK(PinNodeCtrl_TakeAppStart(&areq, true) && areq == 21);
  CHECK(!PinNodeCtrl_TakeAppStart(&areq, true));               // nothing pending
  // credits for the app (CREDITS 0x35): sent on a change only
  int c0 = count(0x35);
  PinNodeCtrl_Credits(3, false);
  PinNodeCtrl_Credits(3, false);
  CHECK(count(0x35) == c0 + 1 && last(0x35)->d[0] == 3 && last(0x35)->d[1] == 0);
  PinNodeCtrl_Credits(2, false);
  PinNodeCtrl_Credits(2, true);
  CHECK(count(0x35) == c0 + 3 && last(0x35)->d[1] == 1);

  // BOOT_SELECT (0xAC) -> BOOT_RESULT (0x36): the answers that don't restart the Mega
  inject(0xAC, {30, 0, 1});                                       // original, on a board rev 300
  PinNodeCtrl_Update(millis(), false);
  CHECK(last(0x36) && last(0x36)->d[0] == 30 && last(0x36)->d[3] == PINNODE_BOOT_UNSUPPORTED);
  inject(0xAC, {31, 0, 0});                                       // this game: already running it
  PinNodeCtrl_Update(millis(), false);
  CHECK(last(0x36)->d[0] == 31 && last(0x36)->d[3] == PINNODE_BOOT_ALREADY);
  PinNodeCtrl_Begin("1.0.0", "rpu-test", NULL, 4, 0, 4);          // an RPU Rev 4 board
  PinNodeCtrl_Mode(PINNODE_MODE_GAME, 1, 1);
  inject(0xAC, {32, 0, 1});                                       // original, mid-game: not now
  PinNodeCtrl_Update(millis(), false);
  CHECK(last(0x36)->d[0] == 32 && last(0x36)->d[2] == 1 && last(0x36)->d[3] == PINNODE_BOOT_NOT_NOW);
  CHECK(!PinNodeCtrl_OriginalChosen());
  PinNodeCtrl_SetBootTarget(1);
  CHECK(PinNodeCtrl_OriginalChosen());
  PinNodeCtrl_SetBootTarget(0);
  CHECK(!PinNodeCtrl_OriginalChosen());

  // the record hook sees the node's records first; SetHello re-sends HELLO under another identity
  static int hooked = 0;
  PinNodeCtrl_SetRecordHook([](byte type, const byte *, byte) -> boolean { if (type == 0xA1) { hooked++; return true; } return false; });
  int changed0 = count(0x2C);
  inject(0xA1, {40, 0, 0, 5, 0, 0, 0});                         // SETTING_SET: taken by the hook
  CHECK(hooked == 1 && count(0x2C) == changed0);
  PinNodeCtrl_SetRecordHook(NULL);
  int hellos0 = count(0x42);
  static const byte mh2[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  PinNodeCtrl_SetHello("other-ruleset", "2.0.0", mh2, 0x12345678UL, 4, 0x01);
  CHECK(count(0x42) == hellos0 + 1);
  CHECK(PinNodeCtrl_GameCounter() > 0);

  printf("%d checks, %d failed\n", checks, fails);
  return fails ? 1 : 0;
}
