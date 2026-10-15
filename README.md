# PinNode

PinNode for RetroPinUpgrade (RPU) games: the Arduino library a game uses to report games,
achievements, stats and settings to its PinNode node, and `pinnode-gen`, the tools that convert
a game, write its `PinNodeGame.h` and check its manifest.

- **Releases:** each version is a [GitHub release](https://github.com/pin-node/PinNode/releases)
  with notes. Notes are in [CHANGELOG.md](CHANGELOG.md).
- **Help and the guides:** pin-node.com/developers.

## The Arduino library

**Adding PinNode to a game:** [docs/ADD-PINNODE.md](docs/ADD-PINNODE.md), step by step. Every call:
[docs/API.md](docs/API.md). The converter: [docs/CONVERTER.md](docs/CONVERTER.md).

Install it once, then `#include <PinNode.h>` in the game. Don't copy its files into a sketch:
every game builds against the one library, so a fix reaches all of them.

- **Arduino IDE:** **Tools → Manage Libraries…**, search **PinNode**, and click **Install**. Updates
  show up there too. (Or, on [github.com/pin-node/PinNode](https://github.com/pin-node/PinNode),
  **Code → Download ZIP**, then **Sketch → Include Library → Add .ZIP Library…**.)
- **arduino-cli:** `arduino-cli lib install PinNode`.

```cpp
#include <PinNode.h>

void setup() {
  RPULink_BeginUart();                  // RPU Rev 4 / 102: ESP32 in the WiFi socket (Serial2)
  // RPULink_BeginSpi(53);              // future PinNode boards that link over SPI
  PinNodeCtrl_Settings(settingsTable, settingsCount, SettingsApplied);   // optional
  PinNodeCtrl_Audits(auditTable, auditCount);                            // optional
  PinNodeCtrl_Begin("1.0.0", "my-ruleset", manifestHash, 4, 0, RPU_OS_HARDWARE_REV);
}

void loop() {
  RPULink_Update(CurrentTime);
  PinNodeCtrl_Mode(mode, ball, slot);
  PinNodeCtrl_Update(CurrentTime, otaAllowed);
}
```

Game hooks (`GameStart`, `PlayerAdded`, `BallStart`/`BallEnd`, `GameEnd`, `Tilt`, `Coin`,
`Achievement`, `Stat`, start from the app, claim codes, settings with the tournament lock, audits,
and operator actions from the phone) are documented in `src/PinNodeCtrl.h`; `examples/UartLink`
and `examples/SpiLink` are the smallest working sketches.

**PinNode adjustments:** one hook in your adjustments menu adds the library's own items after yours
(App starts, PinNode WiFi reset, and any a later version adds). See `docs/ADD-PINNODE.md` step 9a;
the converter places the hook.

`tests/native` builds `PinNodeCtrl.cpp` on the host (`-DPINNODE_HOST_BUILD`, with the shims in
`tests/native/shim` and a fake link) and checks what it queues; pytest runs it.

- Only the transport the game starts is linked (`dot_a_linkage`): the UART one owns Serial2 and its
  interrupts, so a sketch using it must not use `Serial2` for anything else.
- Keep every `loop()` pass well under 100 ms: the link sends a frame every 5 ms, and counts as down after 1 s without one.
- **No node fitted?** The game runs as usual. After 10 s with no node the link slows to one probe
  every 2 s; records wait in the (small) queue, and a node that appears or comes back gets them.
- **No PinNode at all:** `#define PINNODE_DISABLED` before `#include <PinNode.h>` (in the game's config
  header) turns every call into an empty stand-in; none of the library is linked. The arguments
  aren't evaluated, so don't put side effects in them (`PinNodeCtrl_Stat(p, S, ADD, n++)`).
- **Diagnostics:** `PinNodeCtrl_DebugTo(&Serial)` prints link changes ("node found", "link down",
  "no node, quiet") and error counters every minute. PinNode prints nothing unless you ask.
- Mega OTA over the link needs the board's safe state: register it with
  `PinNodeCtrl_SetOtaHandler()` (future SPI boards will; on Rev 4/102 OTA requests are refused).

## Generate and check

```bash
.venv/bin/pip install -e ".[dev]"      # installs the pinnode-gen command
pinnode-gen generate pinnode.game.json  # writes PinNodeGame.h next to it; commit both
pinnode-gen check pinnode.game.json --header PinNodeGame.h --previous old/pinnode.game.json
```

`generate` refuses to write a header from an invalid manifest. `check` exits 1
on any problem. Arduino IDE users run `generate` by hand after editing the
manifest; CI catches a forgotten run.

### GitHub Action

Copy `examples/pinnode-check.yml` into your game repo's `.github/workflows/`. On
each push, PR and release it validates the manifest, checks the committed
header is current, and compares against the `pinnode.game.json` of **every
earlier release**, so an achievement number can't be reused. That's the same
rule the PinNode import enforces, caught before you publish. Problems show as
annotations on the PR.

Pin the action to a release tag (`pin-node/PinNode@v0.3.13`), not `main`.

## Scan settings from a game repo

```bash
python3.12 -m venv .venv && .venv/bin/pip install -e ".[dev]"
.venv/bin/python -m tools.pinnode_gen.scan ~/src/MyRPUGame --json settings.draft.json --report scan-report.md
```

It reads the operator menu (`case OM_...:` with `SetParameterControls`), the EEPROM
`#define`s, the defaults function and `ReadSetting(...)` bounds. It writes:

- a **draft** `settings` list, with `source` (menu id, variable, EEPROM slot) on each entry;
- a **report** of what needs a human: labels, which settings are `difficulty`, guessed `requiresPresence` for coil strengths, state entries it skipped (credits, high score), EEPROM settings that aren't in any menu, and inconsistencies in the game code itself.

Review the draft, then paste it into `pinnode.game.json`. Against
the RPU example game it reproduces every code-derivable field of the
hand-written `rpu-standard` template (see `tests/test_scan.py`). Older BSOS-style
games don't use this menu pattern; the report says so, and their settings are
written by hand.

## Credits

PinNode is built on **RetroPinUpgrade (RPU)**, Dick Hamill's Arduino-based replacement for the
original CPU code in classic Bally, Stern and Williams pinball machines: the RPU boards, the RPU OS
and the games written on it ([github.com/RetroPinUpgrade](https://github.com/RetroPinUpgrade)). Thanks
to Dick for RPU and for his support of PinNode. The converter's tests also use Dave's Think Tank's
[Star Trek 2025](https://github.com/DavesThinkTank/Bally-Star-Trek).

## Licence

**MIT** (see `LICENSE`). MIT is GPL-compatible, so the
`PinNode` library can be compiled into RPU games, which are GPL-3.0, and also into games
and boards under other licences. The test fixtures in `tests/fixtures/example-machine` and `tests/fixtures/st2025-upstream`
are GPL-3.0 (see their NOTICE files) and are not part of the library.
