"""
pinnode-gen pack: a game's Sidecar pack (the screen for phones), ready to publish with the game's release.

    pinnode-gen pack [pinnode.game.json] --tag v0.2.0 [--upload] [--verify]
    pinnode-gen pack [pinnode.game.json] --base-url https://example.com/mygame/0.2.0 [--verify]

The developer keeps, beside the manifest:

    sidecar/pack.json      the layout: PinNode's widgets bound to the game's states, timers and events, and
                           "rights": a statement that every file is the developer's own or licensed to them
    sidecar/media/         the developer's own images and videos (optional), named <id>.<ext>; the layout
                           refers to each as "media:<id>"

and this makes sidecar-dist/: sidecar-pack.json (the layout, each media file's address, size and SHA-256,
and the hash of the manifest's sidecar block it was made for) and the media files to publish.

The developer hosts the pack and its media, never PinNode: by default as files on the game's own GitHub
release (--upload uses the developer's own `gh` login), anywhere public over HTTPS with --base-url, or on
their own network (a NAS, a computer) over plain http with --base-url http://192.168.x.x/... The manifest's
sidecar block says where phones find the pack ("pack": {"url": ...}); phones download it and the media
straight from the developer and refuse any media file whose SHA-256 differs. PinNode never fetches either.
A pack can change without a new firmware: only its address is in the manifest, and the GitHub "latest"
address never changes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import urllib.request
from pathlib import Path
from urllib.parse import urlsplit

from .manifest import canonical_json, is_local_host, pack_address_problem

PACK_VERSION = 1
# images and videos (no sounds), by extension, and the most one file may weigh
MEDIA_TYPES = {
    ".png": ("image/png", 2 << 20), ".jpg": ("image/jpeg", 2 << 20), ".jpeg": ("image/jpeg", 2 << 20),
    ".webp": ("image/webp", 2 << 20),
    ".mp4": ("video/mp4", 10 << 20), ".webm": ("video/webm", 10 << 20),
}
MAX_FILES = 64
MAX_TOTAL = 50 << 20
MEDIA_ID = re.compile(r"^[a-z0-9][a-z0-9-]{0,31}$")
MEDIA_REF = re.compile(r"^media:([a-z0-9][a-z0-9-]{0,31})$")
PACK_ASSET = "sidecar-pack.json"
MAX_VIDEO_SECONDS = 30


# ---- videos: no sound track (the owner's no-sounds rule is enforced here, not only by playing muted), and a length

def _mp4_info(data: bytes) -> tuple[list[str], float | None]:
    """The track handler types ('vide', 'soun'...) and the length in seconds, from an MP4's boxes."""
    handlers, seconds = [], None

    def walk(start: int, end: int, depth: int):
        nonlocal seconds
        i = start
        while i + 8 <= end and depth < 8:
            size = int.from_bytes(data[i:i + 4], "big")
            kind = data[i + 4:i + 8]
            head = 8
            if size == 1 and i + 16 <= end:
                size, head = int.from_bytes(data[i + 8:i + 16], "big"), 16
            elif size == 0:
                size = end - i
            if size < head or i + size > end:
                raise ValueError("a box runs past the end of the file")
            body = i + head
            if kind in (b"moov", b"trak", b"mdia"):
                walk(body, i + size, depth + 1)
            elif kind == b"hdlr" and body + 12 <= i + size:
                handlers.append(data[body + 8:body + 12].decode("latin-1"))
            elif kind == b"mvhd" and depth == 1:
                v = data[body]
                if v == 1:
                    scale = int.from_bytes(data[body + 20:body + 24], "big")
                    dur = int.from_bytes(data[body + 24:body + 32], "big")
                else:
                    scale = int.from_bytes(data[body + 12:body + 16], "big")
                    dur = int.from_bytes(data[body + 16:body + 20], "big")
                seconds = dur / scale if scale else None
            i += size

    walk(0, len(data), 0)
    return handlers, seconds


