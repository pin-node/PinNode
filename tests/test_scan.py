"""pinnode-gen scan against RetroPinUpgrade's ExampleMachine.

The hand-written template (tests/fixtures/rpu-standard.pinnode.json, from
pinnode-spec-docs) was built by reading the same source. The scanner must
reproduce everything that's derivable from code: type, range, list values,
default and EEPROM slot."""

from __future__ import annotations

import json
from pathlib import Path

from tools.pinnode_gen.scan import main, report, scan

FIX = Path(__file__).parent / "fixtures"
EM = FIX / "example-machine"


def _by_menu(settings):
    return {s["source"]["menuId"]: s for s in settings if s.get("source", {}).get("menuId")}


def test_matches_the_hand_written_template():
    drafted = _by_menu([d.setting for d in scan(EM).settings])
    template = _by_menu(json.loads((FIX / "rpu-standard.pinnode.json").read_text())["settings"])
    assert set(template) <= set(drafted)
    for menu_id, want in template.items():
        got = drafted[menu_id]
        assert got["source"].get("variable") == want["source"]["variable"], menu_id
        if "eeprom" in want["source"]:
            assert got["source"].get("eeprom") == want["source"]["eeprom"], menu_id
        assert got["default"] == want["default"], menu_id
        if want["type"] == "flags":           # the scanner can only flag this for review
            assert got["type"] == "int" and got["max"] == 7
            continue
        assert got["type"] == want["type"], menu_id
        for k in ("min", "max"):
            if k in want:
                assert got[k] == want[k], (menu_id, k)
        if "values" in want:
            assert [v["v"] for v in got["values"]] == [v["v"] for v in want["values"]], menu_id


def test_coin_labels_come_from_the_cpc_table():
    s = _by_menu([d.setting for d in scan(EM).settings])["OM_BASIC_ADJ_IDS_CPC_1"]
    assert s["values"][6] == {"v": 6, "label": "2 coins / 1 credit"}
    assert s["values"][0] == {"v": 0, "label": "1 coin / 5 credits"}


def test_state_entries_skipped_and_gaps_reported():
    r = scan(EM)
    assert any("CREDITS" in x for x in r.skipped) and any("HISCR" in x for x in r.skipped)
    assert "EEPROM_GAME_RULES_SELECTION" in r.unmapped_eeprom
    assert "EEPROM_SLINGSHOT_STRENGTH" in r.unmapped_eeprom
    assert any("TILT_WARNINGS" in w and "up to 3" in w for w in r.warnings)


def test_coil_strengths_need_presence():
    drafted = _by_menu([d.setting for d in scan(EM).settings])
    assert drafted["OM_GAME_ADJ_TROUGH_EJECT_STRENGTH"]["requiresPresence"] is True
    assert drafted["OM_GAME_ADJ_TROUGH_EJECT_STRENGTH"]["group"] == "service"


def test_draft_is_valid_manifest_settings():
    import jsonschema

    schema = json.loads((FIX / "pinnode-game.schema.json").read_text())
    manifest = json.loads((FIX / "rpu-standard.pinnode.json").read_text())
    manifest["settings"] = [d.setting for d in scan(EM).settings]
    jsonschema.Draft202012Validator(schema).validate(manifest)


def test_unrecognised_code_says_so(tmp_path):
    (tmp_path / "Game.ino").write_text("void setup() {}\nvoid loop() {}\n")
    r = scan(tmp_path)
    assert r.settings == []
    assert "No RPU adjustments found" in report(r)


def test_cli_writes_files(tmp_path):
    main([str(EM), "--json", str(tmp_path / "d.json"), "--report", str(tmp_path / "r.md")])
    assert len(json.loads((tmp_path / "d.json").read_text())["settings"]) == 22
    assert "Inconsistencies" in (tmp_path / "r.md").read_text()


def test_audits_from_the_audit_menu():
    audits = scan(EM).audits
    template = json.loads((FIX / "rpu-standard.pinnode.json").read_text())["audits"]
    assert [a["id"] for a in audits] == [a["id"] for a in template]
    assert audits[0] == {"index": 0, "id": "total-plays", "name": "Total plays"}


