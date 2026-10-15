# PinNode library reference

Library version 0.3.5. `#include <PinNode.h>` (after `RPU.h`) brings in everything below.
Step-by-step use: [ADD-PINNODE.md](ADD-PINNODE.md).

Each call is marked **Required**, **Recommended** or **Optional**, with why.

## Types on the Mega (AVR)

| Type | Size | Range |
|---|---|---|
| `byte` / `uint8_t` | 8-bit unsigned | 0–255 |
| `boolean` | 8-bit | `true` / `false` |
| `uint16_t`, `unsigned int`, `unsigned short` | 16-bit unsigned | 0–65,535 |
| `long` | 32-bit signed | ±2,147,483,647 |
| `unsigned long`, `uint32_t` | 32-bit unsigned | 0–4,294,967,295 |

**Conventions:** `slot` is the player, 1–4. `ball` counts from 1. `now` is `millis()` (RPU's
`CurrentTime`). Strings are kept by pointer, so pass literals or `#define`s, never a local buffer.

---

## 1. The link

### `void RPULink_BeginUart(unsigned long baud = 1000000)`
**Required (one of the two Begin calls).** Starts the link on RPU Rev 4 / 102, to the ESP32 in the
WiFi socket on `Serial2` (pins 16 and 17).

| Parameter | Type | Meaning |
|---|---|---|
| `baud` | `unsigned long` | Link speed. Leave the default; the node finds 1,000,000 or 500,000. |

The library owns `Serial2` and its interrupts: don't use them elsewhere.

### `void RPULink_BeginSpi(byte csPin = 53)`
**Required (one of the two Begin calls).** Starts the link over SPI (2 MHz), for future PinNode boards that use
SPI. No board uses it yet: RPU Rev 4 and 102 use `RPULink_BeginUart`.

| Parameter | Type | Meaning |
|---|---|---|
| `csPin` | `byte` | Chip-select pin. 53 on PinNode SPI boards. |

### `void RPULink_Update(unsigned long now)`
**Required.** Every `loop()` pass. Sends a frame every 5 ms and takes in the node's.
**Why:** nothing reaches the node without it. Keep each `loop()` pass well under 100 ms.

| Parameter | Type | Meaning |
|---|---|---|
| `now` | `unsigned long` | `millis()` |

### Link status: optional

| Call | Returns | Meaning |
|---|---|---|
| `boolean RPULink_IsUp()` | `boolean` | A node answered within the last second. |
| `boolean RPULink_IsDormant()` | `boolean` | No node for 10 s: the link only probes every 2 s. Records still queue. |
| `boolean RPULink_Pending()` | `boolean` | Records are queued or not yet acknowledged. |

### Bring-up counters: optional
All return counts since start-up: `unsigned int RPULink_DroppedRecords()` (queue full),
`unsigned int RPULink_BadFrames()`, `unsigned int RPULink_RxOverruns()`,
`unsigned int RPULink_FramingErrors()`, `unsigned int RPULink_DataOverruns()`,
`unsigned long RPULink_RawRxBytes()`, `unsigned long RPULink_RawTxBytes()`.

---

## 2. Start-up and every loop

### `void PinNodeCtrl_Begin(const char *version, const char *rulesetId, const byte *manifestHash, byte maxPlayers, byte settingsCount, uint16_t hwRev)`
**Required.** Once, in `setup()`, after the `RPULink_Begin…` call and after registering any
settings, audits and actions. Sends HELLO, and again every time the link comes back.
**Why:** HELLO tells PinNode which game, version and manifest is running, so scores land on the
right leaderboard.

| Parameter | Type | Meaning |
|---|---|---|
| `version` | `const char *` | Your release version: `PINNODE_RULESET_VERSION` |
| `rulesetId` | `const char *` | Your game's id: `PINNODE_RULESET_ID` |
| `manifestHash` | `const byte *` | 8 bytes, from `PINNODE_MANIFEST_HASH` (16 hex characters); `NULL` before you have a manifest |
| `maxPlayers` | `byte` | 1–4 |
| `settingsCount` | `byte` | `0`: the count comes from `PinNodeCtrl_Settings` |
| `hwRev` | `uint16_t` | `RPU_OS_HARDWARE_REV` (4, 102, 300 …), so the node only offers builds this board can run |

### `void PinNodeCtrl_Update(unsigned long now, boolean otaAllowed)`
**Required.** Every `loop()` pass, after your other PinNode calls. Handles HELLO, settings,
audits, requests from the app, and timeouts.

