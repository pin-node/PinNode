# Add PinNode to an RPU game

Your game reports games, scores, achievements, stats and settings to PinNode through the node
(the ESP32 in the RPU board's WiFi socket). These steps add it by
hand to a game built on the RPU example game. `pin-node-example` is the same game with
every step done: when in doubt, look there.

Each step says whether you need it, and why. Every call is listed in [API.md](API.md).

| Steps | What you get |
|---|---|
| 1–7 (required) | Scores, players and games on PinNode: leaderboards and game history |
| 8–10 (recommended) | Achievements, starting from the app, claiming scores later |
| 11 (optional) | Settings, audits, credits and tests from the owner's phone |
| Setup on pin-node.com (required, once per machine) | The node flashed, on your WiFi and registered to you, then fitted |

## 1. Install the library (required)

The PinNode library is your game's link to the PinNode network card (the ESP32). Install it once;
don't copy its files into your sketch.

- **Arduino IDE:** **Tools → Manage Libraries…**, search **PinNode**, and click **Install**. Updates
  show up there too. (Or, on [github.com/pin-node/PinNode](https://github.com/pin-node/PinNode),
  **Code → Download ZIP**, then **Sketch → Include Library → Add .ZIP Library…**.)
- **Board:** **Tools → Board → Arduino AVR Boards → Arduino Mega or Mega 2560**, processor
  **ATmega2560**.
- **arduino-cli:** `arduino-cli lib install PinNode`.
  Then build with `arduino-cli compile --fqbn arduino:avr:mega …`.

To update it later, Library Manager shows an **Update** button when a new version is out.

**Why:** every game builds against the one library, so a fix reaches all of them, and your sketch
stays your own code.

## 2. Write the manifest (required)

In addition to the calls in your code, PinNode needs to know about your game: its name, what its
numbers mean, and what the owner can change. You describe that once, in two files: the manifest
(`pinnode.game.json`, this step) and `PinNodeGame.h`, generated from it (step 3). Not to be confused
with `PinNode.h`, the library's own header.

The manifest, `pinnode.game.json` next to your `.ino`, describes your game to PinNode.

```json
{
  "$schema": "https://raw.githubusercontent.com/pin-node/PinNode/main/tools/pinnode_gen/pinnode-game.schema.json",
  "manifestVersion": 1,
  "ruleset": { "id": "my-game", "name": "My Game", "repo": "you/my-game",
               "version": "1.0.0", "scoringVersion": 1, "platform": "bally-35" },
  "players": { "max": 4 },
  "achievements": [
    { "id": "first-game", "index": 0, "name": "First Game", "desc": "Finish a game", "points": 5, "kind": "event" }
  ],
  "stats": [
    { "id": "pop-hits", "index": 0, "name": "Pop bumper hits", "kind": "counter" }
  ]
}
```

**What it controls.** The manifest is everything PinNode knows about your game that isn't in
your code. The app and website build your game's screens from it:

| Section | Shows up as |
|---|---|
| `ruleset` | Your game's page; which leaderboard scores join |
| `achievements` | Badges with names, descriptions and points (5–100); can be awarded automatically when a stat hits a target |
| `stats` | Counters and bests on player and machine pages |
| `settings` | **Your game's settings menu in the app**: one control per setting |
| `audits` | The owner's audit view |
| `actions`, `lamps`, `switches`, `solenoids`, `sounds` | Tests from the phone, and readable names in fault reports |

A setting's `type` picks its control (`bool` a switch, `int` a slider with `min`/`max`/`step`/`unit`,
`enum` a list with your labels, `score` a score field, `flags` checkboxes) and `group` its section.
`difficulty` splits leaderboards and locks it in tournaments; `operatorOnly` (default on) limits it
to the owner; `liveSafe` allows mid-game changes; `requiresPresence` keeps it off the web (coil
strengths). For example, a quest award the owner can pick:

```json
{ "id": "quest-award", "index": 4, "name": "Quest complete award", "group": "awards",
  "type": "enum", "values": [ { "v": 0, "label": "100,000 points" },
                              { "v": 1, "label": "Extra ball" },
                              { "v": 2, "label": "Special (replay)" } ],
  "default": 1, "difficulty": true }
```

The value still lives in your EEPROM, edited by your operator menu too; the manifest only describes it.
The full format: [pinnode-game.schema.json](../tools/pinnode_gen/pinnode-game.schema.json).

**Why:**
- **Names and points live here, not in your code.** The Mega has little flash, so it only sends
  numbers. The website and the app read the names from the manifest.
- **`ruleset.id` and `scoringVersion` decide which leaderboard a score joins.** Bump
  `scoringVersion` only when scores stop being comparable (new point values); that starts fresh
  boards. New versions (`version`) and new achievements don't.
- **Indexes never change once released.** A player's achievement 3 must mean the same thing
  forever. Add new entries at the end.

`achievements` and `stats` can be empty to start. To draft `settings` and `audits` from your
operator menu, run `pinnode-gen scan path/to/MyGame`.

## 3. Generate the header (required)

```bash
pip install -e path/to/pinnode-rpu      # once: installs pinnode-gen
pinnode-gen generate pinnode.game.json  # writes PinNodeGame.h
```

Commit both files, and run `generate` again whenever you edit the manifest.

**Why:** it turns the manifest into C constants: your ids, a fingerprint of the manifest
(`PINNODE_MANIFEST_HASH`), and a name for each number (`ACH_FIRST_GAME`, `STAT_POP_HITS`). Your
code then can't drift out of step with the manifest. `pinnode-gen check` catches a header you
forgot to regenerate.

### When to regenerate

The header comes from the **manifest**, not from your code. Adding or moving calls never needs it.

| You changed | Regenerate? |
|---|---|
| Added an achievement, stat, setting or audit to the manifest | **Yes**, before using its constant in code |
| `version` (every release), `scoringVersion`, the game's id or name | **Yes**: the fingerprint changes |
| Added, moved or removed calls for numbers already in the manifest | No |

Order for anything new: **manifest → generate → call it in code.** A call to a constant that
isn't in the manifest yet (`ACH_JACKPOT`) won't compile, so you can't report a number PinNode
doesn't know.

## 4. Include it (required)

At the top of your `.ino`, after `RPU.h`:

```cpp
#include <PinNode.h>
#include "PinNodeGame.h"
```

**Why after `RPU.h`:** the claim-code and boot helpers use RPU's display and boot calls, and are
left out if `RPU.h` isn't included first.

## 5. Start the link in `setup()` (required)

After `RPU_InitializeMPU`:

```cpp
static void ManifestBytes(byte *out) {           // PINNODE_MANIFEST_HASH is 16 hex characters
  for (byte i = 0; i < 16; i++) {
    char c = PINNODE_MANIFEST_HASH[i];
    byte n = (c >= 'a') ? c - 'a' + 10 : (c >= 'A') ? c - 'A' + 10 : c - '0';
    if (i % 2 == 0) out[i / 2] = n << 4; else out[i / 2] |= n;
  }
}

  RPULink_BeginUart();                 // RPU Rev 4 / 102 (ESP32 in the WiFi socket)
  // RPULink_BeginSpi(53);             // future PinNode boards that link over SPI
  byte hash[8];
  ManifestBytes(hash);
  PinNodeCtrl_Begin(PINNODE_RULESET_VERSION, PINNODE_RULESET_ID, hash,
                    RPU_NUMBER_OF_PLAYERS_ALLOWED, 0, RPU_OS_HARDWARE_REV);
```

**Why:** `Begin` sends HELLO: which game, which version and which manifest is running, so every
score lands on the right leaderboard. The board revision tells the node which builds this board
can run. The library repeats HELLO whenever the link comes back, so a node that restarts still
knows your game.

With the UART link, don't use `Serial2` anywhere else: the library owns it.

## 6. Run it in `loop()` (required)

```cpp
  RPULink_Update(CurrentTime);                                        // required
  boolean inGame = MachineState > MACHINE_STATE_ATTRACT;
  PinNodeCtrl_Mode(inGame ? PINNODE_MODE_GAME : PINNODE_MODE_ATTRACT,  // recommended
                   inGame ? CurrentBallInPlay : 0, inGame ? CurrentPlayer + 1 : 0);
  PinNodeCtrl_Credits(Credits, FreePlayMode);                         // recommended
  PinNodeCtrl_Update(CurrentTime, false);                             // required
```

**Why:**
- **`RPULink_Update` and `PinNodeCtrl_Update`** move records to and from the node. Without them
  nothing is sent.
- **`Mode`** lets the app show the machine live: in attract, or in a game on ball 2, player 3. It
  also tells the node when the machine is in a game, so it holds off anything that would interrupt one.
- **`Credits`** shows the machine's credits in the app, and hides Start when there are none.
- **`false`:** RPU Rev 4 / 102 can't be reflashed over the link.

Keep every pass of `loop()` well under 100 ms: the link sends a frame every 5 ms from `RPULink_Update`.

## 7. Report the game (required)

Players and balls count from 1.

| Where in your game | Call | Why |
|---|---|---|
| Game starts (after the credit is spent) | `PinNodeCtrl_GameStart(Credits, FreePlayMode);` | Opens the game; checked-in players are tied to it |
| Players 2–4 join | `PinNodeCtrl_PlayerAdded(CurrentNumPlayers);` | Each score goes to the right player |
| Ball starts | `PinNodeCtrl_BallStart(CurrentPlayer + 1, CurrentBallInPlay);` | Live "player up, ball 2" in the app |
| Ball ends | `PinNodeCtrl_BallEnd(CurrentPlayer + 1, CurrentBallInPlay, CurrentScores[CurrentPlayer]);` | Ball-by-ball scores, and stats are sent here |
| Game over | `PinNodeCtrl_GameEnd(CurrentScores, CurrentNumPlayers, PINNODE_END_NORMAL);` | The final scores: what leaderboards use |
| Game abandoned for a new one | `PinNodeCtrl_GameEnd(CurrentScores, CurrentNumPlayers, PINNODE_END_RESET);` | Closes it, so it isn't ranked as a finished game |
| Tilt warning / tilt (optional) | `PinNodeCtrl_Tilt(CurrentPlayer + 1, PINNODE_TILT_WARNING);` (or `PINNODE_TILT_TILT`) | The machine's tilt counts; tournaments count tilted attempts |
| Coin (optional) | `PinNodeCtrl_Coin(chute, Credits);` (credits after the coin) | The owner's coin and earnings stats |

`GameStart`, `BallEnd` and `GameEnd` are the minimum. Build and play a game: the node's web page
shows each record arrive.

## 8. Achievements and stats (recommended)

```cpp
PinNodeCtrl_Achievement(CurrentPlayer + 1, ACH_FIRST_GAME);
PinNodeCtrl_Stat(CurrentPlayer + 1, STAT_POP_HITS, PINNODE_STAT_ADD, 1);
```

**Why:** achievements are what players chase between high scores, and stats give leaderboards
beyond score ("most pop hits"). They're your game's personality on PinNode. Add them whenever you
like: new ones go at the end of the manifest, then regenerate the header before you call them.

## 9. Start from the app (recommended)

In attract, where you read the credit button:

```cpp
uint16_t req;
if (PinNodeCtrl_TakeAppStart(&req, PinNode_AppStartsOn())) {
  if (AddPlayer(true)) { PinNodeCtrl_StartResult(req, PINNODE_START_OK, 1); /* start the game */ }
  else PinNodeCtrl_StartResult(req, PINNODE_START_NO_CREDITS, 0);
}
```

During a game, the same call adds a player.

**Why:** a player who checks in on the phone and presses Start there is tied to that game for
certain, with no guessing who stood at the machine. It runs your own Start path, so credits and
free play work as usual. Without it, the app's Start button is answered "not now".
`PinNode_AppStartsOn()` is the operator's **App starts** adjustment (step 9a): with it Off, the app's
Start is answered "app starts off".

## 9a. PinNode adjustments (recommended)

One hook in your adjustments menu shows the library's own adjustments after yours:

| Adjustment | Values | What it does |
|---|---|---|
| **App starts** | Off / On (default On) | Whether the PinNode app can start a game |
| **PinNode WiFi reset** | Off / On | On, then leave the menu: the network card opens setup mode so the owner can change its WiFi in the app (Change WiFi). Then back to Off |

**Why:** the RPU socket ESP32 has no button for setup mode and sits inside the machine. And a later
library version adds new PinNode adjustments to your game without you changing it. The library keeps
the values itself (EEPROM 4080-4087) and acts on them when the operator menu closes, so report
`PINNODE_MODE_SERVICE` while the menu is open (step 6).

The hook depends on how your game builds its menu; the converter places it for all three:

```cpp
// OperatorMenus (the RPU example game's menus): in the game-adjustments menu
Menus.SetNumSubLevels(OM_GAME_ADJ_FINISHED + PINNODE_NUM_ADJUSTMENTS);
switch (subLevel) {
  // ...your items...
  default: PINNODE_ADJUSTMENT_ITEM(subLevel - OM_GAME_ADJ_FINISHED); break;
}

// Self-test chained as MACHINE_STATE_ADJUST_* states: the old MACHINE_STATE_ADJUST_DONE value
#define PINNODE_ADJ_FIRST_STATE    -40
#define MACHINE_STATE_ADJUST_DONE  (-40 - PINNODE_NUM_ADJUSTMENTS)
//   ...and in the adjustment switch:
  default: PINNODE_ADJUSTMENT_ITEM_CHAINED(PINNODE_ADJ_FIRST_STATE - curState); break;

// Extended self-test settings (RunExtendedSelfTest): one state after your lowest
#define PINNODE_ADJ_FIRST_STATE    -24
#define NUM_EXTENDED_SETTINGS      (4 + PINNODE_NUM_ADJUSTMENTS)
//   ...and at the end of its if / else-if chain of settings:
else if (PinNode_AdjustmentEeprom(PINNODE_ADJ_FIRST_STATE - curState, &highNum, &auditNumStartByte)) {}
```

For the app's Settings tab, App starts is the setting `app-starts` at `PINNODE_EEPROM_APP_STARTS`.

**If your game already has its own App starts adjustment** (as the RPU example game
did), remove it, or the menu shows two: its variable, its menu entry and its EEPROM byte. Pass
`PinNode_AppStartsOn()` to `PinNodeCtrl_TakeAppStart` instead, and point the `app-starts` entry in
your settings table and manifest at `PINNODE_EEPROM_APP_STARTS` (4080).

## 10. Claim codes (recommended)

Last thing in each attract pass, and at game start:

```cpp
PinNode_ShowClaimCodes(CurrentTime, CurrentScores);   // attract
PinNode_ClaimCodesStop();                             // game start
```

**Why:** a player who didn't check in gets a 6-digit code on their score display after the game,
and can claim that score in the app later. Without it, those scores stay anonymous.

## 11. For the owner's phone (optional)

| Feature | Calls | Why |
|---|---|---|
| Settings | `PinNodeCtrl_Settings` | Change settings from the phone. Marking difficulty settings keeps leaderboards fair, and they lock during tournaments. |
| Audits | `PinNodeCtrl_Audits` | Plays, replays and coins in the app, without opening the coin door |
| Credits from the app | `PinNodeCtrl_TakeCreditAdd` | The owner adds credits remotely, through your own credit code |
| Tests and one-shots | `PinNodeCtrl_Actions` | Lamp, coil and switch tests, clearing audits, presets: from the phone |
| Original game | `PinNode_BootOptions` | The owner switches between your game and the machine's original ROM |
| Operator menu guards | `PinNode_SelfTestOpens`, `PinNode_OperatorMenuGuard` | A stray self-test pulse can't stop a game; the menu's blank first page doesn't stay open |

See [API.md](API.md), and `pin-node-example` for each one in a working game.

## Set up the machine on pin-node.com (required, once per machine)

**Why:** the code alone won't make anything happen. The node has to be flashed, on your WiFi, and
registered to you and to this machine before a record can reach PinNode.

On a computer, in Chrome or Edge, signed in: **pin-node.com/setup**. No phone needed.

1. **License:** your RPU Developer license.
2. **Game:** upload your `pinnode.game.json`, or paste your GitHub repo's address (PinNode finds
   the file anywhere in it, including the sketch folder). Your game is registered at once: names,
   achievements and settings, with no release needed. Any problems in the file are listed.
