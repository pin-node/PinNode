"""
pinnode-gen convert: add PinNode to an existing RPU game.

    from tools.pinnode_gen.convert import convert
    r = convert(Path("MyGame"), machine="My Machine (1980)", ruleset_id="rpu-my-game")
    Path("MyGame-pinnode.zip").write_bytes(r.zip_bytes)

Inserts the PinNode hooks where RPU games (Dick Hamill's ExampleMachine family) do the work, drafts
pinnode.game.json (settings and audits via scan, lamp/switch/coil names), writes a machine
definition for RPUMachines, and reports every hook it placed, and every one it couldn't, in
CONVERSION.md. It never edits the source folder: everything comes back in the ZIP.

Pure Python, standard library only, so the web service can run it in-process.
"""

from __future__ import annotations

import io
import json
import re
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

from . import scan as scan_mod

MARK = "// PinNode"
SOURCE_SUFFIXES = (".ino", ".h", ".cpp")


@dataclass
class Hook:
    name: str
    placed: bool
    where: str = ""          # "file:line (function)"
    note: str = ""

    @property
    def key(self) -> str:
        """A stable id for the web converter's AI step: the name in snake case ("game_start",
        "operator_menu_self_test_guard_and_idle_exit"). Names don't change between versions."""
        return re.sub(r"[^a-z0-9]+", "_", self.name.lower()).strip("_")


@dataclass
class ConvertResult:
    zip_bytes: bytes
    report_md: str
    manifest: dict
    machine_def: dict
    hooks: list[Hook] = field(default_factory=list)
    report_info: dict = field(default_factory=dict)   # JSON: render_report(report_info, hooks, ai) rebuilds report_md

    @property
    def unplaced(self) -> list[Hook]:
        return [h for h in self.hooks if not h.placed]


# ------------------------------------------------------------------ source handling

class Source:
    """One sketch file, edited in memory. Positions are recomputed after every insert."""

    def __init__(self, path: Path, text: str):
        self.path, self.text = path, text

    def line_of(self, pos: int) -> int:
        return self.text.count("\n", 0, pos) + 1

    def insert(self, pos: int, code: str) -> None:
        self.text = self.text[:pos] + code + self.text[pos:]


def _skip_code(text: str, i: int) -> int:
    """If text[i] starts a comment or string, return the index after it, else i."""
    if text.startswith("//", i):
        j = text.find("\n", i)
        return len(text) if j < 0 else j
    if text.startswith("/*", i):
        j = text.find("*/", i + 2)
        return len(text) if j < 0 else j + 2
    if text[i] in "\"'":
        q, j = text[i], i + 1
        while j < len(text) and text[j] != q:
            j += 2 if text[j] == "\\" else 1
        return j + 1
    return i


def _match_brace(text: str, open_pos: int) -> int:
    """Index of the } closing the { at open_pos (comments and strings skipped)."""
    depth, i = 0, open_pos
    while i < len(text):
        j = _skip_code(text, i)
        if j != i:
            i = j
            continue
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def _function(src: Source, name: str) -> Optional[tuple[int, int, list[str]]]:
    """(body start after {, body end at }, parameter names) of a function definition."""
    for m in re.finditer(rf"^[ \t]*[\w\s\*&:<>]*?\b{name}\s*\(([^;{{)]*)\)\s*\{{", src.text, re.M):
        close = _match_brace(src.text, m.end() - 1)
        if close < 0:
            continue
        params = [re.split(r"[\s\*&]+", p.split("=")[0].strip())[-1]
                  for p in m.group(1).split(",") if p.strip() and p.strip() != "void"]
        return m.end(), close, params
    return None


def _indent_at(text: str, pos: int) -> str:
    start = text.rfind("\n", 0, pos) + 1
    return re.match(r"[ \t]*", text[start:]).group(0)


def _line_start(text: str, pos: int) -> int:
    return text.rfind("\n", 0, pos) + 1


def _end_of_body(text: str, body_start: int, close: int) -> int:
    """Where to add a statement at the end of a function: before a final `return …;`, if there
    is one (code after it would never run), else before the closing brace."""
    returns = list(re.finditer(r"^[ \t]*return\b[^;]*;\s*", text[body_start:close], re.M))
    last = returns[-1] if returns else None
    if last and text[body_start + last.end():close].strip() == "":
        return _line_start(text, body_start + last.start())
    return _line_start(text, close)


def _block(lines: list[str], indent: str) -> str:
    return "".join(f"{indent}{ln}\n" if ln else "\n" for ln in lines)


# ------------------------------------------------------------------ hooks

