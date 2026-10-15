// PinNode on an RPU Rev 4 / Rev 102 board: the ESP32 in the WiFi socket on Serial2 (pins 16 and 17).
// The smallest working game side: HELLO, then the link kept alive from loop().
// A real game adds the hooks (GameStart, BallStart/End, GameEnd, settings, audits…):
// see PinNodeCtrl.h.
#include <PinNode.h>

static const byte manifestHash[8] = {0};   // from pin-node-service once the manifest is imported

void setup() {
  RPULink_BeginUart();
  PinNodeCtrl_Begin("0.0.1", "example-uart", manifestHash, 4, 0, 4);   // hwRev 4: RPU Rev 4
}

void loop() {
  unsigned long now = millis();
  RPULink_Update(now);
  PinNodeCtrl_Mode(PINNODE_MODE_ATTRACT, 0, 0);
  PinNodeCtrl_Update(now, false);           // no Mega OTA over the WiFi socket
}
