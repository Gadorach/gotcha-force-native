"""Map the conditional branches of the collision walk `8002BB14`.

The walk has 98 conditional branches across the broad-phase, geometry, apply
and post bands. This tool decodes each `bc`, looks back for the flag-setting
compare/mask, and records the condition and target. Addresses and operands
only: no code or asset bytes.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

FUNCTION = 0x8002BB14
SIZE = 3712
PAIR_LOOP_CALL = 0x8002C4CC
CONDS = ("lt", "gt", "eq", "so")
LOADS = {32: ("lwz", "u32"), 34: ("lbz", "u8"), 40: ("lhz", "u16"),
         42: ("lha", "s16"), 48: ("lfs", "f32")}

# Named gates where the field loads and control flow are unambiguous. The
# remaining apply/post branches are published with their operands but no name.
GATES = {
    "8002BBA0": ("actor-active", "s8(actor+0) == 0 skips the actor visit"),
    "8002BBAC": ("actor-timer-2001", "s8(actor+2001) counts down; zero clears actor+2000"),
    "8002BBF8": ("global-flag-1078", "u8(actor+1078) != 0 sets the battle-level flag"),
    "8002BC10": ("peak-u16-1268-1276", "raise u16(actor+1276) to actor+1268"),
    "8002BC20": ("timer-1272", "s16(actor+1272) counts down; zero clears actor+1268"),
    "8002BC44": ("peak-u16-1270-1278", "raise u16(actor+1278) to actor+1270"),
    "8002BC54": ("timer-1274", "s16(actor+1274) counts down; zero clears actor+1270"),
    "8002BD3C": ("actor-loop-6", "six actors, stride 0x1E00"),
    "8002BD94": ("effect-slot-loop-12", "twelve 0x3E4 entries"),
    "8002BED4": ("geometry-actor-loop", "six actors, stride 0x1E00"),
    "8002BDF8": ("geometry-actor-active", "s8(actor+0) == 0 skips geometry"),
    "8002BE04": ("geometry-subactive-477", "s8(actor+477) == 0 skips geometry"),
    "8002BE28": ("height-extra-475", "s8(actor+475) adds the linked height+464"),
    "8002BE38": ("linked-subactive-477", "s8(*(actor+484)+477) gates the linked height"),
    "8002BE54": ("height-limit-36", "(height - actor+1744) > actor+36 skips"),
    "8002BE90": ("actor-460-nonzero", "a nonzero actor+460 vector skips the state write"),
    "8002BEB0": ("ground-normal-y", "the normalized actor+460 Y reaches the ground constant"),
    "8002BF04": ("pair-actor-active", "s8(actor+0) == 0 skips the pair pass"),
    "8002BF10": ("pair-actor-628-bit1", "u8(actor+628) bit 1 gates the pair pass"),
    "8002BF1C": ("global-1652-zero", "a global counter gates the pair pass"),
    "8002BF30": ("pair-order", "f32 slot comparison chooses the pair order"),
    "8002BFAC": ("pair-flag475", "s8(actor+475) == 0 skips the pair pass"),
    "8002BFBC": ("pair-linked-active", "s8(*(actor+484)+131) == 0 skips the model-table path"),
    "8002BFDC": ("pair-model-table", "a zero global[1652+index*4] entry takes the C0F8 path"),
}


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


def _def(word):
    """Decode the destination and value of one defining instruction."""
    op = word >> 26
    rd = (word >> 21) & 31
    ra = (word >> 16) & 31
    imm = word & 0xFFFF
    if op in LOADS:
        name, width = LOADS[op]
        return rd, {"kind": "field", "op": name, "width": width,
                    "base": ra, "offset": _s(imm, 16)}
    if op == 14:
        if ra == 0:
            return rd, {"kind": "imm", "value": _s(imm, 16)}
        return rd, {"kind": "address", "base": ra, "offset": _s(imm, 16)}
    if op == 15:
        return rd, {"kind": "hi", "value": _s(imm, 16) << 16}
    if op == 24:
        return rd, {"kind": "imm", "value": imm}
    if op == 21:
        rs = (word >> 21) & 31
        mb = (word >> 6) & 31
        me = (word >> 1) & 31
        return rd, {"kind": "mask", "source": rs,
                    "mask": f"0x{_mask(mb, me):08X}"}
    if op == 31:
        xo = (word >> 1) & 0x3FF
        rs = (word >> 21) & 31
        rb = (word >> 11) & 31
        if xo in (23, 87, 279, 343):  # lwzx lbzx lhzx lhax
            names = {23: "lwzx", 87: "lbzx", 279: "lhzx", 343: "lhax"}
            return rd, {"kind": "indexed", "op": names[xo],
                        "base": ra, "index": rb}
        if xo == 444 and rs == rb:  # or ra,rs,rs == mr
            return ra, {"kind": "move", "source": rs}
        if xo == 954:  # extsb / extsb.
            return ra, {"kind": "sign", "source": rs, "width": "s8"}
        if xo == 922:  # extsh / extsh.
            return ra, {"kind": "sign", "source": rs, "width": "s16"}
    if op in (59, 63):
        if op == 63 and ((word >> 1) & 0x3FF) == 72:  # fmr
            return (word >> 21) & 31, {"kind": "move",
                                       "source": (word >> 11) & 31}
        xo = (word >> 1) & 0x1F
        frd = (word >> 21) & 31
        if xo == 18:
            return frd, {"kind": "expression", "op": "fdiv"}
        if xo == 20:
            return frd, {"kind": "expression", "op": "fsub"}
        if xo == 21:
            return frd, {"kind": "expression", "op": "fadd"}
        if xo == 25:
            return frd, {"kind": "expression", "op": "fmul"}
    return None, None


def _provenance(code, at, reg, base, depth=0, is_float=False):
    """Find the last load/immediate that fed `reg` before `at`."""
    if depth > 3:
        return None
    offset = at - base
    for step in range(1, 25):
        pos = offset - step * 4
        if pos < 0:
            return None
        word = struct.unpack_from(">I", code, pos)[0]
        volatile = reg <= 13 if is_float else 3 <= reg <= 12
        if word >> 26 == 18 and (word & 1) and volatile:
            return {"kind": "call"}
        dest, value = _def(word)
        if dest != reg or value is None:
            continue
        kind = value["kind"]
        if kind == "field":
            result = dict(value)
            base_via = _provenance(code, base + pos, value["base"], base,
                                   depth + 1, False)
            if base_via and base_via.get("kind") == "field":
                result["via"] = base_via
            return result
        if kind in ("imm", "hi"):
            return value
        if kind in ("indexed", "expression"):
            return value
        if kind == "mask":
            found = _provenance(code, base + pos, value["source"], base,
                                depth + 1, is_float)
            if found:
                found = dict(found)
                found["mask"] = value["mask"]
            return found
        if kind in ("move", "sign"):
            found = _provenance(code, base + pos, value["source"], base,
                                depth + 1, is_float)
            if found and kind == "sign" and found.get("kind") == "field":
                found = dict(found)
                found["width"] = value["width"]
                found["sign_extended"] = True
            return found
        if kind == "address":
            return None
    return None


def _attach(condition, at, code, base):
    """Attach operand provenance to a decoded condition."""
    kind = condition["kind"]
    is_float = kind in ("fcmpu", "fcmpo")
    if kind in ("cmpi", "cmpli", "extsb.", "extsh.", "rlwinm.", "andi."):
        match = re.search(r"r(\d+)", condition["text"])
        if match:
            operand = _provenance(code, at, int(match.group(1)), base)
            if operand:
                if kind in ("extsb.", "extsh.") and operand.get("kind") == "field":
                    operand = dict(operand)
                    operand["width"] = "s8" if kind == "extsb." else "s16"
                    operand["sign_extended"] = True
                if kind in ("rlwinm.", "andi."):
                    operand = dict(operand)
                    mask = re.search(r"0x([0-9A-F]{8})", condition["text"])
                    if mask:
                        operand["mask"] = f"0x{mask.group(1)}"
                condition["operand"] = operand
    elif kind in ("cmp", "cmpl"):
        condition["operands"] = {}
        for reg in re.findall(r"r(\d+)", condition["text"]):
            operand = _provenance(code, at, int(reg), base)
            if operand:
                condition["operands"][f"r{reg}"] = operand
    elif is_float:
        condition["operands"] = {}
        for reg in re.findall(r"f(\d+)", condition["text"]):
            operand = _provenance(code, at, int(reg), base, is_float=True)
            if operand:
                condition["operands"][f"f{reg}"] = operand
    return condition


def _condition(code, site, base):
    """Decode the last flag-setting compare/mask before a branch."""
    offset = site - base
    for step in range(1, 9):
        at = offset - step * 4
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
                        "text": f"f{fa} {name} f{fb}"}, base + at
        if op == 31:
            xo = (word >> 1) & 0x3FF
            ra = (word >> 16) & 31
            rs = (word >> 21) & 31
            rb = (word >> 11) & 31
            if xo == 0:
                return {"kind": "cmp", "cr": (word >> 23) & 7,
                        "text": f"r{ra} cmp r{rb}", "signed": True}, base + at
            if xo == 32:
                return {"kind": "cmpl", "cr": (word >> 23) & 7,
                        "text": f"r{ra} cmpl r{rb}", "signed": False}, base + at
            if xo == 28 and (word & 1):
                return {"kind": "and.", "text": f"(r{ra} & r{rb})"}, base + at
            if xo == 954 and (word & 1):
                return {"kind": "extsb.", "text": f"s8(r{rs})"}, base + at
            if xo == 922 and (word & 1):
                return {"kind": "extsh.", "text": f"s16(r{rs})"}, base + at
        if op == 21 and (word & 1):
            rs = (word >> 21) & 31
            sh = (word >> 11) & 31
            mb = (word >> 6) & 31
            me = (word >> 1) & 31
            return {"kind": "rlwinm.", "text": f"(r{rs} & 0x{_mask(mb, me):08X})",
                    "shift": sh}, base + at
        if op in (28, 29) and (word & 1):
            ra = (word >> 16) & 31
            rs = (word >> 21) & 31
            imm = word & 0xFFFF
            if op == 29:
                imm <<= 16
            return {"kind": "andi.", "text": f"(r{rs} & 0x{imm:08X})"}, base + at
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
        condition, source = _condition(code, site, FUNCTION)
        if source is not None:
            condition = _attach(condition, source, code, FUNCTION)
        entry = {
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
        }
        if f"{site:08X}" in GATES:
            entry["gate"], entry["meaning"] = GATES[f"{site:08X}"]
        branches.append(entry)
    if len(branches) != 98:
        raise ValueError(f"branch count changed: {len(branches)}")
    phases = {}
    for branch in branches:
        phases[branch["phase"]] = phases.get(branch["phase"], 0) + 1
    return {
        "kind": "gf-collision-branch-map",
        "revision": 2,
        "source_dol_sha256": SOURCE_SHA256,
        "function": {"address": f"{FUNCTION:08X}", "size": SIZE,
                     "branches": len(branches)},
        "pair_loop": f"{PAIR_LOOP_CALL:08X}",
        "phases": phases,
        "branches": branches,
        "notes": [
            "Each branch records the nearest flag-setting compare/mask in the "
            "eight instructions before it.",
            "`operand`/`operands` name the last load, immediate or mask that fed "
            "the tested register, following a sign extension or move. For float "
            "compares it is the last load into the compared register, not the "
            "whole expression.",
            "A 'sense' of true branches when the condition holds; false is the "
            "inverse test (for example beq vs bne).",
            "Named gates are the broad-phase and early geometry branches whose "
            "field loads are unambiguous; other handler semantics remain open.",
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
