"""pinnode-gen convert on two real RPU games: every hook placed, in the right function."""

import io
import json
import re
import zipfile
from pathlib import Path

from tools.pinnode_gen.convert import convert, render_report

from tests._fixtures import FIX


def _func(text: str, name: str) -> str:
    m = re.search(rf"\b{name}\s*\([^;{{)]*\)\s*\{{", text)
    assert m, name
    depth, i = 0, m.end() - 1
    while True:
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        if depth == 0:
            return text[m.end():i]
        i += 1


def _zip(r):
    return zipfile.ZipFile(io.BytesIO(r.zip_bytes))


def test_st2025_upstream():
    r = convert(FIX / "st2025-upstream", machine="Bally Star Trek (1979)",
            ruleset_id="rpu-star-trek-2025",
                repo="DavesThinkTank/Bally-Star-Trek")
    placed = {h.name for h in r.hooks if h.placed}
    for hook in ("include", "setup: link and HELLO", "loop: link, mode, start from the app",
            "game start",
                 "player added", "ball start", "ball end", "game end", "coin", "tilt",
                 "attract: start from the app, claim codes"):
        assert hook in placed, hook
    z = _zip(r)
    assert "ST2025p09/PinNodeGame.h" in z.namelist()
    ino = z.read("ST2025p09/ST2025p09.ino").decode()
    assert "PinNodeCtrl_BallEnd" in _func(ino, "BallOver")              # not in InitNewBall
    assert "PinNodeCtrl_BallEnd" not in _func(ino, "InitNewBall")
    assert "PinNodeCtrl_GameEnd" in _func(ino, "BallOver")
    assert "PinNodeCtrl_BallStart" in _func(ino, "InitNewBall")
    attract = _func(ino, "RunAttractMode")                           # the library's claim display
    assert "PinNode_ShowClaimCodes(CurrentTime, CurrentScores" in attract
    assert "RPU_SetDisplay(p - 1, code" not in attract
    assert "PinNode_ClaimCodesStop()" in _func(ino, "InitGamePlay")
    assert "#define PinNodeAppStarts PinNode_AppStartsOn()" in ino                     # app-starts, default On
    assert "PinNodeCtrl_Credits(Credits, FreePlay)" in _func(ino, "loop")      # the game's own name
    assert "PinNodeCtrl_TakeAppStart(&startReq, PinNodeAppStarts)" in _func(ino, "loop")
    assert "PinNodeCtrl_TakeCreditAdd" not in _func(ino, "loop")         # no AddCredit/MaximumCredits here: noted
    assert "credits from the app" in r.report_md
    assert "PinNodeCtrl_TakeAppStart(&startReq, PinNodeAppStarts)" in attract
    assert "PinNodeCtrl_TakeStartRequest" not in ino
    assert any(h.name == "PinNode adjustments" and h.placed for h in r.hooks)   # its extended settings
    hdr = _zip(r).read("ST2025p09/ST2025p09.h").decode()
    assert "#define PINNODE_ADJ_FIRST_STATE           -24" in hdr
    assert "(4 + PINNODE_NUM_ADJUSTMENTS)" in hdr
    setup = _func(ino, "setup")                                          # boot the original game
    assert "unsigned long pnInitResult = RPU_InitializeMPU(PinNode_BootOptions(" in setup
    assert "PinNode_BootChosen(pnInitResult);" in setup and "PinNode_RunOriginal();" in setup
    assert "PinNodeCtrl_GameStart" in _func(ino, "InitGamePlay")
    assert "PinNodeCtrl_TakeAppStart" in _func(ino, "RunAttractMode")
    assert "PinNodeCtrl_Update" in _func(ino, "loop")
    md = json.loads(z.read("machine/bally-star-trek-1979.json"))
    assert len(md["lamps"]) == 60 and len(md["switches"]) == 34 and len(md["solenoids"]) == 9
    assert len(r.manifest["audits"]) == 6