class Converter:
    def __init__(self, files: list[Source]):
        self.files = files
        self.hooks: list[Hook] = []
        self.all_text = "\n".join(f.text for f in files)

    def main(self) -> Optional[Source]:
        for f in self.files:
            if f.path.suffix == ".ino" and _function(f, "setup") and _function(f, "loop"):
                return f
        return None

    def find(self, name: str) -> Optional[tuple[Source, tuple[int, int, list[str]]]]:
        for f in self.files:
            fn = _function(f, name)
            if fn:
                return f, fn
        return None

    def has(self, ident: str) -> bool:
        return re.search(rf"\b{re.escape(ident)}\b", self.all_text) is not None

    def first_ident(self, *candidates: str) -> Optional[str]:
        return next((c for c in candidates if self.has(c)), None)

    def placed(self, name: str, src: Source, pos: int, fn: str, note: str = "") -> None:
        where = "not needed" if fn == "not needed" else f"{src.path.name}:{src.line_of(pos)} ({fn})"
        self.hooks.append(Hook(name, True, where, note))

    def missed(self, name: str, note: str) -> None:
        self.hooks.append(Hook(name, False, "", note))

    # -- the hooks, each placed where ExampleMachine-family games do that work

    def include(self, main: Source) -> None:
        # after the last #include that's at the top level: never inside an #if (Flash2022's last
        # include is conditional, and PinNode.h was left out of its build)
        depth, pos = 0, 0
        for m in re.finditer(r"^[ \t]*#[ \t]*(if|ifdef|ifndef|endif|include)\b[^\n]*\n", main.text, re.M):
            k = m.group(1)
            if k.startswith("if"):
                depth += 1
            elif k == "endif":
                depth = max(0, depth - 1)
            elif depth == 0:
                pos = m.end()
        from . import __version__
        main.insert(pos, f'#include <PinNode.h>              {MARK}: the PinNode library {__version__} or later '
                         f'(converted by the PinNode converter {__version__})\n'
                         f'#include "PinNodeGame.h"           {MARK}: generated from pinnode.game.json\n'
                         f'#ifndef RPU_OS_HARDWARE_REV\n#define RPU_OS_HARDWARE_REV 0              {MARK}: an older RPU OS has no board rev to report\n#endif\n'
                         f'#define PinNodeAppStarts PinNode_AppStartsOn()   {MARK}: App starts, a PinNode adjustment'
                         ' (the library keeps it)\n')
        self.placed("include", main, pos, "top of file")

    def tables(self, main: Source, settings: list[dict], audit_defines: list[str]) -> None:
        fn = _function(main, "setup")
        pos = _line_start(main.text, main.text.rfind("\n", 0, fn[0]) if fn else 0)
        lines = [f"{MARK}: app-adjustable settings (the same EEPROM bytes as the operator menu)",
                 "const PinNodeSetting PinNodeSettingsTable[] = {"]
        for s in settings:
            src = s.get("source", {})
            if src.get("eeprom") is None:
                continue
            size = 4 if s.get("type") == "score" else 1
            lo = s.get("min", 0)
            hi = s.get("max", max((v["v"] for v in s.get("values", [])), default=1))
            lines.append(f"  {{{src['eeprom']}, {size}, {lo}, {hi}, 1, {str(bool(s.get('liveSafe'))).lower()}}},"
                         f"  // {s.get('id')}")
        if len(lines) == 2:
            lines.append("  {0, 1, 0, 0, 1, false}   // none found: add yours, matching pinnode.game.json")
        lines += ["};", f"{MARK}: audits (the RPU OS's standard counters)", "const PinNodeAudit PinNodeAuditsTable[] = {"]
        lines += [f"  {{{d}, 4}}," for d in audit_defines] or ["  {0, 4}   // none found"]
        lines += ["};", "static void PinNodeManifestBytes(byte *out) {   // PINNODE_MANIFEST_HASH (16 hex) -> 8 bytes",
                  "  const char *h = PINNODE_MANIFEST_HASH;",
                  "  for (byte i = 0; i < 8; i++) {",
                  "    byte hi = h[2 * i], lo = h[2 * i + 1];",
                  "    out[i] = (byte)((((hi & 0x0F) + (hi > '9' ? 9 : 0)) << 4) | ((lo & 0x0F) + (lo > '9' ? 9 : 0)));",
                  "  }", "}", ""]
        main.insert(pos, _block(lines, ""))

    def setup(self, main: Source, n_settings: int, n_audits: int) -> None:
        fn = _function(main, "setup")
        body_start, close, _ = fn
        ind = _indent_at(main.text, body_start + 1) or "  "
        lines = ["", f"{MARK}: start the link to the node and say hello",
                 "PinNodeCtrl_Settings(PinNodeSettingsTable, sizeof(PinNodeSettingsTable) / sizeof(PinNodeSettingsTable[0]), NULL);"
                 if n_settings else "// PinNodeCtrl_Settings(...): no settings found to map",
                 "PinNodeCtrl_Audits(PinNodeAuditsTable, sizeof(PinNodeAuditsTable) / sizeof(PinNodeAuditsTable[0]));"
                 if n_audits else "// PinNodeCtrl_Audits(...): no audits found to map",
                 "RPULink_BeginUart();   // RPU board: the ESP32 in the WiFi socket, on Serial2. Network board: RPULink_BeginSpi(53)",
                 "{",
                 "  byte manifestHash[8];",
                 "  PinNodeManifestBytes(manifestHash);",
                 "  PinNodeCtrl_Begin(PINNODE_RULESET_VERSION, PINNODE_RULESET_ID, manifestHash, 4, 0, RPU_OS_HARDWARE_REV);",
                 "}"]
        pos = _line_start(main.text, close)
        main.insert(pos, _block(lines, ind))
        self.placed("setup: link and HELLO", main, pos, "setup")

    def loop(self, main: Source) -> None:
        fn = _function(main, "loop")
        body_start, close, _ = fn
        ind = _indent_at(main.text, body_start + 1) or "  "
        need = [i for i in ("MachineState", "MACHINE_STATE_ATTRACT", "CurrentBallInPlay", "CurrentPlayer",
                            "CurrentNumPlayers", "CurrentTime") if not self.has(i)]
        if need:
            self.missed("loop: link, mode, start from the app", f"not an RPU-style loop: no {', '.join(need)}")
            return
        fp = self.first_ident("FreePlayMode", "FreePlay", "freePlay") or "false"
        # owner credits from the app (CREDIT_ADD), in attract, the game's own way (AddCredit caps, saves, shows)
        credit_add = []
        if self.has("AddCredit") and self.has("MaximumCredits"):
            credit_add = ["  uint16_t creditReq;",
                          "  byte creditsAsked;",
                          "  if (PinNodeCtrl_TakeCreditAdd(&creditReq, &creditsAsked)) {   // credits from the app (owner)",
                          "    if (MachineState != MACHINE_STATE_ATTRACT) PinNodeCtrl_CreditResult(creditReq, PINNODE_CREDIT_NOT_NOW, 0, Credits);",
                          "    else if (creditsAsked == 0 || (unsigned int)Credits + creditsAsked > MaximumCredits)",
                          "      PinNodeCtrl_CreditResult(creditReq, PINNODE_CREDIT_OVER_MAX, 0, Credits);",
                          "    else { AddCredit(false, creditsAsked); PinNodeCtrl_CreditResult(creditReq, PINNODE_CREDIT_OK, creditsAsked, Credits); }",
                          "  }"]
        else:
            self.missed("loop: credits from the app", "no AddCredit / MaximumCredits: the app's Add credit is answered unsupported")
        lines = ["", f"{MARK}: run the link, report the mode, and take Start from the app during a game",
                 "RPULink_Update(CurrentTime);",
                 "{",
                 "  boolean inGame = MachineState > MACHINE_STATE_ATTRACT;",
                 ("  byte mode = (MachineState < 0 || Menus.OperatorMenusActive()) ? PINNODE_MODE_SERVICE"
                  if self.has("OperatorMenusActive") else "  byte mode = MachineState < 0 ? PINNODE_MODE_SERVICE")
                 + " : inGame ? PINNODE_MODE_GAME : PINNODE_MODE_ATTRACT;",
                 "  PinNodeCtrl_Mode(mode, inGame ? CurrentBallInPlay : 0, inGame ? CurrentPlayer + 1 : 0);",
                 *credit_add,
                 f"  PinNodeCtrl_Credits(Credits, {fp});   // the app shows the machine's credits",
                 "  uint16_t startReq;",
                 "  if (inGame && PinNodeCtrl_TakeAppStart(&startReq, PinNodeAppStarts)) {   // add a player, like the credit button",
                 "    if (CurrentBallInPlay >= 2) PinNodeCtrl_StartResult(startReq, PINNODE_START_NOT_NOW, 0);",
                 "    else if (CurrentNumPlayers >= 4) PinNodeCtrl_StartResult(startReq, PINNODE_START_MAX_PLAYERS, 0);",
                 "    else if (AddPlayer()) PinNodeCtrl_StartResult(startReq, PINNODE_START_OK, CurrentNumPlayers);",
                 "    else PinNodeCtrl_StartResult(startReq, PINNODE_START_NO_CREDITS, 0);",
                 "  }",
                 "}",
                 "PinNodeCtrl_Update(CurrentTime, false);"]
        pos = _line_start(main.text, close)
        main.insert(pos, _block(lines, ind))
        self.placed("loop: link, mode, start from the app", main, pos, "loop")

    def _in_state_changed(self, fname: str, code: str, hook: str) -> None:
        """First statement inside the function's `if (curStateChanged) {` block."""
        hit = self.find(fname)
        if not hit:
            self.missed(hook, f"add `{code.strip()}` where this happens in your game (there's no {fname}() to put it in)")
            return
        src, (b, e, _) = hit
        m = re.search(r"if\s*\(\s*curStateChanged\s*\)\s*\{", src.text[b:e])
        if not m:
            self.missed(hook, f"add `{code.strip()}` once, where {fname}() really starts it (after any early "
                        f"returns). The converter looks for an `if (curStateChanged)` block, and {fname}() has none")
            return
        pos = b + m.end()
        nl = src.text.find("\n", pos) + 1
        ind = _indent_at(src.text, nl) or "    "
        src.insert(nl, f"{ind}{code}   {MARK}\n")
        self.placed(hook, src, nl, fname)

    def rpu_displays(self) -> bool:
        """The RPU OS's display calls, which the library's claim-code helpers use. Older games on
        BallySternOS (BSOS_*) or WOS (WOS_*) don't have them: their claim codes stay in the app."""
        return self.has("RPU_SetDisplay")

    def game_start(self) -> None:
        fp = self.first_ident("FreePlayMode", "FreePlay", "freePlay")
        self._in_state_changed("InitGamePlay", f"PinNodeCtrl_GameStart(Credits, {fp or 'false'});"
                               + (" PinNode_ClaimCodesStop();" if self.rpu_displays() else ""), "game start")

    def ball_start(self) -> None:
        self._in_state_changed("InitNewBall", "PinNodeCtrl_BallStart(CurrentPlayer + 1, CurrentBallInPlay);", "ball start")

    def player_added(self) -> None:
        hit = self.find("AddPlayer")
        if not hit:
            self.missed("player added", "no AddPlayer() found")
            return
        src, (b, e, _) = hit
        rets = [m for m in re.finditer(r"return\s+true\s*;", src.text[b:e])]
        if not rets:
            self.missed("player added", "AddPlayer() has no `return true;`")
            return
        pos = _line_start(src.text, b + rets[-1].start())
        ind = _indent_at(src.text, pos)
        src.insert(pos, f"{ind}if (CurrentNumPlayers > 1) PinNodeCtrl_PlayerAdded(CurrentNumPlayers);   {MARK}\n")
        self.placed("player added", src, pos, "AddPlayer")

    def ball_end_and_game_end(self) -> None:
        # game end: where the last ball hands over to the match sequence; ball end: just before the
        # shoot-again decision in that same ball-over code (the nearest one above the game end)
        for src in self.files:
            m = re.search(r"^[ \t]*returnState\s*=\s*MACHINE_STATE_MATCH_MODE\s*;", src.text, re.M)
            if not m:
                continue
            region_start = max(0, m.start() - 4000)
            shoots = [x for x in re.finditer(r"^[ \t]*if\s*\(\s*!?\s*(SamePlayerShootsAgain|PlayerShootsAgain)\b[^\n]*",
                                             src.text[region_start:m.start()], re.M)]
            game_end_pos = m.start()
            # "if (MatchEnabled) { returnState = MATCH } else { ATTRACT }": the game ends either way, so
            # report it before that if, not only on the match path
            prev = src.text.rfind("\n", 0, max(0, m.start() - 1))
            prev_line = src.text[prev + 1:m.start()]
            if re.match(r"[ \t]*if\s*\(.*\)\s*\{\s*$", prev_line) and "else" in src.text[m.end():m.end() + 120]:
                game_end_pos = prev + 1
            ind = _indent_at(src.text, game_end_pos)
            src.insert(game_end_pos, f"{ind}PinNodeCtrl_GameEnd(CurrentScores, CurrentNumPlayers, PINNODE_END_NORMAL);   {MARK}\n")
            self.placed("game end", src, game_end_pos, "ball over → match")
            if shoots:
                pos = region_start + shoots[-1].start()
                ind = _indent_at(src.text, pos)
                src.insert(pos, f"{ind}PinNodeCtrl_BallEnd(CurrentPlayer + 1, CurrentBallInPlay, CurrentScores[CurrentPlayer]);   {MARK}\n")
                self.placed("ball end", src, pos, "ball over", "if your ball save re-serves the ball, skip this call for a saved ball")
            else:
                self.missed("ball end", "no shoot-again decision above the game end: call PinNodeCtrl_BallEnd(player, ball, score) where a ball ends")
            return
        self.missed("game end", "no `returnState = MACHINE_STATE_MATCH_MODE;` found: call "
                    "PinNodeCtrl_GameEnd(CurrentScores, CurrentNumPlayers, PINNODE_END_NORMAL) where the game ends")
        self.missed("ball end", "call PinNodeCtrl_BallEnd(player, ball, score) where a ball ends")

    def coin(self) -> None:
        for fname in ("AddCoin", "AddCredit"):
            hit = self.find(fname)
            if hit:
                src, (b, e, params) = hit
                chute = params[0] if params else None
                if not chute:
                    continue
                pos = _end_of_body(src.text, b, e)
                ind = _indent_at(src.text, b + 1) or "  "
                src.insert(pos, f"{ind}PinNodeCtrl_Coin({chute} + 1, Credits);   {MARK}: a coin in chute 1-3, with the credits after it\n")
                self.placed("coin", src, pos, fname, f"check {fname}() has no early return before the end")
                return
        self.missed("coin", "no AddCoin()/AddCredit() found: call PinNodeCtrl_Coin(chute, creditsAfter) after a coin is credited")

    def tilt(self) -> None:
        mx = self.first_ident("MAX_TILT_WARNINGS", "MaxTiltWarnings")
        for src in self.files:
            m = re.search(r"^[ \t]*NumTiltWarnings\s*(\+\+|\+=\s*1)\s*;[^\n]*\n", src.text, re.M)
            if m and mx:
                pos = m.end()
                ind = _indent_at(src.text, m.start())
                # the game's own tilt test just below ("> MaxTiltWarnings" in the RPU games), so the
                # last warning is reported as a warning and only the tilt as a tilt
                cmp = re.search(r"NumTiltWarnings\s*(>=|>)\s*(\w+)", src.text[pos:pos + 300])
                test = f"NumTiltWarnings {cmp.group(1)} {cmp.group(2)}" if cmp else f"NumTiltWarnings > {mx}"
                src.insert(pos, f"{ind}PinNodeCtrl_Tilt(CurrentPlayer + 1, {test} ? "
                                f"PINNODE_TILT_TILT : PINNODE_TILT_WARNING);   {MARK}\n")
                self.placed("tilt", src, pos, "tilt warning")
                return
        self.missed("tilt", "no `NumTiltWarnings += 1` found: call PinNodeCtrl_Tilt(player, kind) on each warning")

    def hold_high_score(self) -> None:
        """While a claim code is up, skip the attract branch that shows the high score on every
        display (owner, 2026-10-04): its condition gains `&& !PinNode_ClaimCodesShowing()`, so
        attract falls through to the last scores, and its player-up lamps stay with the code."""
        name = "attract: hold the high score while a claim code shows"
        if not self.rpu_displays():
            self.placed(name, self.files[0], 0, "not needed",
                        "claim codes aren't drawn on this game's displays (an older OS): players claim in the app")
            return
        hit = self.find("RunAttractMode")
        if not hit:
            self.missed(name, "no RunAttractMode() found")
            return
        src, (b, e, _) = hit
        body = src.text[b:e]
        for m in re.finditer(r"\bif\s*\(", body):
            open_p = b + m.end() - 1
            depth, i = 0, open_p
            while i < e:
                depth += {"(": 1, ")": -1}.get(src.text[i], 0)
                if depth == 0:
                    break
                i += 1
            close_p = i
            brace = re.match(r"\s*\{", src.text[close_p + 1:e])
            if not brace:
                continue
            block_open = close_p + 1 + brace.end() - 1
            block_close = _match_brace(src.text, block_open)
            block = src.text[block_open:block_close]
            if not re.search(r"\w+\([^;]*\bHighScore\b[^;]*\)\s*;", block):
                continue
            cond = src.text[open_p + 1:close_p]
            src.insert(close_p, ") && !PinNode_ClaimCodesShowing()")
            src.insert(open_p + 1, "(")
            self.placed(name, src, open_p, "RunAttractMode", f"was `if ({cond.strip()})`")
            return
        self.missed(name, "no attract branch showing HighScore found: skip your high-score display "
                          "while PinNode_ClaimCodesShowing() is true")

    def attract(self) -> None:
        hit = self.find("RunAttractMode")
        if not hit:
            self.missed("attract: start from the app, claim codes", "no RunAttractMode() found")
            return
        src, (b, e, _) = hit
        rets = list(re.finditer(r"^[ \t]*return\s+returnState\s*;", src.text[b:e], re.M))
        if not rets or not self.has("MACHINE_STATE_INIT_GAMEPLAY"):
            self.missed("attract: start from the app, claim codes", "RunAttractMode() has no final `return returnState;`")
            return
        pos = b + rets[-1].start()
        ind = _indent_at(src.text, pos)
        up = [f"LAMP_HEAD_PLAYER_{i}_UP" for i in range(1, 5)]
        lamps = "pnPlayerUpLamps" if all(self.has(n) for n in up) else None
        lines = [f"{MARK}: claim codes, each in its own player's display: the code for 5 s, blinking, then",
                 "// the score for 2 s, until it's claimed (the library's PinNode_ShowClaimCodes). Drawn last;",
                 "// if your attract mode redraws scores in its own timing, skip the displays it returns."]
        if not self.rpu_displays():
            lines = [f"{MARK}: claim codes aren't drawn on this game's displays (BallySternOS / WOS, older than",
                     "// the RPU OS): players claim their scores in the app instead."]
        elif lamps:
            lines += [f"static const byte {lamps}[4] = {{{', '.join(up)}}};",
                      f"PinNode_ShowClaimCodes(CurrentTime, CurrentScores, {lamps});"]
        else:
            lines += ["PinNode_ShowClaimCodes(CurrentTime, CurrentScores);   "
                      "// pass the 4 player-up lamps to flash them too"]
        lines += [
                 f"{MARK}: Start from the app, like the credit button",
                 "{",
                 "  uint16_t startReq;",
                 "  if (PinNodeCtrl_TakeAppStart(&startReq, PinNodeAppStarts)) {   // Off: answered APP_OFF",
                 "    if (AddPlayer(true)) {",
                 "      returnState = MACHINE_STATE_INIT_GAMEPLAY;",
                 "      PinNodeCtrl_StartResult(startReq, PINNODE_START_OK, 1);",
                 "    } else {",
                 "      PinNodeCtrl_StartResult(startReq, PINNODE_START_NO_CREDITS, 0);",
                 "    }",
                 "  }",
                 "}"]
        src.insert(pos, _block(lines, ind))
        self.placed("attract: start from the app, claim codes", src, pos, "RunAttractMode",
                    "check the claim code shows: play a game without checking in")


    def operator_menu_guard(self) -> None:
        """ExampleMachine-style games (OperatorMenus): a stray self-test pulse mid-game opened the
        operator menu's blank first page and stopped the game (Star Trek, 2026-10-04)."""
        name = "operator menu: self-test guard and idle exit"
        hit = self.find("HandleSystemSwitches")
        menu = self.find("RunOperatorMenu")
        todo = ("guard the self-test switch, so one stray pulse mid-game doesn't stop the game: open the menu only "
                "if `PinNode_SelfTestOpens(inGame, CurrentTime)`, and close an idle menu with "
                "`PinNode_OperatorMenuGuard(Menus, CurrentTime)` (the guide at pin-node.com/developers/rpu)")
        if not hit or not menu:
            if self.has("EnterOperatorMenu"):
                self.missed(name, todo)
            return                                    # older games (SelfTestAndAudit) have no such menu
        src, (b, e, _) = hit
        m = re.search(r"(case\s+SW_SELF_TEST_SWITCH\s*:\s*)Menus\.EnterOperatorMenu\(\)\s*;", src.text[b:e])
        msrc, (mb, me, _) = menu
        u = re.search(r"if\s*\(\s*!\s*Menus\.UpdateMenu\(\s*CurrentTime\s*\)\s*\)", msrc.text[mb:me])
        if not m or not u:
            # its own guard already (a timed second press, and an idle exit): nothing to add
            case = re.search(r"case\s+SW_SELF_TEST_SWITCH\s*:(.*?)\bbreak\s*;", src.text[b:e], re.S)
            if case and "CurrentTime" in case.group(1) and re.search(r"IDLE|LastActivity", msrc.text[mb:me]):
                self.placed(name, src, b + case.start(), "HandleSystemSwitches", "already in your game: nothing added")
                return
            self.missed(name, todo)
            return
        s0, s1 = b + m.start(), b + m.end()
        src.text = (src.text[:s0] + m.group(1) + "if (PinNode_SelfTestOpens(MachineState > MACHINE_STATE_ATTRACT, CurrentTime)) "
                    + f"Menus.EnterOperatorMenu();   {MARK}: two presses mid-game, one in attract" + src.text[s1:])
        hit2 = self.find("RunOperatorMenu")           # positions moved if it's the same file
        msrc, (mb, me, _) = hit2
        u = re.search(r"if\s*\(\s*!\s*Menus\.UpdateMenu\(\s*CurrentTime\s*\)\s*\)", msrc.text[mb:me])
        u0, u1 = mb + u.start(), mb + u.end()
        msrc.text = (msrc.text[:u0] + "if (!Menus.UpdateMenu(CurrentTime) || PinNode_OperatorMenuGuard(Menus, CurrentTime))"
                     + msrc.text[u1:])
        self.placed(name, msrc, u0, "RunOperatorMenu",
                    "check: one self-test press mid-game doesn't open the menu; its blank first page closes after 15 s")


    def operator_menu_exit(self) -> None:
        """Tell the node the settings and audits may have changed when the operator menu closes (it diffs)."""
        name = "operator menu exit"
        calls = ["PinNodeCtrl_SettingsChanged();   " + MARK + ": the operator may have changed settings (the node diffs)",
                 "PinNodeCtrl_AuditsChanged();     " + MARK + ": or cleared audits"]
        menu = self.find("RunOperatorMenu")            # OperatorMenus games
        if menu:
            src, (b, e, _) = menu
            m = re.search(r"if\s*\(\s*menuDone\s*\)\s*\{", src.text[b:e])
            if m:
                pos = b + m.end()
                ind = (_indent_at(src.text, _line_start(src.text, b + m.start())) or "  ") + "  "
                src.text = src.text[:pos] + "\n" + "\n".join(ind + c for c in calls) + src.text[pos:]
                self.placed(name, src, pos, "RunOperatorMenu (menu done)")
                return
        st = self.find("RunSelfTest")                  # self-test games: the block that returns to attract
        if st:
            src, (b, e, _) = st
            m = re.search(r"if\s*\(\s*returnState\s*==\s*MACHINE_STATE_ATTRACT\s*\)\s*\{", src.text[b:e])
            if m:
                pos = b + m.end()
                ind = (_indent_at(src.text, _line_start(src.text, b + m.start())) or "  ") + "  "
                src.text = src.text[:pos] + "\n" + "\n".join(ind + c for c in calls) + src.text[pos:]
                self.placed(name, src, pos, "RunSelfTest (back to attract)")
                return
        self.missed(name, "call PinNodeCtrl_SettingsChanged() and PinNodeCtrl_AuditsChanged() where your "
                    "self-test / operator menu returns to attract")

    def phone_tests(self, main: Source) -> Optional[list]:
        """Tests from the phone (the library's PinNode_Tests): the game's coil test callbacks in setup, one
        line first in loop()'s machine-state chain, and the mode reported as service meanwhile. Returns the
        manifest's actions[], or None when not placed."""
        name = "tests from the phone"
        if not self.rpu_displays():
            self.placed(name, self.files[0], 0, "not needed",
                        "this game's OS (BSOS or WOS) isn't covered by the library's tests yet")
            return None
        body_start, close, _ = _function(main, "loop")
        body = main.text[body_start:close]
        chain = re.search(r"^([ \t]*)if\s*\(\s*MachineState\s*(?:<\s*0|==\s*MACHINE_STATE_ATTRACT)\s*\)", body, re.M)
        if not re.search(r"\bint\s+newMachineState\s*=", body) or not chain:
            self.missed(name, "loop() has no `int newMachineState` and `if (MachineState ...)` chain: add "
                        "`if (PinNode_Tests(CurrentTime, &newMachineState, MACHINE_STATE_ATTRACT)) {} else` before "
                        "your machine-state code (the guide's Tests section)")
            return None
        begin = re.search(r"^([ \t]*)RPULink_BeginUart\(\);   // RPU board", main.text, re.M)
        if not begin:
            self.missed(name, "the setup hook wasn't placed")
            return None

        def cb(setter: str) -> str:
            m = re.search(rf"\bMenus\.{setter}\s*\(\s*(\w+)\s*\)", self.all_text)
            return m.group(1) if m else "0"
        coil, strength, sound = (cb("SetSolenoidIDLookupCallback"), cb("SetSolenoidStrengthLookupCallback"),
                                 cb("SetSoundCallbackFunction"))
        main.insert(begin.start(), f"{begin.group(1)}PinNode_TestsBegin({coil}, {strength}, {sound});   "
                                   f"{MARK}: tests from the phone (the coin-door coil test's numbers)\n")
        body_start, close, _ = _function(main, "loop")
        body = main.text[body_start:close]
        chain = re.search(r"^([ \t]*)if\s*\(\s*MachineState\s*(?:<\s*0|==\s*MACHINE_STATE_ATTRACT)\s*\)", body, re.M)
        at = body_start + chain.start() + len(chain.group(1))
        main.text = (main.text[:at] + "if (PinNode_Tests(CurrentTime, &newMachineState, MACHINE_STATE_ATTRACT)) {   "
                     + f"{MARK}: the phone's diagnostics has the machine\n{chain.group(1)}}} else "
                     + main.text[at:])
        # the loop hook's mode: PinNode's diagnostics is service, not a game
        main.text = main.text.replace("boolean inGame = MachineState > MACHINE_STATE_ATTRACT;",
                                      "boolean inGame = MachineState > MACHINE_STATE_ATTRACT && MachineState != PINNODE_MACHINE_STATE_TESTS;", 1)
        main.text = main.text.replace("byte mode = (MachineState < 0 ||",
                                      "byte mode = (MachineState < 0 || MachineState == PINNODE_MACHINE_STATE_TESTS ||", 1)
        main.text = main.text.replace("byte mode = MachineState < 0 ? PINNODE_MODE_SERVICE",
                                      "byte mode = (MachineState < 0 || MachineState == PINNODE_MACHINE_STATE_TESTS) ? PINNODE_MODE_SERVICE", 1)
        self.placed(name, main, at, "loop (machine states)",
                    "check: Enter diagnostics in the app, then the lamp, display, coil and switch tests")
        arch = re.search(r"^[ \t]*#define[ \t]+RPU_MPU_ARCHITECTURE[ \t]+(\d+)", self.all_text, re.M)
        lamps = 64 if arch and int(arch.group(1)) >= 10 else 60   # RPU_MAX_LAMPS: Williams 64, Bally/Stern 60
        actions = [
            {"id": "lamp-test", "name": "Lamp test", "kind": "test", "arg": {"min": 0, "max": lamps, "label": f"Lamp ({lamps} = all)"},
             "valueLabel": "Lamp", "valueList": "lamps"},
            {"id": "display-test", "name": "Display test", "kind": "test", "arg": {"min": 0, "max": 7, "label": "Digit (0 = all)"},
             "valueLabel": "Digit"},
            {"id": "solenoid-test", "name": "Coil test", "kind": "test",
             "arg": {"min": 0, "max": 31, "label": "Coil, as numbered in the coin-door test (0 = each in turn)"[:40]},
             "valueLabel": "Coil"},
            {"id": "switch-test", "name": "Switch test", "kind": "test", "valueLabel": "Closures", "valueList": "switches"},
            {"id": "clear-audits", "name": "Clear all audits", "kind": "oneshot", "confirm": True, "valueLabel": "Plays before"},
            {"id": "enter-diagnostics", "name": "Enter diagnostics", "kind": "oneshot", "local": True, "diagnostics": "enter"},
            {"id": "exit-diagnostics", "name": "Exit diagnostics", "kind": "oneshot", "local": True, "diagnostics": "exit"},
        ]
        if sound != "0":
            actions.append({"id": "sound-test", "name": "Sound test", "kind": "test",
                            "arg": {"min": 0, "max": 255, "label": "Sound"}, "valueLabel": "Sound"})
        for i, a in enumerate(actions):
            a["index"] = i
        return actions

    # "#define MACHINE_STATE_ADJUST_DONE  -27" or "(MACHINE_STATE_TEST_DONE-20)": the value up to any comment
    ADJ_DONE_RE = r"^([ \t]*#define[ \t]+MACHINE_STATE_ADJUST_DONE[ \t]+)(\S(?:.*?\S)?)(?=[ \t]*(?://|/\*|$))"

    def pinnode_adjustments(self) -> None:
        """The library's adjustments (App starts, PinNode WiFi reset, and later ones) after the game's own,
        from one hook in the adjustments menu (owner, 2026-10-12: "one line ... existing and potential
        future adjustments")."""
        name = "PinNode adjustments"
        # OperatorMenus games (the RPU example game's menus)
        for src in self.files:
            m = re.search(r"Menus\.SetNumSubLevels\(\s*(OM_GAME_ADJ_FINISHED)\s*\)", src.text)
            g = re.search(r"topLevel\s*==\s*OPERATOR_MENU_GAME_ADJ_MENU", src.text)
            if not m or not g:
                continue
            sw = re.search(r"switch\s*\(\s*subLevel\s*\)\s*\{", src.text[g.end():])
            if not sw:
                break
            open_brace = g.end() + sw.end() - 1
            close = _match_brace(src.text, open_brace)
            if re.search(r"\bdefault\s*:", src.text[open_brace:close]):
                self.missed(name, "the game-adjustments switch already has a default: add "
                            "`PINNODE_ADJUSTMENT_ITEM(subLevel - OM_GAME_ADJ_FINISHED);` to it by hand")
                return
            ind = _indent_at(src.text, _line_start(src.text, close)) or "      "
            src.text = (src.text[:_line_start(src.text, close)]
                        + f"{ind}  default:   {MARK}: the PinNode adjustments (App starts, PinNode WiFi reset, ...)\n"
                        + f"{ind}    PINNODE_ADJUSTMENT_ITEM(subLevel - OM_GAME_ADJ_FINISHED);\n{ind}    break;\n"
                        + src.text[_line_start(src.text, close):])
            m = re.search(r"Menus\.SetNumSubLevels\(\s*OM_GAME_ADJ_FINISHED\s*\)", src.text)
            src.text = (src.text[:m.start()] + "Menus.SetNumSubLevels(OM_GAME_ADJ_FINISHED + PINNODE_NUM_ADJUSTMENTS)"
                        + src.text[m.end():])
            self.placed(name, src, m.start(), "the game-adjustments menu",
                        "check: App starts and PinNode WiFi reset show after the game's own adjustments")
            return
        # older games: MACHINE_STATE_ADJUST_* states chained in the self-test
        for src in self.files:
            d = re.search(self.ADJ_DONE_RE, src.text, re.M)
            if not d:
                continue
            first = d.group(2).strip()          # a number, or an expression like (MACHINE_STATE_TEST_DONE-20)
            for s2 in self.files:
                sw = None
                for c in re.finditer(r"switch\s*\(\s*curState\s*\)\s*\{", s2.text):
                    close = _match_brace(s2.text, c.end() - 1)
                    body = s2.text[c.end():close]
                    if "CurrentAdjustmentByte" in body and re.search(r"case\s+MACHINE_STATE_ADJUST_", body):
                        sw = (c, close)
                        break
                if not sw:
                    continue
                c, close = sw
                if re.search(r"\bdefault\s*:", s2.text[c.end():close]):
                    self.missed(name, "the adjustment switch already has a default: add "
                                "`PINNODE_ADJUSTMENT_ITEM_CHAINED(PINNODE_ADJ_FIRST_STATE - curState);` by hand")
                    return
                ind = _indent_at(s2.text, _line_start(s2.text, close)) or "      "
                s2.text = (s2.text[:_line_start(s2.text, close)]
                           + f"{ind}  default:   {MARK}: the PinNode adjustments (App starts, PinNode WiFi reset, ...)\n"
                           + f"{ind}    PINNODE_ADJUSTMENT_ITEM_CHAINED(PINNODE_ADJ_FIRST_STATE - curState);\n{ind}    break;\n"
                           + s2.text[_line_start(s2.text, close):])
                d = re.search(self.ADJ_DONE_RE, src.text, re.M)
                src.text = (src.text[:d.start()]
                            + f"#define PINNODE_ADJ_FIRST_STATE           {first}   {MARK}: the PinNode adjustments start here\n"
                            + f"{d.group(1)}({first} - PINNODE_NUM_ADJUSTMENTS)   {MARK}: after the PinNode adjustments"
                            + src.text[d.end():])
                self.placed(name, s2, c.start(), "the self-test adjustments",
                            "check: App starts and PinNode WiFi reset follow the game's last adjustment")
                return
        # games whose self-test extends with its own EEPROM settings (RunExtendedSelfTest, NUM_EXTENDED_SETTINGS)
        for src in self.files:
            n = re.search(r"^([ \t]*#define[ \t]+NUM_EXTENDED_SETTINGS[ \t]+)(\d+)", src.text, re.M)
            if not n:
                continue
            ext = self.find("RunExtendedSelfTest")
            if not ext:
                break
            s2, (b, e, _) = ext
            body = s2.text[b:e]
            end = re.search(r"\belse\s+return\s+MACHINE_STATE_ATTRACT\s*;", body)
            if not end or "highNum" not in body or "auditNumStartByte" not in body:
                break
            pos = b + end.start()
            ind = _indent_at(s2.text, _line_start(s2.text, pos)) or "  "
            s2.text = (s2.text[:pos] + "else if (PinNode_AdjustmentEeprom(PINNODE_ADJ_FIRST_STATE - curState, &highNum, "
                       f"&auditNumStartByte)) {{}}   {MARK}: the PinNode adjustments (App starts, PinNode WiFi reset, ...)\n"
                       + ind + s2.text[pos:])
            first = int(n.group(2))
            # the game's own extended states are numbered down from its first; ours follow the lowest
            lows = [int(v) for v in re.findall(r"^[ \t]*#define[ \t]+MACHINE_STATE_TEST_\w+[ \t]+(-\d+)", src.text, re.M)]
            after = f"{min(lows) - 1}" if lows else f"(MACHINE_STATE_TEST_DONE - 1 - {first})"
            n = re.search(r"^([ \t]*#define[ \t]+NUM_EXTENDED_SETTINGS[ \t]+)(\d+)", src.text, re.M)
            src.text = (src.text[:n.start()]
                        + f"#define PINNODE_ADJ_FIRST_STATE           {after}   {MARK}: after the game's own\n"
                        + f"{n.group(1)}({first} + PINNODE_NUM_ADJUSTMENTS)   {MARK}: plus the PinNode adjustments"
                        + src.text[n.end():])
            self.placed(name, s2, pos, "RunExtendedSelfTest",
                        "check: App starts and PinNode WiFi reset follow the game's extended settings")
            return
        self.missed(name, "no adjustments menu recognised: show the library's items with PINNODE_ADJUSTMENT_ITEM "
                    "(OperatorMenus) or PINNODE_ADJUSTMENT_ITEM_CHAINED (MACHINE_STATE_ADJUST_* self-test); "
                    "see the guide at pin-node.com/developers/rpu")

    def boot_original(self) -> None:
        """Owners switch the machine between this game and its original ROM game from the app or web
        (BOOT_SELECT): wrap RPU_InitializeMPU's options, and keep the link up while the original runs."""
        name = "boot the original game"
        rpu_h = [f for f in self.files if f.path.name.lower() == "rpu.h"]
        if not rpu_h and not any("RPU_InitializeMPU" in f.text for f in self.files):
            self.placed(name, self.files[0], 0, "not needed",
                        "your game's OS (BSOS or WOS) can't boot the original game: nothing to add")
            return
        if rpu_h and "RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN" not in rpu_h[0].text:
            self.placed(name, self.files[0], 0, "not needed",
                        "your RPU OS is older than the boot-original commands: nothing to add")
            return
        m = None
        for src in sorted(self.files, key=lambda f: not f.path.name.endswith(".ino")):   # the game first
            for c in re.finditer(r"((\w+)\s*=\s*)?RPU_InitializeMPU\s*\(", src.text):
                # a call, not RPU.cpp's definition or RPU.h's prototype ("unsigned long RPU_InitializeMPU(")
                if c.group(1) or not src.text[_line_start(src.text, c.start()):c.start()].strip():
                    m = c
                    break
            if m:
                break
        else:
            self.missed(name, "no RPU_InitializeMPU() call found")
            return
        open_paren = m.end() - 1
        depth, i, comma = 0, open_paren, -1
        while i < len(src.text):                  # the call's closing paren, and its last top-level comma
            c = src.text[i]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    break
            elif c == "," and depth == 1:
                comma = i
            i += 1
        close = i
        semi = src.text.find(";", close)
        if comma < 0 or semi < 0:
            self.missed(name, "RPU_InitializeMPU(options, switch) not in the expected form: wrap the options "
                        "in PinNode_BootOptions() and add PinNode_BootChosen / PinNode_RunOriginal by hand")
            return
        opts = src.text[open_paren + 1:comma].strip()
        var = m.group(2)
        start = m.start() if not var else m.start(1)
        ind = _indent_at(src.text, _line_start(src.text, start)) or "  "
        call = f"RPU_InitializeMPU(PinNode_BootOptions({opts}),{src.text[comma + 1:close]})"
        if var:
            stmt = f"{var} = {call};"
        else:
            var = "pnInitResult"
            stmt = f"unsigned long {var} = {call};"
        lines = [f"{MARK}: an owner can boot the original game from the app; Credit/Reset at power-on comes back",
                 f"PinNode_BootChosen({var});",
                 f"if ({var} & RPU_RET_ORIGINAL_CODE_REQUESTED) {{   // the original runs: keep the link to the node",
                 "  RPULink_BeginUart();",
                 "  byte manifestHash[8];",
                 "  PinNodeManifestBytes(manifestHash);",
                 "  PinNodeCtrl_Begin(PINNODE_RULESET_VERSION, PINNODE_RULESET_ID, manifestHash, 4, 0, RPU_OS_HARDWARE_REV);",
                 "  PinNode_RunOriginal();                 // never returns; pass a function to run each pass if needed",
                 "}"]
        src.text = src.text[:start] + stmt + "\n" + _block(lines, ind) + src.text[semi + 1:].lstrip("\n")
        note = ""
        clash = sorted({m.group(1) for f in self.files for m in re.finditer(
            r"^[ \t]*#define[ \t]+(\w*EEPROM\w*)[ \t]+(\d+)", f.text, re.M) if 4080 <= int(m.group(2)) <= 4095})
        if clash:                                   # the library keeps EEPROM 4080-4095
            self.missed("EEPROM 4080-4095", f"{', '.join(clash)} use EEPROM the PinNode library reserves "
                        "(4080-4087 PinNode adjustments, 4088-4089 game counter, 4090 boot choice): move them")
        if self.has("RPU_OS_USE_ACCESSORY_LAMP_BOARD"):
            note = ("this game can drive an accessory lamp board: pass PinNode_RunOriginal() a function that "
                    "keeps its watchdog alive, as your own original-code loop does")
        self.placed(name, src, start, "setup", note or "check: Credit/Reset held at power-on boots the original game")