def _ebml_vint(data: bytes, i: int, keep_marker: bool) -> tuple[int, int]:
    first = data[i]
    length = 1
    while length <= 8 and not first & (0x80 >> (length - 1)):
        length += 1
    if length > 8 or i + length > len(data):
        raise ValueError("bad EBML number")
    value = first if keep_marker else first & (0xFF >> length)
    for b in data[i + 1:i + length]:
        value = value << 8 | b
    return value, length


def _webm_info(data: bytes) -> tuple[list[int], float | None]:
    """The track types (1 video, 2 audio...) and the length in seconds, from a WebM's elements."""
    import struct
    types, duration, scale = [], None, 1_000_000
    containers = {0x18538067, 0x1654AE6B, 0xAE, 0x1549A966}     # Segment, Tracks, TrackEntry, Info

    def walk(start: int, end: int, depth: int):
        nonlocal duration, scale
        i = start
        while i < end and depth < 6:
            eid, n = _ebml_vint(data, i, True)
            size, m = _ebml_vint(data, i + n, False)
            body = i + n + m
            if size == (1 << (7 * m)) - 1:                       # unknown size: to the end
                size = end - body
            if body + size > end:
                raise ValueError("an element runs past the end of the file")
            if eid in containers:
                walk(body, body + size, depth + 1)
            elif eid == 0x83:                                    # TrackType
                types.append(int.from_bytes(data[body:body + size], "big"))
            elif eid == 0x2AD7B1:                                # TimecodeScale (ns)
                scale = int.from_bytes(data[body:body + size], "big")
            elif eid == 0x4489:                                  # Duration (float, in TimecodeScale units)
                duration = struct.unpack(">f" if size == 4 else ">d", data[body:body + size])[0]
            i = body + size

    walk(0, len(data), 0)
    return types, (duration * scale / 1e9) if duration is not None else None


def video_problem(p: Path) -> str | None:
    """Why a video can't go in a pack, or None."""
    data = p.read_bytes()
    try:
        if p.suffix.lower() == ".mp4":
            handlers, seconds = _mp4_info(data)
            audio = "soun" in handlers
            has_video = "vide" in handlers
        else:
            types, seconds = _webm_info(data)
            audio = 2 in types
            has_video = 1 in types
    except (ValueError, IndexError):
        return "can't read it: export it again as MP4 (H.264) or WebM (VP9), with no sound"
    if audio:
        return "has a sound track: packs have no sound, so export it again without audio"
    if not has_video:
        return "has no video track"
    if seconds is None:
        return "doesn't say how long it is: export it again"
    if seconds > MAX_VIDEO_SECONDS:
        return f"is {seconds:.0f} s long; the most is {MAX_VIDEO_SECONDS} s"
    return None


def sidecar_hash(m: dict) -> str:
    """The keys a pack is drawn for: the manifest's sidecar states, timers and events (16 hex characters)."""
    sc = m.get("sidecar", {})
    body = {k: sc.get(k, []) for k in ("states", "timers", "events")}
    return hashlib.sha256(canonical_json(body).encode("utf-8")).hexdigest()[:16]


def _refs(value, out: set):
    if isinstance(value, str):
        hit = MEDIA_REF.match(value)
        if hit:
            out.add(hit.group(1))
    elif isinstance(value, list):
        for v in value:
            _refs(v, out)
    elif isinstance(value, dict):
        for v in value.values():
            _refs(v, out)


# ---- the layout, version 1 (pinnode-app docs/sidecar-layout.md): the phone refuses the whole pack for any
# unknown widget or field, so the same rules are checked here before the developer publishes

LAYOUT_FIELDS = {"rights", "layout", "accent", "screen", "events"}
COLOR = re.compile(r"^#[0-9a-fA-F]{6}$")
PLACEHOLDER = re.compile(r"\{([^{}]*)\}")
WHEN_TESTS = ("is", "not", "above", "below", "bit")
CONTAINERS = ("column", "row", "players")
# type: its fields besides "type" and "when"; the required ones first in REQUIRED
WIDGETS = {
    "column": {"children"}, "row": {"children"}, "players": {"children"},
    "text": {"text", "size"},
    "value": {"state", "label", "big"},
    "bar": {"state", "label", "color"}, "ring": {"state", "label", "color"},
    "lamps": {"state", "label", "color"},
    "timer": {"timer", "label", "style", "color"},
    "image": {"media", "height"}, "video": {"media", "height"},
}
REQUIRED = {"column": "children", "row": "children", "players": "children", "text": "text", "value": "state",
            "bar": "state", "ring": "state", "lamps": "state", "timer": "timer", "image": "media", "video": "media"}
