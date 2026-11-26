"""Export the contact scalar ladder selected by the per-team settings byte.

The D100 factor of the amount function is one cell of the 32-entry defender
class row at `0x802C6820`, reached from root `0x80433600[side][model+156]`.
The index is the per-team byte at `0x803C4E60 + actor+136`. It is a battle/team
value, not a Borg value: the same row reads index 16 (1.13) in the Kakeru
savestate and index 30 (1.4) in the Mana savestate.

This is numeric API data, not model, texture or audio bytes.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

LADDER = 0x802C6820
COUNT = 32
SETTINGS = 0x803C4E60
OBSERVATIONS = (
    {"state": "kakeru_battle", "team": 1, "byte": 16, "scalar": 1.13,
     "credit_variant": 0, "credit_difficulty": 1.02, "credit_rank": 0.85,
     "conditional_half": False,
     "verified": True, "hit": {"credit": "80365EC0", "receiver_model": 10,
                               "base": 20, "amount": 22}},
    {"state": "mana_combat_hud", "team": 1, "byte": 30, "scalar": 1.4,
     "credit_variant": 1, "credit_difficulty": 0.92, "credit_rank": 0.85,
     "conditional_half": True,
     "verified": True, "hit": {"credit": "802D4160", "receiver_model": 261,
                               "base": 5, "amount": 3,
                               "carrier": "8064AAB8", "carrier_is_actor": False}},
)


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    entries = list(struct.unpack(
        ">32f", image.read(LADDER, 4 * COUNT, "data")))
    if abs(entries[0] - 0.97) > 1e-6 or abs(entries[16] - 1.13) > 1e-6:
        raise ValueError(f"ladder endpoints changed: {entries[0]}, {entries[16]}")
    if abs(entries[30] - 1.4) > 1e-6 or abs(entries[31] - 2.0) > 1e-6:
        raise ValueError(f"ladder tail changed: {entries[30]}, {entries[31]}")
    for observation in OBSERVATIONS:
        if abs(entries[observation["byte"]] - observation["scalar"]) > 1e-6:
            raise ValueError(f"observation mismatch: {observation}")
    return {
        "kind": "gf-contact-scalar-ladder",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "address": f"{LADDER:08X}",
        "count": COUNT,
        "entries": [round(value, 6) for value in entries],
        "formula": "trunc(base * credit_difficulty(side, settings[attacker+136]) "
                   "* attacker_rank(attacker+1082) "
                   "* (owner998 != 0 && owner136 == 0 ? 0.5 : 1) "
                   "* defender_class_row(model+156, settings[defender+136]) "
                   "* receiver_rank(defender+1082) * tier * relative)",
        "credit_rows": {
            "pointer_table": "804335E0",
            "variants": [
                {"variant": 0, "difficulty": 1.02, "rank": 0.85},
                {"variant": 1, "difficulty": 0.92, "rank": 0.85},
            ],
            "note": "804335E0 is the credit-difficulty pair. The 0.85 attacker rank is the separate pair at 804335F8, element 0.",
        },
        "settings": {
            "address": f"{SETTINGS:08X}",
            "index_expression": "byte at settings + actor+136",
            "observations": list(OBSERVATIONS),
        },
        "class_row": {
            "root": "80433600",
            "row": "model+156 = 1 -> 802C6820",
            "role": "defender class row applied at D100",
        },
        "notes": [
            "The row is a battle/team value, not a Borg value.",
            "Index 16 was captured on the Kakeru hit and index 30 on the Mana hit; both select the defender class row applied at D100.",
            "The Mana hit also applies the conditional 0.5: credit difficulty 0.92 x rank 0.85 x 0.5 = 0.391; the Kakeru hit does not (owner998 = 0).",
            "The credit difficulty pair at 804335E0 is [1.02, 0.92]; the 0.85 attacker rank is the separate pair at 804335F8.",
            "The pair entry carries each side's credit record at object +0x2C; the Mana record 802D4160 appears on the other-side object, so it is not a G-Red shot.",
            "The raw GFDA beam path does not use this ladder; it applies the health/force/difficulty rows.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact scalar output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output)))


if __name__ == "__main__":
    main()
