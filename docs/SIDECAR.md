# Sidecar packs: your game's screen on the phones at the machine

Sidecar (the open RPU library, MIT) lets your game send states, timers and events. On a PinNode 400 those reach
every phone at the machine. A **pack** says how they look there: PinNode's widgets bound to your game's keys,
and, if you like, your own images and videos.

## What you keep in your game's repo

```
pinnode.game.json       your manifest, with its sidecar block (pinnode-gen sidecar fills it from sidecar.json)
sidecar.json            your game's calls: every state, timer and event, by number
sidecar/pack.json       the layout, and "rights": your statement that every file is your own or licensed to you
sidecar/media/          your images and videos, named <id>.<ext>; the layout uses each as "media:<id>"
```

Media: PNG, JPEG or WebP images (up to 2 MB each) and MP4 or WebM videos (up to 10 MB and 30 s each, with no sound
track), at most 64 files and 50 MB. No sounds. Your own photos and video of your own machine are fine. Not allowed
without permission: the maker's art or logos used as your pack's graphics (a cropped backglass, a logo as a badge).
Never anything taken from a game's ROM (display frames, animations), or people who haven't agreed.

## The layout: sidecar/pack.json (format 1)

A pack's `sidecar/pack.json` says how your game's screen looks on the phones at the machine. It's data, not
code: PinNode's own widgets, bound to the states, timers and events in your manifest's `sidecar` block by their
`id`. Phones check every pack before they show it, and show nothing at all for one with a problem. The
developer menu's **Sidecar preview** in the app lists every problem and runs the pack on made-up values;
`pinnode-gen pack` checks the same rules before you publish.

```json
{
  "rights": "Every picture and video in this pack is my own work.",
  "layout": 1,
  "accent": "#33ccff",
  "screen": { "type": "column", "children": [ ... ] },
  "events": [ { "event": "jackpot", "text": "Jackpot! {value}", "media": "media:jackpot", "seconds": 4 } ]
}
```

| Field | | |
|---|---|---|
| `rights` | required | your statement that every file is your own or licensed to you (20 characters or more) |
| `layout` | required | `1` |
| `accent` | optional | `#rrggbb`: bars, rings, lamps, the player up and event borders |
| `screen` | required | one widget, usually a `column` |
| `events` | optional | up to 64: what to show when an event arrives |

### Widgets

Every widget has a `type` and may have a `when` (below). Any other field, or a type not in this list, refuses the
pack.

| type | fields | shows |
|---|---|---|
| `column`, `row` | `children`: 1–24 widgets | widgets one under another, or side by side (wrapping) |
| `players` | `children`: 1–24 widgets | its children once for each player in the game, the player up outlined and the phone's own player marked |
| `text` | `text` (1–120 characters), `size`: `small`, `body`, `heading` or `title` | text; `{state-id}` is that state's value, `{player}` the player's number; any other `{...}` refuses the pack |
| `value` | `state`, `label`, `big` (true/false) | a state's value, named: an enum's name, the flags set, a percent or a number |
| `bar` | `state`, `label`, `color` | a state filling a bar from `min` to `max` (a percent: 0–100) |
| `ring` | `state`, `label`, `color` | the same as twelve lamps in a ring, the value in the middle |
| `lamps` | `state` (kind `flags`), `label`, `color` | one lamp per named bit, lit while it's set |
| `timer` | `timer`, `label`, `style` (`bar` or `text`), `color` | a timer counting down; hidden while it isn't running or paused |
| `image` | `media`, `height` (24–480, default 160) | one of your images |
| `video` | `media`, `height` | one of your videos, looping and silent |

`label` defaults to the manifest's `name`. `color` is `#rrggbb` and defaults to `accent`. `media` is
`"media:<id>"`, a file in `sidecar/media/`; an `image` must name an image and a `video` a video.

**Whose state:** a player-scoped state or timer means that player inside `players`, and the player up anywhere
else. A game-scoped one is the same for everyone.

### `when`: show a widget only sometimes

| | shows while |
|---|---|
| `{"state": "story", "is": 2}` | the state is 2 |
| `{"state": "story", "not": 0}` | the state isn't 0 |
| `{"state": "combos", "above": 3}` | the state is more than 3 |
| `{"state": "combos", "below": 3}` | the state is less than 3 |
| `{"state": "locks", "bit": 2}` | bit 2 of the state is set |
| `{"timer": "multiball"}` | the timer is running or paused |