def test_tournament_switch_is_marked():
    s = _by_menu([d.setting for d in scan(EM).settings])["OM_BASIC_ADJ_IDS_TOURNAMENT_MODE"]
    assert s["tournamentMode"] == 1 and s["difficulty"] is True


def test_the_older_self_test_adjustments():
    """Trident2020RPU, SpaceBattle2022, Scorpion2022-RPU, Demon23…:
    `case MACHINE_STATE_ADJUST_...:` with capitalised names and
    two-argument ReadSetting."""
    r = scan(FIX / "self-test-adjust")
    s = {d.setting["id"]: d.setting for d in r.settings}
    assert list(s) == ["free-play", "ball-save", "music-volume", "tournament-scoring",
                       "balls-override", "extra-ball-award", "award-override", "special-award",
                       "scrolling-scores"]
    assert s["free-play"]["type"] == "bool" and s["free-play"]["group"] == "pricing"
    assert s["free-play"]["source"] == {"menuId": "MACHINE_STATE_ADJUST_FREEPLAY",
                                        "variable": "FreePlayMode", "eeprom": 101}
    assert s["ball-save"]["type"] == "enum"
    assert [v["v"] for v in s["ball-save"]["values"]] == [0, 5, 10, 15, 20]
    assert s["ball-save"]["default"] == 15                  # from ReadSetting(slot, 15)
    assert s["music-volume"] == {**s["music-volume"], "type": "int", "min": 0, "max": 10,
                                 "default": 8, "group": "sound"}
    assert s["tournament-scoring"]["tournamentMode"] == 1
    assert [v["v"] for v in s["balls-override"]["values"]] == [3, 5, 99]
    assert s["extra-ball-award"]["type"] == "score"
    assert s["extra-ball-award"]["default"] == 20000        # from RPU_ReadULFromEEProm
    # 99 = "game default" in an ADJ_TYPE_MIN_MAX_DEFAULT
    assert s["award-override"]["default"] == 99
    assert [v["v"] for v in s["award-override"]["values"]] == [99, 0, 1, 2, 3, 4, 5, 6, 7]
    # a score read with no default, then checked: the check's value
    assert s["special-award"]["default"] == 40000
    assert s["scrolling-scores"]["group"] == "display" and s["scrolling-scores"]["default"] == 1


def test_long_menu_ids_fit_the_manifest():
    from tools.pinnode_gen.scan import _humanise
    sid, name = _humanise("OM_SINGLE_COMBAT_NUMBER_OF_SECONDS")
    assert sid == "single-combat-number-of-seconds" and len(sid) <= 32
    sid, _ = _humanise("OM_BASE_TIME_FOR_KINGS_CHALLENGES_IN_THE_GREAT_HALL")
    assert len(sid) <= 32 and not sid.endswith("-")


def test_two_argument_readsetting_takes_its_own_clamp():
    # SpaceBattle2022: the 2-argument form, each read followed by its clamp. The old pattern ran
    # across lines and paired a setting with the next read's default ("accepts up to 3").
    from tools.pinnode_gen.scan import CLAMP_RE, READSETTING_RE
    src = """
  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
  if (BallSaveNumSeconds > 20) BallSaveNumSeconds = 20;

  SoundSelector = ReadSetting(EEPROM_SOUND_SELECTOR_BYTE, 3);
  if (SoundSelector > 8) SoundSelector = 3;

  MusicVolume = ReadSetting(EEPROM_MUSIC_VOLUME_BYTE, 10, 10);
"""
    reads = {v: (slot, d, mx) for v, slot, d, mx in READSETTING_RE.findall(src)}
    assert reads["BallSaveNumSeconds"] == ("EEPROM_BALL_SAVE_BYTE", "15", "")
    assert reads["SoundSelector"] == ("EEPROM_SOUND_SELECTOR_BYTE", "3", "")
    assert reads["MusicVolume"] == ("EEPROM_MUSIC_VOLUME_BYTE", "10", "10")
    clamps = {v: top for v, top, to in CLAMP_RE.findall(src) if top == to}
    assert clamps == {"BallSaveNumSeconds": "20"}   # SoundSelector resets to 3: not a clamp
