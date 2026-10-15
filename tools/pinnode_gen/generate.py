"""
pinnode-gen generate: pinnode.game.json → PinNodeGame.h (SPEC-game-manifest §11).

Only numbers go into Mega flash: enum constants for achievements, stats,
settings and audits, and a PROGMEM table of setting types, ranges, defaults
and flags so the Mega can check a value sent from the web. Names stay on the
node and the website.
"""

from __future__ import annotations

import re

from .manifest import manifest_hash

TYPE_CODES = {"int": "PN_INT", "bool": "PN_BOOL", "enum": "PN_ENUM", "score": "PN_SCORE",
              "flags": "PN_FLAGS"}


def _const(prefix: str, id_: str) -> str:
    return prefix + re.sub(r"[^A-Z0-9]", "_", id_.upper())


def _flags(s: dict) -> str:
    f = []
    if s.get("difficulty"):
        f.append("PN_DIFFICULTY")
    if s.get("operatorOnly", True):
        f.append("PN_OPERATOR_ONLY")
    if s.get("liveSafe"):
        f.append("PN_LIVE_SAFE")
    if s.get("requiresPresence") or s.get("group") == "service":
        f.append("PN_REQUIRES_PRESENCE")
    return " | ".join(f) or "0"


def _enum_block(title: str, prefix: str, entries: list[dict]) -> list[str]:
    live = [e for e in entries if not e.get("retired")]
    retired = [e for e in entries if e.get("retired")]
    if not entries:
        return [f"// no {title}"]
    out = [f"enum : uint8_t {{   // {title}"]
    for e in sorted(live, key=lambda e: e["index"]):
        out.append(f"  {_const(prefix, e['id'])} = {e['index']},")
    out.append("};")
    for e in sorted(retired, key=lambda e: e["index"]):
        out.append(f"// retired: {_const(prefix, e['id'])} = {e['index']} (index reserved forever)")
    return out


def generate(m: dict, source_name: str = "pinnode.game.json") -> str:
    r = m["ruleset"]
    settings = sorted([s for s in m.get("settings", []) if not s.get("retired")],
                      key=lambda s: s["index"])
    lines = [
        f"// GENERATED from {source_name}. Do not edit.",
        "// After changing the manifest, make a new one in the Achievements builder (pin-node.com/builder).",
        "#pragma once",
        "#include <PinNode.h>",
        "",
        f'#define PINNODE_RULESET_ID      "{r["id"]}"',
        f'#define PINNODE_RULESET_VERSION "{r["version"]}"',
        f'#define PINNODE_MANIFEST_HASH   "{manifest_hash(m)}"',
        f"#define PINNODE_SCORING_VERSION {r['scoringVersion']}",
        "",
        *_enum_block("achievements", "ACH_", m.get("achievements", [])),
        "",
        *_enum_block("stats", "STAT_", m.get("stats", [])),
        "",
        *_enum_block("settings", "SET_", m.get("settings", [])),
        "",
        *_enum_block("audits", "AUD_", m.get("audits", [])),
        "",
    ]
    if m.get("actions"):
        # Operator actions (SPEC-operator-actions): only when declared, so the
        # headers of games without actions don't change.
        lines += [*_enum_block("actions", "ACT_", m["actions"]), ""]
    for s in settings:
        if s["type"] == "enum":
            vals = ", ".join(str(v["v"]) for v in s["values"])
            name = f"PINNODE_ENUM_{_const('', s['id'])}"
            lines.append(f"const int32_t {name}[] PROGMEM = {{ {vals} }};")
    lines.append("")
    lines.append("// index, type, min, max, default, flags, enum values (count), flags mask")
    lines.append("const PinNodeSettingDef PINNODE_SETTINGS[] PROGMEM = {")
    for s in settings:
        t = s["type"]
        if t == "bool":
            lo, hi = 0, 1
        elif t == "enum":
            vs = [v["v"] for v in s["values"]]
            lo, hi = min(vs), max(vs)
        elif t == "flags":
            lo, hi = 0, sum(1 << b["bit"] for b in s["bits"])
        else:
            lo, hi = s["min"], s["max"]
        enum_ptr = f"PINNODE_ENUM_{_const('', s['id'])}" if t == "enum" else "nullptr"
        n_enum = len(s["values"]) if t == "enum" else 0
        mask = sum(1 << b["bit"] for b in s["bits"]) if t == "flags" else 0
        lines.append(f"  {{ {_const('SET_', s['id'])}, {TYPE_CODES[t]}, {lo}, {hi}, "
                     f"{s['default']}, {_flags(s)}, {enum_ptr}, {n_enum}, {mask}u }},")
    lines.append("};")
    lines.append(f"const uint8_t PINNODE_SETTINGS_COUNT = {len(settings)};")
    return "\n".join(lines) + "\n"
