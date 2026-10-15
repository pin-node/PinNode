"""
Manifest rules shared by `pinnode-gen check`, `generate`, and (vendored) the
PinNode service's release import. SPEC-game-manifest §9–§10.

Keep in step with pinnode-service/pinnode_service/manifest.py. Both are
tested against the same fixtures, and the hash must match byte for byte, or
games built from a checked manifest wouldn't verify.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

SCHEMA_PATH = Path(__file__).parent / "pinnode-game.schema.json"
INDEXED_LISTS = ("achievements", "stats", "settings", "audits")
I32_MAX = 2**31 - 1


def canonical_json(value) -> str:
    """RFC 8785-compatible for what manifests contain (objects, arrays,
    strings, integers, booleans)."""
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)


def manifest_hash(manifest: dict) -> str:
    body = {k: v for k, v in manifest.items() if k != "$schema"}
    return hashlib.sha256(canonical_json(body).encode("utf-8")).hexdigest()[:16]


def _schema_errors(m: dict) -> list[str]:
    try:
        import jsonschema
    except ImportError:  # the CLI works without it, but warns
        return ["(schema not checked: pip install jsonschema)"]
    schema = json.loads(SCHEMA_PATH.read_text())
    return [
        f"schema: {'/'.join(str(p) for p in e.absolute_path) or '(root)'}: {e.message}"
        for e in jsonschema.Draft202012Validator(schema).iter_errors(m)
    ]


def setting_allows(s: dict, v: int) -> bool:
    """Is `v` a value setting `s` can take?"""
    t = s["type"]
    if t == "bool":
        return v in (0, 1)
    if t == "enum":
        return v in {x["v"] for x in s.get("values", [])}
    if t == "flags":
        mask = sum(1 << b["bit"] for b in s.get("bits", []))
        return v >= 0 and v & ~mask == 0
    lo, hi = s.get("min"), s.get("max")
    return lo is not None and hi is not None and lo <= v <= hi


def validate(m: dict) -> list[str]:
    errors = _schema_errors(m)
    if any(e.startswith("schema:") for e in errors):
        return errors
    for kind in INDEXED_LISTS:
        for key in ("id", "index"):
            seen: set = set()
            for e in m.get(kind, []):
                if e[key] in seen:
                    errors.append(f"{kind}: duplicate {key} {e[key]!r}")
                seen.add(e[key])
    stat_ids = {s["id"] for s in m.get("stats", [])}
    for a in m.get("achievements", []):
        if a["kind"] == "stat" and a["stat"] not in stat_ids:
            errors.append(f"achievements: {a['id']!r} refers to unknown stat {a['stat']!r}")
    for s in m.get("settings", []):
        if s.get("difficulty") and (s.get("liveSafe") or s.get("operatorOnly") is False):
            errors.append(f"settings: difficulty setting {s['id']!r} must be operatorOnly "
                          "and not liveSafe")
        presence = s.get("requiresPresence") or s.get("group") == "service"
        if presence and (s.get("liveSafe") or s.get("operatorOnly") is False):
            errors.append(f"settings: {s['id']!r} requires presence, so it can't be "
                          "liveSafe or open to players")
        lo, hi = s.get("min"), s.get("max")
        if lo is not None and hi is not None and lo > hi:
            errors.append(f"settings: {s['id']!r} has min > max")
        # The Mega keeps setting values as int32_t (SPEC-node-link).
        for k in ("min", "max", "default"):
            if isinstance(s.get(k), int) and s[k] > I32_MAX:
                errors.append(f"settings: {s['id']!r} {k} exceeds 2,147,483,647")
        if s["type"] == "flags":
            bits = [b["bit"] for b in s.get("bits", [])]
            if len(set(bits)) != len(bits):
                errors.append(f"settings: {s['id']!r} repeats a bit")
        if "tournamentMode" in s and not setting_allows(s, s["tournamentMode"]):
            errors.append(f"settings: {s['id']!r} tournamentMode {s['tournamentMode']} "
                          "isn't one of its values")
    switches = [s["id"] for s in m.get("settings", [])
                if "tournamentMode" in s and not s.get("retired")]
    if len(switches) > 1:
        errors.append(f"settings: only one tournament switch allowed, found {switches}")
    return [e for e in errors if not e.startswith("(schema not checked")] + \
        [e for e in errors if e.startswith("(schema not checked")]


def index_conflicts(previous: list[dict], new: dict) -> list[str]:
    """An index never changes id, an id never changes index, and nothing
    disappears without first being marked retired."""
    problems = []
    for kind in INDEXED_LISTS:
        by_index: dict[int, str] = {}
        by_id: dict[str, int] = {}
        for old in previous:
            for e in old.get(kind, []):
                by_index.setdefault(e["index"], e["id"])
                by_id.setdefault(e["id"], e["index"])
        new_ids = set()
        for e in new.get(kind, []):
            new_ids.add(e["id"])
            if by_index.get(e["index"], e["id"]) != e["id"]:
                problems.append(f"{kind}: index {e['index']} was {by_index[e['index']]!r}, "
                                f"now {e['id']!r}")
            if by_id.get(e["id"], e["index"]) != e["index"]:
                problems.append(f"{kind}: {e['id']!r} moved from index {by_id[e['id']]} "
                                f"to {e['index']}")
        for old_id in by_id:
            if old_id not in new_ids:
                problems.append(f"{kind}: {old_id!r} removed; keep it with \"retired\": true")
    return problems