| Parameter | Type | Meaning |
|---|---|---|
| `now` | `unsigned long` | `millis()` |
| `otaAllowed` | `boolean` | `true` only in attract, menus or diagnostics, and only on a board that supports it (future SPI boards). **Always `false` on RPU Rev 4 / 102.** |

Doesn't return once a firmware update is accepted (the node resets the Mega).

### `void PinNodeCtrl_Mode(byte mode, byte ball, byte slot)`
**Recommended.** Every `loop()` pass; sent only when it changes.
**Why:** the app shows the machine live, and the node holds off anything that would interrupt a game.

| Parameter | Type | Meaning |
|---|---|---|
| `mode` | `byte` | `PINNODE_MODE_ATTRACT` (0), `_GAME` (1), `_SERVICE` (2: menus, diagnostics) |
| `ball` | `byte` | Ball in play, 0 outside a game |
| `slot` | `byte` | Player up, 0 outside a game |

### `void PinNodeCtrl_Credits(byte credits, boolean freePlay)`
**Recommended.** Every `loop()` pass; sent only when it changes.
**Why:** the app shows credits and hides Start when there are none.

| Parameter | Type | Meaning |
|---|---|---|
| `credits` | `byte` | Credits on the machine now |
| `freePlay` | `boolean` | Free play on |

### `void PinNodeCtrl_DebugTo(Print *out)`
**Optional.** Prints link events ("node found", "link down") and error counts every minute.
Silent unless you call it.

| Parameter | Type | Meaning |
|---|---|---|
| `out` | `Print *` | A serial port, e.g. `&Serial` |

### `void PinNodeCtrl_NodeRequest(byte op)`
**Optional.** Asks the PinNode network card to do something only a person at the machine should.
**Why:** the RPU socket ESP32 has no BOOT button, and it's out of reach inside the machine. Put
these in your operator menu.

| `op` | Constant | The card |
|---|---|---|
| 1 | `PINNODE_REQ_SETUP` | Opens its Bluetooth setup window for 10 minutes (Change WiFi in the app). Its saved WiFi is kept |
| 2 | `PINNODE_REQ_FACTORY_RESET` | Releases itself from the machine, then forgets the WiFi and the owner. For selling the machine. Confirm first |
| 3 | `PINNODE_REQ_RELEASE` | Releases itself from the machine, as Release on the website |

### PinNode adjustments
**Recommended.** The library's own items for your adjustments menu: **App starts** and **PinNode WiFi
reset**, and any a later version adds. One hook shows them after yours (ADD-PINNODE step 9a). Values
live in EEPROM 4080-4087 and are saved, and acted on, when the reported mode leaves
`PINNODE_MODE_SERVICE`.

| Name | Type | Meaning |
|---|---|---|
| `PINNODE_NUM_ADJUSTMENTS` | constant | How many there are (0 with `PINNODE_DISABLED`) |
| `PINNODE_ADJUSTMENT_ITEM(index)` | macro | OperatorMenus games: the item's range and value, in the game-adjustments switch's `default` |
| `PINNODE_ADJUSTMENT_ITEM_CHAINED(index)` | macro | `MACHINE_STATE_ADJUST_*` games: the same, in the adjustment switch's `default` |
| `boolean PinNode_AdjustmentEeprom(byte index, long *maxValue, unsigned short *eeprom)` | function | Extended self-test games: the item's top value and EEPROM address; false if `index` isn't ours |
| `boolean PinNode_AdjustmentRange(byte index, byte *values, byte **value)` | function | What the macros call: `values[0..1]` = min, max; `*value` = the library's copy |
| `boolean PinNode_AppStartsOn()` | function | The App starts adjustment: pass it to `PinNodeCtrl_TakeAppStart` |
| `PINNODE_EEPROM_APP_STARTS` | constant | Its EEPROM byte, for a `PinNodeSetting` entry (`app-starts`) |

### `void PinNodeCtrl_Log(const char *text)`
**Optional.** One line of text on the node's web page, for debugging.

| Parameter | Type | Meaning |
|---|---|---|
| `text` | `const char *` | Up to 54 characters; longer is cut off |

---

## 3. The game

### `void PinNodeCtrl_GameStart(byte credits, boolean freePlay)`
**Required.** When a game starts, after the credit is spent. Player 1 is implied.
**Why:** opens the game; players checked in on the phone are tied to it.