3. **Board:** RPU Rev 4 or 102.
4. **Machine:** taken from the manifest's `title`. Change it if it's wrong.
5. **Flash and connect:** ESP32 out of the socket, on USB: **Install PinNode** (the web flasher). Then,
   still on USB, pick your WiFi and type its password. The board joins WiFi and registers itself
   to you and this machine on your desk. The password goes over the cable, never to pin-node.com.
6. **Fit it:** unplug USB, machine off, ESP32 into the socket, machine on. It comes online by itself.
7. **App** (optional): the PinNode app on your phone.
8. **Check:** flash your game board with the Arduino IDE, and the page waits for its HELLO.

**Never plug USB into the ESP32 while it's in the socket:** the computer's 5 V back-feeds the RPU
board and the machine.

Changing WiFi later: **PinNode WiFi reset** in the operator menu (step 9a), then **Change WiFi** on
the machine in the PinNode app.

Games record **unranked** for now.
After changing the manifest, run `pinnode-gen generate` and upload the new `pinnode.game.json`.
You can convert and set up a game you didn't write: that's how most owners will use it.

## Building without PinNode (optional)

Put `#define PINNODE_DISABLED` before `#include <PinNode.h>`.

**Why:** one code base for machines with and without PinNode, or for sharing upstream. Every call
becomes empty and the library isn't linked. A game *with* PinNode also runs fine on a machine with
no node, so you only need this to save the flash.

## Doing it automatically

For a game built on the RPU example game, `pinnode-gen convert` does steps 2–7, 9 and 10 for you and
lists anything it couldn't place. Achievements and stats (step 8) are yours to add. See
[CONVERTER.md](CONVERTER.md).
