"""Recover the ordinary body-local contact timer and emit an authored pose hold."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import contact_sound, reaction_melee
from tools.analyze.reaction_attack_table import SIZE
from tools.analyze.retail_movement_operands import ACTOR_BYTES
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

MAX_REVIEWED_SEED = 8
COUNTDOWN_OFFSET = 8
RECEIVER_SKIP_FLAGS = 0x0FC0
WITNESSES = (
    (0x8002E2C8, 36, "875b3664413b6e1cc3c99e9cbf9e17e663983421520e8999482f1d4f9a55253d"),
    (0x8002E604, 40, "f6ca43bb41064dc7e3489f44ab8897c5282ebd9b9a1f0a77b2f417a3eec2b557"),
    (0x8002E660, 76, "71437c8a1e1bb8a2fcc9c0e1116bd224a1d4ebc1f9a23ef8c02294edf0aa1fa0"),
    (0x8005A50C, 116, "e66cab9e76f20313b3728b2f0dacd7d096236fb85c91cc15ca4eb0bb82a114a1"),
    (0x8004C6B4, 64, "be0f863b7e6f613d30c45c45496d0db8c4c4c1bde41a027e4437fc97170be169"),
    (0x8008AC80, 172, "fd813d58dd70c04d8355e8c88909eadfc4731a0fcd000028ca20859e08bac1eb"),
)


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    if any(sha256(image.read(site, size, "text")).hexdigest() != digest for site, size, digest in WITNESSES):
        raise ValueError("contact timer operands or activity gate changed")
    slowdown, = struct.unpack(">f", image.read(0x804373DC, 4, "data"))
    if round(slowdown, 6) != .03:
        raise ValueError("contact timer slowdown constant changed")
    return {"kind": "gf-contact-pose-pause", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
            "source": {"attack_record_offset": COUNTDOWN_OFFSET,
                       "body_timer_offset": 1820, "combine": "max(current, u8(record+8))",
                       "attacking_object": {"pair_offset": 32, "kind_offset": 131, "kind": 0,
                                            "write": "8002E628", "does_not_follow_credit_link": True},
                       "receiving_object": {"pair_offset": 32, "kind_offset": 131, "kind": 0,
                                            "write": "8002E6A8", "record_flags_offset": 18,
                                            "skip_mask": RECEIVER_SKIP_FLAGS},
                       "melee_body": {"opcode_handler": "8004C6B4", "allocator_call": "8004C6F0",
                                      "carrier": "opcode-10 executing actor"},
                       "decrement": "8005A520",
                       "slowdown": {"constant": "804373DC", "factor": round(slowdown, 6),
                                    "writes": [1524, 7624], "condition": "timer still positive after decrement"}},
            "native": {"scope": "held skeletal presentation sample only; no change to simulation or live bone/socket ownership",
                       "formula": "max(0, source seed - 1) whole native ticks; starts at accepted hit presentation",
                       "receiver_only": "ordinary direct projectile contact",
                       "both_bodies": "proved opcode-10 direct melee, with each normal native participant opted in",
                       "max_reviewed_seed": MAX_REVIEWED_SEED}}


def move_hold(image, binding):
    table = int(binding["table"], 16)
    rows = contact_sound.contact_records(image, binding)
    for row in rows:
        row["seed"] = image.read(table + row["index"] * SIZE + COUNTDOWN_OFFSET, 1, "data")[0]
    refusal = contact_sound.ordinary_refusal(rows)
    scope = "bodies" if "contacts" in binding else "receiver"
    if not refusal and scope == "bodies" and binding.get("allocator_call") != f"{reaction_melee.ALLOCATOR_CALL:08X}":
        refusal = "unproved body carrier"
    if not refusal and any(row["flags"][1] & RECEIVER_SKIP_FLAGS for row in rows):
        refusal = "source receiver timer bypass"
    seeds = {row["seed"] for row in rows}
    if not refusal and len(seeds) != 1:
        refusal = "source timer seeds disagree"
    held_ticks = None
    if not refusal:
        seed, = seeds
        if seed > MAX_REVIEWED_SEED:
            refusal = "source seed outside reviewed native scope"
        elif seed <= 1:
            refusal = "no slow tick after decrement"
        else:
            held_ticks = seed - 1
    return {"table": binding["table"], "entries": rows, "held_ticks": held_ticks,
            "scope": scope if held_ticks else None, "status": refusal or "qualified local pose hold"}


def profile_rows(image, moves, record_binding, actor_size, *, native):
    receiver = bool(native and actor_size == ACTOR_BYTES and record_binding and
                    record_binding["status"] == "one-static-slot-0-record" and record_binding["selected_record"] == 0)
    lines = ["contact-pose-hold-receive=normal"] if receiver else []
    report = {move: move_hold(image, binding) for move, binding in sorted(moves.items())} if native else {}
    for move, row in report.items():
        if row["held_ticks"]:
            lines.append(f"contact-pose-hold={move}|{row['held_ticks']}|{row['scope']}")
    return {"receiver": receiver, "moves": report, "lines": lines}


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact pose pause output already exists")
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
