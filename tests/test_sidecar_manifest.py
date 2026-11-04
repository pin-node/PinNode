"""The manifest's sidecar block (PHASE0-2026-11-02-sidecar): schema, checks, header, and keys fixed once released."""

from __future__ import annotations

import copy
import json
import shutil
import subprocess
from pathlib import Path

import pytest

from tools.pinnode_gen.generate import generate
from tools.pinnode_gen.manifest import index_conflicts, validate

from tests._fixtures import FIX
ROOT = Path(__file__).parents[1]

SIDECAR = {
    "states": [
        {"id": "rail-guns", "key": 1, "name": "Rail Guns charge", "scope": "player", "kind": "percent", "min": 0, "max": 100},
        {"id": "weapon-selected", "key": 9, "name": "Weapon selected", "scope": "player", "kind": "enum",
         "values": [{"n": 0, "name": "None"}, {"n": 1, "name": "Rail Guns"}]},
        {"id": "stories", "key": 10, "name": "Stories finished", "scope": "player", "kind": "flags",
         "bits": [{"n": 0, "name": "Defend"}, {"n": 1, "name": "Attack"}]},
    ],
    "timers": [{"id": "eggs", "key": 1, "name": "Eggs countdown", "scope": "player"}],
    "events": [{"id": "rail-guns-ready", "code": 0x0101, "name": "Rail Guns at 100%", "scope": "player"},
               {"id": "lock-ready", "code": 0x0200, "name": "Lock ready", "scope": "player", "value": "1 blue, 2 green"}],
}


def _m():
    m = json.loads((FIX / "strange-science.pinnode.json").read_text())
    m["sidecar"] = copy.deepcopy(SIDECAR)
    return m


def test_a_good_block_passes():
    assert validate(_m()) == []


@pytest.mark.parametrize("change, expect", [
    (lambda sc: sc["states"].append({"id": "x-dup", "key": 1, "name": "x", "scope": "player"}), "duplicate key 1"),
    (lambda sc: sc["events"].append({"id": "lock-ready", "code": 0x300, "name": "x", "scope": "game"}), "duplicate id 'lock-ready'"),
    (lambda sc: sc["states"][0].update(min=200), "min > max"),
    (lambda sc: sc["states"][2].pop("bits"), "names no bits"),
    (lambda sc: sc["states"][1].pop("values"), "names no values"),
    (lambda sc: sc["states"][0].update(key=240), "schema"),            # 240-255 are PinNode's
    (lambda sc: sc["events"][0].update(code=0xFF00), "schema"),        # 0xFF00-0xFFFF are PinNode's
    (lambda sc: sc["timers"][0].update(scope="table"), "schema"),
])
def test_bad_blocks_are_caught(change, expect):
    m = _m()
    change(m["sidecar"])
    assert any(expect in e for e in validate(m)), validate(m)


def test_numbers_stay_fixed_once_released():
    old = _m()
    assert index_conflicts([old], _m()) == []
    moved = _m()
    moved["sidecar"]["states"][0]["key"] = 2
    assert any("sidecar.states: 'rail-guns' moved from key 1 to 2" in p for p in index_conflicts([old], moved))
    reused = _m()
    reused["sidecar"]["events"][0]["id"] = "something-else"
    assert any("code 257 was 'rail-guns-ready'" in p for p in index_conflicts([old], reused))
    removed = _m()
    removed["sidecar"]["timers"] = []
    assert any("'eggs' removed" in p for p in index_conflicts([old], removed))
    retired = _m()
    retired["sidecar"]["timers"][0]["retired"] = True
    assert index_conflicts([old], retired) == []


def test_header_constants():
    h = generate(_m())
    assert "enum : uint8_t {   // Sidecar states (PinNodeCtrl_SidecarState)" in h
    assert "  SC_STATE_RAIL_GUNS = 1," in h and "  SC_STATE_STORIES = 10," in h
    assert "  SC_TIMER_EGGS = 1," in h
    assert "enum : uint16_t {   // Sidecar events (PinNodeCtrl_Sidecar)" in h
    assert "  SC_EVENT_RAIL_GUNS_READY = 0x0101," in h
    m = _m()
    m["sidecar"]["timers"][0]["retired"] = True
    h = generate(m)
    assert "SC_TIMER_EGGS = 1," not in h and "// retired: SC_TIMER_EGGS = 1 (key reserved forever)" in h


def test_games_without_sidecar_keep_their_header():
    m = json.loads((FIX / "strange-science.pinnode.json").read_text())
    assert "SC_" not in generate(m)


def test_header_compiles_and_calls_build(tmp_path):
    compiler = shutil.which("clang++") or shutil.which("g++")
    if not compiler:
        pytest.skip("no C++ compiler")
    (tmp_path / "PinNodeGame.h").write_text(generate(_m()))
    (tmp_path / "use.cpp").write_text(
        '#include "PinNodeGame.h"\n'
        "int main() {\n"
        "  PinNodeCtrl_SidecarState(1, SC_STATE_RAIL_GUNS, 100);\n"
        "  PinNodeCtrl_SidecarTimer(1, SC_TIMER_EGGS, 20000, 20000);\n"
        "  PinNodeCtrl_Sidecar(1, SC_EVENT_RAIL_GUNS_READY);\n"
        "  return 0;\n}\n")
    # as the library builds them (PINNODE_HOST_BUILD), and as a game built without PinNode (the stubs)
    # (-Werror only on the first: PinNode.h and PinNodeCtrl.h both define PINNODE_NUM_ADJUSTMENTS, an
    # old warning in disabled builds)
    for flags in (["-DPINNODE_HOST_BUILD", "-Werror"], ["-DPINNODE_HOST_BUILD", "-DPINNODE_DISABLED"]):
        r = subprocess.run([compiler, "-std=c++17", "-fsyntax-only", "-Wall", *flags,
                            f"-I{ROOT / 'src'}", f"-I{ROOT / 'tests' / 'native' / 'shim'}", str(tmp_path / "use.cpp")],
                           capture_output=True, text=True)
        assert r.returncode == 0, (flags, r.stderr)
