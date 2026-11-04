/**************************************************************************
 * RPU-Link over SPI, for future PinNode boards that use it (the Mega is SPI
 * master, the board's ESP32-S3 the slave). Linked only when the game calls
 * RPULink_BeginSpi().
 *
 * PinNode 400 (RP2350B): the RP is master, the XIAO ESP32-S3 the slave, on
 * SPI0. 400-J5: GPIO34 SCK, GPIO35 MOSI, GPIO36 MISO, GPIO37 CS, GPIO38 ATTN.
 * 400-IC: GPIO38 SCK, GPIO35 MOSI, GPIO36 MISO, GPIO37 CS, GPIO39 ATTN (its
 * GPIO34 is the CPU socket's RESET); RPU Rev 401 says which by defining
 * PinNode400_Board(). The 400 has no UART link, so RPULink_BeginUart() starts
 * this one too: games don't change.
 **************************************************************************/
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_RP2040)

#include <SPI.h>
#if defined(ARDUINO_ARCH_RP2040)
#include "hardware/dma.h"
#include "hardware/spi.h"
#endif
#include "../RPULink.h"
#include "../RPULinkInternal.h"

#if defined(ARDUINO_ARCH_RP2040)
#define PINNODE_400_MOSI  35
#define PINNODE_400_MISO  36
#define PINNODE_400_CS    37
// RPU Rev 401 (the 400-IC) defines this to return 401; the 400-J5 (Rev 400) doesn't define it
extern "C" int PinNode400_Board() __attribute__((weak));
static byte linkSckPin = 34;
static byte linkAttnPin = 38;   // node -> RP: a command is waiting (bulk mode); pulled up (RPU.cpp)
static byte linkCsPin = PINNODE_400_CS;
#else
static byte linkCsPin = 53;
#endif

#if defined(ARDUINO_ARCH_RP2040)
// ---- PinNode 400 bulk mode (PinNodeBulk.h): files to and from the RP's SD card
#include "../PinNodeBulk.h"
void PinNode_Log(const char *fmt, ...);   // PinNodeCtrl.cpp
extern "C" int PinNode400_FileOp(unsigned char op, const char *name, unsigned long arg0, unsigned long arg1,
                                 const unsigned char *data, unsigned int len, unsigned char *out,
                                 unsigned int *outLen, unsigned long *r0, unsigned long *r1) __attribute__((weak));
boolean PinNodeCtrl_BulkAllowed();           // PinNodeCtrl.cpp: attract or the operator menus

#define BULK_KEEPALIVE_MS 20           // an idle session still exchanges this often
#define BULK_PASS_MS 250               // the most one loop pass gives the bulk link (outside games only)
static boolean bulkActive;
static unsigned long bulkLastMs, bulkRetryAtMs;
static byte bulkSeq, bulkStatus, bulkOpEcho;
static unsigned long bulkA0Echo, bulkOutCrc;
static unsigned long bulkR0, bulkR1;
static unsigned int bulkOutLen;
static byte bulkTx[BULK_LEN], bulkRx[BULK_LEN];
static byte bulkOut[BULK_DATA];
static boolean bulkReplyStale = true;     // bulkTx needs rebuilding (a new reply)