MAX_WIDGETS, MAX_DEPTH, MAX_CHILDREN, MAX_EVENTS = 200, 8, 24, 64
MAX_PACK_BYTES = 512 << 10


def _is_int(v) -> bool:
    return isinstance(v, int) and not isinstance(v, bool)


def layout_problems(layout: dict, m: dict, media: dict[str, Path], has_html: bool = False) -> list[str]:
    """Why the phone would refuse this layout (pinnode-app's layout format 1), as messages. With an HTML pack
    the layout is optional: pack.json may then hold only "rights"."""
    sc = m.get("sidecar", {})
    states = {s["id"]: s for s in sc.get("states", []) if not s.get("retired")}
    timers = {t["id"] for t in sc.get("timers", []) if not t.get("retired")}
    events = {e["id"] for e in sc.get("events", []) if not e.get("retired")}
    out: list[str] = []
    count = 0

    def media_ref(where: str, v, want: str | None):
        hit = MEDIA_REF.match(v) if isinstance(v, str) else None
        if not hit:
            out.append(f'{where}: media must be "media:<id>"')
        elif hit.group(1) in media and want:
            kind = MEDIA_TYPES[media[hit.group(1)].suffix.lower()][0]
            if not kind.startswith(want + "/"):
                out.append(f"{where}: media:{hit.group(1)} is {kind}, not {'an' if want == 'image' else 'a'} {want}")

    def text_ok(where: str, v, names: set[str]):
        if not isinstance(v, str) or not 1 <= len(v) <= 120:
            out.append(f"{where}: text must be 1-120 characters")
            return
        for name in PLACEHOLDER.findall(v):
            if name not in names:
                out.append(f"{where}: {{{name}}} isn't a state id or one of {', '.join(sorted(names - set(states)))}")

    def when_ok(where: str, w):
        if not isinstance(w, dict):
            out.append(f"{where}: when must be an object")
        elif "timer" in w:
            if set(w) != {"timer"}:
                out.append(f'{where}: a timer when is just {{"timer": "<id>"}}')
            elif w["timer"] not in timers:
                out.append(f"{where}: no timer {w['timer']!r} in the manifest")
        else:
            tests = [t for t in WHEN_TESTS if t in w]
            if set(w) - {"state", *WHEN_TESTS} or "state" not in w or len(tests) != 1:
                out.append(f"{where}: when is a state and one of {', '.join(WHEN_TESTS)}, or a timer")
            elif w["state"] not in states:
                out.append(f"{where}: no state {w['state']!r} in the manifest")
            elif not _is_int(w[tests[0]]) or (tests[0] == "bit" and not 0 <= w["bit"] <= 31):
                out.append(f"{where}: {tests[0]} needs a whole number{' 0-31' if tests[0] == 'bit' else ''}")

    def widget(where: str, w, depth: int):
        nonlocal count
        count += 1
        if depth > MAX_DEPTH:
            out.append(f"{where}: nested more than {MAX_DEPTH} deep")
            return
        if not isinstance(w, dict) or w.get("type") not in WIDGETS:
            out.append(f"{where}: not a widget (type one of {', '.join(WIDGETS)})")
            return
        t = w["type"]
        where = f"{where} ({t})"
        for f in sorted(set(w) - WIDGETS[t] - {"type", "when"}):
            out.append(f"{where}: {t} has no field {f!r}")
        if REQUIRED[t] not in w:
            out.append(f"{where}: needs {REQUIRED[t]!r}")
        if "when" in w:
            when_ok(where, w["when"])
        if "label" in w and not (isinstance(w["label"], str) and 1 <= len(w["label"]) <= 120):
            out.append(f"{where}: label must be 1-120 characters")
        if "color" in w and not (isinstance(w["color"], str) and COLOR.match(w["color"])):
            out.append(f"{where}: color must be #rrggbb")
        if t in CONTAINERS and "children" in w:
            kids = w["children"]
            if not isinstance(kids, list) or not 1 <= len(kids) <= MAX_CHILDREN:
                out.append(f"{where}: children must be 1-{MAX_CHILDREN} widgets")
            else:
                for i, k in enumerate(kids):
                    widget(f"{where}.children[{i}]", k, depth + 1)
        elif t == "text":
            if "text" in w:
                text_ok(where, w["text"], set(states) | {"player"})
            if "size" in w and w["size"] not in ("small", "body", "heading", "title"):
                out.append(f"{where}: size is small, body, heading or title")
        elif "state" in w:
            if w["state"] not in states:
                out.append(f"{where}: no state {w['state']!r} in the manifest")
            elif t == "lamps" and states[w["state"]].get("kind") != "flags":
                out.append(f"{where}: lamps need a flags state; {w['state']!r} isn't one")
            if "big" in w and not isinstance(w["big"], bool):
                out.append(f"{where}: big is true or false")
        elif t == "timer" and "timer" in w:
            if w["timer"] not in timers:
                out.append(f"{where}: no timer {w['timer']!r} in the manifest")
            if "style" in w and w["style"] not in ("bar", "text"):
                out.append(f"{where}: style is bar or text")
        elif t in ("image", "video") and "media" in w:
            media_ref(where, w["media"], t)
            if "height" in w and not (_is_int(w["height"]) and 24 <= w["height"] <= 480):
                out.append(f"{where}: height is 24-480")

    for f in sorted(set(layout) - LAYOUT_FIELDS):
        out.append(f"pack.json has no field {f!r}")
    if has_html and set(layout) <= {"rights"}:          # an HTML pack with no layout beside it
        return out
    if layout.get("layout") != 1:
        out.append('pack.json needs "layout": 1')
    if "accent" in layout and not (isinstance(layout["accent"], str) and COLOR.match(layout["accent"])):
        out.append("pack.json: accent must be #rrggbb")
    if "screen" not in layout:
        out.append('pack.json needs "screen": one widget, usually a column')
    else:
        widget("screen", layout["screen"], 1)
        if count > MAX_WIDGETS:
            out.append(f"the screen has {count} widgets; the most is {MAX_WIDGETS}")
    evs = layout.get("events", [])
    if not isinstance(evs, list) or len(evs) > MAX_EVENTS:
        out.append(f"pack.json: events must be a list of at most {MAX_EVENTS}")
        evs = []
    for i, e in enumerate(evs):
        where = f"events[{i}]"
        if not isinstance(e, dict):
            out.append(f"{where}: not an object")
            continue
        for f in sorted(set(e) - {"event", "text", "media", "seconds"}):
            out.append(f"{where}: an event has no field {f!r}")
        if e.get("event") not in events:
            out.append(f"{where}: no event {e.get('event')!r} in the manifest")
        if "text" not in e and "media" not in e:
            out.append(f"{where}: needs text, media or both")
        if "text" in e:
            text_ok(where, e["text"], {"value", "player"})
        if "media" in e:
            media_ref(where, e["media"], None)
        if "seconds" in e and not (_is_int(e["seconds"]) and 1 <= e["seconds"] <= 10):
            out.append(f"{where}: seconds is 1-10")
    return out