One test per `when`; `bit` is 0–31. A state not heard yet hides the widget.

### Events

Each entry names an event `id` from the manifest, and shows `text`, `media` (an image or video), or both, for
`seconds` (1–10, default 3). In `text`, `{value}` is the event's value and `{player}` its player; nothing else in braces. Events with no
entry show nothing. A phone shows each event once, and none from before its screen opened.

### Limits

At most 200 widgets, nested 8 deep. The pack file is at most 512 KB. Media as `pinnode-gen pack` checks them:
PNG, JPEG or WebP up to 2 MB, MP4 or WebM up to 10 MB and 30 s with no sound, 64 files and 50 MB in all.

## Or your own page: sidecar/pack.html

Instead of the layout, or beside it, a pack can carry one HTML page of your own. Phones show it in a locked-down
view with no network, and use it in place of the layout when both are there.

- One self-contained file, up to 2 MB: styles, scripts and images inline (images as `data:` addresses).
- Nothing from the network: no `src=` or `href=` to an address, no CSS `url()` to one, no `@import`, `fetch()`,
  `XMLHttpRequest`, `WebSocket` or `EventSource`. `pinnode-gen pack` refuses a page that has any of them.
- `sidecar/pack.json` still holds your `"rights"` statement; with a page and no layout, that's all it needs.
- Files in `sidecar/media/` are for the layout. A page can't load them, so put what it shows inside it.

**What your page receives.** The app calls `window.sidecar.receive(text)` on your page. `text` is one or more
lines in the SidecarSerial format, separated by `\n`, the same lines the library sends to a laptop:

| Line | Means |
|---|---|
| `SC G <started> <game>` | `started` 1: a game is on; a 1 with a new `game` (the game number's low byte) is a new game; 0: it's over |
| `SC S <slot> <key> <value>` | a state: slot 0 is the whole game, 1-4 a player; `key` as in your `sidecar.json` |
| `SC T <slot> <key> <op> <left ms> <total ms>` | a timer: op 0 stopped, 1 running (count down from now), 2 paused |
| `SC E <slot> <code> <value>` | an event |

When the page opens, the app sends everything it has: the `G` line, every state, and the running or paused
timers with their time left. Then it sends each change as it arrives. Ignore line kinds you don't know; more may be added.

Events are best effort: a phone can miss one, and a page never gets events from before it opened. Anything your
screen must get right belongs in a state or a timer.

**What your page can't do:** load anything (only inline scripts and styles, and `data:` or `blob:` images, fonts and
media), keep storage between visits, open links or windows, or send anything back. It's shown at about 72% of the
phone's height with its own scrolling off, so lay it out to fit.

## Publishing (one command)

Publish your game's release as usual, then:

```
pinnode-gen pack --tag v0.2.0 --upload --verify
```

- It checks the layout and the media, and writes `sidecar-dist/sidecar-pack.json`: the layout, each file's
  address, size and SHA-256, and which of your game's keys it's drawn for.
- `--upload` puts the pack and your media on that GitHub release with your own `gh` login.
- `--verify` downloads every file again and checks it, as a phone would.

**You host your pack and media, not PinNode.** PinNode never downloads or checks them. Phones download them from your release (so the repo must be public)
and refuse any media file that doesn't match its SHA-256. Your manifest's sidecar block says where the pack is,
once: `"pack": {"url": "https://github.com/you/your-game/releases/latest/download/sidecar-pack.json"}`
(`pinnode-gen pack` prints the address to use). The "latest" address always finds your newest pack.

- **A private game repo:** put the media on a public repo's release (`--repo you/my-game-media`), or host them
  anywhere public over HTTPS with `--base-url https://your.site/my-game/0.2.0`, then upload the files from `sidecar-dist/`
  there yourself.
- **On your own network:** a NAS or a computer at home can serve the pack over plain http:
  `--base-url http://192.168.1.20/my-game`, then copy the files from `sidecar-dist/` there. Plain http works only
  for local addresses (192.168.x, 10.x, 172.16-31.x, or a `.local` name). Only phones joined to that network
  see the pack, and the first time one loads it, iPhones ask the player for Local Network permission.
- **Changing the art:** make the pack again and upload it. No new firmware is needed while your keys stay the same.
  If you change `sidecar.json`, make the pack again too: a pack only shows on builds with the keys it was drawn for.