# ------------------------------------------------------------------ machine definition

DEF_RE = re.compile(r"^[ \t]*#define[ \t]+((?:LA|LAMP|SW|SO|SOL)_\w+)[ \t]+(\d+)[ \t]*(?://[ \t]*(.*))?$", re.M)
SKIP_DEFS = re.compile(r"_(EEPROM|BYTE|STACK|MS|TIME|STRENGTH|HOLD|NUM|COUNT|MAX)\b|^SW_SELF_TEST|^SOL_NONE|^SW_NONE")


NAME_MAX = 40   # the manifest schema's limit for a lamp, switch or coil name


def _name(ident: str, comment: Optional[str]) -> str:
    """A readable name from the define's comment, else from the identifier. Comments are often notes
    ("the 2022 code's name; on the machine it's the apron credit lamp"): one longer than the manifest
    allows keeps its first clause if that fits, else the identifier's name is used (Star Trek 2022)."""
    if comment and comment.strip():
        c = comment.strip().rstrip(".")
        if len(c) > NAME_MAX:
            c = re.split(r";\s|:\s|\s\(|\s-\s|,\s", c)[0].strip().rstrip(".")
        if 0 < len(c) <= NAME_MAX:
            return c[0].upper() + c[1:].lower() if c.isupper() else c
    core = re.sub(r"^(LA|LAMP|SW|SO|SOL)_", "", ident)
    return core.replace("_", " ").capitalize()[:NAME_MAX]


