/**************************************************************************
 * PinNode output for Sidecar (the open RPU library: Sidecar.h, MIT, wifijt).
 *
 * A game that uses Sidecar includes Sidecar.h, then this, and adds the output once in setup():
 *
 *   #include "Sidecar.h"
 *   #include <PinNodeSidecar.h>
 *   ...
 *   Sidecar_AddOutput(&PinNodeSidecarOutput);
 *
 * Sidecar's states, timers and events then also go to the node (PinNodeCtrl_Sidecar*), which shows
 * them on the phones at the machine. Sidecar keeps calling its other outputs (a serial screen) as before.
 * The node needs the game's sidecar block in pinnode.game.json (the same as its sidecar.json).
 * On boards without Sidecar support in PinNode (the Mega today) the calls compile to nothing.
 **************************************************************************/
#ifndef PINNODE_SIDECAR_H
#define PINNODE_SIDECAR_H

#ifndef SIDECAR_H
#error "Include Sidecar.h before PinNodeSidecar.h"
#endif
#include <PinNode.h>

static void PinNodeSidecar_Game(boolean, uint16_t) {}   // the node already has GAME_START / GAME_END
static void PinNodeSidecar_State(byte slot, byte key, long value) { PinNodeCtrl_SidecarState(slot, key, value); }
static void PinNodeSidecar_Timer(byte slot, byte key, byte op, unsigned long msLeft, unsigned long msTotal) {
  if (op == SIDECAR_TIMER_RUNNING) PinNodeCtrl_SidecarTimer(slot, key, msLeft, msTotal);
  else if (op == SIDECAR_TIMER_PAUSED) PinNodeCtrl_SidecarTimerPause(slot, key, msLeft);
  else PinNodeCtrl_SidecarTimerStop(slot, key);
}
static void PinNodeSidecar_Event(byte slot, uint16_t code, long value) { PinNodeCtrl_Sidecar(slot, code, value); }

static const SidecarOutput PinNodeSidecarOutput = {PinNodeSidecar_Game, PinNodeSidecar_State, PinNodeSidecar_Timer,
                                                   PinNodeSidecar_Event};

#endif