def test_example_machine():
    r = convert(FIX / "example-machine", machine="Example Machine",
            repo="RetroPinUpgrade/ExampleMachine")
    z = _zip(r)
    # the folder is named after the main .ino, as Arduino needs
    assert any(n.startswith("ExampleMachineMEGA/") for n in z.namelist())
    ino = z.read("ExampleMachineMEGA/ExampleMachineMEGA.ino").decode()
    loop = _func(ino, "loop")                                            # owner credits from the app, attract only
    assert "PinNodeCtrl_TakeCreditAdd(&creditReq, &creditsAsked)" in loop
    assert "AddCredit(false, creditsAsked)" in loop
    assert loop.index("PinNodeCtrl_TakeCreditAdd") < loop.index("PinNodeCtrl_Credits(Credits")
    coin = _func(ino, "AddCoin")
    assert coin.index("PinNodeCtrl_Coin") < coin.rindex("return")   # not dead code after return
    assert len(r.manifest["settings"]) > 10                        # from the operator menus
    assert "PinNodeCtrl_PlayerAdded" in _func(ino, "AddPlayer")


def test_refuses_converted_game(tmp_path):
    src = FIX / "st2025-upstream"
    d = tmp_path / "g"
    d.mkdir()
    for f in src.glob("*.*"):
        (d / f.name).write_text(f.read_text().replace("void setup() {",
                "void setup() {\n  PinNodeCtrl_Update(0, false);", 1))
    try:
        convert(d)
    except ValueError as e:
        assert "already" in str(e)
    else:
        raise AssertionError("expected a refusal")


def test_skips_symlinks(tmp_path):
    src = FIX / "st2025-upstream"
    d = tmp_path / "g"
    d.mkdir()
    for f in src.glob("*.*"):
        (d / f.name).write_text(f.read_text())
    secret = tmp_path / "secret.h"
    secret.write_text("#define SECRET 1\n")
    (d / "leak.h").symlink_to(secret)
    r = convert(d)
    assert not any(n.endswith("leak.h") for n in _zip(r).namelist())


def test_out_of_range_defines_are_not_switches():
    r = convert(FIX / "self-test-adjust", machine="Self Test")
    assert [sw["n"] for sw in r.machine_def["switches"]] == [8]
    assert not any(h.name == "PinNodeGame.h" and not h.placed for h in r.hooks)


def test_the_machine_title_and_platform_go_in_the_manifest():
    r = convert(FIX / "self-test-adjust", title="williams-scorpion", platform="williams-sys6")
    assert r.manifest["ruleset"]["title"] == "williams-scorpion"
    assert r.manifest["ruleset"]["platform"] == "williams-sys6"
    assert "Machine: `williams-scorpion`, platform `williams-sys6`." in r.report_md
    r = convert(FIX / "self-test-adjust")
    assert r.manifest["ruleset"]["platform"] == "bally-35" and "placeholders" in r.report_md


def test_the_high_score_waits_while_a_claim_code_shows():
    for fix, cond in (("example-machine", "(CurrentTime / 8000) % 2 == 0"),
                      ("st2025-upstream", "(CurrentTime / 6500) % 2 == 1")):
        r = convert(FIX / fix)
        hook = next(h for h in r.hooks if "high score" in h.name)
        assert hook.placed, fix
        ino = next(n for n in _zip(r).namelist() if n.endswith(".ino") and "/" in n
                   and _zip(r).read(n).decode().count("RunAttractMode"))
        text = _zip(r).read(ino).decode()
        assert f"(({cond}) && !PinNode_ClaimCodesShowing())" in text, fix


MENU_GAME = """#include "RPU.h"
#define EEPROM_MY_HIGH_SCORE 4090
OperatorMenus Menus;
int MachineState;
unsigned long CurrentTime;
void RunOperatorMenu() {
  if (!Menus.UpdateMenu(CurrentTime)) {
    // Menu is done
    return;
  }
}
int HandleSystemSwitches(int curState, byte switchHit) {
  int returnState = curState;
  switch (switchHit) {
    case SW_SELF_TEST_SWITCH:
      Menus.EnterOperatorMenu();
      break;
  }
  return returnState;
}
void setup() {
  unsigned long initResult;
  initResult = RPU_InitializeMPU(RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET |
                                 RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN, SW_CREDIT_RESET);
}
void loop() {}
"""


