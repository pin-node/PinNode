"""pinnode-gen pack: the Sidecar pack (layout and the developer's own images), ready for the game's release."""

from __future__ import annotations

import hashlib
import json
import shutil
import struct
import zlib
from pathlib import Path

from tests.test_sidecar_manifest import _m
from tools.pinnode_gen import pack

RIGHTS = "Every file in this pack is my own work."


def _png() -> bytes:
    """A 1x1 PNG, made here."""
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(b"\x00\xff\x00\x00")) + chunk(b"IEND", b""))


def _game(tmp: Path, layout: dict, media: dict[str, bytes] | None = None) -> Path:
    (tmp / "pinnode.game.json").write_text(json.dumps(_m()))
    (tmp / "sidecar" / "media").mkdir(parents=True)
    (tmp / "sidecar" / "pack.json").write_text(json.dumps(layout))
    for name, data in (media or {}).items():
        (tmp / "sidecar" / "media" / name).write_bytes(data)
    return tmp / "pinnode.game.json"


LAYOUT = {"rights": RIGHTS, "layout": 1, "screen": {"type": "column", "children": [
    {"type": "image", "media": "media:rail-guns"}, {"type": "bar", "state": "rail-guns"}]}}
EMPTY = {"rights": RIGHTS, "layout": 1, "screen": {"type": "text", "text": "Scorpion"}}


def test_builds_a_pack_with_the_developers_addresses(tmp_path):
    mf = _game(tmp_path, LAYOUT, {"rail-guns.png": _png()})
    assert pack.main([str(mf), "--base-url", "https://example.com/scorpion/0.1"]) == 0
    p = json.loads((tmp_path / "sidecar-dist" / "sidecar-pack.json").read_text())
    assert p["sidecarPack"] == 1 and p["game"] == _m()["ruleset"]["id"]
    assert p["sidecar"] == pack.sidecar_hash(_m()) and p["rights"] == RIGHTS and "rights" not in p["layout"]
    (e,) = p["media"]
    assert e == {"id": "rail-guns", "url": "https://example.com/scorpion/0.1/sidecar-rail-guns.png",
                 "type": "image/png", "bytes": len(_png()), "sha256": hashlib.sha256(_png()).hexdigest()}
    assert (tmp_path / "sidecar-dist" / "sidecar-rail-guns.png").read_bytes() == _png()


def _box(kind: bytes, body: bytes) -> bytes:
    return struct.pack(">I", 8 + len(body)) + kind + body


def _mp4(handlers=(b"vide",), seconds=10) -> bytes:
    """A skeleton MP4: ftyp, and moov with mvhd (timescale 1000) and a trak per handler."""
    mvhd = _box(b"mvhd", b"\0" * 4 + b"\0" * 8 + struct.pack(">II", 1000, seconds * 1000) + b"\0" * 80)
    traks = b"".join(_box(b"trak", _box(b"mdia", _box(b"hdlr", b"\0" * 8 + h + b"\0" * 12))) for h in handlers)
    return _box(b"ftyp", b"mp42\0\0\0\0") + _box(b"moov", mvhd + traks)