static uint32_t Crc32(const byte *p, unsigned int n) {   // IEEE, a table: 4 KB frames, several a pass
  static uint32_t table[256];
  if (!table[1])
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t c = i;
      for (byte k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320UL & (0UL - (c & 1)));
      table[i] = c;
    }
  uint32_t c = 0xFFFFFFFFUL;
  while (n--) c = table[(c ^ *p++) & 0xFF] ^ (c >> 8);
  return ~c;
}
static uint32_t Le32(const byte *p) { return p[0] | p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void Put32(byte *p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }

static boolean BulkFrameValid(const byte *f) {
  if (f[0] != BULK_MAGIC || f[1] != BULK_VERSION) return false;
  return Crc32(f, BULK_HDR + BULK_DATA) == Le32(f + BULK_HDR + BULK_DATA);
}

static void BulkReply() {
  if (!bulkReplyStale) return;
  bulkReplyStale = false;
  memset(bulkTx, 0, BULK_HDR);
  bulkTx[0] = BULK_MAGIC; bulkTx[1] = BULK_VERSION; bulkTx[2] = bulkSeq; bulkTx[3] = bulkStatus;
  Put32(bulkTx + 4, bulkR0); Put32(bulkTx + 8, bulkR1);
  bulkTx[12] = bulkOutLen; bulkTx[13] = bulkOutLen >> 8;
  bulkTx[16] = bulkOpEcho; Put32(bulkTx + 20, bulkA0Echo);   // what this reply answers (the node checks)
  Put32(bulkTx + 24, bulkOutCrc);                             // the data's own CRC, as the RP made it
  memcpy(bulkTx + BULK_HDR, bulkOut, BULK_DATA);
  Put32(bulkTx + BULK_HDR + BULK_DATA, Crc32(bulkTx, BULK_HDR + BULK_DATA));
}

static void BulkRun(const byte *f) {
  byte op = f[3];
  char name[BULK_NAME_MAX];
  memcpy(name, f + 16, BULK_NAME_MAX);
  name[BULK_NAME_MAX - 1] = 0;
  unsigned int len = f[12] | f[13] << 8;
  if (len > BULK_DATA) len = BULK_DATA;
  bulkOutLen = 0; bulkR0 = bulkR1 = 0;
  if (op == BULK_OP_END) { bulkStatus = BULK_OK; bulkActive = false; }
  else if (op == BULK_OP_NOP) bulkStatus = BULK_OK;
  else if (!PinNodeCtrl_BulkAllowed()) bulkStatus = BULK_E_REFUSED;
  else if (op == BULK_OP_WRITE && len && Le32(f + 8) && Crc32(f + BULK_HDR, len) != Le32(f + 8)) {   // end to end
    PinNode_Log("bulk: op %u a0 %lu: data CRC mismatch", op, (unsigned long)Le32(f + 4));
    bulkStatus = BULK_E_IO;
  } else bulkStatus = (byte)PinNode400_FileOp(op, name, Le32(f + 4), Le32(f + 8), f + BULK_HDR, len, bulkOut,
                                            &bulkOutLen, &bulkR0, &bulkR1);
  bulkOutCrc = bulkOutLen ? Crc32(bulkOut, bulkOutLen) : 0;
  bulkSeq = f[2];
  bulkOpEcho = op; bulkA0Echo = Le32(f + 4);
  bulkReplyStale = true;
}

// One frame by DMA: the SDK's copy loop managed about half the wire's 15 MHz for a frame this size. Our
// own two channels, claimed once, with no interrupt: SPI.transferAsync() hooks the shared DMA_IRQ_0,
// which the 400's I2S sound output uses from the other core, and the sound stopped after the first
// file transfer (2026-10-20). Waits by polling; falls back to the copy loop if no channel is free.
static int bulkDmaTx = -1, bulkDmaRx = -1;
static void BulkTransfer() {
  if (bulkDmaTx < 0) {
    bulkDmaTx = dma_claim_unused_channel(false);
    bulkDmaRx = bulkDmaTx < 0 ? -1 : dma_claim_unused_channel(false);
    if (bulkDmaRx < 0 && bulkDmaTx >= 0) { dma_channel_unclaim(bulkDmaTx); bulkDmaTx = -2; }
    if (bulkDmaTx < 0) bulkDmaTx = -2;                  // none free: the copy loop from now on
  }
  if (bulkDmaTx == -2) { SPI.transfer(bulkTx, bulkRx, BULK_LEN); return; }
  spi_hw_t *hw = spi_get_hw(spi0);
  dma_channel_config c = dma_channel_get_default_config(bulkDmaTx);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  channel_config_set_dreq(&c, spi_get_dreq(spi0, true));
  channel_config_set_irq_quiet(&c, true);
  dma_channel_configure(bulkDmaTx, &c, &hw->dr, bulkTx, BULK_LEN, false);
  c = dma_channel_get_default_config(bulkDmaRx);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);
  channel_config_set_dreq(&c, spi_get_dreq(spi0, false));
  channel_config_set_irq_quiet(&c, true);
  dma_channel_configure(bulkDmaRx, &c, bulkRx, &hw->dr, BULK_LEN, false);
  hw->dmacr = SPI_SSPDMACR_TXDMAE_BITS | SPI_SSPDMACR_RXDMAE_BITS;
  dma_start_channel_mask((1u << bulkDmaTx) | (1u << bulkDmaRx));
  dma_channel_wait_for_finish_blocking(bulkDmaRx);       // the last byte in means the last byte out
  hw->dmacr = 0;                                          // the 64-byte frames use the copy loop
}

