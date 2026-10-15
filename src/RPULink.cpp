/**************************************************************************
 * RPU-Link core: frames, record queues, dispatch. See RPULink.h for the wire
 * format. The transport (UART or SPI) lives in transport/ and is chosen by
 * calling RPULink_BeginUart() or RPULink_BeginSpi(); only that one is linked.
 **************************************************************************/
#if defined(ARDUINO_ARCH_AVR) || defined(PINNODE_HOST_BUILD)

#include "RPULink.h"
#include "RPULinkInternal.h"

static RPULinkRecordHandler linkHandler = NULL;
static const RPULinkTransport *transport = NULL;
volatile unsigned int rpuLinkRxOverruns = 0;
volatile unsigned long rpuLinkRxBytes = 0, rpuLinkTxBytes = 0;    // UART bring-up counters
volatile unsigned int rpuLinkFramingErrors = 0, rpuLinkDataOverruns = 0;

// Mega -> S3 record queues: byte rings of whole records [type][len][data...]. Audio records
// (0x10-0x17) have their own ring and go into each frame first, so a burst of PinNode
// records never delays a sound (SPEC-node-link §4).
struct LinkRing {
  byte *buf;
  unsigned int size, head, tail, used;
};
static byte txQueue[RPULINK_QUEUE_BYTES];
static byte txAudioQueue[RPULINK_AUDIO_QUEUE_BYTES];
static LinkRing txRing = {txQueue, RPULINK_QUEUE_BYTES, 0, 0, 0};
static LinkRing txAudio = {txAudioQueue, RPULINK_AUDIO_QUEUE_BYTES, 0, 0, 0};

// payload currently in flight (resent until acked)
static byte txPayload[RPULINK_PAYLOAD_MAX];
static byte txPayloadLen = 0;
static byte txSeq = 0;            // seq of txPayload; 0 = nothing sent yet
static byte peerAck = 0;          // last ack the S3 reported
static byte rxLastSeq = 0;        // last S3 seq we processed
static uint16_t rxLastCrc = 0;    // CRC-16 of that payload
static byte txFlags = 0;

static unsigned long lastFrameMs = 0;
static unsigned long lastGoodRxMs = 0;
static boolean everGood = false;
// Dormant: no node answered for RPULINK_DORMANT_AFTER_MS (none fitted, unplugged, rebooting or
// updating). Only the frame rate drops, to one probe every RPULINK_PROBE_MS; records still queue
// (up to the queue's size) and nothing is thrown away, so a node that comes back gets them. The
// first valid frame wakes the link, and PinNodeCtrl then sends HELLO as after any link loss.
static boolean dormant = false;
static unsigned long quietSinceMs = 0;
static unsigned int droppedRecords = 0;
static unsigned int badFrames = 0;

// forward to the audio shim (LinkAudio.cpp); weak so the link builds without it
void LinkAudio_OnRecord(byte type, const byte *data, byte len) __attribute__((weak));


uint16_t RPULink_CRC16(const byte *data, unsigned int len) {
  uint16_t crc = 0xFFFF;
  for (unsigned int i = 0; i < len; i++) {
    crc ^= ((uint16_t)data[i]) << 8;
    for (byte b = 0; b < 8; b++) crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
  }
  return crc;
}


void RPULink_Attach(const RPULinkTransport *t) {
  txRing.head = txRing.tail = txRing.used = 0;
  txAudio.head = txAudio.tail = txAudio.used = 0;
  txPayloadLen = 0;
  txSeq = 0;
  peerAck = 0;
  rxLastSeq = 0;
  lastFrameMs = 0;
  everGood = false;
  dormant = false;
  quietSinceMs = millis();
  transport = t;
}


void RPULink_SetHandler(RPULinkRecordHandler handler) {
  linkHandler = handler;
}


boolean RPULink_Queue(byte type, const byte *data, byte len) {
  if (len > RPULINK_PAYLOAD_MAX - 2) { droppedRecords++; return false; }
  LinkRing *r = (type >= RPULINK_AUDIO_PLAY_POLY && type <= RPULINK_AUDIO_RATE_OFFSET) ? &txAudio : &txRing;
  unsigned int need = 2 + len;
  byte sreg = SREG;
  cli();
  if (r->size - r->used < need) {
    SREG = sreg;
    droppedRecords++;
    return false;
  }
  r->buf[r->head] = type; r->head = (r->head + 1) % r->size;
  r->buf[r->head] = len;  r->head = (r->head + 1) % r->size;
  for (byte i = 0; i < len; i++) {
    r->buf[r->head] = data[i];
    r->head = (r->head + 1) % r->size;
  }
  r->used += need;
  SREG = sreg;
  return true;
}


void RPULink_DropQueued() {
  byte sreg = SREG;
  cli();
  txRing.head = txRing.tail = txRing.used = 0;
  SREG = sreg;
}

boolean RPULink_QueueText(byte type, const char *text) {
  size_t n = strlen(text);
  if (n > RPULINK_PAYLOAD_MAX - 2) n = RPULINK_PAYLOAD_MAX - 2;
  return RPULink_Queue(type, (const byte *)text, (byte)n);
}


