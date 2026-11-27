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
        "sound_id": 38,
        "state": "object+0x2AE = 4",
        "size": "receiver+0x7FC",
        "thresholds": {"80436FB8": 1.5, "80436F94": 2.5},
        "size_branch": "receiver+0x83 gates the read; the matched source calls "
                         "800F04B4(38, position, s8(receiver+0x96)) in every branch",
        "role": "fixed impact sound",
    },
    "8002FC18": {
        "dispatch": "800F088C",
        "selector": "when bit 0x80 is set, value & 0xFFFFFF7C plus one per exceeded receiver size threshold",
        "state": "object+0x2AE = 4",
        "thresholds": {"80436FB8": 1.5, "80436F94": 2.5, "80436FBC": 3.5},
        "role": "indexed impact sound",
    },
    "800F088C": {
        "role": "impact sound selection",
        "sound_table": "8031B874",
        "counter_row": "803D3AE8 + s8(object+0x96)*13",
        "slot": "code & ~0xC0, +12 when code&0x40",
        "alternate": "value += row[slot] & 1 before incrementing the row counter",
    },
    "8002F8DC": {
        "role": "force/ledger update",
        "details": "battle-hit-ledger.json",
        "fields": ["actor+0x4AA", "actor+0x424", "attacker+0x420",
                   "team+0x118", "ring +0x464/+0x484"],
    },
    "80019550": {"role": "impact visual descriptor", "table": "802C7ED0",
                 "selector": "attack record byte +9; family, variant and allocation code, not RGB"},
    "8013F300": {"role": "object spawn", "allocator": "80088AA0"},
    "80176EC0": {"role": "object spawn", "allocator": "80088AA0"},
    "8010E158": {
        "role": "effect-spawn",
        "allocator": "80088AA0(owner, 2, 0x18, 0, entry[0])",
        "object": "type=1, +0x10=0x15, +0x11=index, +0x12=entry[1]",
        "callbacks": ["8010E2A8@+0x0C", "8010E80C@+0x10C"],
        "fields": "+0x1C entry+2, +0x54 entry+4, +0x8C owner",
        "owner_bit": "owner+0x8C4 |= 1<<index when entry[1]==1",
    },
    "801EC1BC": {"role": "reaction effect"},
    "801DFF24": {"role": "reaction effect"},
    "800DB250": {
        "role": "effect-values",
        "allocator": "8008893C(2, 0x18, 1, 8031227C[index*2])",
        "object": "type=1, +0x10=0x0F, +0x11=index, +0x13=definition",
        "callbacks": ["800DB378@+0x0C", "800DB838@+0x10C"],
        "values": "values[0..2] -> +0x20/+0x24/+0x28 (0x803B0720 vector)",
        "owner": "+0x8C = src",
    },
    "80006C24": {"role": "slot update"},
    "8018214C": {
        "role": "reaction-id",
        "behavior": "mode==0 lookup in 80436354+0x1518 by (arg1<<28)|(arg0<<16); "
                    "random 1..16 delta; returns 0..15",
    },
    "800300BC": {
        "role": "reaction-direction",
        "modes": {
            "0": "attacking object's current - previous origin (+0x20 - +0x2C); zero uses object +0x38",
            "1": "receiver object +0x20 - attacker object +0x20",
            "2": "pair's current - previous transformed shape centre (+0x30 - +0x3C); zero uses object +0x38",
            "3": "receiver pair's current shape centre +0x64 - attacker pair's +0x64",
            "4": "third matrix column at +0x8DC/8EC/8FC on the attacking object or its link; "
                 "if both are non-body, use the attacking object's matrix column at +0x11C/12C/13C",
            "other": "branches to 80030238 without initializing the local vector; no supported direction inferred",
        },
        "yaw": "atan2(x,z)",
        "pitch": "-atan2(y,x*x+z*z)",
        "fallback": "(0,0,-1) when x*x+y*y+z*z < 0.01",
        "writes": "receiver+0x284 yaw, receiver+0x282 pitch; mirrored to linked actor when +0x1DA bit1",
        "table_offsets": "signed table+0x14/+0x15 * 0x100 subtracted; row selected by attack pair+0x11",
    },
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


CONSTANT_ADDRS = (
    "80436F68", "80436F74", "80436F94",
    "80436FB8", "80436FBC", "80436FC0",
)
VALUE_TABLE = 0x8031227C


def _constants(image):
    return {addr: struct.unpack(">f", image.read(int(addr, 16), 4, "data"))[0]
            for addr in CONSTANT_ADDRS}


