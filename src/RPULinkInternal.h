// Internal to the PinNode library: the core <-> transport seam. Not part of the API.
#ifndef RPU_LINK_INTERNAL_H
#define RPU_LINK_INTERNAL_H

#include <Arduino.h>

struct RPULinkTransport {
  void (*poll)(void);     // build and exchange one frame (every RPULINK_PERIOD_MS, or RPULink_Poll)
  void (*drain)(void);    // take whatever has arrived (UART), or NULL
};

void RPULink_Attach(const RPULinkTransport *t);   // resets the link state
void RPULink_BuildFrame(byte *tx);
boolean RPULink_FrameValid(const byte *rx);
void RPULink_HandleFrame(const byte *rx);
void RPULink_CountBadFrame();
extern volatile unsigned int rpuLinkRxOverruns;
extern volatile unsigned long rpuLinkRxBytes, rpuLinkTxBytes;
extern volatile unsigned int rpuLinkFramingErrors, rpuLinkDataOverruns;

#endif
