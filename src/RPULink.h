/**************************************************************************
 * RPU-Link: the Mega <-> PinNode node framed link (SPEC-node-link).
 *
 * Transport : SPI (network board: mode 0, 2 MHz, 64-byte full-duplex frames,
 *             CS-framed) or UART (RPU Rev 4/102: USART2, 500 kbaud, the node
 *             answers each frame). The Mega sends one frame every
 *             RPULINK_PERIOD_MS from the main loop (RPULink_Update) or on
 *             demand (RPULink_Poll).
 * Frame     : [0]  0xA5 magic
 *             [1]  protocol version (1)
 *             [2]  seq  - sequence number of this frame's payload (1..255, 0 = none yet)
 *             [3]  ack  - last seq received intact from the peer
 *             [4]  flags (RPULINK_FLAG_*)
 *             [5]  payload length 0..56
 *             [6..61] payload: records [type u8][len u8][data...]
 *             [62..63] CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection,
 *                      no xorout) over bytes 0..61, stored BIG-endian.
 * Records   : multi-byte fields are LITTLE-endian (AVR and ESP32 native order).
 * Reliability: stop-and-wait per direction. A payload is resent with the same
 *             seq until the peer's ack equals it; the receiver drops repeats of
 *             the last seq it processed. Frames with bad magic/version/CRC are
 *             ignored (S3 absent or booting) and the link is reported down.
 *             When a valid frame arrives after the link has been down (>1 s
 *             without one), the receiver forgets its last seq, so a rebooted
 *             peer restarting at seq 1 is not mistaken for a repeat.
 **************************************************************************/
#ifndef RPU_LINK_H
#define RPU_LINK_H

#include <Arduino.h>

#define RPULINK_FRAME_LEN        64
#define RPULINK_PAYLOAD_MAX      56
#define RPULINK_MAGIC            0xA5
#define RPULINK_VERSION          1
#define RPULINK_PERIOD_MS        5
#define RPULINK_TIMEOUT_MS       1000      // no valid frame for this long = link down (a burst of
                                           // lost frames under the RPU OS ISRs is not a link loss)
#define RPULINK_QUEUE_BYTES      256       // Mega -> S3 record queue (type,len,data...)
#define RPULINK_AUDIO_QUEUE_BYTES 96        // audio records, sent ahead of everything else
#define RPULINK_SPI_HZ           2000000UL
#define RPULINK_DORMANT_AFTER_MS 10000     // no valid frame this long: no node, go quiet
#define RPULINK_PROBE_MS         2000      // while quiet, one frame this often to find a node
#define RPULINK_UART_BAUD        1000000UL // default; the node hunts 1M / 500k. Short frames lose fewer
                                           // bytes to the RPU OS zero-crossing ISR (2026-10-03 bench)

// frame flags (byte 4)
#define RPULINK_FLAG_LINK_UP     0x01
#define RPULINK_FLAG_SAFE_STATE  0x02
#define RPULINK_FLAG_OTA_PENDING 0x04

// ---- record types, Mega -> S3 ----
#define RPULINK_AUDIO_PLAY_POLY      0x10   // track u16, lock u8
#define RPULINK_AUDIO_STOP           0x11   // track u16
#define RPULINK_AUDIO_GAIN           0x12   // track u16, gain_db i8
#define RPULINK_AUDIO_FADE           0x13   // track u16, gain_db i8, ms u16, stop u8
#define RPULINK_AUDIO_LOOP           0x14   // track u16, on u8
#define RPULINK_AUDIO_STOP_ALL       0x15   // -
#define RPULINK_AUDIO_MASTER_GAIN    0x16   // gain_db i8
#define RPULINK_AUDIO_RATE_OFFSET    0x17   // offset i16
#define RPULINK_PINNODE_FIRST_M2S    0x20   // 0x20..0x2F: pinnode-spec-docs SPEC-node-link §3.1
#define RPULINK_PINNODE_LAST_M2S     0x2F
#define RPULINK_LOG                  0x30   // text
#define RPULINK_CTRL_OTA_READY       0x40   // -
#define RPULINK_CTRL_OTA_REFUSED     0x41   // reason u8
#define RPULINK_HELLO                0x42   // proto 2: buildId, manifestHash, version, maxPlayers, settingsCount
#define RPULINK_HELLO_EXT            0x43   // rulesetId (len-prefixed), always straight after HELLO
#define RPULINK_NODE_REQUEST         0x46   // op u8: 1 setup window, 2 factory reset, 3 release

// ---- record types, S3 -> Mega ----
#define RPULINK_AUDIO_STATUS         0x90   // count u8, count x track u16
#define RPULINK_AUDIO_INFO           0x91   // version string (NUL-terminated), voices u8, tracks u16
#define RPULINK_PINNODE_FIRST_S2M    0xA0   // 0xA0..0xAF: SPEC-node-link §3.2
#define RPULINK_PINNODE_LAST_S2M     0xAF
#define RPULINK_CTRL_OTA_REQUEST     0xC0   // -
#define RPULINK_CTRL_TIME            0xC1   // unix time u32

// Called for every record received from the S3 that the link layer itself doesn't consume.
typedef void (*RPULinkRecordHandler)(byte type, const byte *data, byte len);

// Start the link on one transport (only the one called is linked into the sketch):
// RPU Rev 4 / 102: the ESP32 in the WiFi socket on USART2. baud must match the node's link baud.
void RPULink_BeginUart(unsigned long baud = RPULINK_UART_BAUD);
void RPULink_BeginSpi(byte csPin = 53);             // network board (rev 300): D53 = PB0 = MEGA_CS
void RPULink_SetHandler(RPULinkRecordHandler handler);
void RPULink_Update(unsigned long now);             // call once per loop(); sends a frame every 5 ms
void RPULink_Poll();                                // send a frame right now
boolean RPULink_Queue(byte type, const byte *data, byte len);   // false if the queue is full
void RPULink_DropQueued();   // forget PinNode records not yet framed (audio is untouched)
boolean RPULink_QueueText(byte type, const char *text);
boolean RPULink_IsUp();
boolean RPULink_IsDormant();                       // no node answering: one probe every 2 s (records still queue)
boolean RPULink_Pending();                         // records queued or a payload not yet acked by the node
void RPULink_SetFlag(byte flag, boolean on);
unsigned int RPULink_DroppedRecords();              // records refused because the queue was full
unsigned int RPULink_BadFrames();
unsigned int RPULink_RxOverruns();
unsigned long RPULink_RawRxBytes();                // UART transport: bytes in / out on the wire (bring-up)
unsigned long RPULink_RawTxBytes();
// UART transport: bytes received with a framing error (a bad stop bit: the signal or noise) and
// USART data overruns (bytes lost because the receive interrupt ran late: long ISRs elsewhere)
unsigned int RPULink_FramingErrors();
unsigned int RPULink_DataOverruns();
uint16_t RPULink_CRC16(const byte *data, unsigned int len);

#endif
