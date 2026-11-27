"""Recover the HP base operand of qualified direct contact routes."""
import argparse
import json
import struct
from pathlib import Path

from tools.analyze.reaction_attack_table import COUNT, SIZE
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile


def verify_source(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    # The attacker pair picks a 24-byte record with pair+0x11. At the
    # amount call it passes that record (not its +2 operand) in r3.
    for address, word in {
        0x8002E2C8: 0x88830011, 0x8002E2D0: 0x1CA40018, 0x8002E2D4: 0x80D4002C,
        0x8002E2EC: 0x7FA62A14, 0x8002E3C4: 0xA09D0002, 0x8002F0FC: 0x7FA3EB78,
        0x8002F108: 0x4800DC55, 0x8003CD88: 0xA0630000, 0x8003CDB4: 0x38600000,
    }.items():
        if image.read(address, 4, "text") != struct.pack(">I", word):
            raise ValueError("HP contact-power operand changed")


def move_power(image, binding):
    """One native move consumes a base only when all its source rows agree."""
    routes = binding.get("contacts", binding.get("releases", ()))
    if not 1 <= len(routes) <= 256 or any(not 1 <= len(r["steps"]) <= 4 for r in routes):
        raise ValueError("contact-power route extent")
    indices = sorted({step["index"] for route in routes for step in route["steps"]})
    table = int(binding["table"], 16)
    entries = []
    for index in indices:
        if not 0 <= index < COUNT:
            raise ValueError("contact-power index outside verified budget")
        power, separate = struct.unpack(">2H", image.read(table + index * SIZE, 4, "data"))
        entries.append({"index": index, "hp_base": power, "separate_contact_operand": separate})
    values = {row["hp_base"] for row in entries}
    if len(values) != 1:
        status, value = "source steps disagree; authored base retained", None
    else:
        value, = values
        if not 1 <= value <= 10000:
            status, value = "source base is zero or outside native bounds; authored base retained", None
        else:
            status = "uniform positive HP base"
    return {"table": binding["table"], "entries": entries, "base": value, "status": status}


def append(gameplay, receipt, image):
    bindings = receipt.get("reaction", {})
    if not bindings:
        return gameplay
    powers = {move: move_power(image, binding) for move, binding in sorted(bindings.items())}
    for move, result in powers.items():
        if result["base"] is not None:
            gameplay += (f"# Recovered HP base; this Battle Test still uses authored contact geometry.\n"
                         f"contact-power={move}|{result['base']}\n").encode("ascii")
    receipt["contact_power_test"] = powers
    return gameplay


def append_floor(gameplay, receipt, image, *, minimum):
    if minimum != 1:
        raise ValueError("unsupported contact minimum")
    moves = {}
    for move, binding in sorted(receipt.get("reaction", {}).items()):
        report = move_power(image, binding)
        bases = [entry["hp_base"] for entry in report["entries"]]
        eligible = all(1 <= base <= 10000 for base in bases)
        moves[move] = {"table": report["table"], "source_bases": bases,
                       "minimum": minimum if eligible else None,
                       "status": "qualified positive direct contact" if eligible else "zero or out-of-range source base"}
        if eligible:
            gameplay += f"contact-minimum={move}|{minimum}\n".encode("ascii")
    receipt["contact_floor_test"] = moves
    return gameplay


def recover(image, archive):
    # The public sample uses the same independently qualified queue joins
    # as package conversion; importing these tools here avoids a roster cycle.
    from tools.analyze import reaction_melee_map, reaction_projectile_map

    verify_source(image)
    shots = reaction_projectile_map.recover(image, archive)["pilots"]
    melee = reaction_melee_map.recover(image, archive)["pilots"]
    return {"kind": "gf-damage-contact-power", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
            "source": {"amount_call": "8002F108", "base_load": "8003CD88", "base_offset": 0,
                       "separate_operand_load": "8002E3C4", "record_size": SIZE},
            "scope": "raw positive base only; no claim about original landed shape, scaling or effect",
            "projectiles": {model: move_power(image, binding) for model, binding in shots.items()},
            "close_b": {model: move_power(image, row["binding"]) for model, row in melee.items()
                        if row.get("binding")}}


def export(dol, afs, output):
    from tools.extract.retail_borg_roster import RetailArchive

    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact power output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
    with Path(afs).open("rb") as stream:
        result = recover(image, RetailArchive(stream))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(result, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("afs", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    export(args.dol, args.afs, args.output)
