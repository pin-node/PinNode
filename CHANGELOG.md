# Changes

## 0.3.20

- `PinNodeCtrl_Scores` waits for the game's first `BallStart`: no live score with player 0, ball 0 as a
  game opens (found by the Star Trek sim).

## 0.3.19

- `PinNodeCtrl_Scores(scores, numPlayers)`: live scores for phones at the machine. Call it every
  `loop()` after `PinNodeCtrl_Mode`. During a game it sends LIVE_SCORE (0x3C) on a change (at most
  4 a second) and at least every 2 s, with the player up and ball from the last `BallStart`. The node
  puts them in its Bluetooth broadcast (pinnode-node 0.1.32 and later).
- Converter: converted games call `PinNodeCtrl_Scores(CurrentScores, CurrentNumPlayers)` in `loop()`.

## 0.3.18

- PinNode 400: a pairing code (the owner pairing a computer at the machine) takes over every player
  display: the code types in digit by digit, holds, blanks, repeats; the scores return after.
- PinNode 400: a volume changed from the app plays a short pip at the new level
  (`PinNode_VolumePip`, also for a game's own volume adjustment).
- Converter: converted games apply app settings at once (`PinNodeSettingsApplied` calls
  `ReadStoredParameters`), and a new music volume fades the music playing instead of restarting it.
- `PinNode_Log(fmt, ...)`: a game's own log lines reach the node's developer log (SPEC-dev-flash §10),
  readable in the Arduino IDE's Serial Monitor on a network port (boards package 0.2.0) and the
  app's Game log. Rate-capped (`PINNODE_LOG_PER_S`, 10). The 400's sound player reports its status
  lines there too.
- PinNode 400, bulk mode for the sound file manager (SPEC-node-link): folders, make a folder, move,
  copy (card to card), read (downloads); answers checked by an op/arg0 echo and a CRC-32 of their
  data. Version 2: 8 KB frames by DMA at 15 MHz (its own channels, no interrupt: the sound output owns
  DMA_IRQ_0), and the LINK_ATTN handshake (GPIO38): the link moves
  about 315 KB/s (was about 50). The node must be the matching version (pinnode-node #26).
- PinNode 400: the app plays a file from the card on the machine (SOUND_PREVIEW 0xAD, answered by
  SOUND_STATE 0x3A once its file opens, and again when it ends). Any .wav, in folders too, on a voice
  of its own; outside games only.
- Native tests: the menu test stubs `PinNodeCtrl_NoteInitResult` (it failed to link since 0.3.17).

## 0.3.17

- After each HELLO the game sends RPU's start-up result to the node as a log line (`rpu init 0x....`,
  the `RPU_RET_` bits), so the node log says why a start chose the original game.

## 0.3.16

- Builds for the RP2350 (the PinNode 400), alongside the Mega. On the RP the link is SPI to the
  board's XIAO (SPI0: GPIO34 SCK, 35 MOSI, 36 MISO, 37 CS); `RPULink_BeginUart()` starts it too, so
  games don't change. EEPROM (flash on the RP) is opened on first use and saved before a restart.
  Boot select works on hardware rev 400. Mega builds are unchanged.
- PinNode 400 game updates: the node flashes the RP over SWD. Updates are taken in attract or the
  operator menus, with a safe state from RPU's own calls (sound silenced, coils, flippers and lamps
  off, displays blank). The build ID covers whole 256-byte pages, as a `.uf2` carries them.
- PinNode 400 sounds: the link's bulk mode (`PinNodeBulk.h`) carries the owner's WAVs to the RP's SD
  card, and `CARD_STATUS` (0x39) tells the node about the card.

## 0.3.15

- The OLED status screen from 0.3.13 is removed. `PINNODE_OLED` and the U8g2 library are no longer
  used, and the converter no longer adds the display calls. Games that called `PinNode_OledBegin` or
  `PinNode_OledUpdate` should remove those two lines.

## 0.3.14

- Issues are open on this repo, with a form for library and converter problems. For account,
  machine or setup help, email support@pin-node.com.
- Comments: the SPI link is for future PinNode boards.
- Test inputs are no longer seen as sketches by Arduino's library checks.

## 0.3.13

- Status screen on the RPU board's OLED (Rev 4 and 102): the link, WiFi and cloud, setup messages,
  the mode, check-ins, claim codes and phone tests. Optional: `#define PINNODE_OLED` and the U8g2
  library. The converter places the calls.
- The Library Manager's "More info" link goes to this repo.

## 0.3.12

- Install from the Arduino IDE's Library Manager: search **PinNode**. Updates show up there too.
- The converter names Williams Scorpion (1980) lamps, switches and coils from the manual, from
  [RPUMachines](https://github.com/pin-node/RPUMachines), the machine definitions library.
- Credits for RetroPinUpgrade (Dick Hamill) in the README.

## 0.3.11 — first public release

The PinNode library and converter for RetroPinUpgrade games.

**Library**
- Scores, achievements, stats and settings from your game to its PinNode node, over the ESP32 in the
  RPU WiFi socket (Rev 4 and 102). SPI is ready for future PinNode boards.
- Start a game from the app, claim codes on the score displays, settings with the tournament lock,
  and audits.
- Tests from the phone: lamp, coil, switch, display and sound tests for RPU OS games
  (`PinNode_TestsBegin` / `PinNode_Tests`).
- PinNode adjustments: one hook in your adjustments menu adds App starts and PinNode WiFi reset.
- `#define PINNODE_DISABLED` builds the game with no PinNode at all.

**Converter (`pinnode-gen`)**
- Converts an existing RPU game: places the PinNode calls, writes `pinnode.game.json` and
  `PinNodeGame.h`, and a report of what's left to do. Checked against 32 RPU, BSOS and WOS games.
- `check` and the GitHub Action stop an achievement, stat or setting number from being reused
  between releases.

Phone tests are checked on a Williams System 6 game (lamps, coils, switches). The display test now
turns on all four player displays.
