"""
pinnode-gen: manifest tools for RPU games.

    pinnode-gen generate [pinnode.game.json] [-o PinNodeGame.h]
    pinnode-gen check    [pinnode.game.json] [--header PinNodeGame.h] [--previous old.json ...]
    pinnode-gen scan     <repo> [--json draft.json] [--report report.md]
    pinnode-gen convert  <sketch dir> [--machine NAME] [--ruleset-id ID] [-o out.zip]

`check` exits 1 on any problem, so it can gate CI (see action.yml).
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from . import convert as convert_mod
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


def cmd_generate(a) -> int:
    m = _load(a.manifest)
    problems = validate(m)
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
    problems = validate(m)
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

    a = p.parse_args(argv)
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