# ---- an HTML pack (owner, 2026-11-03): one self-contained file, shown in a locked-down view with no network.
# The phone blocks every request anyway; this tells the developer before publishing.

HTML_ASSET = "sidecar-pack.html"
MAX_HTML_BYTES = 2 << 20
EXTERNAL = [
    (re.compile(r"""\b(?:src|href|srcset|action|poster|data|formaction)\s*=\s*["']?\s*(?:https?:)?//""", re.I),
     "loads a file from the network (src=/href= to an address)"),
    (re.compile(r"""url\(\s*["']?\s*(?:https?:)?//""", re.I), "loads a file from the network (CSS url())"),
    (re.compile(r"@import\b", re.I), "imports a stylesheet"),
    (re.compile(r"http-equiv\s*=\s*[\"']?refresh", re.I), "redirects with a meta refresh"),
    (re.compile(r"\bfetch\s*\("), "calls fetch()"),
    (re.compile(r"\bXMLHttpRequest\b"), "uses XMLHttpRequest"),
    (re.compile(r"\bWebSocket\b"), "opens a WebSocket"),
    (re.compile(r"\bEventSource\b"), "opens an EventSource"),
    (re.compile(r"\bsendBeacon\b"), "calls sendBeacon"),
    (re.compile(r"\bimportScripts\b|\bimport\s*\("), "imports a script"),
]