def machine_definition(raw_files: list[tuple[str, str]], machine: str) -> dict:
    kinds = {"LA": "lamps", "LAMP": "lamps", "SW": "switches", "SO": "solenoids", "SOL": "solenoids"}
    out: dict = {"machine": machine, "lamps": [], "switches": [], "solenoids": [], "defines": []}
    seen: dict[str, set] = {"lamps": set(), "switches": set(), "solenoids": set()}
    for _fname, text in raw_files:
        for ident, num, comment in DEF_RE.findall(text):
            if SKIP_DEFS.search(ident):
                continue
            kind = kinds[ident.split("_", 1)[0]]
            n = int(num)
            if n > 255:
                continue                     # not a lamp/switch/coil number (SW_..._BASE 60000)
            out["defines"].append((ident, n, kind))
            if n in seen[kind]:
                continue                     # aliases (e.g. several tilt switches on one number)
            seen[kind].add(n)
            out[kind].append({"n": n, "name": _name(ident, comment)})
    for k in seen:
        out[k].sort(key=lambda x: x["n"])
    return out


MACHINES_DIR = Path(__file__).resolve().parent / "machines"


def library_names(title: Optional[str]) -> Optional[tuple[str, dict]]:
    """The PinNode machine library's names (from the manual) for this title: the file named after it,
    or the one file that starts with it ("bally-star-trek" -> bally-star-trek-1979.json)."""
    if not title or not MACHINES_DIR.is_dir():
        return None
    files = sorted(MACHINES_DIR.glob("*.json"))
    hit = [f for f in files if f.stem == title] or [f for f in files if f.stem.startswith(title + "-")]
    if len(hit) != 1:
        return None
    d = json.loads(hit[0].read_text())
    return hit[0].stem, {k: {e["n"]: e["name"] for e in d.get(k, []) if "n" in e and "name" in e}
                         for k in ("lamps", "switches", "solenoids")}