def _value_definitions(image):
    raw = image.read(VALUE_TABLE, 9 * 8, "data")
    entries = []
    for index in range(9):
        definition, pointer = struct.unpack_from(">II", raw, index * 8)
        entries.append({"index": index, "definition": definition,
                        "pointer": f"{pointer:08X}"})
    return entries


def _body_root_producer(image):
    # The qualified body pose path builds an actor world matrix at +0x8D4
    # before dispatching child joints, and copies the same root to +0x114.
    sites = {
        0x80046BF8: 0x8020AEFC,  # identity
        0x80046C10: 0x8020B4AC,  # translation
        0x80046C4C: 0x80045838,  # status-adjusted rotation
        0x80046C68: 0x80045838,  # ordinary rotation
        0x80046C7C: 0x8020B4F8,  # scale matrix
        0x80046C8C: 0x8020AF5C,  # concatenate
        0x80046C98: 0x8020AF28,  # copy
        0x80046CCC: 0x8004679C,  # child joint pass, ordinary arm
    }
    for site, target in sites.items():
        if decode_calls(image.read(site, 4, "text"), site) != [
                {"site": f"{site:08X}", "target": f"{target:08X}"}]:
            raise ValueError("mode-4 body matrix call chain changed")
    for site, op, offset in ((0x80046BF0, 14, 0x8D4), (0x80046C94, 14, 0x114),
                             (0x80046BFC, 48, 0x20), (0x80046C04, 48, 0x24), (0x80046C0C, 48, 0x28),
                             (0x80046C54, 42, 0x70), (0x80046C5C, 42, 0x72), (0x80046C64, 42, 0x74),
                             (0x80046C6C, 48, 0x58), (0x80046C74, 48, 0x5C), (0x80046C78, 48, 0x60),
                             (0x80045874, 14, ord("z")), (0x800458B0, 14, ord("y")),
                             (0x800458E4, 14, ord("x")), (0x800301D8, 48, 0x8DC),
                             (0x800301E0, 48, 0x8EC), (0x800301E8, 48, 0x8FC)):
        word, = struct.unpack(">I", image.read(site, 4, "text"))
        if word >> 26 != op or word & 0xFFFF != offset:
            raise ValueError("mode-4 body matrix operand changed")
    return {
        "producer": "80046BD0", "matrix_offset": 0x8D4, "copy_offset": 0x114,
        "position_offsets": [0x20, 0x24, 0x28], "binary_angle_offsets": [0x70, 0x72, 0x74],
        "scale_offsets": [0x58, 0x5C, 0x60], "rotation_order": ["Z", "Y", "X"],
        "read_sites": ["800301D8", "800301E0", "800301E8"],
        "positive_z_offsets": [0x8DC, 0x8EC, 0x8FC],
        "guarded_yaw_adjustment": "80046C14..80046C4C can add timer at +0x6B8 shifted left by 10",
        "native_scope": "actor-bound melee uses the native planar collision-root +Z column",
        "calls": {f"{site:08X}": f"{target:08X}" for site, target in sites.items()},
    }


def _motion_inputs(image):
    calls = {
        0x80030128: 0x8020BA78, 0x80030180: 0x8020BA78,
        0x80030130: 0x8020BB14, 0x80030188: 0x8020BB14,
        0x8008A728: 0x8020B7A8, 0x8008A888: 0x8020B7A8,
        0x80083B6C: 0x8020BA54, 0x80083B80: 0x8020BA54,
    }
    for site, target in calls.items():
        if decode_calls(image.read(site, 4, "text"), site) != [
                {"site": f"{site:08X}", "target": f"{target:08X}"}]:
            raise ValueError("reaction motion call chain changed")
    for site, op, offset in (
        (0x80089910, 32, 0x20), (0x8008991C, 36, 0x2C),
        (0x80089920, 36, 0x30), (0x80089928, 36, 0x34),
        (0x80089A1C, 32, 0x20), (0x80089A28, 36, 0x2C),
        (0x80056874, 32, 0x20), (0x8005687C, 36, 0x2C),
        (0x8008A720, 14, 0x0C), (0x8008A724, 14, 0x30),
        (0x8008A734, 36, 0x3C), (0x8008A738, 36, 0x40), (0x8008A740, 36, 0x44),
        (0x8008A864, 32, 0x30), (0x8008A874, 36, 0x3C),
        (0x8008A878, 36, 0x40), (0x8008A880, 36, 0x44), (0x8008A884, 32, 0x60),
        (0x8003011C, 14, 0x20), (0x80030120, 14, 0x2C), (0x80030144, 32, 0x38),
        (0x80030174, 14, 0x30), (0x80030178, 14, 0x3C), (0x8003019C, 32, 0x38),
        (0x80083B74, 14, 0x20), (0x80083B78, 14, 0x38),
    ):
        word, = struct.unpack(">I", image.read(site, 4, "text"))
        if word >> 26 != op or word & 0xFFFF != offset:
            raise ValueError("reaction motion operand changed")
    return {
        "object": {"current_offset": 0x20, "previous_offset": 0x2C,
                   "snapshot_sites": ["80089910", "80089A1C", "80056874"],
                   "vector_offset": 0x38, "ordinary_vector_add": "80083B80"},
        "pair": {"current_offset": 0x30, "previous_offset": 0x3C,
                 "definition_point_offset": 0x0C, "selected_matrix_pointer_offset": 0x60,
                 "initial_point": "8008A728", "initial_equal_copy": "8008A734",
                 "roll_previous": "8008A864", "update_point": "8008A888"},
        "selection": {"mode_0_subtract": "80030128", "mode_2_subtract": "80030180",
                      "object_vector_only_on_zero": True, "then_apply_direction_epsilon": "80436FC0"},
        "native_scope": "main-move free-flight direct actor hits; clipped object motion vs full proxy sweep",
    }