def test_operator_menu_guard(tmp_path):
    # ExampleMachine-style menus: two self-test presses mid-game, and the blank first page closes itself
    game = tmp_path / "MenuGame"
    game.mkdir()
    (game / "MenuGame.ino").write_text(MENU_GAME)
    r = convert(game, machine="Test Machine", ruleset_id="rpu-menu-game")
    assert any(h.name.startswith("operator menu: self-test guard") and h.placed for h in r.hooks)
    ino = _zip(r).read("MenuGame/MenuGame.ino").decode()
    assert ("if (PinNode_SelfTestOpens(MachineState > MACHINE_STATE_ATTRACT, CurrentTime)) Menus.EnterOperatorMenu();"
            in _func(ino, "HandleSystemSwitches"))                    # one press in attract, two mid-game
    assert "if (!Menus.UpdateMenu(CurrentTime) || PinNode_OperatorMenuGuard(Menus, CurrentTime))" in \
        _func(ino, "RunOperatorMenu")
    setup = _func(ino, "setup")                                          # the game's own result variable
    assert ("initResult = RPU_InitializeMPU(PinNode_BootOptions(RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET |" in setup)
    assert "PinNode_BootChosen(initResult);" in setup and "if (initResult & RPU_RET_ORIGINAL_CODE_REQUESTED)" in setup
    assert any(h.name == "EEPROM 4080-4095" and not h.placed and "EEPROM_MY_HIGH_SCORE" in h.note for h in r.hooks)


def test_boot_hook_skips_the_rpu_definition(tmp_path):
    # SpaceBattle2022: RPU.cpp defines RPU_InitializeMPU in the sketch folder. The hook used to
    # rewrite that definition ("unsigned long unsigned long pnInitResult = ...") and not compile.
    game = tmp_path / "MenuGame"
    game.mkdir()
    (game / "MenuGame.ino").write_text(MENU_GAME)
    rpu = "unsigned long RPU_InitializeMPU(unsigned long initOptions, byte creditResetSwitch) {\n  return 0;\n}\n"
    (game / "AAA_RPU.cpp").write_text(rpu)                    # sorts before the .ino
    r = convert(game, machine="Test Machine", ruleset_id="rpu-menu-game")
    z = _zip(r)
    assert z.read("MenuGame/AAA_RPU.cpp").decode() == rpu
    assert "PinNode_BootChosen(initResult);" in _func(z.read("MenuGame/MenuGame.ino").decode(), "setup")


def test_converter_version_matches_the_library():
    import re
    from tools.pinnode_gen import __version__
    props = (Path(__file__).resolve().parent.parent / "library.properties").read_text()
    assert re.search(r"^version=(.+)$", props, re.M).group(1).strip() == __version__


def test_long_io_comments_stay_within_the_manifest_limit():
    # Star Trek 2022's machine header: notes in the define comments broke the manifest (and the build)
    from tools.pinnode_gen.convert import NAME_MAX, _name
    assert _name("LA_CREDIT", "the 2022 code's name; on the machine it's the apron credit lamp") == "the 2022 code's name"
    assert _name("SW_SHOOTER_LANE", "the shooter lane switch: the ball passing back into the lane") == "the shooter lane switch"
    long = "x" * 60
    assert _name("SW_OUTHOLE", long) == "Outhole"
    assert len(_name("SW_" + "A" * 80, None)) <= NAME_MAX


def test_machine_library_names_win_over_define_comments(tmp_path):
    # Star Trek's manual names (RPUMachines) replace the notes in its define comments
    from tools.pinnode_gen.convert import apply_library_names
    md = {"lamps": [{"n": 0, "name": "the 2022 code's name"}], "switches": [], "solenoids": []}
    assert apply_library_names(md, "bally-star-trek") == "bally-star-trek-1979"
    assert md["lamps"][0]["name"] == "Bonus 1K"
    assert len(md["lamps"]) > 1 and all(len(e["name"]) <= 40 for k in ("lamps", "switches", "solenoids") for e in md[k])
    assert apply_library_names({"lamps": [], "switches": [], "solenoids": []}, "williams-stellar-wars") is None


def test_walkthrough_fixes_on_the_example_machine():
    # clean-room walkthroughs, 2026-10-13: game end missed with match off, tilt one warning early,
    # CONVERSION.md naming the upload folder instead of the sketch, internal tools and placeholders
    r = convert(FIX / "example-machine", machine="Example Machine")
    ino = _zip(r).read("ExampleMachineMEGA/ExampleMachineMEGA.ino").decode()
    end = ino.index("PinNodeCtrl_GameEnd(")
    assert end < ino.index("if (MatchEnabled) {") and ino.index("if (MatchEnabled) {") - end < 200
    assert "PinNodeCtrl_Tilt(CurrentPlayer + 1, NumTiltWarnings > MaxTiltWarnings ?" in ino
    assert "`ExampleMachineMEGA/ExampleMachineMEGA.ino`" in r.report_md
    for internal in ("pinnode-gen", "ADD-PINNODE", "API.md"):
        assert internal not in r.report_md, internal
    assert "pin-node.com/setup/new" in r.report_md
    assert "repo" not in r.manifest["ruleset"]                        # no OWNER/REPO placeholder
    assert r.manifest["ruleset"]["version"].startswith("2026.")       # the game's own version


