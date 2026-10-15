// Native test of the RPU-Link core: dormancy when no node answers, waking on a valid frame.
#include <Arduino.h>
#include "RPULink.h"
#include "RPULinkInternal.h"

unsigned long fake_millis = 1000;
const uint8_t *fake_flash = (const uint8_t *)"";
uint32_t fake_flash_len = 0;
uint8_t SREG;
void LinkAudio_OnRecord(byte, const byte *, byte) {}   // optional hook (weak on AVR)

static int polls = 0;
static void FakePoll() { polls++; byte tx[RPULINK_FRAME_LEN]; RPULink_BuildFrame(tx); }
static const RPULinkTransport fake = {FakePoll, NULL};

static int fails = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void run(unsigned long ms) {             // loop() passes every millisecond
  for (unsigned long i = 0; i < ms; i++) { fake_millis++; RPULink_Update(fake_millis); }
}

static void validFrame(byte ack = 0) {
  byte rx[RPULINK_FRAME_LEN] = {0};
  rx[0] = RPULINK_MAGIC; rx[1] = RPULINK_VERSION; rx[3] = ack;
  uint16_t crc = RPULink_CRC16(rx, 62);
  rx[62] = crc >> 8; rx[63] = crc & 0xFF;
  CHECK(RPULink_FrameValid(rx));
  RPULink_HandleFrame(rx);
}

int main() {
  RPULink_Attach(&fake);
  byte d[4] = {1, 2, 3, 4};
  CHECK(RPULink_Queue(0x20, d, 4));               // at boot, records queue while the node starts
  run(1000);
  CHECK(!RPULink_IsDormant() && polls >= 190);     // a frame every 5 ms
  run(9200);
  CHECK(RPULink_IsDormant());                      // 10 s with no node: quiet
  CHECK(RPULink_Queue(0x21, d, 4));                // records still queue (a node may come back)
  polls = 0;
  run(10000);
  CHECK(polls >= 4 && polls <= 6);                 // a probe every 2 s
  validFrame();                                    // a node answers
  CHECK(!RPULink_IsDormant() && RPULink_IsUp());
  {                                                // and gets what was queued while it was away
    byte tx[RPULINK_FRAME_LEN];
    RPULink_BuildFrame(tx);
    CHECK(tx[5] >= 6 && tx[6] == 0x20);            // the boot record, still in flight
    CHECK(RPULink_Pending());                      // in flight, not acked yet
    validFrame(tx[2]);                             // the node acks it
    RPULink_BuildFrame(tx);
    CHECK(tx[5] >= 6 && tx[6] == 0x21);            // then the one queued while quiet
    CHECK(RPULink_Pending());
    validFrame(tx[2]);                             // acked too, and nothing queued
    RPULink_BuildFrame(tx);
    CHECK(!RPULink_Pending());
  }
  CHECK(RPULink_Queue(0x20, d, 4));
  polls = 0;
  run(100);
  CHECK(polls >= 19);                              // back to every 5 ms
  run(9000);
  CHECK(!RPULink_IsDormant());                     // under 10 s since the last frame
  run(1500);
  CHECK(RPULink_IsDormant());                      // node gone 10 s: quiet again
  printf("%d checks, %d failed\n", checks, fails);
  return fails ? 1 : 0;
}
