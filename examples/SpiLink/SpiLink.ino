// PinNode over SPI, for future PinNode boards that link that way (no board uses it yet;
// RPU Rev 4 and 102 use the UartLink example). SPI to the board's ESP32-S3.
// The board's OTA safe state is game code (it knows the hardware): register it, and the
// node can then reflash the Mega over WiFi when the game says it's safe.
#include <PinNode.h>

static const byte manifestHash[8] = {0};   // from pin-node-service once the manifest is imported

static void OtaSafeState(byte phase) {
  // NOTICE: show "UPDATING"; SAFE: coils, K1 and lamps off, arm the reset line; ABORT: disarm.
  // The board's own support code supplies this.
  (void)phase;
}

void setup() {
  RPULink_BeginSpi(53);                     // D53 = PB0 = MEGA_CS
  PinNodeCtrl_SetOtaHandler(OtaSafeState);
  PinNodeCtrl_Begin("0.0.1", "example-spi", manifestHash, 4, 0, 300);
}

void loop() {
  unsigned long now = millis();
  RPULink_Update(now);
  PinNodeCtrl_Mode(PINNODE_MODE_ATTRACT, 0, 0);
  PinNodeCtrl_Update(now, true);            // attract: a Mega OTA may run
}
