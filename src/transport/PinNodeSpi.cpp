/**************************************************************************
 * RPU-Link over SPI (the PinNode network board, hardware rev 300: the Mega is
 * SPI master, the ESP32-S3 slave). Linked only when the game calls
 * RPULink_BeginSpi().
 **************************************************************************/
#if defined(ARDUINO_ARCH_AVR)

#include <SPI.h>
#include "../RPULink.h"
#include "../RPULinkInternal.h"

static byte linkCsPin = 53;

static void SpiPoll() {
  byte tx[RPULINK_FRAME_LEN], rx[RPULINK_FRAME_LEN];
  RPULink_BuildFrame(tx);
  SPI.beginTransaction(SPISettings(RPULINK_SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(linkCsPin, LOW);
  for (byte i = 0; i < RPULINK_FRAME_LEN; i++) rx[i] = SPI.transfer(tx[i]);
  digitalWrite(linkCsPin, HIGH);
  SPI.endTransaction();
  // the reply to frame N arrives in frame N+1; validate whatever came back
  if (!RPULink_FrameValid(rx)) { RPULink_CountBadFrame(); return; }
  RPULink_HandleFrame(rx);
}

static const RPULinkTransport spiTransport = {SpiPoll, NULL};

void RPULink_BeginSpi(byte csPin) {
  linkCsPin = csPin;
  pinMode(linkCsPin, OUTPUT);
  digitalWrite(linkCsPin, HIGH);
  pinMode(53, OUTPUT);            // SS must be an output for the AVR to stay SPI master
  SPI.begin();
  RPULink_Attach(&spiTransport);
}

#endif