// every loop pass while the session is open (the transport's drain hook)
static void SpiBulkPump() {
  if (!bulkActive) return;
  unsigned long now = millis();
  if (now - bulkLastMs > BULK_IDLE_MS || !PinNodeCtrl_BulkAllowed()) {   // the node went quiet, or a game began
    bulkActive = false;
    if (PinNode400_FileOp) PinNode400_FileOp(BULK_OP_END, "", 0, 0, NULL, 0, bulkOut, &bulkOutLen, &bulkR0, &bulkR1);
    return;
  }
  if ((long)(now - bulkRetryAtMs) < 0) return;
  // LINK_ATTN says when the node's loaded frame carries a command not yet answered: exchange then, as
  // many as come within BULK_PASS_MS (one op is command in, reply out), with a wait (up to 10 ms) for the
  // node's turnaround between ops; otherwise only a keep-alive. (A node that doesn't drive it reads high,
  // pulled up: the RP then exchanges continuously, as before LINK_ATTN.)
  int misses = 0;
  do {
    if (!digitalRead(linkAttnPin)) {
      unsigned long w = micros();
      while (!digitalRead(linkAttnPin) && micros() - w < 10000) {}
      if (!digitalRead(linkAttnPin) && millis() - bulkLastMs < BULK_KEEPALIVE_MS && !bulkReplyStale) return;
    }
    BulkReply();
    SPI.beginTransaction(SPISettings(BULK_SPI_HZ, MSBFIRST, SPI_MODE0));
    digitalWrite(linkCsPin, LOW);
    BulkTransfer();
    digitalWrite(linkCsPin, HIGH);
    SPI.endTransaction();
    if (!BulkFrameValid(bulkRx)) {                                     // the node hadn't a frame ready
      if (++misses > 3) { bulkRetryAtMs = millis() + 2; return; }
      delayMicroseconds(300);
      continue;
    }
    bulkLastMs = millis();
    if (bulkRx[2] != bulkSeq) BulkRun(bulkRx);                         // a new command
    if (!bulkActive) {
      if (PinNode400_FileOp) PinNode400_FileOp(BULK_OP_END, "", 0, 0, NULL, 0, bulkOut, &bulkOutLen, &bulkR0, &bulkR1);
      return;
    }
  } while (millis() - now < BULK_PASS_MS);
}

// the node put BULK_MAGIC first in a 64-byte frame: open a session if the game allows
static boolean BulkAsked(const byte *rx) {
  if (rx[0] != BULK_MAGIC) return false;
  if (PinNode400_FileOp && PinNodeCtrl_BulkAllowed()) {
    bulkActive = true;
    bulkLastMs = millis();
    bulkRetryAtMs = 0;
    bulkSeq = 0; bulkStatus = BULK_OK; bulkOutLen = 0; bulkR0 = bulkR1 = 0;
    bulkReplyStale = true;
  }
  return true;
}
#endif

static void SpiPoll() {
#if defined(ARDUINO_ARCH_RP2040)
  if (bulkActive) return;                    // the bulk frames keep the link alive meanwhile
#endif
  byte tx[RPULINK_FRAME_LEN], rx[RPULINK_FRAME_LEN];
  RPULink_BuildFrame(tx);
  SPI.beginTransaction(SPISettings(RPULINK_SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(linkCsPin, LOW);
  for (byte i = 0; i < RPULINK_FRAME_LEN; i++) rx[i] = SPI.transfer(tx[i]);
  digitalWrite(linkCsPin, HIGH);
  SPI.endTransaction();
  // the reply to frame N arrives in frame N+1; validate whatever came back
#if defined(ARDUINO_ARCH_RP2040)
  if (BulkAsked(rx)) return;
#endif
  if (!RPULink_FrameValid(rx)) { RPULink_CountBadFrame(); return; }
  RPULink_HandleFrame(rx);
}

#if defined(ARDUINO_ARCH_RP2040)
static const RPULinkTransport spiTransport = {SpiPoll, SpiBulkPump};
#else
static const RPULinkTransport spiTransport = {SpiPoll, NULL};
#endif

void RPULink_BeginSpi(byte csPin) {
#if defined(ARDUINO_ARCH_RP2040)
  if (PinNode400_Board && PinNode400_Board() == 401) { linkSckPin = 38; linkAttnPin = 39; }   // the 400-IC
  linkCsPin = csPin == 53 ? PINNODE_400_CS : csPin;   // 53 is the header's Mega default
  pinMode(linkAttnPin, INPUT_PULLUP);    // high unless the node drives it: never low while the XIAO boots
  pinMode(linkCsPin, OUTPUT);
  digitalWrite(linkCsPin, HIGH);
  SPI.setSCK(linkSckPin);
  SPI.setTX(PINNODE_400_MOSI);
  SPI.setRX(PINNODE_400_MISO);
  SPI.begin();
#else
  linkCsPin = csPin;
  pinMode(linkCsPin, OUTPUT);
  digitalWrite(linkCsPin, HIGH);
  pinMode(53, OUTPUT);            // SS must be an output for the AVR to stay SPI master
  SPI.begin();
#endif
  RPULink_Attach(&spiTransport);
}

#if defined(ARDUINO_ARCH_RP2040)
// The 400 has no UART link to the XIAO (its UART pins would be the 6800's data bus): the SPI link
// it is, whichever Begin the game calls
void RPULink_BeginUart(unsigned long) { RPULink_BeginSpi(PINNODE_400_CS); }
#endif

#endif
