# Changes

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