def apply_library_names(md: dict, title: Optional[str]) -> Optional[str]:
    """Manual names win over define comments; library-only numbers are added. The library's id or None."""
    lib = library_names(title)
    if not lib:
        return None
    lib_id, names = lib
    for kind, by_n in names.items():
        have = {e["n"]: e for e in md[kind]}
        for n, name in by_n.items():
            name = _name("", name) if len(name) > NAME_MAX else name
            if n in have:
                have[n]["name"] = name
            else:
                md[kind].append({"n": n, "name": name})
        md[kind].sort(key=lambda x: x["n"])
    md["library"] = lib_id
    return lib_id


def machine_header(md: dict, guard: str) -> str:
    lines = [f"// {md['machine']}: lamps, switches and coils (generated by the PinNode converter from the game's",
             "// defines; check the names against the machine's manual and schematics before contributing)",
             f"#ifndef {guard}", f"#define {guard}", ""]
    for ident, n, _kind in md["defines"]:
        lines.append(f"#define {ident:<32} {n}")
    lines += ["", "#endif", ""]
    return "\n".join(lines)


# ------------------------------------------------------------------ entry point

def safe_sources(src_dir: Path) -> list[Path]:
    """The sketch's source files: regular files directly in src_dir. Symlinks, and anything that
    resolves outside src_dir, are skipped, so an uploaded ZIP can't pull in other files."""
    root = src_dir.resolve()
    out = []
    for p in sorted(src_dir.iterdir()):
        if p.is_symlink() or not p.is_file() or p.suffix not in SOURCE_SUFFIXES:
            continue
        if not p.resolve().is_relative_to(root):
            continue
        out.append(p)
    return out