| Parameter | Type | Meaning |
|---|---|---|
| `credits` | `byte` | Credits left after the start |
| `freePlay` | `boolean` | Free play on |

### `void PinNodeCtrl_PlayerAdded(byte slot)`
**Recommended.** When players 2–4 join. **Why:** each score goes to the right player.

| Parameter | Type | Meaning |
|---|---|---|
| `slot` | `byte` | The new player, 2–4 |

### `void PinNodeCtrl_BallStart(byte slot, byte ball)`
**Recommended.** When a ball starts. **Why:** live "player 2, ball 3" in the app.

| Parameter | Type | Meaning |
|---|---|---|
| `slot` | `byte` | Player up, 1–4 |
| `ball` | `byte` | Ball number, from 1 |

### `void PinNodeCtrl_BallEnd(byte slot, byte ball, unsigned long score)`
**Required.** When a ball ends. **Why:** ball-by-ball scores; pending stats are sent here.

| Parameter | Type | Meaning |
|---|---|---|
| `slot` | `byte` | Player, 1–4 |
| `ball` | `byte` | Ball number |
| `score` | `unsigned long` | That player's score now |

### `void PinNodeCtrl_GameEnd(const unsigned long *scores, byte numPlayers, byte endedBy, byte ballsSource = 0)`
**Required.** Once per game. **Why:** the final scores are what leaderboards rank.

| Parameter | Type | Meaning |
|---|---|---|
| `scores` | `const unsigned long *` | Array of `numPlayers` scores, player 1 first |
| `numPlayers` | `byte` | 1–4 |
| `endedBy` | `byte` | `PINNODE_END_NORMAL` (0), `_SLAM` (1), `_RESET` (2: abandoned for a new game, not ranked as finished) |
| `ballsSource` | `byte` | Leave `0` |

### `void PinNodeCtrl_Tilt(byte slot, byte kind)`
**Optional.** On a tilt warning or tilt. **Why:** the machine's tilt counts; tournaments count tilted attempts.

| Parameter | Type | Meaning |
|---|---|---|
| `slot` | `byte` | Player, 1–4 |
| `kind` | `byte` | `PINNODE_TILT_WARNING` (0), `_TILT` (1), `_SLAM` (2) |

### `void PinNodeCtrl_Coin(byte chute, byte creditsAfter)`
**Optional.** When a coin is accepted. **Why:** the owner's coin and earnings stats; audits follow 2 s later.

| Parameter | Type | Meaning |
|---|---|---|
| `chute` | `byte` | 1–3 |
| `creditsAfter` | `byte` | Credits after the coin |

### `uint16_t PinNodeCtrl_GameCounter()`
**Optional.** Returns this game's number (kept in EEPROM at 4088–4089, counts up at each
`GameStart`). Don't use EEPROM 4088–4090 for anything else.

---

## 4. Achievements and stats

**Recommended.** **Why:** what players chase beyond the high score, and leaderboards beyond score.

### `void PinNodeCtrl_Achievement(byte slot, byte ach, long v = 0)`
Sent at once.

| Parameter | Type | Meaning |
|---|---|---|
| `slot` | `byte` | Player, 1–4 |
| `ach` | `byte` | `ACH_…` from `PinNodeGame.h` |
| `v` | `long` | Optional detail (e.g. the score it was earned at) |

### `void PinNodeCtrl_Stat(byte slot, byte stat, byte op, long v)`
Combined in RAM and sent within 500 ms, and at each ball end. Counted only during a game.

| Parameter | Type | Meaning |
|---|---|---|
| `slot` | `byte` | Player, 1–4 |
| `stat` | `byte` | `STAT_…` from `PinNodeGame.h` |
| `op` | `byte` | `PINNODE_STAT_ADD` (0): add `v` · `PINNODE_STAT_MAX` (1): keep the highest `v` |
| `v` | `long` | The amount or value |

---

## 5. Start from the app

**Recommended.** **Why:** a player who checks in and presses Start on the phone is tied to that game for certain.

### `boolean PinNodeCtrl_TakeAppStart(uint16_t *reqId, boolean appStartsOn)`
Call where you read the credit button (attract, and during a game to add a player). Returns
`true` when the app pressed Start; answer it with `PinNodeCtrl_StartResult`. With `appStartsOn`
false it answers `PINNODE_START_APP_OFF` itself and returns `false`.

