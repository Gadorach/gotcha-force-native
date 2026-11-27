"""Publish verified projectile factory -> queue -> reaction operands (GG4E08)."""
import argparse
import json
import struct
from pathlib import Path

from tools.analyze import reaction_emit, reaction_installer_tables
from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_worm_shot import MODELS as WORMS, FAMILIES as WORM_FAMILIES
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage, ReadOnlyFile


# Reviewed ordinary-B release sites, independently joined to their model by
# the roster's release proofs. Pin the exact calls and operands here too.
PILOTS = {
    0x000A: ("type-14", 4, 0x800705AC, 0x8006EE14),
    0x0615: ("type-2", 43, 0x8018DD34, 0x800C3BE0),
    0x061E: ("type-2", 85, 0x8015AAE4, 0x800C3BE0),
    0x0800: ("type-0", 12, 0x80076C10, 0x80082824),
    0x0801: ("type-0", 125, 0x800A403C, 0x80082824),
    0x0802: ("type-0", 45, 0x8013D77C, 0x80082824),
    **{model: (f"type-{family}", key, call, WORM_FAMILIES[family][4])
       for model, (family, key, call) in WORMS.items()},
}


def word(image, address, kind="text"):
    return struct.unpack(">I", image.read(address, 4, kind))[0]


def selector_code(image, family):
    offset, read_site, call_site, dispatch = reaction_emit.SELECTORS[family]
    base, stride, count = reaction_emit.FACTORY_ROWS[family]
    load = word(image, read_site)
    call = word(image, call_site)
    initializer = word(image, dispatch, "data")
    if (load & 0xFFE0FFFF != (0x88800000 | offset) or
            call & 0xFC000003 != 0x48000001 or direct_branch(call_site, call) != 0x8008AC80 or
            not initializer <= read_site < call_site <= initializer + 0x400):
        raise ValueError(f"{family}: projectile queue selector proof changed")
    return {"factory_table": f"{base:08X}", "stride": stride, "reviewed_key_count": count,
            "selector_offset": offset, "initializer": f"{initializer:08X}",
            "dispatch": f"{dispatch:08X}", "read_site": f"{read_site:08X}", "allocator_call": f"{call_site:08X}"}


def recover(image, archive):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    selectors = {family: selector_code(image, family) for family in reaction_emit.SELECTORS}
    models = {}
    for model, (family, key, site, factory) in PILOTS.items():
        call = word(image, site)
        if (word(image, site - 4) != (0x38800000 | key) or
                call & 0xFC000003 != 0x48000001 or direct_branch(site, call) != factory):
            raise ValueError(f"{model:04x}: reviewed release proof changed")
        _, table = reaction_installer_tables.model_table(image, model)
        payload = reaction_emit.qualified_hit_block(archive, model)
        binding = (reaction_emit.move_binding(image, payload, table, "basic-shot", [(family, key)])
                   if table is not None else None)
        if binding is None:
            raise ValueError(f"{model:04x}: no ordinary B reaction binding")
        models[f"{model:04x}"] = {"release_site": f"{site:08X}", "factory": f"{factory:08X}", **binding}
    return {"kind": "gf-reaction-projectile-map", "revision": 2,
            "source_dol_sha256": SOURCE_SHA256, "selectors": selectors, "pilots": models,
            "scope": "ordinary allocation, normal hit block; static source operands only",
            "notes": [
                "The factory key is independent of controller input-context coordinates.",
                "8008AC80 reads the owner hit block and caches the low byte of step +4 as the reaction table index.",
                "One native move receives a binding only if all reviewed launches and all queued steps agree on mode and signed offsets.",
                "Reviewed type-0/49/94 mode-4 contacts use the normal fighter link that the pair caches at allocation, not the carrier's aim or motion.",
            ]}


def export(dol, afs, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("reaction projectile map output already exists")
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