def test_adjust_done_as_an_expression(tmp_path):
    # BigGame2022: "#define MACHINE_STATE_ADJUST_DONE (MACHINE_STATE_TEST_DONE-20)"
    game = tmp_path / "SelfTestGame"
    game.mkdir()
    src = (FIX / "self-test-adjust" / "SelfTestGame.ino").read_text()
    src = re.sub(r"#define MACHINE_STATE_ADJUST_DONE\s+-27", "#define MACHINE_STATE_ADJUST_DONE (MACHINE_STATE_TEST_DONE-20)", src)
    (game / "SelfTestGame.ino").write_text(src)
    r = convert(game, machine="Test Machine", ruleset_id="rpu-self-test")
    assert any(h.name == "PinNode adjustments" and h.placed for h in r.hooks)
    ino = _zip(r).read("SelfTestGame/SelfTestGame.ino").decode()
    assert "#define PINNODE_ADJ_FIRST_STATE           (MACHINE_STATE_TEST_DONE-20)" in ino
    assert "MACHINE_STATE_ADJUST_DONE ((MACHINE_STATE_TEST_DONE-20) - PINNODE_NUM_ADJUSTMENTS)" in ino


def test_render_report_rebuilds_the_report():
    # a stored conversion (hooks and report_info as JSON) renders the same CONVERSION.md
    r = convert(FIX / "example-machine", machine="Example Machine")
    info = json.loads(json.dumps(r.report_info))
    hooks = [{"name": h.name, "key": h.key, "placed": h.placed, "where": h.where, "note": h.note} for h in r.hooks]
    assert render_report(info, hooks) == r.report_md

def test_phone_tests_on_the_example_machine():
    # owner, 2026-10-13: "this game doesn't offer tests" after converting: the library's tests, one line each
    r = convert(FIX / "example-machine", machine="Example Machine")
    assert any(h.name == "tests from the phone" and h.placed and h.where != "not needed" for h in r.hooks)
    ino = _zip(r).read("ExampleMachineMEGA/ExampleMachineMEGA.ino").decode()
    setup = _func(ino, "setup")
    assert setup.index("PinNode_TestsBegin(") < setup.rindex("PinNodeCtrl_Begin(")     # actions before HELLO
    loop = _func(ino, "loop")
    assert "if (PinNode_Tests(CurrentTime, &newMachineState, MACHINE_STATE_ATTRACT)) {" in loop
    assert loop.index("PinNode_Tests(") < loop.index("RunAttractMode(")
    assert "MachineState != PINNODE_MACHINE_STATE_TESTS" in loop and "MachineState == PINNODE_MACHINE_STATE_TESTS ||" in loop
    ids = [a["id"] for a in r.manifest["actions"]]
    assert ids[:7] == ["lamp-test", "display-test", "solenoid-test", "switch-test", "clear-audits",
                       "enter-diagnostics", "exit-diagnostics"]                        # PinNode.h's order
    assert [a["index"] for a in r.manifest["actions"]] == list(range(len(ids)))


def test_oled_status_screen_on_the_example_machine():
    # RPU Rev 4/102 OLED header: the converter places both calls; they do nothing without PINNODE_OLED
    r = convert(FIX / "example-machine", machine="Example Machine")
    assert any(h.name == "OLED status screen" and h.placed and h.where != "not needed" for h in r.hooks)
    ino = _zip(r).read("ExampleMachineMEGA/ExampleMachineMEGA.ino").decode()
    setup = _func(ino, "setup")
    assert setup.index("PinNode_OledBegin();") < setup.index("RPULink_BeginUart();   // RPU board")
    loop = _func(ino, "loop")
    assert loop.index("CurrentTime = millis();") < loop.index("PinNode_OledUpdate(CurrentTime);")
