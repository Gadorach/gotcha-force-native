"""Verify original type-15 shape priority, registration, and no-shape disposal."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables
from tools.analyze.retail_bomb_release import encoded, source_operands
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_small_projectile_roster import CHARGED_BOMBS, NINJA_BOMBS
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


SLICES = {
    0x80089A94: (88, "dcb8bf7edce7313e0e258ee526af82153e5e35eac0987938a42cccc6a270dde4"),
    0x80089898: (228, "126a25b8262b9ec16da7e6121fd72318cce3c39faab481f718d2a559307c1299"),
    0x8008997C: (280, "3d628d2b7b32b3cd2c7e0816370547898bb863673bd68351246c797720e57eab"),
    0x8007DCEC: (152, "ee9edd0cdc0d53686a872a42378eb638ee4b44fdf26e25f7259f6198f71b815a"),
    0x8007DD84: (1148, "e8732a065c806b75274606baf717082e907c68db87400e1a4299b77820148ec2"),
    0x8007E200: (1084, "47042a30e6220d5de32bfaeb52c01c55f36b3c5c005057018ca513ae08494d86"),
    0x8007E63C: (60, "955fe1fa85b6c60a40c7e078f9787575519bb7a4455394bc523ae78ec5c932ad"),
    0x8007E678: (32, "3e7f2e5665b4173038c13f23df183d62e6483e1258dff141a07504568b6ef994"),
    0x8008AC80: (400, "88d16df3bda187ed3cb011269a356a7d0646ca1c946dcd194664a5abf4a317d0"),
    0x80088AA0: (416, "800882bf558ba7e7f9e955e1884be71e0c9aad8870807bf7dc0876f56baaeab6"),
    0x80089554: (260, "14d54d2183c44456974e44418019670cfd857a373e5baced583924c4c74280df"),
    0x8008A28C: (48, "94cb040d0d0bd380e6ab2d81aec5d82583aaf212f52408a7050212f7e39acc08"),
    0x80089AEC: (276, "03e4a562cd8742937f8f85f602f0f3a3a682457f8f5887accc0b15cbf24f4802"),
    0x8008A65C: (996, "92fa6953e7aea5fe06460450ea539bf738406dc0632251b89ccba4c973b3adf4"),
}
# Model, collision group, descriptor, reaction index, owner table, raw power, exact normal AFS block.
PILOTS = (
    (0x0101, 15, 8, 1, 0x8034EAF8, 20, "ce5210cd200a24705ab8f2084e291e526367664acdfde28e15d21bbc9c4f36de"),
    (0x000A, 1, 1, 1, 0x802D4160, 10, "8d90bfdd7a4d598b4c0869fff93364b785d4cc98c876d56c99ce44fc86579414"),
)
# Model, free carriers, free descriptors, free shape cells.
CASES = (
    (0x0101, 4, 4, 0), (0x0101, 4, 4, 1), (0x0101, 4, 4, 2), (0x0101, 4, 4, 4),
    (0x0101, 2, 4, 1), (0x0101, 4, 2, 1), (0x000A, 1, 1, 0), (0x000A, 1, 1, 1),
)
ROW = re.compile(r"bomb-bind:(?: [0-9]{1,4}){14}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(CASES):
        raise ValueError("bomb bind report extent")
    result = []
    for line, (model, carriers, descriptors, shapes) in zip(lines, CASES, strict=True):
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed bomb bind row")
        gun = model == 0x0101
        calls = CHARGED_BOMBS if gun else NINJA_BOMBS[model]
        requests = [key for _, key in calls]
        published = requests[:min(carriers, descriptors)]
        visit = published[::-1]
        bound = visit[:shapes]
        refused = visit[shapes:]
        registry = bound[::-1]
        group, step = (15, 8) if gun else (1, 1)
        retired = 0 if shapes else 3
        expected = (model, carriers, descriptors, shapes, len(visit), encoded(visit), len(bound), encoded(bound),
                    group, step, 1, len(registry), encoded(registry), retired)
        if tuple(map(int, line.split()[1:])) != expected:
            raise ValueError(f"bomb bind transaction disagrees: {model:04X}/{carriers}/{descriptors}/{shapes}")
        result.append({
            "borg": f"gg4e08:{model:04x}", "free_carriers_at_release": carriers,
            "free_descriptors_at_release": descriptors, "free_shapes_at_first_visit": shapes,
            "factory_requests": requests, "published_in_request_order": published,
            "first_visit_order": visit, "bound_in_visit_order": bound, "refused_in_visit_order": refused,
            "attack_registry_order": registry, "shape_group": group, "shape_descriptor": step,
            "stock_after_bind": 1, "refused_carriers_control": {
                "new_free_shapes_before_visit_two": 1, "bind_attempts_after_visit_one": 0,
                "phase_on_visit_two": 3, "carrier_and_descriptor_return_visit": retired,
            } if retired else None,
        })
    return result


def qualified_inlet(image, archive):
    source_operands(image)
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"bomb bind source signature mismatch at {address(entry)}")
    if struct.unpack(">4I", image.read(0x802D6F98, 16, "data")) != (0x8007DD84, 0x8007E200, 0x8007E63C, 0x8007E678):
        raise ValueError("bomb bind phase table mismatch")
    blocks, sources = [], []
    for model, group, step, index, table, power, digest in PILOTS:
        block = reaction_emit.qualified_hit_block(archive, model)
        record = table+24*index
        calls = CHARGED_BOMBS if model == 0x0101 else NINJA_BOMBS[model]
        if (len(block) != 7840 or sha256(block).hexdigest() != digest
                or reaction_installer_tables.model_table(image, model)[1] != table
                or reaction_emit.read_group(block, group) != [(step, index)]
                or any(image.read(0x802D6D68+56*key+3, 1, "data") != bytes([group]) for _, key in calls)
                or image.read(record, 2, "data") != struct.pack(">H", power)):
            raise ValueError(f"bomb opening hit record mismatch: {model:04X}")
        blocks.append(block)
        sources.append({
            "borg": f"gg4e08:{model:04x}", "group": group, "descriptor": step, "reaction_index": index,
            "reaction_record": address(record), "raw_power": power, "normal_hit_block_sha256": digest,
        })
    return b"".join(blocks), sources


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("bomb bind output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("bomb source AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--first-bind"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"bomb bind fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-bomb-first-bind", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "normal_opening_records": sources, "cases": cases,
        "entries": {"bucket_visit": "80089A94", "type_15_dispatch": "8007DCEC", "initialize": "8007DD84",
                    "bind_group": "8008AC80", "shape_list_visit": "80089AEC"},
        "notes": [
            "The original seat-zero bucket visitor processes prepended class-3 carriers from head to tail.",
            "Accepted class-4 shapes are themselves prepended; registry order is not allocation priority.",
            "No-retry and visit-three disposal were replayed in the initially empty-shape cases.",
            "Stage queries saw declared empty cells; render resources and separate effect cells were unavailable.",
            "Only opening shape registration is qualified, not contact, HP, impact effects or live battle scheduling.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
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