def _ebml(eid: int, body: bytes) -> bytes:
    idb = eid.to_bytes((eid.bit_length() + 7) // 8, "big")
    return idb + bytes([0x80 | len(body)]) + body if len(body) < 127 else idb + b"\x40" + bytes([len(body)]) + body


def _webm(types=(1,), seconds=10.0) -> bytes:
    info = _ebml(0x1549A966, _ebml(0x2AD7B1, (1_000_000).to_bytes(3, "big")) + _ebml(0x4489, struct.pack(">f", seconds * 1000)))
    tracks = _ebml(0x1654AE6B, b"".join(_ebml(0xAE, _ebml(0x83, bytes([t]))) for t in types))
    return _ebml(0x1A45DFA3, b"") + _ebml(0x18538067, info + tracks)


def test_videos_with_no_sound_go_in(tmp_path):
    layout = {**EMPTY, "screen": {"type": "video", "media": "media:jackpot"}}
    mf = _game(tmp_path, layout, {"jackpot.mp4": _mp4()})
    assert pack.main([str(mf), "--base-url", "https://example.com/x"]) == 0
    p = json.loads((tmp_path / "sidecar-dist" / "sidecar-pack.json").read_text())
    assert p["media"][0]["type"] == "video/mp4" and p["media"][0]["id"] == "jackpot"


def test_videos_with_sound_too_long_or_unreadable_are_refused(tmp_path, capsys):
    cases = [("a.mp4", _mp4((b"vide", b"soun"))), ("b.webm", _webm((1, 2))), ("c.mp4", _mp4(seconds=45)),
             ("d.webm", _webm(seconds=31.0)), ("e.mp4", b"not a video"), ("f.mp4", _mp4((b"soun",)))]
    expect = {"a": "sound track", "b": "sound track", "c": "45 s long", "d": "31 s long", "e": "can't read it",
              "f": "sound track"}
    for name, data in cases:
        d = tmp_path / name
        d.mkdir()
        mf = _game(d, EMPTY, {name: data})
        assert pack.main([str(mf), "--base-url", "https://example.com/x"]) == 1
        assert expect[name[0]] in capsys.readouterr().err, name
    d = tmp_path / "ok"
    d.mkdir()
    assert pack.main([str(_game(d, EMPTY, {"spin.webm": _webm()})), "--base-url", "https://example.com/x"]) == 0


def test_github_addresses_from_the_tag(tmp_path, monkeypatch):
    monkeypatch.setattr(pack.shutil, "which", lambda _: None)        # no gh here: no visibility check
    mf = _game(tmp_path, LAYOUT, {"rail-guns.png": _png()})
    assert pack.main([str(mf), "--tag", "v0.1.0", "--repo", "someone/scorpion"]) == 0
    p = json.loads((tmp_path / "sidecar-dist" / "sidecar-pack.json").read_text())
    assert p["media"][0]["url"] == "https://github.com/someone/scorpion/releases/download/v0.1.0/sidecar-rail-guns.png"


def test_verify_downloads_and_checks(tmp_path, monkeypatch):
    mf = _game(tmp_path, LAYOUT, {"rail-guns.png": _png()})
    host = tmp_path / "host"
    host.mkdir()
    base = "https://example.com/scorpion"
    monkeypatch.setattr(pack, "_fetch", lambda url: (host / url.rsplit("/", 1)[1]).read_bytes())   # the developer's host
    assert pack.main([str(mf), "--base-url", base]) == 0
    shutil.copy(tmp_path / "sidecar-dist" / "sidecar-rail-guns.png", host)
    shutil.copy(tmp_path / "sidecar-dist" / "sidecar-pack.json", host)        # the developer hosts the pack too
    assert pack.main([str(mf), "--base-url", base, "--verify"]) == 0
    (host / "sidecar-rail-guns.png").write_bytes(b"swapped")            # someone changed the file
    assert pack.main([str(mf), "--base-url", base, "--verify"]) == 1


def test_https_or_http_on_your_own_network(tmp_path, capsys):
    mf = _game(tmp_path, LAYOUT, {"rail-guns.png": _png()})
    for url in ("http://example.com/x", "http://8.8.8.8/x", "http://127.0.0.1/x", "ftp://192.168.1.20/x"):
        try:
            pack.main([str(mf), "--base-url", url])
            assert False, f"{url} was accepted"
        except SystemExit as e:
            assert "https://" in str(e)
    for url in ("http://192.168.1.20/packs/scorpion", "http://10.0.0.5/x", "http://172.20.1.1/x",
                "http://nas.local/x", "http://[fe80::1]/x"):
        assert pack.main([str(mf), "--base-url", url]) == 0, url
    p = json.loads((tmp_path / "sidecar-dist" / "sidecar-pack.json").read_text())
    assert p["media"][0]["url"] == "http://[fe80::1]/x/sidecar-rail-guns.png"
    assert "only on phones joined to that network" in capsys.readouterr().out


def test_problems_stop_it(tmp_path, capsys):
    cases = [
        ({"layout": 1, "screen": {"type": "text", "text": "x"}}, {}, "rights"),                                             # no rights statement
        (LAYOUT, {}, '"media:rail-guns"'),                                         # referenced, missing
        (EMPTY, {"theme.mp3": b"x"}, "not a pack file type"),         # images only
        (EMPTY, {"Big Logo.png": _png()}, "name it <id>"),
        (EMPTY, {"huge.png": b"\0" * (2 << 20 + 1)}, "the most for image/png"),
    ]
    for i, (layout, media, expect) in enumerate(cases):
        d = tmp_path / str(i)
        d.mkdir()
        mf = _game(d, layout, media)
        assert pack.main([str(mf), "--base-url", "https://example.com/x"]) == 1
        assert expect in capsys.readouterr().err
        assert not (d / "sidecar-dist").exists()


def test_the_pack_follows_the_keys_not_the_manifest_hash(tmp_path):
    m = _m()
    h = pack.sidecar_hash(m)
    m["ruleset"]["version"] = "9.9.9"                     # a new build with the same keys: the same pack fits
    assert pack.sidecar_hash(m) == h
    m["sidecar"]["states"][0]["key"] = 2                  # keys changed: the pack must be made again
    assert pack.sidecar_hash(m) != h


def test_the_manifest_says_where_the_pack_is(tmp_path, capsys, monkeypatch):
    from tools.pinnode_gen.manifest import validate
    monkeypatch.setattr(pack.shutil, "which", lambda _: None)
    mf = _game(tmp_path, LAYOUT, {"rail-guns.png": _png()})
    assert pack.main([str(mf), "--tag", "v1", "--repo", "you/game"]) == 0
    assert "https://github.com/you/game/releases/latest/download/sidecar-pack.json" in capsys.readouterr().out
    m = _m()
    m["sidecar"]["pack"] = {"url": "https://github.com/you/game/releases/latest/download/sidecar-pack.json"}
    assert validate(m) == []
    m["sidecar"]["pack"]["url"] = "http://example.com/sidecar-pack.json"     # public: HTTPS only
    assert any("pack" in e for e in validate(m))
    m["sidecar"]["pack"]["url"] = "http://192.168.1.20/scorpion/sidecar-pack.json"   # a NAS at home
    assert validate(m) == []


def test_a_full_layout_passes():
    layout = {**EMPTY, "accent": "#33ccff", "screen": {"type": "column", "children": [
        {"type": "text", "text": "Rail Guns {rail-guns}% for player {player}", "size": "heading"},
        {"type": "players", "children": [{"type": "value", "state": "weapon-selected", "big": True},
                                         {"type": "lamps", "state": "stories", "color": "#ff0000"}]},
        {"type": "ring", "state": "rail-guns", "label": "Charge", "when": {"state": "stories", "bit": 1}},
        {"type": "timer", "timer": "eggs", "style": "bar", "when": {"timer": "eggs"}},
        {"type": "video", "media": "media:jackpot", "height": 200, "when": {"state": "rail-guns", "above": 50}}]},
        "events": [{"event": "lock-ready", "text": "Lock {value}!", "media": "media:jackpot", "seconds": 4}]}
    assert pack.layout_problems(layout, _m(), {"jackpot": Path("jackpot.mp4")}) == []


def test_layouts_the_phone_would_refuse_are_caught():
    media = {"pic": Path("pic.png"), "clip": Path("clip.mp4")}
    cases = [
        ({"layout": 2}, 'needs "layout": 1'),
        ({"theme": "dark"}, "no field 'theme'"),
        ({"accent": "blue"}, "accent must be #rrggbb"),
        ({"screen": {"type": "gauge", "state": "rail-guns"}}, "not a widget"),
        ({"screen": {"type": "bar", "state": "rail-guns", "icon": "media:pic"}}, "bar has no field 'icon'"),
        ({"screen": {"type": "bar"}}, "needs 'state'"),
        ({"screen": {"type": "bar", "state": "nope"}}, "no state 'nope'"),
        ({"screen": {"type": "lamps", "state": "rail-guns"}}, "lamps need a flags state"),
        ({"screen": {"type": "timer", "timer": "eggs", "style": "dial"}}, "style is bar or text"),
        ({"screen": {"type": "image", "media": "media:clip"}}, "is video/mp4, not an image"),
        ({"screen": {"type": "video", "media": "media:pic", "height": 1000}}, "height is 24-480"),
        ({"screen": {"type": "column", "children": []}}, "children must be 1-24"),
        ({"screen": {"type": "text", "text": "{bogus}"}}, "{bogus} isn't a state id"),
        ({"screen": {"type": "text", "text": "x", "when": {"state": "rail-guns", "is": 1, "above": 2}}}, "one of is"),
        ({"screen": {"type": "text", "text": "x", "when": {"timer": "nope"}}}, "no timer 'nope'"),
        ({"events": [{"event": "lock-ready"}]}, "needs text, media or both"),
        ({"events": [{"event": "nope", "text": "x"}]}, "no event 'nope'"),
        ({"events": [{"event": "lock-ready", "text": "x", "seconds": 30}]}, "seconds is 1-10"),
    ]
    for change, expect in cases:
        got = pack.layout_problems({**EMPTY, **change}, _m(), media)
        assert any(expect in p for p in got), (change, got)


def test_too_deep_or_too_many_widgets():
    deep = {"type": "text", "text": "x"}
    for _ in range(8):
        deep = {"type": "column", "children": [deep]}
    assert any("8 deep" in p for p in pack.layout_problems({**EMPTY, "screen": deep}, _m(), {}))
    wide = {"type": "column", "children": [{"type": "row", "children": [{"type": "text", "text": "x"}] * 24}] * 9}
    assert any("the most is 200" in p for p in pack.layout_problems({**EMPTY, "screen": wide}, _m(), {}))


HTML = "<!doctype html><title>Scorpion</title><style>b{color:#f00}</style><img src='data:image/png;base64,AA=='>" \
       "<script>window.addEventListener('message', e => {})</script>"


def _html_game(tmp: Path, html: str, layout: dict | None = None) -> Path:
    mf = _game(tmp, layout or {"rights": RIGHTS})
    (tmp / "sidecar" / "pack.html").write_text(html)
    return mf


def test_an_html_pack(tmp_path, monkeypatch):
    mf = _html_game(tmp_path, HTML)
    assert pack.main([str(mf), "--base-url", "http://192.168.1.20/scorpion"]) == 0
    p = json.loads((tmp_path / "sidecar-dist" / "sidecar-pack.json").read_text())
    assert "layout" not in p and p["media"] == []
    assert p["html"] == {"url": "http://192.168.1.20/scorpion/sidecar-pack.html", "bytes": len(HTML.encode()),
                         "sha256": hashlib.sha256(HTML.encode()).hexdigest()}
    assert (tmp_path / "sidecar-dist" / "sidecar-pack.html").read_text() == HTML
    host = tmp_path / "sidecar-dist"
    monkeypatch.setattr(pack, "_fetch", lambda url: (host / url.rsplit("/", 1)[1]).read_bytes())
    assert pack.main([str(mf), "--base-url", "http://192.168.1.20/scorpion", "--verify"]) == 0


def test_html_and_a_layout_both_go_in(tmp_path):
    mf = _html_game(tmp_path, HTML, EMPTY)
    assert pack.main([str(mf), "--base-url", "https://example.com/x"]) == 0
    p = json.loads((tmp_path / "sidecar-dist" / "sidecar-pack.json").read_text())
    assert p["layout"]["layout"] == 1 and "html" in p


def test_no_html_and_no_layout_is_refused(tmp_path, capsys):
    mf = _game(tmp_path, {"rights": RIGHTS})
    assert pack.main([str(mf), "--base-url", "https://example.com/x"]) == 1
    assert '"screen"' in capsys.readouterr().err


def test_html_that_reaches_for_the_network_is_refused(tmp_path, capsys):
    cases = {
        "<script src='https://cdn.example.com/x.js'></script>": "src=/href=",
        '<link rel=stylesheet href="//fonts.example.com/a.css">': "src=/href=",
        "<div style=\"background:url('http://x.example/a.png')\">": "CSS url()",
        "<style>@import 'a.css';</style>": "imports a stylesheet",
        "<script>fetch('/scores')</script>": "fetch()",
        "<script>new XMLHttpRequest()</script>": "XMLHttpRequest",
        "<script>new WebSocket('wss://x')</script>": "WebSocket",
        '<meta http-equiv="refresh" content="0;url=https://x">': "meta refresh",
    }
    for i, (html, expect) in enumerate(cases.items()):
        d = tmp_path / str(i)
        d.mkdir()
        assert pack.main([str(_html_game(d, html)), "--base-url", "https://example.com/x"]) == 1
        err = capsys.readouterr().err
        assert expect in err and "line 1" in err, (html, err)
    d = tmp_path / "big"
    d.mkdir()
    assert pack.main([str(_html_game(d, "x" * (2 << 20 + 1))), "--base-url", "https://example.com/x"]) == 1
    assert "the most is 2,097,152" in capsys.readouterr().err
