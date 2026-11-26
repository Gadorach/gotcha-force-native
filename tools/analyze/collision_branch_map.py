"""Map the conditional branches of the collision walk `8002BB14`.

The walk has 98 conditional branches across the broad-phase, geometry, apply
and post bands. This tool decodes each `bc`, looks back for the flag-setting
compare/mask, and records the condition and target. Addresses and operands
only: no code or asset bytes.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

FUNCTION = 0x8002BB14
SIZE = 3712
PAIR_LOOP_CALL = 0x8002C4CC
CONDS = ("lt", "gt", "eq", "so")


def _s(v, bits):
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


def _phase(site):
    if site < 0x8002BE00:
        return "broad-phase"
    if site < 0x8002C000:
        return "geometry"
    if site <= PAIR_LOOP_CALL:
        return "apply"
    return "post"


def _mask(mb, me):
    value = 0
    for bit in range(mb, me + 1):
        value |= 1 << (31 - bit)
    return value


def _condition(code, site):
    """Decode the last flag-setting compare/mask before a branch."""
    base = site - FUNCTION
    for step in range(1, 9):
        at = base - step * 4
        if at < 0:
            break
        word = struct.unpack_from(">I", code, at)[0]
        op = word >> 26
        if op == 11:
            cr = (word >> 23) & 7
            ra = (word >> 16) & 31
            imm = _s(word & 0xFFFF, 16)
            return {"kind": "cmpi", "cr": cr, "text": f"r{ra} cmp {imm}",
                    "signed": True}, site - step * 4
        if op == 10:
            cr = (word >> 23) & 7
            ra = (word >> 16) & 31
            imm = word & 0xFFFF
            return {"kind": "cmpli", "cr": cr, "text": f"r{ra} cmpl {imm}",
                    "signed": False}, site - step * 4
        if op == 63:
            xo = (word >> 1) & 0x3FF
            if xo in (0, 32):
                cr = (word >> 23) & 7
                fa = (word >> 16) & 31
                fb = (word >> 11) & 31
                name = "fcmpu" if xo == 0 else "fcmpo"
                return {"kind": name, "cr": cr,
                        "text": f"f{fa} {name} f{fb}"}, FUNCTION + at
        if op == 31:
            xo = (word >> 1) & 0x3FF
            ra = (word >> 16) & 31
            rs = (word >> 21) & 31
            rb = (word >> 11) & 31
            if xo == 0:
                return {"kind": "cmp", "cr": (word >> 23) & 7,
                        "text": f"r{ra} cmp r{rb}", "signed": True}, FUNCTION + at
            if xo == 32:
                return {"kind": "cmpl", "cr": (word >> 23) & 7,
                        "text": f"r{ra} cmpl r{rb}", "signed": False}, FUNCTION + at
            if xo == 28 and (word & 1):
                return {"kind": "and.", "text": f"(r{ra} & r{rb})"}, FUNCTION + at
            if xo == 954 and (word & 1):
                return {"kind": "extsb.", "text": f"s8(r{rs})"}, FUNCTION + at
            if xo == 922 and (word & 1):
                return {"kind": "extsh.", "text": f"s16(r{rs})"}, FUNCTION + at
        if op == 21 and (word & 1):
            rs = (word >> 21) & 31
            sh = (word >> 11) & 31
            mb = (word >> 6) & 31
            me = (word >> 1) & 31
            return {"kind": "rlwinm.", "text": f"(r{rs} & 0x{_mask(mb, me):08X})",
                    "shift": sh}, FUNCTION + at
        if op in (28, 29) and (word & 1):
            ra = (word >> 16) & 31
            rs = (word >> 21) & 31
            imm = word & 0xFFFF
            if op == 29:
                imm <<= 16
            return {"kind": "andi.", "text": f"(r{rs} & 0x{imm:08X})"}, FUNCTION + at
    return {"kind": "unknown", "text": "unknown condition"}, None


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    code = image.read(FUNCTION, SIZE, "text")
    branches = []
    for off in range(0, len(code), 4):
        word = struct.unpack_from(">I", code, off)[0]
        if word >> 26 != 16:
            continue
        site = FUNCTION + off
        bo = (word >> 21) & 31
        bi = (word >> 16) & 31
        displacement = word & 0xFFFC
        if displacement & 0x8000:
            displacement -= 0x10000
        target = site + displacement if not (word & 2) else displacement
        condition, source = _condition(code, site)
        branches.append({
            "site": f"{site:08X}",
            "target": f"{target & 0xFFFFFFFF:08X}",
            "phase": _phase(site),
            "bo": bo,
            "bi": bi,
            "cr": bi >> 2,
            "cond": CONDS[bi & 3],
            "sense": "true" if bo & 8 else "false",
            "condition": condition,
            **({"source": f"{source:08X}"} if source is not None else {}),
        })
    if len(branches) != 98:
        raise ValueError(f"branch count changed: {len(branches)}")
    phases = {}
    for branch in branches:
        phases[branch["phase"]] = phases.get(branch["phase"], 0) + 1
    return {
        "kind": "gf-collision-branch-map",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "function": {"address": f"{FUNCTION:08X}", "size": SIZE,
                     "branches": len(branches)},
        "pair_loop": f"{PAIR_LOOP_CALL:08X}",
        "phases": phases,
        "branches": branches,
        "notes": [
            "Each branch records the nearest flag-setting compare/mask in the "
            "eight instructions before it.",
            "A 'sense' of true branches when the condition holds; false is the "
            "inverse test (for example beq vs bne).",
            "This is control-flow data; handler semantics remain to be named.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("collision branch output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "branches": len(payload["branches"]), "phases": payload["phases"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
