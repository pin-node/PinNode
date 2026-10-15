"""
pinnode-gen scan: draft a manifest's `settings` from an RPU game's source
(SPEC-game-manifest §12).

RPU keeps adjustments in code, in one of two styles:

- ExampleMachine's operator menus: each is a `case OM_...:` that sets
  `adjustmentType`, `adjustmentValues[]`, the variable
  (`currentAdjustmentByte` / `currentAdjustmentUL`) and the EEPROM slot
  (`currentAdjustmentStorageByte`).
- The older self-test adjustments (Trident2020RPU, SpaceBattle2022,
  Scorpion2022-RPU, Demon23…): each is a `case MACHINE_STATE_ADJUST_...:`
  setting the same things with capitalised names (`AdjustmentType = ADJ_TYPE_LIST`,
  `CurrentAdjustmentByte`, …).

Settings before the `switch` are the block's defaults. Defaults come from the
game's defaults function (`Var = value;`), or else from the load-time
`ReadSetting(slot, default[, max])` / `RPU_ReadULFromEEProm(slot, default)`.

The output is a DRAFT for the author: names come from menu ids, and nothing
in the source says which settings change leaderboards (`difficulty`) or are
unsafe to change remotely, so those are only guessed and flagged in the
report. It never guesses silently: every guess is listed.

    python -m tools.pinnode_gen.scan path/to/GameRepo [--json draft.json] [--report report.md]
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

SOURCE_GLOBS = ("*.ino", "*.cpp", "*.h")

# Menu entries that are state the game changes during play, not configuration
# (SPEC-game-manifest §5, "Settings versus state").
STATE_HINTS = ("CREDITS", "HISCR", "HIGH_SCORE", "HIGHSCORE")

DEFINE_RE = re.compile(r"^\s*#define\s+([A-Za-z_]\w*)\s+(0x[0-9A-Fa-f]+|-?\d+)\b", re.M)
CASE_RE = re.compile(r"case\s+([A-Za-z_]\w*)\s*:")
ASSIGN_RE = re.compile(r"([A-Za-z_]\w*(?:\[[^\]]+\])?)\s*=\s*([^;]+);")
# Var = ReadSetting(EEPROM_X, default[, max]); the args never span another call (no parens, no ';')
READSETTING_RE = re.compile(r"(\w+)\s*=\s*ReadSetting\(\s*(\w+)\s*,\s*([^,();]+?)\s*(?:,\s*([^,();]+?)\s*)?\)")
# the 2-argument form's real top: `if (Var > 20) Var = 20;` (or `Var>10`)
CLAMP_RE = re.compile(r"if\s*\(\s*(\w+)\s*>\s*(\w+)\s*\)\s*\1\s*=\s*(\w+)\s*;")
# By slot, with or without a max: ReadSetting(EEPROM_X, 15) / (EEPROM_X, 15, 30),
# and RPU_ReadULFromEEProm(EEPROM_X_UL, 20000) for scores.
READ_SLOT_RE = re.compile(r"\b(?:ReadSetting|\w*ReadULFromEEProm)\(\s*(\w+)\s*,\s*([^,()]+?)\s*"
                          r"(?:,\s*([^,()]+?)\s*)?\)")
MENU_CASE = r"case\s+(?:OM_|MACHINE_STATE_ADJUST_)"
MENU_PREFIX = re.compile(r"^(?:OM_(?:(?:BASIC_ADJ_IDS|GAME_ADJ|ADJ_IDS|ADJ)_)?|MACHINE_STATE_ADJUST_)")
CPC_TABLE_RE = re.compile(r"CPCPairs\s*\[[^\]]*\]\s*\[\s*2\s*\]\s*=\s*\{(.*?)\};", re.S)


@dataclass
class Draft:
    setting: dict
    notes: list[str] = field(default_factory=list)


@dataclass
class ScanResult:
    settings: list[Draft]
    audits: list[dict]
    skipped: list[str]
    unmapped_eeprom: list[str]
    warnings: list[str]
    files: list[str]


def _num(v: str, defines: dict[str, int]) -> Optional[int]:
    v = v.strip().removeprefix("(byte)").strip()
    if v in ("true", "TRUE"):
        return 1
    if v in ("false", "FALSE"):
        return 0
    if re.fullmatch(r"0x[0-9A-Fa-f]+", v):
        return int(v, 16)
    if re.fullmatch(r"-?\d+", v):
        return int(v)
    return defines.get(v)


def _strip_comments(src: str) -> str:
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    return re.sub(r"//[^\n]*", "", src)


def _humanise(menu_id: str) -> tuple[str, str]:
    core = MENU_PREFIX.sub("", menu_id)
    core = (core.replace("FREEPLAY", "FREE_PLAY").replace("SOUNDFX", "SFX")
            .replace("CPC_", "COINS_PER_CREDIT_"))
    words = core.lower().split("_")
    name = " ".join(words).capitalize()
    name = re.sub(r"^Coins per credit (\d)$", r"Coins per credit, chute \1", name)
    name = name.replace("Sfx", "Sound effects").replace("Hiscr", "High score")
    sid = "-".join(words)
    while len(sid) > 32 and "-" in sid:      # the manifest's id limit: drop whole words
        sid = sid.rsplit("-", 1)[0]
    return sid[:32], name


def _group(menu_id: str, var: str) -> tuple[str, bool]:
    """(group, requiresPresence). A guess, always reported."""
    key = f"{menu_id} {var}".upper()
    if "STRENGTH" in key or "SOLENOID" in key or "COIL" in key:
        return "service", True
    if "VOLUME" in key or "SOUND" in key or "MUSIC" in key or "CALLOUT" in key:
        return "sound", False
    if "CPC" in key or "FREEPLAY" in key or "FREE_PLAY" in key or "CREDIT" in key:
        return "pricing", False
    if "SCROLL" in key or "DISPLAY" in key:
        return "display", False
    if "SCORE" in key or "VALUE" in key or "AWARD" in key:
        return "awards", False
    if "MATCH" in key:
        return "features", False
    return "game", False


def _cpc_labels(src: str) -> Optional[list[dict]]:
    m = CPC_TABLE_RE.search(src)
    if not m:
        return None
    pairs = re.findall(r"\{\s*(\d+)\s*,\s*(\d+)\s*\}", m.group(1))
    out = []
    for i, (coins, credits) in enumerate(pairs):
        c, k = int(coins), int(credits)
        out.append({"v": i, "label": f"{c} coin{'s' if c != 1 else ''} / "
                                     f"{k} credit{'s' if k != 1 else ''}"})
    return out


FUNC_RE = re.compile(r"\b(?:void|byte|int|boolean|bool)\s+[A-Za-z_]\w*\s*\([^)]*\)\s*\{")


def _defaults(src: str, defines: dict[str, int]) -> tuple[dict[str, int], dict[str, set]]:
    """The factory defaults, taken from the function that assigns the most
    literal values (RPU games have one, e.g. a "set defaults" routine), with
    every literal ever assigned to each variable kept for conflict reports."""
    lit = re.compile(r"^[A-Za-z_]\w*(\[\d+\])?$")
    all_literals: dict[str, set] = {}
    for lhs, rhs in ASSIGN_RE.findall(src):
        v = _num(rhs, defines)
        if v is not None and lit.match(lhs):
            all_literals.setdefault(lhs, set()).add(v)
    best: dict[str, int] = {}
    for m in FUNC_RE.finditer(src):
        depth, i = 1, m.end()
        while depth and i < len(src):
            depth += {"{": 1, "}": -1}.get(src[i], 0)
            i += 1
        found = {}
        for lhs, rhs in ASSIGN_RE.findall(src[m.end():i - 1]):
            v = _num(rhs, defines)
            if v is not None and lit.match(lhs):
                found[lhs] = v
        if len(found) > len(best):
            best = found
    # Variables the defaults function doesn't touch fall back to their only literal.
    for var, vals in all_literals.items():
        if var not in best and len(vals) == 1:
            best[var] = next(iter(vals))
    return best, all_literals


def _blocks(src: str) -> list[tuple[str, str]]:
    """(preamble, switch body) for each `switch` with menu cases (either style).
    The preamble is the text between the previous block boundary and the
    switch: where the block-level defaults are set."""
    out = []
    for m in re.finditer(r"switch\s*\([^)]*\)\s*\{", src):
        depth, i = 1, m.end()
        while depth and i < len(src):
            depth += {"{": 1, "}": -1}.get(src[i], 0)
            i += 1
        body = src[m.end():i - 1]
        if not re.search(MENU_CASE, body) or "currentadjustment" not in body.lower():
            continue
        pre_start = max(src.rfind("{", 0, m.start() - 1), src.rfind("}", 0, m.start() - 1))
        out.append((src[pre_start:m.start()], body))
    return out


def _state(stmts: str, defines: dict[str, int], base: Optional[dict] = None) -> dict:
    st = dict(base or {"type": "OPERATOR_MENU_ADJ_TYPE_MIN_MAX", "n": 2, "values": [0] * 8,
                       "var": None, "ul": False, "eeprom": None})
    st["values"] = list(st["values"])
    for lhs, rhs in ASSIGN_RE.findall(stmts):
        rhs = rhs.strip()
        key = lhs[:1].lower() + lhs[1:]           # the older style capitalises these
        if key == "adjustmentType":
            # ADJ_TYPE_LIST (older) reads as OPERATOR_MENU_ADJ_TYPE_LIST
            st["type"] = re.sub(r"^ADJ_TYPE_", "OPERATOR_MENU_ADJ_TYPE_", rhs)
        elif key == "numAdjustmentValues":
            st["n"] = _num(rhs, defines) or st["n"]
        elif key.startswith("adjustmentValues["):
            idx = _num(key[len("adjustmentValues["):-1], defines)
            val = _num(rhs, defines)
            if idx is not None and val is not None and idx < 8:
                st["values"][idx] = val
        elif key in ("currentAdjustmentByte", "currentAdjustmentUL"):
            var = re.sub(r"^\(\s*\w+\s*\*\s*\)\s*", "", rhs).lstrip("&").strip()
            var = var.strip("()").strip()          # &(CPCSelection[0])
            if var != "NULL":
                st["var"], st["ul"] = var, key == "currentAdjustmentUL"
        elif key == "currentAdjustmentStorageByte":
            st["eeprom"] = rhs if rhs != "0" else None
    return st


def scan(repo: Path) -> ScanResult:
    root = repo.resolve()
    files = sorted({p for g in SOURCE_GLOBS for p in repo.glob(g)
                    if not p.is_symlink() and p.resolve().is_relative_to(root)})
    raw = "\n".join(p.read_text(errors="replace") for p in files)
    src = _strip_comments(raw)
    defines = {k: _num(v, {}) for k, v in DEFINE_RE.findall(src)}
    defines = {k: v for k, v in defines.items() if v is not None}

    defaults, all_literals = _defaults(src, defines)
    clamps = {var: _num(top, defines) for var, top, to in CLAMP_RE.findall(src) if top == to}
    read_bounds = {var: (slot, _num(d, defines), _num(mx, defines) if mx else clamps.get(var))
                   for var, slot, d, mx in READSETTING_RE.findall(src)}
    read_by_slot = {slot: (_num(d, defines), _num(mx, defines) if mx else None)
                    for slot, d, mx in READ_SLOT_RE.findall(src)}
    cpc = _cpc_labels(src)

    drafts: list[Draft] = []
    skipped: list[str] = []
    warnings: list[str] = []
    used_eeprom: set[str] = set()

    for preamble, body in _blocks(src):
        base = _state(preamble, defines)
        cases = list(CASE_RE.finditer(body))
        for n, c in enumerate(cases):
            end = cases[n + 1].start() if n + 1 < len(cases) else len(body)
            menu_id = c.group(1)
            st = _state(body[c.end():end], defines, base)
            if not st["var"]:
                continue
            if any(h in menu_id.upper() for h in STATE_HINTS):
                skipped.append(f"{menu_id} ({st['var']}): state, not a setting; use a command")
                continue
            sid, name = _humanise(menu_id)
            group, presence = _group(menu_id, st["var"])
            kind = st["type"].removeprefix("OPERATOR_MENU_ADJ_TYPE_")
            notes: list[str] = []
            s: dict = {"id": sid, "name": name, "group": group}
            dflt = defaults.get(st["var"])
            slot_default = read_by_slot.get(st["eeprom"], (None, None))[0]
            if slot_default is not None and (dflt is None or menu_id.startswith("MACHINE_STATE_")):
                # Older games have no defaults function: the factory default is the
                # one ReadSetting(slot, default) writes to a blank EEPROM.
                dflt = slot_default
            elif menu_id.startswith("MACHINE_STATE_") and st["ul"]:
                # ...or a score read without one, then checked:
                # if (ExtraBallValue % 1000 || ExtraBallValue > 100000) ExtraBallValue = 20000;
                v = re.escape(st["var"])
                m = re.search(rf"if\s*\([^;{{]*\b{v}\b[^;{{]*\)\s*{v}\s*=\s*(\d+)\s*;", src)
                if m:
                    dflt = int(m.group(1))
            if st["ul"] or kind.startswith("SCORE"):
                s.update(type="score", min=0, max=99999990, step=10)
                notes.append("score range 0–99,999,990 assumed (RPU enters scores digit by digit)")
            elif kind == "CPC":
                if cpc:
                    s.update(type="enum", values=cpc)
                else:
                    s.update(type="int", min=st["values"][0], max=st["values"][1])
                    notes.append("CPCPairs table not found; labels needed")
            elif kind == "LIST":
                vals = st["values"][: st["n"]]
                s.update(type="enum", values=[{"v": v, "label": str(v)} for v in vals])
                notes.append("list labels are raw numbers; add units or words")
            elif kind == "MIN_MAX_DEFAULT" and dflt == 99:
                # RPU's "or default": 99 means use the game's own value
                lo, hi = st["values"][0], st["values"][1]
                s.update(type="enum", values=[{"v": 99, "label": "Game default"}]
                         + [{"v": v, "label": str(v)} for v in range(lo, hi + 1)])
                notes.append("MIN_MAX_DEFAULT: 99 is \"game default\"; add words to the numbers")
            else:
                lo, hi = st["values"][0], st["values"][1]
                if (lo, hi) == (0, 1):
                    s["type"] = "bool"
                else:
                    s.update(type="int", min=lo, max=hi)
                if kind == "MIN_MAX_DEFAULT":
                    notes.append("MIN_MAX_DEFAULT: check whether this is really a bitmask (flags)")
            if dflt is not None:
                s["default"] = dflt
            else:
                s["default"] = s.get("min", 0)
                notes.append("default not found in source; used the minimum")
            if presence:
                s["requiresPresence"] = True
                notes.append("guessed service/requiresPresence (coil or solenoid)")
            if menu_id.endswith(("_TOURNAMENT_MODE", "_TOURNAMENT_SCORING")) and s["type"] == "bool":
                # RPU's Tournament scoring: 1 = on. Tournaments check it.
                s["tournamentMode"] = 1
                s["difficulty"] = True
                notes.append("marked as the tournament switch (tournamentMode: 1)")
            source: dict = {"menuId": menu_id, "variable": st["var"]}
            slot = st["eeprom"]
            if slot:
                used_eeprom.add(slot)
                if slot in defines:
                    source["eeprom"] = defines[slot]
            s["source"] = source
            rb = read_bounds.get(st["var"])
            if rb:
                _, rdefault, rmax = rb
                top = s.get("max") or (max(v["v"] for v in s["values"]) if "values" in s else 1)
                if rmax is not None and rmax != top and s["type"] != "score":
                    warnings.append(f"{menu_id}: menu allows up to {top} but ReadSetting "
                                    f"accepts up to {rmax}")
                if rdefault is not None and dflt is not None and rdefault != dflt:
                    warnings.append(f"{menu_id}: default {dflt} in the defaults function, "
                                    f"{rdefault} in ReadSetting")
            notes.append("difficulty? decide whether this changes leaderboards")
            drafts.append(Draft(s, notes))

    audits = _audits(src, defines, used_eeprom)

    eeprom_names = {k for k in defines if k.startswith("EEPROM_") and not k.endswith("_PROOF_UL")
                    and "INIT_PROOF" not in k}
    unmapped = sorted(n for n in eeprom_names - used_eeprom)
    for i, d in enumerate(drafts):
        d.setting = {"index": i, **d.setting}
    return ScanResult(drafts, audits, skipped, unmapped, warnings, [p.name for p in files])


# RPU's standard audit slots → the manifest's standard audit ids
# (SPEC-game-manifest §6), so operator views line up across games.
STANDARD_AUDITS = {
    "RPU_TOTAL_PLAYS_EEPROM_START_BYTE": ("total-plays", "Total plays"),
    "RPU_CHUTE_1_COINS_START_BYTE": ("chute-1-coins", "Chute 1 coins"),
    "RPU_CHUTE_2_COINS_START_BYTE": ("chute-2-coins", "Chute 2 coins"),
    "RPU_CHUTE_3_COINS_START_BYTE": ("chute-3-coins", "Chute 3 coins"),
    "RPU_TOTAL_REPLAYS_EEPROM_START_BYTE": ("total-replays", "Total replays"),
    "RPU_TOTAL_HISCORE_BEATEN_START_BYTE": ("high-score-beaten", "High score beaten"),
}


def _audits(src: str, defines: dict[str, int], used: set[str]) -> list[dict]:
    """Audit counters from the operator menu's audits `switch` (the one that
    ends in SetAuditControls), in menu order."""
    out: list[dict] = []
    for m in re.finditer(r"switch\s*\([^)]*\)\s*\{", src):
        depth, i = 1, m.end()
        while depth and i < len(src):
            depth += {"{": 1, "}": -1}.get(src[i], 0)
            i += 1
        after = src[i:i + 400]
        if "SetAuditControls" not in after:
            continue
        for slot in re.findall(r"currentAdjustmentStorageByte\s*=\s*(\w+)\s*;", src[m.end():i]):
            used.add(slot)
            core = re.sub(r"^RPU_|_EEPROM|_START_BYTE|_BYTE$", "", slot)
            aid, name = STANDARD_AUDITS.get(slot) or (
                core.lower().replace("_", "-"), core.replace("_", " ").capitalize())
            out.append({"index": len(out), "id": aid, "name": name})
    return out


def report(r: ScanResult) -> str:
    lines = ["# pinnode-gen scan report", "",
             f"Scanned: {', '.join(r.files)}", ""]
    if not r.settings and not r.skipped:
        lines += ["**No RPU adjustments found.** This scanner reads ExampleMachine's "
                  "operator menus (`case OM_...:`) and the older self-test adjustments "
                  "(`case MACHINE_STATE_ADJUST_...:`). This game keeps settings some other "
                  "way: write the manifest `settings` by hand, starting from "
                  "examples/rpu-standard.pinnode.json.", ""]
    lines += [
             f"## Drafted settings ({len(r.settings)})", ""]
    for d in r.settings:
        s = d.setting
        lines.append(f"- **{s['name']}** (`{s['id']}`, {s['type']}): " + "; ".join(d.notes))
    if r.audits:
        lines += ["", f"## Audits ({len(r.audits)})", ""] + \
                 [f"- **{a['name']}** (`{a['id']}`)" for a in r.audits]
    if r.skipped:
        lines += ["", "## Skipped (state, not settings)", ""] + [f"- {x}" for x in r.skipped]
    if r.unmapped_eeprom:
        lines += ["", "## EEPROM slots not in any menu (check by hand)", ""] + \
                 [f"- `{x}`" for x in r.unmapped_eeprom]
    if r.warnings:
        lines += ["", "## Inconsistencies in the game code", ""] + [f"- {x}" for x in r.warnings]
    return "\n".join(lines) + "\n"


def main(argv: Optional[list[str]] = None) -> int:
    p = argparse.ArgumentParser(prog="pinnode-gen scan", description=__doc__.split("\n\n")[0])
    p.add_argument("repo", type=Path)
    p.add_argument("--json", type=Path, help="write the draft settings here")
    p.add_argument("--report", type=Path, help="write the review report here")
    a = p.parse_args(argv)
    r = scan(a.repo)
    draft = json.dumps({"settings": [d.setting for d in r.settings], "audits": r.audits},
                       indent=2)
    if a.json:
        a.json.write_text(draft + "\n")
    else:
        print(draft)
    text = report(r)
    if a.report:
        a.report.write_text(text)
    else:
        print(text, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
