"""
Manifest rules shared by `pinnode-gen check`, `generate`, and (vendored) the
PinNode service's release import. SPEC-game-manifest §9–§10.

Keep in step with pinnode-service/pinnode_service/manifest.py. Both are
tested against the same fixtures, and the hash must match byte for byte, or
games built from a checked manifest wouldn't verify.
"""

from __future__ import annotations

import hashlib
import ipaddress
import json
from pathlib import Path
from urllib.parse import urlsplit

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
    errors += _sidecar_errors(m)
    return [e for e in errors if not e.startswith("(schema not checked")] + \
        [e for e in errors if e.startswith("(schema not checked")]


# Sidecar's lists (PHASE0-2026-11-02-sidecar): numbered by key (states, timers) or code (events)
SIDECAR_LISTS = (("states", "key"), ("timers", "key"), ("events", "code"))


def _numbered_lists(m: dict):
    """(label, entries, number field) for every list whose numbers are fixed once released."""
    for kind in INDEXED_LISTS:
        yield kind, m.get(kind, []), "index"
    sc = m.get("sidecar", {})
    for kind, field in SIDECAR_LISTS:
        yield f"sidecar.{kind}", sc.get(kind, []), field


def _sidecar_errors(m: dict) -> list[str]:
    errors = []
    sc = m.get("sidecar", {})
    for kind, field in SIDECAR_LISTS:
        for key in ("id", field):
            seen: set = set()
            for e in sc.get(kind, []):
                if e[key] in seen:
                    errors.append(f"sidecar.{kind}: duplicate {key} {e[key]!r}")
                seen.add(e[key])
    for s in sc.get("states", []):
        lo, hi = s.get("min"), s.get("max")
        if lo is not None and hi is not None and lo > hi:
            errors.append(f"sidecar.states: {s['id']!r} has min > max")
        for k in ("min", "max"):
            if isinstance(s.get(k), int) and not -2**31 <= s[k] <= I32_MAX:
                errors.append(f"sidecar.states: {s['id']!r} {k} doesn't fit a 32-bit value")
        kind = s.get("kind")
        if kind == "flags" and not s.get("bits"):
            errors.append(f"sidecar.states: {s['id']!r} is flags but names no bits")
        if kind == "enum" and not s.get("values"):
            errors.append(f"sidecar.states: {s['id']!r} is enum but names no values")
        if kind == "flags" and any(b["n"] > 31 for b in s.get("bits", [])):
            errors.append(f"sidecar.states: {s['id']!r} has a bit above 31")
    url = sc.get("pack", {}).get("url")
    if url and (p := pack_address_problem(url)):
        errors.append(f"sidecar.pack: {p}")
    return errors


# A home network: phones may load a pack from here over plain http (owner, 2026-11-03)
LOCAL_NETWORKS = [ipaddress.ip_network(n) for n in
                  ("10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16", "169.254.0.0/16", "fc00::/7", "fe80::/10")]


def is_local_host(host: str) -> bool:
    if host.lower().endswith(".local"):
        return True
    try:
        ip = ipaddress.ip_address(host)
    except ValueError:
        return False
    return any(ip in n for n in LOCAL_NETWORKS)


def pack_address_problem(url: str) -> str | None:
    """Phones load a pack and its media over HTTPS, or over plain http from the local network only."""
    u = urlsplit(url)
    if u.scheme not in ("https", "http") or not u.hostname:
        return f"{url} isn't an https:// address"
    if u.scheme == "http" and not is_local_host(u.hostname):
        return (f"{url}: plain http is only for your own network (10.x, 172.16-31.x, 192.168.x, link-local "
                "or a .local name); use https://")
    return None


def index_conflicts(previous: list[dict], new: dict) -> list[str]:
    """An index never changes id, an id never changes index, and nothing
    disappears without first being marked retired. The same for Sidecar's
    keys and codes."""
    problems = []
    new_lists = {label: (entries, field) for label, entries, field in _numbered_lists(new)}
    for label, (entries, field) in new_lists.items():
        by_index: dict[int, str] = {}
        by_id: dict[str, int] = {}
        for old in previous:
            for old_label, old_entries, _ in _numbered_lists(old):
                if old_label != label:
                    continue
                for e in old_entries:
                    by_index.setdefault(e[field], e["id"])
                    by_id.setdefault(e["id"], e[field])
        new_ids = set()
        for e in entries:
            new_ids.add(e["id"])
            if by_index.get(e[field], e["id"]) != e["id"]:
                problems.append(f"{label}: {field} {e[field]} was {by_index[e[field]]!r}, "
                                f"now {e['id']!r}")
            if by_id.get(e["id"], e[field]) != e[field]:
                problems.append(f"{label}: {e['id']!r} moved from {field} {by_id[e['id']]} "
                                f"to {e[field]}")
        for old_id in by_id:
            if old_id not in new_ids:
                problems.append(f"{label}: {old_id!r} removed; keep it with \"retired\": true")
    return problems
