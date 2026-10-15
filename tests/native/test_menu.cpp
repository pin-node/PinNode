// PinNode_SelfTestOpens / PinNode_OperatorMenuGuard (PinNode.h, RPU games) against a mock menu.
#include <cstdio>
#include <Arduino.h>
#define RPU_OS_H                                   // as if RPU.h were included first
#define RPU_NUMBER_OF_PLAYER_DISPLAYS 4
#define RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET             0x0002
#define RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET         0x0004
#define RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED        0x0010
#define RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN   0x0100
#define RPU_RET_ORIGINAL_CODE_REQUESTED                   0x8000
void RPU_SetDisplay(int, unsigned long, boolean, byte) {}
void RPU_SetDisplayBlank(int, byte) {}
void RPU_SetLampState(int, byte, byte = 0, int = 0) {}
#define SWITCH_STACK_EMPTY 0xFF
void RPU_TurnOffAllLamps() {}
void RPU_CycleAllDisplays(unsigned long, byte = 0, byte = 0xFF) {}
void RPU_PushToSolenoidStack(byte, byte, boolean = false) {}
boolean RPU_ReadSingleSwitchState(byte) { return false; }
byte RPU_PullFirstFromSwitchStack() { return SWITCH_STACK_EMPTY; }
void RPU_EnableSolenoidStack() {}
void RPU_DisableSolenoidStack() {}
#include "PinNode.h"

static int fails = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

// the library side the guard calls, stubbed: one pending action at a time
static bool pending = false;
static PinNodeActionRequest pend;
static byte lastResult = 0xFF;
boolean PinNodeCtrl_TakeAction(PinNodeActionRequest *r) { if (!pending) return false; pending = false; *r = pend; return true; }
void PinNodeCtrl_ActionResult(uint16_t, byte, byte result, long, byte) { lastResult = result; }
void PinNodeCtrl_Actions(const PinNodeAction *, byte) {}
void PinNodeCtrl_SwitchEvent(byte, boolean) {}
long PinNodeCtrl_ClearAudits() { return 0; }
static bool chosen = false;
boolean PinNodeCtrl_OriginalChosen() { return chosen; }
void PinNodeCtrl_SetBootTarget(byte original) { chosen = original != 0; }
byte PinNodeCtrl_ActionFlags(byte action) { return action == 7 ? PINNODE_ACTION_EXIT_DIAG : PINNODE_ACTION_TEST; }

struct MockMenus {
  byte top = 0;
  int exits = 0;
  byte GetTopLevel() { return top; }
  void ExitOperatorMenu() { exits++; }
};

int main() {
  // self-test: one press opens the menu in attract; mid-game it takes two, 0.3 to 3 s apart
  CHECK(PinNode_SelfTestOpens(false, 1000));
  CHECK(!PinNode_SelfTestOpens(true, 5000));
  CHECK(!PinNode_SelfTestOpens(true, 5100));             // a bounce (< 0.3 s) restarts the wait
  CHECK(PinNode_SelfTestOpens(true, 5700));
  CHECK(!PinNode_SelfTestOpens(true, 9000));
  CHECK(!PinNode_SelfTestOpens(true, 13000));            // too late (> 3 s): starts again
  CHECK(PinNode_SelfTestOpens(true, 14000));

  // the blank first page closes itself after 15 s; moving off it resets the clock
  MockMenus m;
  unsigned long t = 100000;
  CHECK(!PinNode_OperatorMenuGuard(m, t));
  m.top = 2; CHECK(!PinNode_OperatorMenuGuard(m, t += 100));
  m.top = 0;
  for (int i = 0; i < 140; i++) CHECK(!PinNode_OperatorMenuGuard(m, t += 100));   // 14 s
  bool closed = false;
  for (int i = 0; i < 30 && !closed; i++) closed = PinNode_OperatorMenuGuard(m, t += 100);
  CHECK(closed && m.exits == 1);
  // reopened later: a fresh visit, not closed at once
  t += 60000;
  CHECK(!PinNode_OperatorMenuGuard(m, t));
  // the phone's exit-diagnostics closes it (DONE); another action is NOT_NOW
  pending = true; pend = {11, 3, PINNODE_OP_START, 0, 0};
  CHECK(!PinNode_OperatorMenuGuard(m, t += 100) && lastResult == PINNODE_ACTION_NOT_NOW);
  pending = true; pend = {12, 7, PINNODE_OP_START, 0, 0};
  CHECK(PinNode_OperatorMenuGuard(m, t += 100) && lastResult == PINNODE_ACTION_DONE && m.exits == 2);

  // boot options: unchanged normally; with the original chosen, original unless Credit/Reset is held
  unsigned long opts = RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED;
  CHECK(PinNode_BootOptions(opts) == opts);
  chosen = true;
  unsigned long o2 = PinNode_BootOptions(opts);
  CHECK(!(o2 & RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET) && (o2 & RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET));
  CHECK((o2 & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) && (o2 & RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED));
  PinNode_BootChosen(RPU_RET_ORIGINAL_CODE_REQUESTED);           // the original booted: choice kept
  CHECK(chosen);
  PinNode_BootChosen(0);                                         // Credit/Reset held: this game, choice cleared
  CHECK(!chosen);

  printf("%d checks, %d failed\n", checks, fails);
  return fails ? 1 : 0;
}
