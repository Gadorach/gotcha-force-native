"""Map the reaction-band effects and their call-site operands.

The reaction scaffold `8002E2A8` has 29 calls after the amount/HP boundary.
This tool reads their direct call windows from the qualified DOL and records
the last write to each argument register (r3-r7): an immediate, a field load,
or a paired lis+addi/ori address. Known dispatch parameters from the matched
helpers are attached as summaries.

Addresses, offsets, immediates and labels only: no code or asset bytes.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.analyze.reaction_scaffold_map import (
    FUNCTION, HP_WRITE, ROLES, ROLE_NOTES, SIZE, decode_calls,
)
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

LOAD = {32: "lwz", 34: "lbz", 40: "lhz", 42: "lha", 48: "lfs"}
STORE = {36: "stw", 38: "stb", 44: "sth", 52: "stfs"}

# Known parameters from the matched helper sources (summaries, not source).
EFFECTS = {
    "8002FCBC": {
        "dispatch": "800F04B4",
        "opcode": "0x26",
        "state": "object+0x2AE = 4",
        "volume": "object+0x7FC",
        "role": "impact record",
    },
    "8002FC18": {
        "dispatch": "800F088C",
        "selector": "value from record+10",
        "state": "object+0x2AE = 4",
        "thresholds": ["80436FB8", "80436F94", "80436FBC"],
        "role": "impact record variant",
    },
    "8002F8DC": {
        "role": "force/ledger update",
        "fields": ["actor+0x4AA", "actor+0x424", "attacker+0x420",
                   "team+0x118", "ring +0x464/+0x484"],
    },
    "80019550": {"role": "color", "table": "802C7ED0"},
    "8013F300": {"role": "object spawn", "allocator": "80088AA0"},
    "80176EC0": {"role": "object spawn", "allocator": "80088AA0"},
    "8010E158": {"role": "effect spawn", "allocator": "80088AA0"},
    "801EC1BC": {"role": "reaction effect"},
    "801DFF24": {"role": "reaction effect"},
    "800DB250": {"role": "effect values"},
    "80006C24": {"role": "slot update"},
    "8018214C": {"role": "reaction helper"},
    "8002F8DC": {"role": "force/ledger update"},
    "800300BC": {"role": "impact mode"},
    "8003CD5C": {"role": "amount"},
    "8003D344": {"role": "HP write"},
}


def _s(v, bits):
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


def _decode_write(word):
    """Return the destination and decoded source of one instruction."""
    op = word >> 26
    rd = (word >> 21) & 31
    ra = (word >> 16) & 31
    imm = word & 0xFFFF
    if op in STORE:
        return None, None
    if op == 14:
        if ra == 0:
            return rd, {"kind": "imm", "value": _s(imm, 16)}
        return rd, {"kind": "address", "base": ra, "offset": _s(imm, 16)}
    if op == 15:
        if ra == 0:
            return rd, {"kind": "hi", "value": _s(imm, 16) << 16}
        return rd, {"kind": "address", "base": ra,
                    "offset": _s(imm, 16) << 16}
    if op == 24:
        return rd, {"kind": "lo", "value": imm}
    if op in LOAD:
        return rd, {"kind": "field", "op": LOAD[op], "base": ra,
                    "offset": _s(imm, 16)}
    if op == 31:
        rs = (word >> 21) & 31
        rb = (word >> 11) & 31
        if rs == rb:
            return ra, {"kind": "move", "source": rs}
    return None, None


def call_arguments(code, site):
    offset = site - FUNCTION
    last = {}
    consts = {}
    for at in range(max(0, offset - 48), offset, 4):
        word = struct.unpack_from(">I", code, at)[0]
        reg, write = _decode_write(word)
        if reg is None:
            continue
        kind = write["kind"]
        if kind == "hi":
            consts[reg] = write["value"]
            write = {"kind": "hi", "value": write["value"]}
        elif kind in ("address", "address_hi"):
            base = consts.get(write["base"])
            if base is not None:
                value = base + write["offset"]
                consts[reg] = value
                write = {"kind": "absolute", "value": value}
            else:
                consts[reg] = None
                write = {"kind": "address", "base": write["base"],
                         "offset": write["offset"]}
        elif kind in ("imm", "lo"):
            consts[reg] = write["value"]
        elif kind == "move":
            consts[reg] = consts.get(write["source"])
            write = {"kind": "move", "source": write["source"]}
        else:
            consts[reg] = None
        last[reg] = write
    args = {}
    for reg in range(3, 8):
        write = last.get(reg)
        if write is None:
            continue
        entry = dict(write)
        if "value" in entry:
            entry["hex"] = f"{entry['value'] & 0xFFFFFFFF:08X}"
        args[f"r{reg}"] = entry
    return args


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    code = image.read(FUNCTION, SIZE, "text")
    calls = [call for call in decode_calls(code, FUNCTION)
             if int(call["site"], 16) > HP_WRITE]
    if len(calls) != 29:
        raise ValueError(f"reaction call count changed: {len(calls)}")
    mapped = []
    for call in calls:
        target = int(call["target"], 16)
        role, confidence = ROLES.get(target, ("unclassified", "unknown"))
        mapped.append({
            "site": call["site"],
            "target": call["target"],
            "role": role,
            "confidence": confidence,
            "arguments": call_arguments(code, int(call["site"], 16)),
            **({"note": ROLE_NOTES[target]} if target in ROLE_NOTES else {}),
            **({"effect": EFFECTS[f"{target:08X}"]}
               if f"{target:08X}" in EFFECTS else {}),
        })
    return {
        "kind": "gf-reaction-effect-map",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "function": {"address": f"{FUNCTION:08X}", "size": SIZE,
                     "calls": len(mapped)},
        "effects": {key: value for key, value in sorted(EFFECTS.items())},
        "calls": mapped,
        "notes": [
            "The argument map is the last write to r3-r7 in the 12-instruction "
            "window before each call.",
            "Immediates and field offsets are decoded from the DOL; the effect "
            "labels come from matched helper summaries.",
            "This is presentation/reaction data, not a recovered renderer.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("reaction effect output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "calls": len(payload["calls"])}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