def html_problems(path: Path) -> list[str]:
    """Why sidecar/pack.html can't go in a pack: too big, or it reaches for the network."""
    data = path.read_bytes()
    if len(data) > MAX_HTML_BYTES:
        return [f"pack.html: {len(data):,} bytes; the most is {MAX_HTML_BYTES:,} (put images inline, small)"]
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        return ["pack.html: save it as UTF-8"]
    out = []
    for rx, why in EXTERNAL:
        hit = rx.search(text)
        if hit:
            line = text.count("\n", 0, hit.start()) + 1
            out.append(f"pack.html line {line}: {why}; a pack is one file with everything inline, "
                       "and phones block the network")
    return out


def collect(m: dict, src: Path) -> tuple[dict, Path | None, list[Path], list[str], list[str]]:
    """The layout, the HTML file (or None), the media files, problems (block the pack) and notes."""
    problems, notes = [], []
    html = src / "pack.html"
    html = html if html.is_file() else None
    if html:
        problems += html_problems(html)
    if not m.get("sidecar"):
        problems.append("the manifest has no sidecar block: run pinnode-gen sidecar first")
    layout_path = src / "pack.json"
    try:
        layout = json.loads(layout_path.read_text())
    except FileNotFoundError:
        return {}, html, [], problems + [f"{layout_path} not found: the pack's rights statement and layout"], notes
    except json.JSONDecodeError as e:
        return {}, html, [], problems + [f"{layout_path} is not valid JSON: {e}"], notes
    rights = layout.get("rights") if isinstance(layout, dict) else None
    if not isinstance(rights, str) or len(rights.strip()) < 20:
        problems.append('pack.json needs "rights": a statement that every file in the pack is your own work or '
                        "licensed to you (no maker art, logos or anything taken from a ROM)")
    media_dir = src / "media"
    files = sorted(p for p in media_dir.iterdir() if p.is_file() and not p.name.startswith(".")) \
        if media_dir.is_dir() else []
    ids, total = {}, 0
    for p in files:
        kind = MEDIA_TYPES.get(p.suffix.lower())
        if not kind:
            problems.append(f"media/{p.name}: not a pack file type ({', '.join(sorted(MEDIA_TYPES))})")
            continue
        if not MEDIA_ID.match(p.stem):
            problems.append(f"media/{p.name}: name it <id>{p.suffix}, the id lower case letters, digits and '-'")
            continue
        if p.stem in ids:
            problems.append(f"media/{p.name}: id {p.stem!r} is used twice")
            continue
        size = p.stat().st_size
        if size > kind[1]:
            problems.append(f"media/{p.name}: {size:,} bytes; the most for {kind[0]} is {kind[1]:,}")
        elif kind[0].startswith("video/"):
            why = video_problem(p)
            if why:
                problems.append(f"media/{p.name}: {why}")
        total += size
        ids[p.stem] = p
    if len(files) > MAX_FILES:
        problems.append(f"{len(files)} media files; the most is {MAX_FILES}")
    if total > MAX_TOTAL:
        problems.append(f"media total {total:,} bytes; the most is {MAX_TOTAL:,}")
    if isinstance(layout, dict):
        problems += layout_problems(layout, m, ids, has_html=html is not None)
    else:
        problems.append("pack.json must be an object")
    used: set = set()
    _refs(layout, used)
    for missing in sorted(used - set(ids)):
        problems.append(f'pack.json refers to "media:{missing}", which isn\'t in media/')
    for unused in sorted(set(ids) - used):
        notes.append(f"media/{ids[unused].name} isn't used by pack.json (published anyway)")
    return layout, html, [ids[i] for i in sorted(ids)], problems, notes


