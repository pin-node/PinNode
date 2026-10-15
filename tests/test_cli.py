"""pinnode-gen generate/check."""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

from tools.pinnode_gen.cli import main
from tools.pinnode_gen.generate import generate
from tools.pinnode_gen.manifest import index_conflicts, manifest_hash, validate

from tests._fixtures import FIX
ROOT = Path(__file__).parents[1]
MANIFESTS = ["strange-science.pinnode.json", "rpu-standard.pinnode.json"]


def _m(name):
    return json.loads((FIX / name).read_text())


@pytest.mark.parametrize("name", MANIFESTS)
def test_generated_header_compiles(name, tmp_path):
    compiler = shutil.which("clang++") or shutil.which("g++")
    if not compiler:
        pytest.skip("no C++ compiler")
    (tmp_path / "PinNodeGame.h").write_text(generate(_m(name)))
    (tmp_path / "use.cpp").write_text(
        '#include "PinNodeGame.h"\n'
        "int main() { return PINNODE_SETTINGS_COUNT + (int)PINNODE_SETTINGS[0].type; }\n")
    r = subprocess.run([compiler, "-std=c++17", "-fsyntax-only", "-Wall", "-Werror",
                        f"-I{ROOT / 'src'}", str(tmp_path / "use.cpp")],
                       capture_output=True, text=True)
    assert r.returncode == 0, r.stderr


def test_constants_match_the_manifest():
    h = generate(_m("strange-science.pinnode.json"))
    assert "ACH_FULL_VOLTAGE = 1," in h
    assert "STAT_BUMPER_HITS = 0," in h
    mh = manifest_hash(_m("strange-science.pinnode.json"))
    assert f'#define PINNODE_MANIFEST_HASH   "{mh}"' in h


def test_flags_enum_and_presence_in_the_table():
    h = generate(_m("rpu-standard.pinnode.json"))
    assert "PINNODE_ENUM_COINS_PER_CREDIT_1[] PROGMEM = { 0, 1, 2, 3, 4, 5, 6, 7, 8 }" in h
    assert "{ SET_SCORE_AWARD_REPLAY, PN_FLAGS, 0, 7, 7," in h and "7u }" in h
    assert "PN_REQUIRES_PRESENCE" in h.split("SET_TROUGH_EJECT_STRENGTH, PN_ENUM")[1].split("\n")[0]


def test_retired_entries_are_comments_only():
    m = _m("strange-science.pinnode.json")
    m["achievements"][2]["retired"] = True
    h = generate(m)
    assert "  ACH_ANTI_GRAVITY = 2," not in h
    assert "// retired: ACH_ANTI_GRAVITY = 2" in h


def test_hash_matches_the_pinnode_service():
    """A header built here must verify there: same canonical JSON, same hash."""
    svc = ROOT.parent / "pinnode-service"
    if not (svc / "pinnode_service" / "manifest.py").exists():
        pytest.skip("pinnode-service not checked out alongside")
    code = ("import json,sys; sys.path.insert(0, %r); from pinnode_service.manifest import "
            "manifest_hash; print(manifest_hash(json.load(open(%r))))")
    for name in MANIFESTS:
        out = subprocess.run([str(svc / ".venv/bin/python"), "-c",
                              code % (str(svc), str(FIX / name))],
                             capture_output=True, text=True, check=True).stdout.strip()
        assert out == manifest_hash(_m(name)), name


def test_check_catches_a_stale_header(tmp_path, capsys):
    man = tmp_path / "pinnode.game.json"
    man.write_text((FIX / "strange-science.pinnode.json").read_text())
    assert main(["generate", str(man)]) == 0
    assert main(["check", str(man), "--header", str(tmp_path / "PinNodeGame.h")]) == 0
    m = json.loads(man.read_text())
    m["achievements"].append({"id": "new-one", "index": 9, "name": "New", "desc": "d",
                              "points": 5, "kind": "event"})
    man.write_text(json.dumps(m))
    assert main(["check", str(man), "--header", str(tmp_path / "PinNodeGame.h")]) == 1
    assert "out of date" in capsys.readouterr().out


def test_check_against_previous_release(tmp_path, capsys):
    old = tmp_path / "old.json"
    old.write_text((FIX / "strange-science.pinnode.json").read_text())
    m = json.loads(old.read_text())
    m["achievements"][0]["id"] = "renamed"          # reuses index 0
    new = tmp_path / "new.json"
    new.write_text(json.dumps(m))
    assert main(["check", str(new), "--previous", str(old), "--github"]) == 1
    out = capsys.readouterr().out
    assert "index 0 was 'first-light'" in out and "::error file=" in out


def test_generate_refuses_an_invalid_manifest(tmp_path):
    m = _m("strange-science.pinnode.json")
    m["achievements"][1]["index"] = 0
    p = tmp_path / "bad.json"
    p.write_text(json.dumps(m))
    assert main(["generate", str(p)]) == 1
    assert not (tmp_path / "PinNodeGame.h").exists()


def test_validate_rules():
    m = _m("rpu-standard.pinnode.json")
    m["settings"][21]["operatorOnly"] = False
    assert any("requires presence" in e for e in validate(m))
    assert index_conflicts([_m("rpu-standard.pinnode.json")], _m("rpu-standard.pinnode.json")) == []


def test_installed_command_runs():
    exe = Path(sys.executable).parent / "pinnode-gen"
    r = subprocess.run([str(exe), "check", str(FIX / "strange-science.pinnode.json")],
                       capture_output=True, text=True)
    assert r.returncode == 0 and "OK" in r.stdout


def test_tournament_switch_rules():
    m = json.loads((FIX / "rpu-standard.pinnode.json").read_text())
    assert validate(m) == []
    bad = json.loads(json.dumps(m))
    bad["settings"][7]["tournamentMode"] = 2            # a bool can't be 2
    assert any("tournamentMode 2" in e for e in validate(bad))
    two = json.loads(json.dumps(m))
    two["settings"][19]["tournamentMode"] = 1           # match-feature too
    assert any("only one tournament switch" in e for e in validate(two))