| Parameter | Type | Meaning |
|---|---|---|
| `reqId` | `uint16_t *` | Out: the request id, for the answer |
| `appStartsOn` | `boolean` | Your "starts from the app" setting; `true` if you don't have one |

### `void PinNodeCtrl_StartResult(uint16_t reqId, byte result, byte slot)`

| Parameter | Type | Meaning |
|---|---|---|
| `reqId` | `uint16_t` | From `TakeAppStart` |
| `result` | `byte` | `PINNODE_START_OK` (0), `_NO_CREDITS` (1), `_MAX_PLAYERS` (2), `_NOT_NOW` (3), `_UNSUPPORTED` (4), `_APP_OFF` (5) |
| `slot` | `byte` | The player who started, 0 if refused |

A request not taken within 500 ms is answered `NOT_NOW`. (`PinNodeCtrl_TakeStartRequest(uint16_t *reqId)`
is the same without the setting.)

---

## 6. Credits from the app

**Optional.** **Why:** the owner adds credits remotely, through your own credit code.

### `boolean PinNodeCtrl_TakeCreditAdd(uint16_t *reqId, byte *n)`
Returns `true` when the owner asked to add credits. Take it in attract.

| Parameter | Type | Meaning |
|---|---|---|
| `reqId` | `uint16_t *` | Out: the request id |
| `n` | `byte *` | Out: credits to add |

### `void PinNodeCtrl_CreditResult(uint16_t reqId, byte result, byte n, byte credits)`

| Parameter | Type | Meaning |
|---|---|---|
| `reqId` | `uint16_t` | From `TakeCreditAdd` |
| `result` | `byte` | `PINNODE_CREDIT_OK` (0), `_NOT_NOW` (1), `_UNSUPPORTED` (2), `_OVER_MAX` (3), `_WRITE_FAIL` (4) |
| `n` | `byte` | Credits added (0 if refused) |
| `credits` | `byte` | Credits on the machine now |

Untaken after 3 s: answered `UNSUPPORTED`.

---

## 7. Claim codes

**Recommended.** **Why:** players who didn't check in get a 6-digit code after the game, and can
claim the score in the app.

### `byte PinNode_ShowClaimCodes(unsigned long now, const unsigned long *scores, const byte *playerUpLamps = 0)`
Last thing in each attract pass. Blinks each code on that player's score display (5 s code, 2 s
score). Returns one bit per display it drew (bit 0 = player 1): skip those in your own drawing.

| Parameter | Type | Meaning |
|---|---|---|
| `now` | `unsigned long` | `millis()` |
| `scores` | `const unsigned long *` | The last game's scores, 4 entries |
| `playerUpLamps` | `const byte *` | Optional: 4 lamp numbers, flashed while a code shows |

### `void PinNode_ClaimCodesStop()`
At game start. Clears the codes and their lamps.

### `bool PinNode_ClaimCodesShowing()`
`true` while any code shows: hold back attract screens that cover every display.

### `boolean PinNodeCtrl_ClaimCode(byte slot, unsigned long *code)`
Only if you draw codes yourself. Returns `true` and the code for that player (`slot` 1–4).

---

## 8. Settings in the app

**Optional.** **Why:** the owner changes settings from the phone. Marking difficulty settings
keeps leaderboards fair, and tournaments lock them.

### `struct PinNodeSetting`

| Field | Type | Meaning |
|---|---|---|
| `eeprom` | `unsigned short` | EEPROM address of the setting |
| `size` | `byte` | 1 = byte, 4 = `unsigned long` (little-endian, as RPU stores it) |
| `minValue`, `maxValue` | `long` | Range, in app units |
| `scale` | `unsigned int` | App value = stored value × scale (usually 1) |
| `liveSafe` | `boolean` | May change mid-game (volume yes, ball save no) |
| `difficulty` | `boolean` | Affects leaderboards; locked during tournaments |
| `step` | `long` | App units; 0 = any value in range |

```cpp
const PinNodeSetting MySettings[] = {
  // eeprom                     size min max scale liveSafe difficulty step
  { EEPROM_BALLS_OVERRIDE_BYTE,  1,   3,  10,  1,   false,   true,      0 },   // SET_BALLS_PER_GAME
  { EEPROM_MUSIC_VOLUME_BYTE,    1,   0,  10,  1,   true,    false,     0 },   // SET_MUSIC_VOLUME
};
```

### `void PinNodeCtrl_Settings(const PinNodeSetting *table, byte count, void (*applied)(void))`
Before `Begin`. One entry per manifest setting, in index order.