def _position_inputs(image):
    calls = {
        0x8003016C: 0x8020BA78, 0x800301C4: 0x8020BA78,
        0x8003A4E0: 0x8020B7A8, 0x8003A634: 0x8020B7A8,
        0x80039FC4: 0x8020BC88, 0x8002DE90: 0x8020BA54, 0x8002DEB0: 0x8020BA54,
    }
    for site, target in calls.items():
        if decode_calls(image.read(site, 4, "text"), site) != [
                {"site": f"{site:08X}", "target": f"{target:08X}"}]:
            raise ValueError("reaction centre call chain changed")
    for site, op, offset in (
        (0x800300EC, 32, 0x20), (0x800300F0, 32, 0x20),
        (0x80030160, 14, 0x20), (0x80030164, 14, 0x20),
        (0x800301B8, 14, 0x64), (0x800301BC, 14, 0x64),
        (0x8003A4CC, 14, 0x64), (0x8003A4D8, 32, 0x60), (0x8003A4DC, 14, 0x0C),
        (0x8003A608, 14, 0x64), (0x8003A618, 36, 0x70),
        (0x8003A614, 14, 0x0C), (0x8003A630, 32, 0x60),
        (0x8002DE84, 14, 0x20), (0x8002DEA4, 14, 0x64),
    ):
        word, = struct.unpack(">I", image.read(site, 4, "text"))
        if word >> 26 != op or word & 0xFFFF != offset:
            raise ValueError("reaction centre operand changed")
    return {
        "mode_1": {"pair_object_pointer_offset": 0x20, "origin_offset": 0x20,
                   "subtract": "8003016C", "uses_owner_link": False},
        "mode_3": {"pair_centre_offset": 0x64, "subtract": "800301C4",
                   "ordinary_sphere_centre_seed": "8003A4E0", "ordinary_sphere_centre_update": "8003A634",
                   "local_point_offset": 0x0C, "matrix_pointer_offset": 0x60,
                   "centre_distance_gate": "80039FC4"},
        "nearest_contact_correction": {"object_add": "8002DE90", "pair_centre_add": "8002DEB0"},
        "native_scope": "explicit mode 1 can read the direct projectile origin; mode 3 uses accepted native proxy centres",
    }


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
        "revision": 4,
        "source_dol_sha256": SOURCE_SHA256,
        "function": {"address": f"{FUNCTION:08X}", "size": SIZE,
                     "calls": len(mapped)},
        "effects": {key: value for key, value in sorted(EFFECTS.items())},
        "constants": _constants(image),
        "mode_4_body_root": _body_root_producer(image),
        "motion_inputs": _motion_inputs(image),
        "position_inputs": _position_inputs(image),
        "effect_values": {
            "table": f"{VALUE_TABLE:08X}",
            "entries": _value_definitions(image),
        },
        "calls": mapped,
        "notes": [
            "The argument map is the last write to r3-r7 in the 12-instruction "
            "window before each call.",
            "`constants` holds the float thresholds and fallback operands read "
            "from the initialized data section.",
            "`effect_values` maps the nine 800DB250 indices to their definition "
            "ids and definition pointers; indices 0 and 8 share definition 2.",
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