def asset_name(p: Path) -> str:
    return f"sidecar-{p.stem}{p.suffix.lower()}"


def build(m: dict, layout: dict, html: Path | None, media: list[Path], base_url: str, version: str) -> dict:
    entries = []
    for p in media:
        data = p.read_bytes()
        entries.append({"id": p.stem, "url": f"{base_url.rstrip('/')}/{asset_name(p)}",
                        "type": MEDIA_TYPES[p.suffix.lower()][0], "bytes": len(data),
                        "sha256": hashlib.sha256(data).hexdigest()})
    pack = {"sidecarPack": PACK_VERSION, "game": m["ruleset"]["id"], "version": version,
            "sidecar": sidecar_hash(m), "rights": layout.get("rights", "")}
    if html:
        data = html.read_bytes()
        pack["html"] = {"url": f"{base_url.rstrip('/')}/{HTML_ASSET}", "bytes": len(data),
                        "sha256": hashlib.sha256(data).hexdigest()}
    rest = {k: v for k, v in layout.items() if k != "rights"}
    if rest:
        pack["layout"] = rest
    pack["media"] = entries
    return pack


def _gh(*args: str) -> subprocess.CompletedProcess:
    if not shutil.which("gh"):
        raise SystemExit("pinnode-gen pack: --upload needs the GitHub CLI (gh), logged in: https://cli.github.com")
    return subprocess.run(["gh", *args], capture_output=True, text=True)


