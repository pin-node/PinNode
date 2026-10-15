# The converter: add PinNode automatically

`pinnode-gen convert` adds PinNode to an existing game built on the RPU example game.
It places the same calls as [ADD-PINNODE.md](ADD-PINNODE.md), drafts the manifest, and reports
what it did. It never edits your folder: everything comes back in a ZIP.

## Install

You need Python 3.11 or newer.

```bash
git clone https://github.com/pin-node/PinNode
cd PinNode
python3 -m venv .venv
.venv/bin/pip install -e .
```

That installs the `pinnode-gen` command in `.venv/bin/`. Also install the PinNode Arduino library
(step 1 of [ADD-PINNODE.md](ADD-PINNODE.md)) to build the result.

## Run

```bash
.venv/bin/pinnode-gen convert path/to/MyGame \
    --ruleset-id my-game --name "My Game" --version 1.0.0 \
    --machine "My Machine (1980)" --repo you/my-game
```

| Option | What it is | Default |
|---|---|---|
| `path/to/MyGame` | The folder with your `.ino` | required |
| `--ruleset-id` | Your game's id on PinNode (lowercase, dashes) | from the folder name |
| `--name` | Your game's name | from the folder name |
| `--version` | Your game's version | `0.1.0` |
| `--machine` | The machine it runs on | the folder name |
| `--repo` | Your GitHub repo | empty |
| `-o` | Where to write the ZIP | `MyGame-pinnode.zip` |

It prints how many hooks it placed and how many are left for you.

## What you get

The ZIP holds:

- **Your sketch** with the PinNode calls added. Each one is marked `// PinNode`, so
  `git grep -n "// PinNode"` lists them.
- **`pinnode.game.json`**: the manifest, with settings and audits read from your operator menu.
- **`PinNodeGame.h`**: generated from it.
- **`CONVERSION.md`**: every hook it placed (file and line), and every one it couldn't place,
  with what to do.
- **`machine/`**: a machine definition for your machine.

It places:

- the include, settings and audit tables;
- the link and HELLO in `setup()`, and the link, mode and app starts in `loop()`;
- game start, players added, ball start and end, game over;
- coins and tilts;
- claim codes in attract;
- the operator-menu guards;
- the PinNode adjustments (App starts, PinNode WiFi reset) after your own adjustments, in any of the three RPU
  menu styles (OperatorMenus, `MACHINE_STATE_ADJUST_*`, extended self-test settings);
- switching to the original game.

## Then

1. Read `CONVERSION.md` and add any hook it couldn't place, by hand.
2. Check the settings in `pinnode.game.json`: labels, ranges, which ones are `difficulty`.
3. Add your achievements and stats to the manifest, run `pinnode-gen generate pinnode.game.json`,
   and report them from your rules (step 8 of [ADD-PINNODE.md](ADD-PINNODE.md)).
4. **pin-node.com/setup → Game:** upload `pinnode.game.json`, or paste your repo's address. The
   setup then takes you through the node ([ADD-PINNODE.md](ADD-PINNODE.md), "Set up the machine").
5. Build, flash it with the Arduino IDE, and play a game. Games record unranked for now.
6. Copy your folder over the original and commit.

The converter refuses a game that already has PinNode calls, and a folder with no `.ino`
containing `setup()` and `loop()`.

## For the web converter

`convert()` returns `hooks` and `report_info` (JSON). After its AI step the web rebuilds CONVERSION.md with
`render_report(report_info, hooks, ai)`, where `ai` is `{"placed": [{hook, file, line, why}],
"achievements": [{id, name, desc, points, where, wired}], "skipped": [{hook, why}]}` and `hook` is a
hook's `key` (or name). The AI's hooks leave the to-do list; unwired achievements join it.
