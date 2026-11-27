"""Replay type-89 first casts through shape admission and attack registration."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables
from tools.analyze import retail_type89 as type89, retail_type89_cast as cast, retail_type89_roster as roster
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


SOURCES = {
    0x0503: (0x802FEF08, 20, "f9a5873a18409ae72f784668d5897b1545ea881316a9b4875a56e10ad412365c"),
    0x0506: (0x802FEF08, 20, "d3c335c3a453b257896c960d223d892695bfc3442e526e9455361f18f69d03d1"),
    0x050F: (0x802FEFB0, 10, "330650654c5168a5dbfa9d04388648c2021646aefca24524b0c31aa243d33e2c"),
    0x0512: (0x802FEFB0, 10, "172b8b71a627f1f0ae3974d3d8ae50a6074c10e66cbd149079cdf1cdadde0421"),
}
CASES = (("no-shapes", 2, 2, 0), ("one-shape", 2, 2, 1), ("two-shapes", 2, 2, 2),
         ("one-carrier", 1, 2, 1), ("one-descriptor", 2, 1, 1))
ROOT = {3: [1000, 200, 300], 9: [800, 600, 300], 10: [1200, 600, 300]}
ROW = re.compile(r"type89-opening:(?: [0-9]{1,4}){11}")
TEXT = (
    (0x80089A94, 88, "dcb8bf7edce7313e0e258ee526af82153e5e35eac0987938a42cccc6a270dde4"),
    (0x80089554, 260, "14d54d2183c44456974e44418019670cfd857a373e5baced583924c4c74280df"),
    (0x8008A28C, 48, "94cb040d0d0bd380e6ab2d81aec5d82583aaf212f52408a7050212f7e39acc08"),
    (0x80089AEC, 276, "03e4a562cd8742937f8f85f602f0f3a3a682457f8f5887accc0b15cbf24f4802"),
    (0x8008A2BC, 788, "2c811080af75624f6664c00f6e11b229fdb83bea44b41482b53b2ed14d2f0528"),
    (0x8008AC80, 400, "88d16df3bda187ed3cb011269a356a7d0646ca1c946dcd194664a5abf4a317d0"),
    (0x8016D398, 276, "b9e909c3b6b15fca848af776faefdc32ced59da060ed2bdbc984dcd7ee81c3cc"),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(cast.MODELS)*len(CASES):
        raise ValueError("type-89 opening report extent")
    observations = []
    for pos, line in enumerate(lines):
        model = cast.MODELS[pos//len(CASES)]
        case = pos % len(CASES)
        name, carriers, descriptors, shapes = CASES[case]
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed type-89 opening row")
        observed = tuple(map(int, line.split()[1:]))
        requested = [9, 10] if model in (0x0503, 0x050F) else [3]
        allocated = requested[:min(carriers, descriptors)]
        attempted = allocated[::-1]
        bound = attempted[:shapes]
        registered = bound[::-1]
        if observed != (model, case, len(requested), len(allocated), *(attempted+[0, 0])[:2],
                        len(bound), 0, 32 if bound else 0, *(registered+[0, 0])[:2]):
            raise ValueError(f"type-89 bind, activation or registry mismatch: {model:04X}/{name}")
        observations.append({
            "borg": f"gg4e08:{model:04x}", "case": name,
            "inlet": {"carriers": carriers, "descriptors": descriptors, "shapes": shapes},
            "request_order": requested, "carrier_allocation_order": allocated,
            "bind_attempt_order": attempted, "bound": [{"key": k, "opening_sphere_center": ROOT[k]} for k in bound],
            "opening_register_count": 0, "attack_registry": {"bucket_visit": 32, "keys": registered} if bound else None,
            "shape_slots_returned_at_bucket_visit": 33 if bound else None,
        })
    return observations


def qualified_inlet(image, archive):
    type89.verify_text(image, TEXT+type89.TIMED_TEXT)
    type89.verify_text(image, tuple(span for span in roster.TEXT if span[0] == cast.ENTRY))
    type89.link(image, 0x8016D2F0, 0x8008AFF0)
    if any(type89.word(image, at) != word for at, word in ((0x8016CE34, 0xB07C0272), (0x8016D0FC, 0xB01B0272),
                                                          (0x8008A448, 0xA8050272), (0x8008A450, 0x4082002C))):
        raise ValueError("type-89 registration exclusion mask changed")
    rows = roster.factory_rows(image)
    selector = roster.indexed_selectors(image)
    if ([bank["ordered_keys_by_cursor"][0] for bank in selector["banks"]] != [[9, 10], [3]]
            or any(rows[key]["group"] != 3 or rows[key]["phase_duration_units"] != [25, 6, 10] for key in ROOT)):
        raise ValueError("type-89 opening selector or timer changed")
    payload, sources = [], []
    for model in cast.MODELS:
        table, power, digest = SOURCES[model]
        block = reaction_emit.qualified_hit_block(archive, model)
        step = reaction_emit.STEPS+3*reaction_emit.STEP_SIZE
        if (sha256(block).hexdigest() != digest or reaction_emit.read_group(block, 3) != [(3, 3)]
                or struct.unpack_from(">BBHHhhH4f", block, step) != (0, 0, 1, 3, 0, -1, 0, 0, 0, 0, 30)
                or reaction_installer_tables.model_table(image, model)[1] != table
                or image.read(table+3*24, 2, "data") != struct.pack(">H", power)):
            raise ValueError(f"type-89 opening descriptor source mismatch: {model:04X}")
        payload.append(block)
        sources.append({"borg": f"gg4e08:{model:04x}", "hit_block_sha256": digest, "reaction_table": roster.address(table),
                        "raw_power": power})
    return b"".join(payload), sources


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-89 opening output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), roster.SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-89 opening AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--opening"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-89 opening fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type89-opening", "revision": 1,
        "source_dol_sha256": roster.SOURCE_SHA256, "source_afs_sha256": roster.AFS_SHA256,
        "sources": sources, "cases": cases,
        "opening_source": {"group": 3, "descriptor": 3, "reaction_index": 3, "raw_radius": 30},
        "attack_exclusion_mask": {"object_offset": "0x272", "opening_write_site": "8016CE34",
                                  "activation_write_site": "8016D0FC", "registry_read_site": "8008A448"},
        "notes": [
            "The supplied phase-two leaf creates uninitialized carriers. No release debit or controller entry is replayed.",
            "One original bucket traversal is visit 1. The owner socket matrices and empty stage are fixture inlets.",
            "The transform pass receives inert render nodes only for its duration. No retail model data or playback is needed.",
            "Shapes can bind before being attack-eligible. Registry order is not the order that assigned scarce shape slots.",
            "The registry is observed without receivers or a damage pass, and the replay ends at the clear-stage shape unlink.",
        ],
    }
    raw = encode(payload, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
