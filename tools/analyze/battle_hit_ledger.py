"""Publish numerical field and ring provenance of the shared knockout ledger."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

FUNCTION = 0x8002F8DC
TEXT_SIZE = 828
TEXT_SHA256 = "a43da7a452462e71cf27cdfe9965001b9dfb306fdc5c693d6df8f4d2df14bb00"


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    text = image.read(FUNCTION, TEXT_SIZE, "text")
    if sha256(text).hexdigest() != TEXT_SHA256:
        raise ValueError("hit ledger source path changed")

    def operand(site):
        return struct.unpack_from(">I", text, site - FUNCTION)[0] & 0xFFFF

    return {
        "kind": "gf-battle-hit-ledger", "revision": 2, "source_dol_sha256": SOURCE_SHA256,
        "function": f"{FUNCTION:08X}",
        "admission": {"receiver_kind_offset": operand(0x8002F8DC), "required_kind": 0},
        "amount": {"recipient_cache_offset": operand(0x8002F8F0), "read": "8002F8F0", "type": "s16",
                   "note": "uses the recipient's initialized Force cost, not the HP request in incoming r5"},
        "recipient_totals": {"count_offset": operand(0x8002FAAC), "amount_offset": operand(0x8002FAB8)},
        "credit": {"count_offset": operand(0x8002FAEC), "amount_offset": operand(0x8002FAF8),
                   "condition": "attacker kind 0 and attacker player slot differs from recipient player slot",
                   "player_tally_increment": operand(0x8002FB0C)},
        "observers": {
            "actor_array_pointer": "803C4E84",
            "slots": operand(0x8002FB18), "stride": operand(0x8002FC0C),
            "active_byte_offset": operand(0x8002FB28), "active_test": "nonzero; not a health check",
            "group_offset": operand(0x8002FB38), "group_semantics": "player slot, not portable team",
            "recipient_slot_offset": operand(0x8002FB70), "recipient_slot_type": "sign-extended byte stored as halfword",
            "exclude_recipient": True,
            "history_offset": operand(0x8002FB50), "cursor_offset": operand(0x8002FB34),
            "records": 8, "record_bytes": 4, "fields": ["u16 relation", "i16 recipient slot"],
            "cursor_rule": "increment modulo eight",
            "different_player": {"relation": operand(0x8002FBC8), "write": "8002FBCC"},
            "same_player": {"relation": operand(0x8002FB68), "write": "8002FB6C",
                            "count_offset": operand(0x8002FBB4), "amount_offset": operand(0x8002FBC0)},
        },
        "native_analogue": "authored accepted-hit history, not the source knockout ledger; "
                           "native teams replace source player slots for observer relation",
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("hit ledger output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        report = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(report, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    export(args.dol, args.output)