| Parameter | Type | Meaning |
|---|---|---|
| `table` | `const PinNodeSetting *` | Your table |
| `count` | `byte` | Entries |
| `applied` | `void (*)(void)` | Called after the app changed one (it's in EEPROM already): re-read your settings |

### `void PinNodeCtrl_SettingsChanged()`
After your operator menu closes, so the app sees edits made at the machine.

### `boolean PinNodeCtrl_SettingsLocked()` / `boolean PinNodeCtrl_SettingLocked(byte index)`
`true` during a tournament / `true` if that setting (manifest index) is a difficulty one and
locked. Check it in your operator menu before saving an edit.

---

## 9. Audits in the app

**Optional.** **Why:** plays, replays and coins in the app without opening the coin door.

### `struct PinNodeAudit`

| Field | Type | Meaning |
|---|---|---|
| `eeprom` | `unsigned short` | EEPROM address of the counter |
| `size` | `byte` | 1 or 4 |

### `void PinNodeCtrl_Audits(const PinNodeAudit *table, byte count)`
Before `Begin`. One entry per manifest audit, in index order. Sent after HELLO, each game end,
every 24 h, and when asked.

### `void PinNodeCtrl_AuditsChanged()`
After your operator menu closes (audits may have been reset).

### `boolean PinNodeCtrl_SendAuditsNow()`
Just before you clear the audits, so the old values are kept. Returns `true` if queued.

---

## 10. Tests and one-shots from the phone

**Optional.** **Why:** lamp, coil and switch tests, clearing audits and presets, run from the phone.

### The ready-made tests: `PinNode_TestsBegin` and `PinNode_Tests` (RPU-OS games)

Lamp, display, coil and switch tests, clear audits, and entering and leaving diagnostics, with two lines.
The converter adds both, and writes the matching `actions` list into the manifest.

```cpp
// setup(), before PinNodeCtrl_Begin: your coin-door coil test's callbacks (0 = the menu's defaults),
// and your sound test's callback (0 = no sound test)
PinNode_TestsBegin(SolenoidConvertDisplayNumberToIndex, SolenoidConvertDisplayNumberToTestStrength, 0);

// loop(), first in the machine-state chain
if (PinNode_Tests(CurrentTime, &newMachineState, MACHINE_STATE_ATTRACT)) {
} else if (MachineState == MACHINE_STATE_ATTRACT) {
```

The phone puts the machine into PinNode's own diagnostics state (`PINNODE_MACHINE_STATE_TESTS`, 125:
define it before including PinNode.h if your game uses 125). It does this from attract only. While
there, your attract and game code don't run. Report it as `PINNODE_MODE_SERVICE`. Leaving returns the
game to attract. The library stops a test after 60 s without the phone, and leaves diagnostics after
5 minutes with nothing running. To write your own tests instead, use the calls below.

### `struct PinNodeAction` and flags

| Field | Type | Meaning |
|---|---|---|
| `flags` | `byte` | OR of: `PINNODE_ACTION_TEST` (0x01: runs until stopped), `_LOCAL` (0x02: only at the machine), `_CONFIRM` (0x04: the phone asks first), `_LOCK` (0x08: refused during tournaments), `_ENTER_DIAG` (0x10), `_EXIT_DIAG` (0x20) |

### `struct PinNodeActionRequest`

| Field | Type | Meaning |
|---|---|---|
| `reqId` | `uint16_t` | For the answer |
| `action` | `byte` | Manifest action index |
| `op` | `byte` | `PINNODE_OP_START` (0) or `PINNODE_OP_STOP` (1) |
| `arg` | `long` | The action's argument (e.g. a lamp number) |
| `reason` | `byte` | For a stop: pass it back in your `DONE` answer |

### `void PinNodeCtrl_Actions(const PinNodeAction *table, byte count)`
Before `Begin`. One entry per manifest action, in index order.

### `boolean PinNodeCtrl_TakeAction(PinNodeActionRequest *req)`
In attract and diagnostics. Returns `true` with a request. Untaken after 500 ms: answered `NOT_NOW`.

### `void PinNodeCtrl_ActionResult(uint16_t reqId, byte action, byte result, long value, byte reason = 0)`