def _slug(s: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", s.lower()).strip("-") or "game"


def convert(src_dir: Path, *, machine: Optional[str] = None, ruleset_id: Optional[str] = None,
            ruleset_name: Optional[str] = None, version: Optional[str] = None, repo: str = "",
            title: Optional[str] = None, platform: Optional[str] = None) -> ConvertResult:
    """`title` is the machine's id in PinNode's title catalog (e.g. "bally-star-trek",
    from pin-node.com/v1/titles) and `platform` its board family; without them the
    manifest gets a placeholder title from the game's name, and bally-35."""
    src_dir = Path(src_dir)
    paths = safe_sources(src_dir)
    raw = [(p.name, p.read_text(errors="replace")) for p in paths]
    files = [Source(p, t) for p, (_, t) in zip(paths, raw, strict=True)]
    conv = Converter(files)
    main = conv.main()
    if main is None:
        raise ValueError("no .ino with setup() and loop() in this folder")
    games = [f.path.name for f in files if f.path.suffix == ".ino"
             and re.search(r"\bvoid\s+setup\s*\(", f.text) and re.search(r"\bvoid\s+loop\s*\(", f.text)]
    if len(games) > 1:                       # the Arduino IDE would build them as one sketch
        raise ValueError(f"this folder holds {len(games)} games ({', '.join(games)}): put each in its own "
                         "folder named after its .ino, and convert one at a time")
    if "PinNodeCtrl_" in conv.all_text:
        raise ValueError("this game already has PinNode hooks")

    scanned = scan_mod.scan(src_dir)
    settings = [d.setting for d in scanned.settings]
    seen_ids: set[str] = set()
    for st in settings:                      # one id per setting: a second menu entry for the same id gets -2
        base, k = st["id"], 2
        while st["id"] in seen_ids:
            st["id"] = f"{base}-{k}"
            k += 1
        seen_ids.add(st["id"])
    if not any(s.get("id") == "app-starts" for s in settings):   # a PinNode adjustment: the library keeps it
        settings.append({"index": len(settings), "id": "app-starts", "name": "Starts from the app", "group": "pricing",
                         "type": "enum", "values": [{"v": 0, "label": "Off"}, {"v": 1, "label": "On"}],
                         "default": 1, "operatorOnly": True, "liveSafe": True,
                         "source": {"menuId": "PINNODE_ADJ_APP_STARTS", "eeprom": 4080, "variable": "PinNode_AppStartsOn()"}})
    audit_defines = [k for k in scan_mod.STANDARD_AUDITS if conv.has(k)]
    audits = scanned.audits or [{"index": i, "id": scan_mod.STANDARD_AUDITS[k][0], "name": scan_mod.STANDARD_AUDITS[k][1]}
                                for i, k in enumerate(audit_defines)]

    machine = machine or src_dir.name
    md = machine_definition(raw, machine)
    apply_library_names(md, title)

    # hooks: the order matters only in that each re-finds its anchor after the edits before it
    conv.include(main)
    conv.tables(main, settings, audit_defines)
    conv.setup(main, len([s for s in settings if s.get("source", {}).get("eeprom") is not None]), len(audit_defines))
    conv.loop(main)
    conv.game_start()
    conv.player_added()
    conv.ball_start()
    conv.ball_end_and_game_end()
    conv.coin()
    conv.tilt()
    conv.attract()
    conv.hold_high_score()
    conv.operator_menu_guard()
    conv.pinnode_adjustments()
    conv.boot_original()
    conv.operator_menu_exit()
    actions = conv.phone_tests(main)

    rid = ruleset_id or f"rpu-{_slug(src_dir.name)}"
    if not version:                          # the game's own GAME_MAJOR/MINOR_VERSION, when it has them
        mj = re.search(r"#define\s+GAME_MAJOR_VERSION\s+(\d+)", conv.all_text)
        mn = re.search(r"#define\s+GAME_MINOR_VERSION\s+(\d+)", conv.all_text)
        version = f"{int(mj.group(1))}.{int(mn.group(1)) if mn else 0}.0" if mj else "0.1.0"
    manifest = {
        "$schema": "https://raw.githubusercontent.com/pin-node/PinNode/main/tools/pinnode_gen/pinnode-game.schema.json",
        "manifestVersion": 1,
        "ruleset": {"id": rid, "title": title or _slug(ruleset_name or src_dir.name),
                    "name": ruleset_name or src_dir.name, "version": version,
                    **({"repo": repo} if repo else {}), "scoringVersion": 1, "platform": platform or "bally-35"},
        "players": {"max": 4},
        "achievements": [],
        "stats": [],
        "settings": settings,
        "audits": audits,
        **({"actions": actions} if actions else {}),
        "lamps": md["lamps"], "switches": md["switches"], "solenoids": md["solenoids"],
    }

    header = ""
    try:
        from .generate import generate
        from .manifest import validate
        problems = validate(manifest)
        if problems:
            conv.hooks.append(Hook("PinNodeGame.h", False, "", "manifest needs: " + "; ".join(problems)))
        else:
            header = generate(manifest, "pinnode.game.json")
    except Exception as e:  # report it; the author runs pinnode-gen generate after editing
        header = ""
        conv.hooks.append(Hook("PinNodeGame.h", False, "", f"not generated ({e}): fix the manifest, then make it in the Achievements builder"))

    info = report_info(main.path.stem, scanned, md, manifest, bool(header), bool(title))
    report = render_report(info, conv.hooks)
    guard = re.sub(r"\W", "_", _slug(machine)).upper() + "_H"
    machine_file = re.sub(r"\W", "", machine.title()) or "Machine"
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as z:
        root = f"{main.path.stem}/"          # Arduino needs the folder named after the main .ino
        for f in files:
            z.writestr(root + f.path.name, f.text)
        z.writestr(root + "pinnode.game.json", json.dumps(manifest, indent=1) + "\n")
        if header:
            z.writestr(root + "PinNodeGame.h", header)
        z.writestr(root + "CONVERSION.md", report)
        z.writestr("README.txt", _zip_readme(main.path.stem, machine_file, _slug(machine)))
        z.writestr(f"machine/{machine_file}.h", machine_header(md, guard))
        z.writestr(f"machine/{_slug(machine)}.json",
                   json.dumps({k: md[k] for k in ("machine", "lamps", "switches", "solenoids")}, indent=1) + "\n")
    return ConvertResult(buf.getvalue(), report, manifest, md, conv.hooks, info)


def _zip_readme(game: str, machine_file: str, machine_slug: str) -> str:
    """README.txt at the ZIP's root: what each of the two folders is."""
    return "\n".join([
        f"PinNode conversion of {game}",
        "",
        "This ZIP has two folders:",
        "",
        f"{game}/",
        "  Your game, with PinNode added. Open it in the Arduino IDE and build it for the Mega.",
        f"  - your sketch files: every line PinNode added is marked // PinNode",
        f"  - pinnode.game.json: your game's manifest (names, settings, achievements). You upload it",
        f"    at pin-node.com/setup/new (Add a machine)",
        f"  - PinNodeGame.h: generated from the manifest; your sketch includes it",
        f"  - CONVERSION.md: what was added, what's left for you, and the next steps. Read it first.",
        "",
        "machine/",
        f"  Your machine's definition, made from your code: lamp, switch and coil numbers and names",
        f"  ({machine_slug}.json, and {machine_file}.h for C code). You don't need it to build your game.",
        f"  PinNode keeps a library of these so every owner of the machine sees readable names in the app",
        f"  and in fault reports.",
        "",
    ])


def report_info(game: str, scanned, md: dict, manifest: dict, header: bool, titled: bool = False) -> dict:
    """What CONVERSION.md needs besides the hooks, as JSON, so the web can store it and re-render later."""
    rs = manifest["ruleset"]
    return {"game": game, "header": header, "titled": titled,
            "ruleset": {k: rs.get(k) for k in ("id", "version", "title", "platform")},
            "settings": len(manifest["settings"]), "audits": len(manifest["audits"]),
            "names": {"lamps": len(md["lamps"]), "switches": len(md["switches"]),
                      "solenoids": len(md["solenoids"]), "library": md.get("library")},
            "warnings": list(scanned.warnings)}


def _hook(h) -> Hook:
    if isinstance(h, Hook):
        return h
    return Hook(h["name"], bool(h.get("placed")), h.get("where", ""), h.get("note", ""))


from .generate import _const  # noqa: E402  (ACH_ names, as PinNodeGame.h writes them)


def render_report(info: dict, hooks: list, ai: Optional[dict] = None) -> str:
    """CONVERSION.md from the converter's hooks (Hook objects or {name, key, placed, where, note} dicts)
    and, after the web's AI step, what it did:
        ai = {"placed": [{hook, file, line, why}],                 hook = the Hook's key or name
              "achievements": [{id, name, desc, points, where, wired}],
              "skipped": [{hook, why}]}
    The AI's hooks move from the to-do list to "What was added", marked as the AI's, to check."""
    hooks = [_hook(h) for h in hooks]
    ai = ai or {}
    by_ai = {}
    for a in ai.get("placed") or []:
        by_ai[a.get("hook", "")] = a
    skipped = {a.get("hook", ""): a.get("why", "") for a in ai.get("skipped") or []}
    placed, ai_placed, todo = [], [], []
    for h in hooks:
        a = by_ai.get(h.key) or by_ai.get(h.name)
        if not h.placed and a:
            ai_placed.append((h, a))
        elif h.placed:
            placed.append(h)
        else:
            why = skipped.get(h.key) or skipped.get(h.name)
            todo.append(h if not why else Hook(h.name, False, "", f"{h.note} (the AI couldn't: {why})"))
    achs = ai.get("achievements") or []
    unwired = [a for a in achs if not a.get("wired")]
    from . import __version__
    game = info["game"]
    rs = info["ruleset"]
    n_placed = len(placed) + len(ai_placed)
    out = [f"# PinNode conversion: {game}", "",
           f"Converted by the PinNode converter {__version__}, for PinNode library {__version__} or later.", "",
           f"**{n_placed} PinNode hooks in place**" + (f" ({len(ai_placed)} by AI: check them)." if ai_placed else ".")
           + (f" **{len(todo)} for you to do** (below)." if todo else " Nothing left for you to place by hand."),
           f"Every line PinNode added is marked `{MARK}`" + (" (`// PinNode (AI)` for the AI's)." if ai_placed or achs else "."), ""]
    out += ["## What you need to do", ""]
    steps = [f"{h.name[0].upper() + h.name[1:]}: {h.note}" for h in todo]
    if not info["header"]:
        steps.append("The header PinNodeGame.h wasn't generated: fix the manifest, then open it in the "
                     "**Achievements builder** (pin-node.com/builder) and download PinNodeGame.h into your sketch folder")
    for a in unwired:
        const = _const("ACH_", str(a.get("id", "")))         # the name PinNodeGame.h gives it
        where = (a.get("where") or "where it happens").rstrip(". ")
        steps.append(f"Achievement **{a.get('name', a.get('id'))}**: call "
                     f"`PinNodeCtrl_Achievement(player, {const});` {where[0].lower() + where[1:]}. "
                     "If your game can't do it, remove it in the Achievements builder")
    if ai_placed or achs:
        steps.append("Check every line marked `// PinNode (AI)`: the AI's edits are suggestions")
    if not achs:
        steps.append("Add your achievements and stats (they're your rules): open `pinnode.game.json` in the "
                     "**Achievements builder** (pin-node.com/builder), download the new `pinnode.game.json` and "
                     "`PinNodeGame.h` into your sketch folder, and call `PinNodeCtrl_Achievement(player, ACH_…)` where "
                     "each one happens (the guide's Achievements section). Optional, recommended")
    out += [f"{i}. {t}." if not t.endswith(".") else f"{i}. {t}" for i, t in enumerate(steps, 1)]
    out += ["", "## Build and test", "",
            "1. **Install the PinNode library** (once): it's your game's link to the PinNode network card "
            "(the ESP32). On **[github.com/pin-node/PinNode](https://github.com/pin-node/PinNode)**, click "
            "**Code → Download ZIP**, then in the Arduino IDE **Sketch → Include Library → Add .ZIP Library…** "
            "and pick that ZIP.",
            "2. **Choose the board:** **Tools → Board → Arduino AVR Boards → Arduino Mega or Mega 2560**, processor "
            "**ATmega2560**. No AVR boards in the list? Install \"Arduino AVR Boards\" in **Tools → Board → Boards Manager**.",
            f"3. **Open** `{game}/{game}.ino` (unzip this ZIP first) and press **Verify**.",
            "4. **Upload:** the Mega on USB, **Tools → Port** set to the Mega's port. The machine must be on, or the "
            "USB cable's 5 V wire cut. Only the Mega on USB, not the network card's ESP32.",
            "5. **Play a game** to check it still plays as before.",
            "",
            "PinNode adds about 1.5 KB of RAM, so the IDE may warn \"Low memory available\". Play a full game "
            "to check it runs as before.",
            "",
            "Command line instead: `arduino-cli compile --fqbn arduino:avr:mega --library <the unzipped PinNode folder> "
            f"{game}`.", "",
            "## Put it on PinNode", "",
            f"1. **pin-node.com/setup/new (Add a machine) → Game:** upload `{game}/pinnode.game.json`, or paste your GitHub repo's "
            "address (PinNode finds the file in the sketch folder). Your game is named straight away: "
            "achievements, settings and audits.",
            "2. Follow the setup to the end: board, machine, flash the network card and fit it.",
            "3. Power on: the Check step waits for your game to say hello.",
            "",
            "Games record **unranked** for now. After changing the manifest, make a new `PinNodeGame.h` in the "
            "Achievements builder and upload the new `pinnode.game.json`.", "",
            "## Check the manifest", "",
            f"- Ruleset `{rs['id']}` {rs['version']}: check "
            + ("`name`" if info["titled"] else "`name` and `title` (the machine's id from pin-node.com/v1/titles?q=…)")
            + "; you may add `repo` (your GitHub owner/name). Make a new `PinNodeGame.h` in the Achievements builder after changing them.",
            f"- Machine: `{rs['title']}`, platform `{rs['platform']}`"
            + ("." if info["titled"] else " (placeholders: check both)."),
            f"- {info['settings']} settings and {info['audits']} audits found."
            + ("" if info["settings"] else
               " No operator settings were recognised: add your adjustments to `settings` in the manifest and "
               "`PinNodeSettingsTable` in the sketch, so they show in the app's Settings tab.")]
    nm = info["names"]
    out += [(f"- Names: {nm['lamps']} lamps, {nm['switches']} switches, {nm['solenoids']} coils, "
             f"from the PinNode machine library ({nm['library']}, from the manual)."
             if nm.get("library") else
             f"- Names: {nm['lamps']} lamps, {nm['switches']} switches, {nm['solenoids']} coils "
             "from your code's comments. Check them against the manual."), "",
            "## What was added", ""]
    out += [f"- **{h.name}**: {h.where}" + (f". {h.note}" if h.note else "") for h in placed if h.where != "not needed"]
    out += [f"- **{h.name}**: {a.get('file', '')}:{a.get('line', '')} (by AI: check it)"
            + (f". {a['why']}" if a.get("why") else "") for h, a in ai_placed]
    out += [""]
    if achs:
        out += ["## Achievements (suggested by AI)", ""]
        out += [f"- **{a.get('name', a.get('id'))}** (`{a.get('id')}`, {a.get('points', '?')} points): {a.get('desc', '')}"
                + (f" Wired in: {(a.get('where') or '').rstrip('. ')}." if a.get("wired") else " Not wired in yet (see What you need to do).")
                for a in achs]
        out += [""]
    not_needed = [h for h in placed if h.where == "not needed"]
    if not_needed:
        out += ["## Not needed for this game", ""] + [f"- **{h.name}**: {h.note}" for h in not_needed] + [""]
    if info["warnings"]:
        out += ["## Scan warnings", ""] + [f"- {w}" for w in info["warnings"]] + [""]
    return "\n".join(out)


def main(argv: Optional[list[str]] = None) -> int:
    import argparse
    p = argparse.ArgumentParser(prog="pinnode-gen convert")
    p.add_argument("sketch", type=Path)
    p.add_argument("--machine")
    p.add_argument("--ruleset-id")
    p.add_argument("--name")
    p.add_argument("--version", default=None, help="default: the game's GAME_MAJOR/MINOR_VERSION, else 0.1.0")
    p.add_argument("--repo", default="")
    p.add_argument("--title", help="the machine's id from pin-node.com/v1/titles, e.g. bally-star-trek")
    p.add_argument("--platform", help="its board family, e.g. bally-35, williams-sys4")
    p.add_argument("-o", "--output", type=Path)
    a = p.parse_args(argv)
    r = convert(a.sketch, machine=a.machine, ruleset_id=a.ruleset_id, ruleset_name=a.name,
                version=a.version, repo=a.repo, title=a.title, platform=a.platform)
    out = a.output or Path(f"{a.sketch.name}-pinnode.zip")
    out.write_bytes(r.zip_bytes)
    print(f"wrote {out}: {len(r.hooks) - len(r.unplaced)} hooks placed, {len(r.unplaced)} to do (see CONVERSION.md)")
    return 0