def _fetch(url: str) -> bytes:
    with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "pinnode-gen"}), timeout=60) as r:
        return r.read()


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="pinnode-gen pack", description=__doc__.strip().split("\n")[0])
    ap.add_argument("manifest", nargs="?", type=Path, default=Path("pinnode.game.json"))
    ap.add_argument("--dir", type=Path, help="the pack's sources (default: sidecar/ beside the manifest)")
    ap.add_argument("--out", type=Path, help="where to write (default: sidecar-dist/ beside the manifest)")
    ap.add_argument("--tag", help="the game's GitHub release the media go on (their addresses use it)")
    ap.add_argument("--repo", help="owner/name (default: the manifest's ruleset.repo)")
    ap.add_argument("--base-url", help="host the media yourself: their addresses are <base-url>/<file>")
    ap.add_argument("--version", help="the pack's version (default: the ruleset's)")
    ap.add_argument("--upload", action="store_true", help="put the files on the GitHub release (your gh login)")
    ap.add_argument("--verify", action="store_true", help="download every file again and check its SHA-256")
    a = ap.parse_args(argv)

    m = json.loads(a.manifest.read_text())
    src = a.dir or a.manifest.with_name("sidecar")
    out = a.out or a.manifest.with_name("sidecar-dist")
    repo = a.repo or m.get("ruleset", {}).get("repo")
    if a.upload and a.base_url:
        raise SystemExit("pinnode-gen pack: --upload puts the files on GitHub; with --base-url you host them yourself")
    if not a.base_url and not (a.tag and repo):
        raise SystemExit("pinnode-gen pack: say where the media live: --tag (the game's GitHub release) or --base-url")
    base_url = a.base_url or f"https://github.com/{repo}/releases/download/{a.tag}"
    if p := pack_address_problem(base_url):
        raise SystemExit(f"pinnode-gen pack: --base-url {p}")

    layout, html, media, problems, notes = collect(m, src)
    if not a.base_url and repo and shutil.which("gh"):
        vis = _gh("repo", "view", repo, "--json", "visibility", "-q", ".visibility")
        if vis.returncode == 0 and vis.stdout.strip() == "PRIVATE" and media:
            problems.append(f"{repo} is private, so phones can't download files from its releases: make the repo "
                            "public, put the media on a public repo's release (--repo), or host them (--base-url)")
    for n in notes:
        print(f"note: {n}")
    if problems:
        print(f"pinnode-gen pack: {len(problems)} problem(s), nothing written:", file=sys.stderr)
        for p in problems:
            print(f"  - {p}", file=sys.stderr)
        return 1

    pack = build(m, layout, html, media, base_url, a.version or m["ruleset"]["version"])
    text = json.dumps(pack, indent=1, ensure_ascii=False) + "\n"
    if len(text.encode()) > MAX_PACK_BYTES:
        print(f"pinnode-gen pack: {PACK_ASSET} would be {len(text.encode()):,} bytes; the most is "
              f"{MAX_PACK_BYTES:,}, nothing written", file=sys.stderr)
        return 1
    # where phones find the pack: stable, so the manifest needn't change with each new pack
    pack_url = f"{a.base_url.rstrip('/')}/{PACK_ASSET}" if a.base_url else \
        f"https://github.com/{repo}/releases/latest/download/{PACK_ASSET}"
    have_url = m.get("sidecar", {}).get("pack", {}).get("url")
    out.mkdir(parents=True, exist_ok=True)
    for old in out.glob("sidecar-*"):
        old.unlink()
    (out / PACK_ASSET).write_text(text)
    for p in media:
        shutil.copyfile(p, out / asset_name(p))
    files = [out / PACK_ASSET] + [out / asset_name(p) for p in media]
    if html:
        shutil.copyfile(html, out / HTML_ASSET)
        files.append(out / HTML_ASSET)
    size = sum(e["bytes"] for e in pack["media"])
    print(f"wrote {out}/: {PACK_ASSET}{f', {HTML_ASSET}' if html else ''} and {len(media)} media file(s), "
          f"{size:,} bytes; keys {pack['sidecar']}")
    if have_url != pack_url:
        print(f'note: phones find the pack through the manifest: set "pack": {{"url": "{pack_url}"}} in its sidecar '
              f"block{f' (it says {have_url})' if have_url else ''}, then pinnode-gen generate and build the firmware once")

    if a.upload:
        if _gh("release", "view", a.tag, "--repo", repo).returncode != 0:
            raise SystemExit(f"pinnode-gen pack: no release {a.tag} on {repo}: publish the game's release first "
                             f"(gh release create {a.tag} --repo {repo} ...)")
        up = _gh("release", "upload", a.tag, *map(str, files), "--repo", repo, "--clobber")
        if up.returncode != 0:
            raise SystemExit(f"pinnode-gen pack: upload failed: {up.stderr.strip()}")
        print(f"uploaded {len(files)} file(s) to {repo} release {a.tag}")
    elif not a.base_url:
        print(f"next: upload {out}/ to {repo} release {a.tag} (or run again with --upload)")
    else:
        print(f"next: put the files from {out}/ at {a.base_url.rstrip('/')}/")
        if is_local_host(urlsplit(base_url).hostname):
            print("note: a pack on your own network shows only on phones joined to that network")

    if a.verify:
        bad = 0
        try:
            if _fetch(pack_url) != (out / PACK_ASSET).read_bytes():
                print(f"  - {pack_url}: not the pack just made (an older one, or not uploaded yet)", file=sys.stderr)
                bad += 1
        except Exception as ex:   # noqa: BLE001
            print(f"  - {pack_url}: can't download ({ex})", file=sys.stderr)
            bad += 1
        for e in pack["media"] + ([pack["html"]] if html else []):
            try:
                got = hashlib.sha256(_fetch(e["url"])).hexdigest()
            except Exception as ex:   # noqa: BLE001 - any failure to fetch is a failed check
                print(f"  - {e['url']}: can't download ({ex})", file=sys.stderr)
                bad += 1
                continue
            if got != e["sha256"]:
                print(f"  - {e['url']}: SHA-256 differs from the pack's", file=sys.stderr)
                bad += 1
        if bad:
            print(f"pinnode-gen pack: {bad} check(s) failed: phones would refuse or miss them", file=sys.stderr)
            return 1
        print(f"verified: the pack and all {len(pack['media'])} media file(s) download and match")
    return 0


if __name__ == "__main__":
    sys.exit(main())