| Parameter | Type | Meaning |
|---|---|---|
| `reqId`, `action` | `uint16_t`, `byte` | From the request |
| `result` | `byte` | `PINNODE_ACTION_OK` (0: running), `_NOT_NOW` (1), `_UNKNOWN` (2), `_LOCAL_ONLY` (3), `_DONE` (4) |
| `value` | `long` | Optional result value |
| `reason` | `byte` | With `DONE`: `PINNODE_REASON_FINISHED` (0), `_STOPPED` (1), `_TIMEOUT` (2), `_DIAGNOSTICS` (4) |

The phone keeps a running test alive; the library stops it (a stop request for you) after 60 s
without a keepalive, or 3 s without the link.

### `byte PinNodeCtrl_ActionFlags(byte action)` / `void PinNodeCtrl_SwitchEvent(byte sw, boolean closed)`
The flags you registered for an action (0 if unknown) / during a switch test, report each switch
change (`sw` = switch number).

---

## 11. The operator menu (RPU example game menus)

**Optional.** **Why:** a stray self-test pulse can't stop a game, and the menu's blank first page
doesn't stay open.

### `boolean PinNode_SelfTestOpens(boolean inGame, unsigned long now)`
In your switch handler. `true` when the self-test switch should open the menu: one press outside
a game, two presses 0.3–3 s apart during one.

### `boolean PinNode_OperatorMenuGuard(M &menus, unsigned long now)`
Each menu pass, after `Menus.UpdateMenu()`. `menus` is your `OperatorMenus` object. Returns `true`
when it has just closed the menu (15 s idle on the blank first page, or the phone's exit).

---

## 12. Switching to the original game (RPU Rev 4 and up)

**Optional.** **Why:** the owner switches between your game and the machine's original ROM from the app.

```cpp
initOptions = PinNode_BootOptions(initOptions);
initResult = RPU_InitializeMPU(initOptions, SW_CREDIT_RESET);
PinNode_BootChosen(initResult);
if (initResult & RPU_RET_ORIGINAL_CODE_REQUESTED) { RPULink_BeginUart(); PinNode_RunOriginal(); }
```

| Call | Use |
|---|---|
| `unsigned long PinNode_BootOptions(unsigned long options)` | Before `RPU_InitializeMPU`: returns your options adjusted for the owner's choice |
| `void PinNode_BootChosen(unsigned long initResult)` | After it. Holding Credit/Reset at power-on always boots your game and clears the choice |
| `void PinNode_RunOriginal(void (*idle)(void) = 0)` | While the original runs: keeps the link up. Never returns. `idle` runs every pass. |
| `boolean PinNodeCtrl_OriginalChosen()` | `true` if the owner chose the original |
| `void PinNodeCtrl_SetBootTarget(byte original)` | Set the choice yourself: 1 = original, 0 = your game (kept in EEPROM 4090) |

---

## 13. Time

**Optional.** **Why:** real dates for high scores.

| Call | Returns |
|---|---|
| `boolean PinNodeCtrl_TimeKnown()` | `true` once the node has sent the time |
| `unsigned long PinNodeCtrl_UnixTime(unsigned long now)` | Unix time (seconds), or 0 if not known |

---

## 14. Firmware updates over the link (future SPI boards only)

**Optional.** RPU Rev 4 / 102 can't be reflashed over the link.

### `void PinNodeCtrl_SetOtaHandler(void (*handler)(byte phase))`
Without a handler, updates are refused. `phase`: `PINNODE_OTA_PHASE_NOTICE` (0: show "updating"),
`_SAFE` (1: coils and lamps off, arm the reset line), `_ABORT` (2: no reset came within 60 s).

### `uint32_t PinNodeCtrl_BuildId()`
The CRC-32 of your program, sent in HELLO.

---

## 15. Advanced

**Rarely needed:** for programs that report on behalf of something else.

| Call | Use |
|---|---|
| `void PinNodeCtrl_SetHello(const char *rulesetId, const char *version, const byte *manifestHash, uint32_t build, byte maxPlayers, byte flags)` | Report under another identity. Register settings, actions and audits first. |
| `void PinNodeCtrl_SetRecordHook(boolean (*hook)(byte type, const byte *data, byte len))` | See the node's records first; return `true` to take one. |

---

## Building without PinNode

`#define PINNODE_DISABLED` before `#include <PinNode.h>`: every call becomes an empty stand-in
(returning `false` or 0) and nothing is linked. Arguments aren't evaluated, so don't put side
effects in them.

## EEPROM the library uses

| Address | Size | Use |
|---|---|---|
| 4088–4089 | 2 | Game counter |
| 4090 | 1 | Boot choice (original or your game) |
