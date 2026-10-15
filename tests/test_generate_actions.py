"""Operator actions: the schema accepts actions[], and the header gets ACT_ constants."""

from tools.pinnode_gen.generate import generate
from tools.pinnode_gen.manifest import validate

BASE = {"manifestVersion": 1,
        "ruleset": {"id": "x-game", "title": "x-title", "name": "X", "version": "1.0.0",
                    "scoringVersion": 1}}


def test_actions_validate_and_get_constants():
    m = {**BASE, "actions": [
        {"id": "lamp-test", "index": 0, "name": "Lamp test", "kind": "test",
         "arg": {"min": 0, "max": 59, "label": "Lamp"}, "valueLabel": "Lamp"},
        {"id": "preset-hard", "index": 3, "name": "Hard", "kind": "oneshot", "confirm": True,
         "lock": True},
        {"id": "enter-diagnostics", "index": 4, "name": "Enter", "kind": "oneshot",
         "local": True, "diagnostics": "enter"}]}
    assert not validate(m)
    h = generate(m)
    assert "ACT_LAMP_TEST = 0," in h and "ACT_PRESET_HARD = 3," in h
    assert "ACT_ENTER_DIAGNOSTICS = 4," in h


def test_no_actions_no_block():
    assert "actions" not in generate(BASE)
