/**************************************************************************
 * RPU-Link over USART2 (RPU Rev 4 / Rev 102: the ESP32 in the WiFi socket,
 * Mega D16 TX2 / D17 RX2, 500 kbaud). Linked only when the game calls
 * RPULink_BeginUart(); it owns the USART2 interrupts, so never use Serial2.
 **************************************************************************/
#include <Arduino.h>              // first: UDR2 comes from <avr/io.h>
#if defined(ARDUINO_ARCH_AVR) && defined(UDR2)

#include "../RPULink.h"
#include "../RPULinkInternal.h"

// Own USART2 driver, not HardwareSerial: Serial2's 63-byte receive ring can't hold one 64-byte
// frame if the game loop is slow, and its blocking write would stall the loop. So never use
// Serial2 in a sketch with this link.
#define LINK_RX_RING      256               // 4 frames
#define LINK_TX_RING      128               // 2 frames
static volatile byte uRx[LINK_RX_RING];
static volatile byte uRxHead = 0, uRxTail = 0;
static volatile byte uTx[LINK_TX_RING];
static volatile byte uTxHead = 0, uTxTail = 0;

ISR(USART2_RX_vect) {
  byte status = UCSR2A;                     // read before UDR2: the flags belong to this byte
  byte b = UDR2;
  if (status & _BV(FE2)) rpuLinkFramingErrors++;    // bad stop bit: a corrupted byte (signal, noise)
  if (status & _BV(DOR2)) rpuLinkDataOverruns++;    // bytes lost before this one: the ISR ran too late
  rpuLinkRxBytes++;
  byte next = (byte)(uRxHead + 1);          // 256-entry ring: byte arithmetic wraps for free
  if (next == uRxTail) { rpuLinkRxOverruns++; return; }
  uRx[uRxHead] = b;
  uRxHead = next;
}

ISR(USART2_UDRE_vect) {
  if (uTxHead == uTxTail) { UCSR2B &= ~_BV(UDRIE2); return; }
  UDR2 = uTx[uTxTail];
  rpuLinkTxBytes++;
  uTxTail = (byte)((uTxTail + 1) % LINK_TX_RING);
}

static void UartBegin(unsigned long baud) {
  DDRH &= ~_BV(PH0);                        // RX2 (D17): input with the pull-up on, so an empty
  PORTH |= _BV(PH0);                        // WiFi socket reads idle instead of floating
  UBRR2 = (F_CPU / 8 / baud) - 1;           // U2X: exact at 16 MHz for 500k (3), 250k (7), 125k (15)
  UCSR2A = _BV(U2X2);
  UCSR2C = _BV(UCSZ21) | _BV(UCSZ20);       // 8N1
  UCSR2B = _BV(RXEN2) | _BV(TXEN2) | _BV(RXCIE2);
}

static boolean UartWrite(const byte *d, byte n) {
  byte freeBytes = (byte)((uTxTail + LINK_TX_RING - uTxHead - 1) % LINK_TX_RING);
  if (freeBytes < n) return false;          // previous frame still going out: skip this one
  for (byte i = 0; i < n; i++) {
    uTx[uTxHead] = d[i];
    uTxHead = (byte)((uTxHead + 1) % LINK_TX_RING);
  }
  UCSR2B |= _BV(UDRIE2);
  return true;
}

static byte UartAvailable() { return (byte)(uRxHead - uRxTail); }
static byte UartPeek(byte i) { return uRx[(byte)(uRxTail + i)]; }
static void UartDrop(byte n) { uRxTail = (byte)(uRxTail + n); }

// whatever has arrived: find frames (magic, version, CRC), resyncing a byte at a time, so the
// ESP32's boot chatter and any line noise are skipped
static void UartDrain() {
  byte rx[RPULINK_FRAME_LEN];
  while (UartAvailable() >= RPULINK_FRAME_LEN) {
    if (UartPeek(0) != RPULINK_MAGIC || UartPeek(1) != RPULINK_VERSION) { UartDrop(1); continue; }
    for (byte i = 0; i < RPULINK_FRAME_LEN; i++) rx[i] = UartPeek(i);
    if (!RPULink_FrameValid(rx)) { RPULink_CountBadFrame(); UartDrop(1); continue; }
    UartDrop(RPULINK_FRAME_LEN);
    RPULink_HandleFrame(rx);
  }
}

// the ESP32 answers each frame with one of its own; it's picked up by UartDrain
static void UartPoll() {
  byte tx[RPULINK_FRAME_LEN];
  UartDrain();
  RPULink_BuildFrame(tx);
  UartWrite(tx, RPULINK_FRAME_LEN);   // false: still sending the last frame; try next period
}

static const RPULinkTransport uartTransport = {UartPoll, UartDrain};

void RPULink_BeginUart(unsigned long baud) {
  UartBegin(baud);
  RPULink_Attach(&uartTransport);
}

#endif
