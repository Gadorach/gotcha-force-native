"""Recover the receiver-owned contact meter and its damage tier table."""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

from tools.analyze import damage_contact_power
from tools.analyze.damage_content_row import _number
from tools.analyze.reaction_attack_table import SIZE
from tools.analyze.retail_actor_data_binding import binding_schema
from tools.analyze.retail_movement_operands import ACTOR_BYTES
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

TABLE = 0x802C7CA0
CELLS = 64
BYPASS = 0x4000
RESET_WEAR_OFFSET = 4


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    for address, size, digest in (
        (0x8002F304, 324, "5fee4534d55e382eaddb6af68f87962580504b6873cc5a48aa8c5221c55ba88f"),
        (0x80055BAC, 60, "90fadfb85ac2c77e46e3e41610abef157bef5d7ebe54768cd64180ff8638a157"),
        (0x8003D264, 48, "92759c5e78c1d1aed66162437007c13a09ca99447d968706eeda42cfa7859b5b"),
        (0x80056414, 24, "f59a629c2624b8f544d7eeb742050f44bfd7623e268e54ee72aa827138136a27"),
        (0x8002F3A8, 96, "43068666b40474edb5ce9e8204835441eb3ba8102368b70f36018cd453b8e590"),
        (0x80055AF8, 68, "08cc79d8191ef794dfdb11917c1fdd98c29b99e26009832c211d613636646adc"),
        (0x8002BC6C, 12, "76973b12c34c6a2a0e37f49e874645c8d063c6c7b95e9c72ab90d26e2ea4611e"),
        (0x8005C2B8, 40, "ab310537bf5df241c9adfcfa41bf5006ec853fa5a6f14a6ec266efeb67d61340"),
    ):
        if hashlib.sha256(image.read(address, size, "text")).hexdigest() != digest:
            raise ValueError("contact tier meter, expiry or selection changed")
    (window,) = struct.unpack(">f", image.read(0x80436FAC, 4, "data"))
    if window != 60:
        raise ValueError("contact tier expiry constant changed")
    factors = [round(value, 6) for value, in struct.iter_unpack(">f", image.read(TABLE, 4 * CELLS, "data"))]
    if any(not math.isfinite(value) or not .01 <= value <= 4 for value in factors):
        raise ValueError("contact tier factor outside native domain")
    return {"kind": "gf-damage-contact-tiers", "revision": 2, "source_dol_sha256": SOURCE_SHA256,
            "table": f"{TABLE:08X}", "factors": factors,
            "source": {"multiply": "8003D290", "flag_offset": 18, "bypass_mask": BYPASS, "charge_offset": 5,
                       "meter_offset": 1736, "tier_offset": 1738, "idle_offset": 1676,
                       "refresh": "8002F3B0", "increment": "8002F3FC..8002F440", "expiry": "80055BAC..80055BE4",
                       "threshold": 100, "cap": 63, "idle_value": window,
                       "expiry_clock": "actor+7628 subtraction, not wall time"},
            "native_idle_ticks": 60,
            "scope": "opted-in direct contacts, fixed native idle clock; no retail stagger action or scheduler",
            "reset": {
                "capacity": {"record_offset": 0, "encoding": "be-s16", "init": "80056414..80056428",
                             "actor_capacity_offset": 1726, "actor_gauge_offset": 1730,
                             "actor_data": binding_schema(image)},
                "wear": {"record_offset": RESET_WEAR_OFFSET, "encoding": "u8", "subtract": "8002F3C0..8002F3CC"},
                "gate": {"actor_flag": 1789, "bypass_mask": 0x80, "set": "8002F3F8", "clear": "8002BC74"},
                "deplete": {"site": "8002F3D0..8002F3EC", "condition": "remaining <= 0",
                            "effect": "refill gauge and clear meter and tier before applying this hit's +5 charge"},
                "refresh": {"actor_idle_offset": 1668, "site": "8002F3AC", "value": window},
                "refill": "80055AF8..80055B38; uses actor clock and a phase flag",
                "native_scope": "optional reset on accepted direct contacts; one reset per receiver per native tick, "
                                "separate native idle clock, no stagger state or action lock",
            }}


def move_tier(image, binding, *, reset=False):
    power = damage_contact_power.move_power(image, binding)
    table = int(power["table"], 16)
    entries = []
    for entry in power["entries"]:
        index = entry["index"]
        row = image.read(table + SIZE * index, SIZE, "data")
        flags, = struct.unpack_from(">H", row, 18)
        entries.append({"index": index, "base": entry["hp_base"], "charge": row[5], "scale": not (flags & BYPASS),
                        **({"reset_wear": row[RESET_WEAR_OFFSET]} if reset else {})})
    choices = {(entry["charge"], entry["scale"]) for entry in entries}
    if any(not 1 <= entry["base"] <= 10000 for entry in entries):
        charge, scale, status = None, None, "zero or out-of-range HP base"
    elif len(choices) != 1:
        charge, scale, status = None, None, "source charges or scale flags disagree"
    else:
        (charge, scale), = choices
        status = "qualified direct contact tier"
    policy = {"table": power["table"], "entries": entries, "charge": charge, "scale": scale, "status": status}
    if reset:
        wear = {entry["reset_wear"] for entry in entries}
        policy["reset_wear"] = next(iter(wear)) if charge is not None and len(wear) == 1 else None
        policy["reset_status"] = ("qualified direct contact wear" if policy["reset_wear"] is not None else
                                  "contact tier unbound" if charge is None else "source wear disagrees")
    return policy


def receiver_reset(actor, binding, *, receiver):
    if not receiver or len(actor) != ACTOR_BYTES or not binding or (
            binding["status"] != "one-static-slot-0-record" or binding["selected_record"] != 0):
        return {"capacity": None, "status": "no proved single normal receiving record"}
    digest = hashlib.sha256(actor).hexdigest()
    candidates = binding["return_candidates"]
    if not candidates or any(row.get("record_sha256") != digest or row.get("byte_offset") != 0
                             for row in candidates):
        raise ValueError("receiving tier reset record does not match installer binding")
    capacity, = struct.unpack_from(">h", actor)
    if capacity <= 0:
        return {"capacity": None, "status": "nonpositive source capacity"}
    return {"capacity": capacity, "status": "qualified single normal receiving record",
            "record_sha256": digest}


def profile_rows(report, image, bindings, *, receiver, reset=None):
    moves = {move: move_tier(image, binding, reset=reset is not None) for move, binding in sorted(bindings.items())}
    lines = []
    if receiver:
        factors = "|".join(_number(value) for value in report["factors"])
        lines.append(f"damage-tier-receive={report['native_idle_ticks']}|{factors}")
        if reset is not None and reset["capacity"] is not None:
            lines.append(f"damage-tier-reset={reset['capacity']}")
    for move, policy in moves.items():
        if policy["charge"] is not None:
            lines.append(f"contact-tier={move}|{policy['charge']}|{'scale' if policy['scale'] else 'bypass'}")
            if policy.get("reset_wear") is not None:
                lines.append(f"contact-tier-reset={move}|{policy['reset_wear']}")
    return {"receiver": receiver, "moves": moves, "lines": lines,
            **({"reset_receiver": reset} if reset is not None else {})}


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact tier output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        result = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(result, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    export(args.dol, args.output)
