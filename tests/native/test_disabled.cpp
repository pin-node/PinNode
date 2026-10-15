// PINNODE_DISABLED: every call compiles to nothing, so this links with no library code at all.
#define PINNODE_DISABLED
#include <Arduino.h>
#include <PinNode.h>
unsigned long fake_millis = 0;
const uint8_t *fake_flash = (const uint8_t *)"";
uint32_t fake_flash_len = 0;
int main() {
  static const PinNodeSetting s[] = {{100, 1, 0, 1, 1, false}};
  static const PinNodeAudit a[] = {{10, 4}};
  static const PinNodeAction act[] = {{PINNODE_ACTION_TEST}};
  static const byte mh[8] = {0};
  unsigned long scores[1] = {0}, code = 0;
  uint16_t req = 0;
  PinNodeActionRequest r;
  RPULink_BeginUart(); RPULink_BeginSpi(53); RPULink_Update(0); RPULink_Poll();
  PinNodeCtrl_Settings(s, 1, NULL); PinNodeCtrl_Audits(a, 1); PinNodeCtrl_Actions(act, 1);
  PinNodeCtrl_Begin("1", "x", mh, 4, 0, 4); PinNodeCtrl_DebugTo(NULL);
  PinNodeCtrl_Mode(0, 0, 0); PinNodeCtrl_GameStart(1, false); PinNodeCtrl_PlayerAdded(2);
  PinNodeCtrl_BallStart(1, 1); PinNodeCtrl_BallEnd(1, 1, 0); PinNodeCtrl_GameEnd(scores, 1, 0);
  PinNodeCtrl_Tilt(1, 0); PinNodeCtrl_Coin(1, 1); PinNodeCtrl_Achievement(1, 0); PinNodeCtrl_Stat(1, 0, 0, 1);
  PinNodeCtrl_SettingsChanged(); PinNodeCtrl_AuditsChanged(); PinNodeCtrl_SwitchEvent(1, true);
  PinNodeCtrl_ActionResult(0, 0, 0, 0); PinNodeCtrl_StartResult(0, 0, 0); PinNodeCtrl_Log("x");
  PinNodeCtrl_Update(0, false); PinNodeCtrl_SetOtaHandler(NULL);
  bool any = PinNodeCtrl_TakeStartRequest(&req) || PinNodeCtrl_ClaimCode(1, &code) || PinNodeCtrl_TakeAction(&r) ||
             PinNodeCtrl_SettingLocked(0) || PinNodeCtrl_SettingsLocked() || RPULink_IsUp() || PinNodeCtrl_TimeKnown();
  (void)code; (void)req; (void)r;
  printf("disabled: %s\n", any ? "FAIL" : "ok");
  return any || !RPULink_IsDormant() || PinNodeCtrl_BuildId() != 0 ? 1 : 0;
}