// Move as many whole queued records as fit into a fresh payload: audio first.
static void TakeFromRing(LinkRing *r) {
  while (r->used >= 2) {
    byte len = r->buf[(r->tail + 1) % r->size];
    unsigned int recLen = 2 + (unsigned int)len;
    if (txPayloadLen + recLen > RPULINK_PAYLOAD_MAX) break;
    for (unsigned int i = 0; i < recLen; i++) {
      txPayload[txPayloadLen++] = r->buf[r->tail];
      r->tail = (r->tail + 1) % r->size;
    }
    r->used -= recLen;
  }
}

static void BuildNextPayload() {
  txPayloadLen = 0;
  byte sreg = SREG;
  cli();
  TakeFromRing(&txAudio);
  TakeFromRing(&txRing);
  SREG = sreg;
  if (txPayloadLen) {
    txSeq = (txSeq == 255) ? 1 : txSeq + 1;     // 0 is reserved for "nothing sent yet"
  }
}


static void DispatchRecords(const byte *payload, byte len) {
  byte pos = 0;
  while (pos + 2 <= len) {
    byte type = payload[pos];
    byte rlen = payload[pos + 1];
    if (pos + 2 + rlen > len) break;               // truncated record: drop the rest
    const byte *d = payload + pos + 2;
    if ((type == RPULINK_AUDIO_STATUS || type == RPULINK_AUDIO_INFO) && LinkAudio_OnRecord) {
      LinkAudio_OnRecord(type, d, rlen);
    } else if (linkHandler) {
      linkHandler(type, d, rlen);
    }
    pos += 2 + rlen;
  }
}


void RPULink_BuildFrame(byte *tx) {
  // the payload in flight has been acked (or there never was one): take the next records
  if (txPayloadLen == 0 || peerAck == txSeq) {
    if (txPayloadLen && peerAck == txSeq) txPayloadLen = 0;
    if (txPayloadLen == 0) BuildNextPayload();
  }
  memset(tx, 0, RPULINK_FRAME_LEN);
  tx[0] = RPULINK_MAGIC;
  tx[1] = RPULINK_VERSION;
  tx[2] = txSeq;
  tx[3] = rxLastSeq;
  tx[4] = txFlags | (RPULink_IsUp() ? RPULINK_FLAG_LINK_UP : 0);
  tx[5] = txPayloadLen;
  memcpy(tx + 6, txPayload, txPayloadLen);
  uint16_t crc = RPULink_CRC16(tx, 62);
  tx[62] = crc >> 8;
  tx[63] = crc & 0xFF;
}

boolean RPULink_FrameValid(const byte *rx) {
  uint16_t rcrc = ((uint16_t)rx[62] << 8) | rx[63];
  return rx[0] == RPULINK_MAGIC && rx[1] == RPULINK_VERSION && rx[5] <= RPULINK_PAYLOAD_MAX &&
         rcrc == RPULink_CRC16(rx, 62);
}

void RPULink_HandleFrame(const byte *rx) {
  lastGoodRxMs = millis();
  quietSinceMs = lastGoodRxMs;
  dormant = false;
  everGood = true;
  peerAck = rx[3];
  // new payload from the S3: a repeat is the same seq with the same bytes (a restarted S3 sends
  // other bytes under its restarted seq). No "link down" reset: a stall must not cause a replay.
  uint16_t pcrc = rx[5] ? RPULink_CRC16(rx + 6, rx[5]) : 0;
  if (rx[5] && (rx[2] != rxLastSeq || pcrc != rxLastCrc)) {
    rxLastSeq = rx[2];
    rxLastCrc = pcrc;
    DispatchRecords(rx + 6, rx[5]);
  }
}

void RPULink_CountBadFrame() { badFrames++; }

void RPULink_Poll() {
  if (transport) transport->poll();
}


void RPULink_Update(unsigned long now) {
  if (transport && transport->drain) transport->drain();   // UART: drain every loop pass
  if (!dormant && (long)(now - quietSinceMs) > (long)RPULINK_DORMANT_AFTER_MS)
    dormant = true;                                        // no node: probe slowly, keep the queue
  unsigned long period = dormant ? RPULINK_PROBE_MS : RPULINK_PERIOD_MS;
  if (now - lastFrameMs < period) return;
  lastFrameMs = now;
  RPULink_Poll();
}

boolean RPULink_IsDormant() { return dormant; }
boolean RPULink_Pending() { return txRing.used > 0 || (txPayloadLen && peerAck != txSeq); }


boolean RPULink_IsUp() {
  return everGood && (millis() - lastGoodRxMs) < RPULINK_TIMEOUT_MS;
}


void RPULink_SetFlag(byte flag, boolean on) {
  if (on) txFlags |= flag;
  else txFlags &= ~flag;
}


unsigned int RPULink_DroppedRecords() { return droppedRecords; }
unsigned int RPULink_BadFrames() { return badFrames; }
unsigned int RPULink_RxOverruns() { return rpuLinkRxOverruns; }
unsigned long RPULink_RawRxBytes() { return rpuLinkRxBytes; }
unsigned long RPULink_RawTxBytes() { return rpuLinkTxBytes; }
unsigned int RPULink_FramingErrors() { return rpuLinkFramingErrors; }
unsigned int RPULink_DataOverruns() { return rpuLinkDataOverruns; }

#endif
