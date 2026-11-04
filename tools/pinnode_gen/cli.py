"""
pinnode-gen: manifest tools for RPU games.

    pinnode-gen generate [pinnode.game.json] [-o PinNodeGame.h]
    pinnode-gen check    [pinnode.game.json] [--header PinNodeGame.h] [--previous old.json ...]
    pinnode-gen scan     <repo> [--json draft.json] [--report report.md]
    pinnode-gen convert  <sketch dir> [--machine NAME] [--ruleset-id ID] [-o out.zip]
    pinnode-gen sidecar  [pinnode.game.json] [--from sidecar.json]   copy the game's Sidecar manifest in
    pinnode-gen pack     [pinnode.game.json] --tag TAG [--upload] [--verify]   the Sidecar pack for phones

A game that uses Sidecar (the open RPU library) keeps its calls in sidecar.json; its sidecar block in
pinnode.game.json must be the same. `generate` and `check` say when they differ.

`check` exits 1 on any problem, so it can gate CI (see action.yml).
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from . import convert as convert_mod
from . import pack as pack_mod
from . import scan as scan_mod
from .generate import generate
from .manifest import index_conflicts, validate


def _load(path: Path) -> dict:
    try:
        return json.loads(path.read_text())
    except FileNotFoundError:
        raise SystemExit(f"pinnode-gen: {path} not found") from None
    except json.JSONDecodeError as e:
        raise SystemExit(f"pinnode-gen: {path} is not valid JSON: {e}") from None


SIDECAR_KEYS = ("states", "timers", "events")


def _sidecar_file_problems(manifest_path: Path, m: dict) -> list[str]:
    """The sidecar block must match the game's sidecar.json (beside the manifest), when there is one."""
    f = manifest_path.with_name("sidecar.json")
    if not f.exists():
        return []
    want = {k: v for k, v in _load(f).items() if k in SIDECAR_KEYS}
    have = {k: v for k, v in m.get("sidecar", {}).items() if k in SIDECAR_KEYS}
    if want != have:
        return [f"the sidecar block differs from {f.name}: run pinnode-gen sidecar {manifest_path.name}"]
    return []


def cmd_sidecar(a) -> int:
    m = _load(a.manifest)
    src = a.source or a.manifest.with_name("sidecar.json")
    block = dict(m.get("sidecar", {}))
    block.update({k: v for k, v in _load(src).items() if k in SIDECAR_KEYS})
    m["sidecar"] = block
    problems = validate(m)
    if problems:
        print(f"pinnode-gen: with {src.name}, {a.manifest} would have problems; not written:", file=sys.stderr)
        for p in problems:
            print(f"  - {p}", file=sys.stderr)
        return 1
    a.manifest.write_text(json.dumps(m, indent=1, ensure_ascii=False) + "\n")
    print(f"wrote the sidecar block of {a.manifest} from {src}: then run pinnode-gen generate")
    return 0


def cmd_generate(a) -> int:
    m = _load(a.manifest)
    problems = validate(m) + _sidecar_file_problems(a.manifest, m)
    if problems:
        print(f"pinnode-gen: {a.manifest} has problems, header not written:", file=sys.stderr)
        for p in problems:
            print(f"  - {p}", file=sys.stderr)
        return 1
    header = generate(m, a.manifest.name)
    out = a.output or a.manifest.with_name("PinNodeGame.h")
    out.write_text(header)
    print(f"wrote {out}")
    return 0


def cmd_check(a) -> int:
    m = _load(a.manifest)
    problems = validate(m) + _sidecar_file_problems(a.manifest, m)
    previous = [_load(p) for p in (a.previous or [])]
    if previous and not problems:
        problems += index_conflicts(previous, m)
    if a.header:
        if not a.header.exists():
            problems.append(f"{a.header} missing: run pinnode-gen generate {a.manifest.name}")
        elif a.header.read_text() != generate(m, a.manifest.name):
            problems.append(f"{a.header} is out of date with {a.manifest.name}: "
                            f"run pinnode-gen generate {a.manifest.name}")
    if problems:
        print(f"pinnode-gen check: {len(problems)} problem(s)")
        for p in problems:
            print(f"  - {p}")
            if a.github:  # annotations on the PR's Files tab
                print(f"::error file={a.manifest}::{p}")
        return 1
    r = m["ruleset"]
    print(f"pinnode-gen check: OK  {r['id']} {r['version']}  "
          f"{len(m.get('achievements', []))} achievements, {len(m.get('settings', []))} settings"
          + (f", checked against {len(previous)} earlier manifest(s)" if previous else ""))
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(prog="pinnode-gen", description=__doc__.split("\n\n")[0])
    sub = p.add_subparsers(dest="cmd", required=True)

    g = sub.add_parser("generate", help="write PinNodeGame.h from the manifest")
    g.add_argument("manifest", nargs="?", type=Path, default=Path("pinnode.game.json"))
    g.add_argument("-o", "--output", type=Path)
    g.set_defaults(fn=cmd_generate)

    c = sub.add_parser("check", help="validate the manifest (and header, and earlier releases)")
    c.add_argument("manifest", nargs="?", type=Path, default=Path("pinnode.game.json"))
    c.add_argument("--header", type=Path, help="fail if this generated header is stale")
    c.add_argument("--previous", type=Path, nargs="*", help="earlier releases' manifests")
    c.add_argument("--github", action="store_true", help="emit GitHub Actions annotations")
    c.set_defaults(fn=cmd_check)

    s = sub.add_parser("scan", help="draft settings and audits from an RPU game repo")
    s.add_argument("repo", type=Path)
    s.add_argument("--json", type=Path)
    s.add_argument("--report", type=Path)
    s.set_defaults(fn=lambda a: scan_mod.main(
        [str(a.repo)] + (["--json", str(a.json)] if a.json else [])
        + (["--report", str(a.report)] if a.report else [])))

    v = sub.add_parser("convert",
                       help="add PinNode to an RPU game: a ZIP with the hooks, manifest and report")
    v.add_argument("rest", nargs=argparse.REMAINDER)
    v.set_defaults(fn=lambda a: convert_mod.main(a.rest))

    sc = sub.add_parser("sidecar", help="copy the game's sidecar.json into the manifest's sidecar block")
    sc.add_argument("manifest", nargs="?", type=Path, default=Path("pinnode.game.json"))
    sc.add_argument("--from", dest="source", type=Path, help="default: sidecar.json beside the manifest")
    sc.set_defaults(fn=cmd_sidecar)

    pk = sub.add_parser("pack", help="the Sidecar pack: layout and media, ready for the game's release")
    pk.add_argument("rest", nargs=argparse.REMAINDER)
    pk.set_defaults(fn=lambda a: pack_mod.main(a.rest))

    a = p.parse_args(argv)
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
