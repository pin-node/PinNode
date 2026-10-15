// PinNode_Tests (PinNode.h): the phone's diagnostics and tests against a mock RPU OS and library.
#include <cstdio>
#include <vector>
#include <Arduino.h>
#define RPU_OS_H                                   // as if RPU.h were included first
#define RPU_NUMBER_OF_PLAYER_DISPLAYS 4
#define SWITCH_STACK_EMPTY 0xFF
static byte lamps[64], lampFlash[64];
static std::vector<int> fired;                     // solenoid stack pushes
static bool stackOn = false, closedSw[64];
static int switchStack = 0, cycles = 0;
void RPU_SetDisplay(int, unsigned long, boolean, byte) {}
static byte blanks[5];
void RPU_SetDisplayBlank(int d, byte m) { if (d >= 0 && d < 5) blanks[d] = m; }
void RPU_SetLampState(int l, byte s, byte = 0, int f = 0) { lamps[l] = s; lampFlash[l] = f ? 1 : 0; }
void RPU_TurnOffAllLamps() { for (int i = 0; i < 64; i++) lamps[i] = lampFlash[i] = 0; }
void RPU_CycleAllDisplays(unsigned long, byte = 0, byte = 0xFF) { cycles++; }
void RPU_PushToSolenoidStack(byte s, byte, boolean = false) { if (stackOn) fired.push_back(s); }
boolean RPU_ReadSingleSwitchState(byte sw) { return closedSw[sw]; }
byte RPU_PullFirstFromSwitchStack() { if (switchStack) { switchStack--; return 1; } return SWITCH_STACK_EMPTY; }
void RPU_EnableSolenoidStack() { stackOn = true; }
void RPU_DisableSolenoidStack() { stackOn = false; }
#include "PinNode.h"

static int fails = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static bool pending = false;
static PinNodeActionRequest pend;
struct Result { byte action, result; long value; byte reason; };
static std::vector<Result> results;
static byte registered = 0;
static std::vector<std::pair<byte, bool>> switchEvents;
boolean PinNodeCtrl_TakeAction(PinNodeActionRequest *r) { if (!pending) return false; pending = false; *r = pend; return true; }
void PinNodeCtrl_ActionResult(uint16_t, byte a, byte result, long v, byte reason) { results.push_back({a, result, v, reason}); }
void PinNodeCtrl_Actions(const PinNodeAction *, byte n) { registered = n; }
void PinNodeCtrl_SwitchEvent(byte sw, boolean c) { switchEvents.push_back({sw, (bool)c}); }
long PinNodeCtrl_ClearAudits() { return 1234; }
byte PinNodeCtrl_ActionFlags(byte) { return 0; }
boolean PinNodeCtrl_OriginalChosen() { return false; }
void PinNodeCtrl_SetBootTarget(byte) {}

static void ask(byte action, byte op = PINNODE_OP_START, long arg = 0) {
  pend.reqId = 7; pend.action = action; pend.op = op; pend.arg = arg; pend.reason = PINNODE_REASON_STOPPED;
  pending = true;
}
// the game's coil test: 1-3 are coils 5, 6, 7; 4 is unused; 5 is past the end
unsigned short Coil(byte n) { return n == 0 ? 0xFD : n <= 3 ? 4 + n : n == 4 ? 0xFD : 0xFE; }
byte Strength(byte) { return 6; }

int main() {
  const int ATTRACT = 0, GAME = 4;
  PinNode_TestsBegin(Coil, Strength, 0);
  CHECK(registered == 7);                                  // no sound callback: no sound test

  int state = GAME;                                        // in a game nobody takes it (the library says not now)
  ask(PINNODE_TEST_ENTER);
  CHECK(!PinNode_Tests(1000, &state, ATTRACT) && pending && state == GAME);
  pending = false;

  state = ATTRACT;                                         // a test from attract: not now
  ask(PINNODE_TEST_LAMPS, PINNODE_OP_START, 3);
  CHECK(!PinNode_Tests(1000, &state, ATTRACT));
  CHECK(results.back().result == PINNODE_ACTION_NOT_NOW && state == ATTRACT);

  ask(PINNODE_TEST_CLEAR_AUDITS);                          // a oneshot runs in attract
  CHECK(!PinNode_Tests(1000, &state, ATTRACT));
  CHECK(results.back().result == PINNODE_ACTION_DONE && results.back().value == 1234);

  ask(PINNODE_TEST_ENTER);                                 // into PinNode's diagnostics
  CHECK(PinNode_Tests(1100, &state, ATTRACT));
  CHECK(state == PINNODE_MACHINE_STATE_TESTS && stackOn);
  CHECK(results.back().result == PINNODE_ACTION_DONE && results.back().reason == PINNODE_REASON_FINISHED);

  ask(PINNODE_TEST_LAMPS, PINNODE_OP_START, 12);           // one lamp
  CHECK(PinNode_Tests(1200, &state, ATTRACT));
  CHECK(results.back().result == PINNODE_ACTION_OK && results.back().value == 12 && lamps[12] && !lamps[11]);
  ask(PINNODE_TEST_LAMPS, PINNODE_OP_START, 99);           // all of them, flashing
  PinNode_Tests(1300, &state, ATTRACT);
  CHECK(results.back().value == PINNODE_TEST_ALL_LAMPS && lamps[0] && lampFlash[59]);

  ask(PINNODE_TEST_DISPLAYS, PINNODE_OP_START, 0);         // switching test turns the lamps off
  PinNode_Tests(1400, &state, ATTRACT);
  CHECK(!lamps[12] && cycles == 1);
  CHECK(blanks[0] == PINNODE_ALL_DIGITS && blanks[3] == PINNODE_ALL_DIGITS);   // players 3 and 4 too

  fired.clear();                                           // coils in turn: 5, 6, 7, then 5 again (4 unused)
  ask(PINNODE_TEST_SOLENOIDS, PINNODE_OP_START, 0);
  unsigned long t = 2000;
  for (int i = 0; i < 4; i++, t += 2000) PinNode_Tests(t, &state, ATTRACT);
  CHECK(fired.size() == 4 && fired[0] == 5 && fired[1] == 6 && fired[2] == 7 && fired[3] == 5);
  PinNode_Tests(t - 1000, &state, ATTRACT);                // not yet 2 s: nothing more
  CHECK(fired.size() == 4);
  CHECK(results.back().result == PINNODE_ACTION_OK && results.back().value == 1);   // the app is told which coil

  fired.clear();                                           // one coil, repeatedly
  ask(PINNODE_TEST_SOLENOIDS, PINNODE_OP_START, 2);
  PinNode_Tests(t += 2000, &state, ATTRACT);
  PinNode_Tests(t += 2000, &state, ATTRACT);
  CHECK(fired.size() == 2 && fired[0] == 6 && fired[1] == 6);

  closedSw[7] = true;                                      // a switch already closed: reported, not counted
  switchEvents.clear();
  ask(PINNODE_TEST_SWITCHES);
  PinNode_Tests(t += 10, &state, ATTRACT);
  PinNode_Tests(t += 10, &state, ATTRACT);
  closedSw[9] = true;
  PinNode_Tests(t += 10, &state, ATTRACT);
  closedSw[9] = false;
  PinNode_Tests(t += 10, &state, ATTRACT);
  CHECK(switchEvents.size() == 3 && switchEvents[0].first == 7 && switchEvents[1].first == 9 && !switchEvents[2].second);
  ask(PINNODE_TEST_SWITCHES, PINNODE_OP_STOP);
  PinNode_Tests(t += 10, &state, ATTRACT);
  CHECK(results.back().result == PINNODE_ACTION_DONE && results.back().value == 1 &&
        results.back().reason == PINNODE_REASON_STOPPED);

  ask(PINNODE_TEST_SOUNDS);                                // no sound test here
  PinNode_Tests(t += 10, &state, ATTRACT);
  CHECK(results.back().result == PINNODE_ACTION_UNKNOWN);

  ask(PINNODE_TEST_LAMPS, PINNODE_OP_START, 3);            // leaving with a test running: it's reported done
  PinNode_Tests(t += 10, &state, ATTRACT);
  switchStack = 5;
  results.clear();
  ask(PINNODE_TEST_EXIT);
  CHECK(PinNode_Tests(t += 10, &state, ATTRACT));
  CHECK(state == ATTRACT && !stackOn && switchStack == 0 && !lamps[3]);
  CHECK(results.size() == 2 && results[0].action == PINNODE_TEST_LAMPS && results[0].reason == PINNODE_REASON_DIAGNOSTICS &&
        results[1].action == PINNODE_TEST_EXIT && results[1].result == PINNODE_ACTION_DONE);
  CHECK(!PinNode_Tests(t += 10, &state, ATTRACT));        // back to the game's attract

  printf("%d checks, %d failed\n", checks, fails);
  return fails ? 1 : 0;
}
