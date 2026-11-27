"""Replay type-55 emission, partial type-11 shape admission and shared hit tokens."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables
from tools.analyze import retail_type11_roster as roster, retail_type32_contact as contact, retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256
from tools.analyze.retail_shape_sweep import source_operands
from tools.analyze.retail_type0_contact import RECEIVER, RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


MODEL = 0x0704
ACTOR_TABLE, AMOUNT_RECORD = 0x80351C78, 0x80351EE8
BLOCK_DIGEST = "b76c87cce315ee196559f525538ecd82b8e5aaf3524bf37ce77d0f9376355bba"
ACTOR_DIGEST = "cbfe1d88da6fabd0ce7443d948a40253b1789633dd911dc0eacb092fc3442ca5"
STEPS, OFFSETS = (13, 28, 29, 30), (30, 90, 150, 210)
SHAPES = ((0, 210), (1, 210), (2, 210), (3, 210), (4, 210), (2, 60), (4, 60))
SHAPE_ROW = re.compile(r"type55-shapes:(?: [0-9]{1,3}){6}")
WINDOW_ROW = re.compile(r"type55-window:(?: [0-9]{1,3}){4}")
TEXT = (
    (0x8018EFF4, 348, "054b5b56fc647b1099a5c0a23436c60c359364cacc624d6b9102659a16a45f7d"),
    (0x8018F1E0, 172, "aad2ac451a7af20e99fd7ac2f04aef54f1821419168e6d81ccde8850b659533f"),
    (0x8018F3FC, 264, "e297c313f5bcd990a17ed6cffcd70696b4e5c5d9fdc107834c565d80d94e7c1a"),
    (0x800840B8, 184, "3744690e6cd4ef9f8ec2d3a6fee6c45e8db9f420b82c3b456a1e1b3818dc5118"),
    *tuple(span for span in roster.TEXT if span[0] in (0x8018EEA4, 0x8018EFA4, 0x8018F150, 0x8018F28C)),
    *tuple(span for span in beam.TEXT if span[0] in (beam.SEGMENT, beam.SEGMENT_BIND)),
    *tuple(span for span in contact.TEXT if span[0] != 0x8003E978),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(SHAPES)+2:
        raise ValueError("type-55 segment report extent")
    binds = []
    for i, ((slots, target_y), line) in enumerate(zip(SHAPES, lines[:len(SHAPES)], strict=True)):
        if SHAPE_ROW.fullmatch(line) is None:
            raise ValueError("malformed type-55 shape row")
        count = sum(abs(y-target_y) < 52 for y in OFFSETS[:slots])
        damage = 4 if count else 0
        values = tuple(map(int, line.split()[1:]))
        if values != (i, slots, count, damage, 180-damage if slots else 0, int(bool(count))):
            raise ValueError(f"type-55 partial group or hit token mismatch: {slots}/{target_y}")
        binds.append({
            "shape_cells_at_first_bind": slots, "receiver_offset_y": target_y if slots else None,
            "allocation_order": list(STEPS[:slots]), "attack_registry_order": list(reversed(STEPS[:slots])),
            "later_supplied_shape_cells": 4-slots, "retried_bind": False,
            "overlapping_shapes": count if slots else None, "receiver": {
                "hp_after": values[4], "hp_loss": damage, "segment_token": 1 if damage else None,
                "same_segment_token_refused_on_repeat": bool(damage),
            } if slots else None,
        })
    windows = []
    for i, line in enumerate(lines[len(SHAPES):], len(SHAPES)):
        if WINDOW_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != (i, 10, 59, 70):
            raise ValueError("type-55 constructor-to-close window mismatch")
        windows.append({
            "owner_clock_units_per_visit": 1 if i == len(SHAPES) else 0.5, "spare_segment_cells": 0,
            "warmup_visits": 10, "first_request_visit": 11, "last_request_visit": 69,
            "requests": 59, "phase_two_visit": 70, "segments_allocated": 0,
        })
    return binds, windows


def qualified_inlet(image, archive):
    beam.verify_text(image, TEXT)
    for pc, target in ((0x8018F384, beam.SEGMENT), (0x80176140, 0x8018EEA4), (0x8018F4CC, 0x800076D0)):
        beam.link(image, pc, target)
    if (image.read(0x8036EC50, 20, "data").hex() != "070800080000000000000000000000008036ebd0"
            or struct.unpack(">4I", image.read(0x8036EC64, 16, "data")) != (0x8018EFF4, 0x8018F150, 0x8018F3A0, 0x8018F3B4)
            or struct.unpack(">2I", image.read(0x804356B0, 8, "data")) != (0x8018F1E0, 0x8018F28C)
            or image.read(AMOUNT_RECORD, 24, "data").hex() != "000a0023050800000108c802000804000001000000000000"):
        raise ValueError("type-55 parent row, dispatch or amount record changed")
    attack, owner_spec = read_normal(archive, MODEL, BLOCK_DIGEST, ACTOR_DIGEST)
    body, target_spec = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
    if (reaction_emit.read_group(attack, 8) != [(step, 26) for step in STEPS]
            or reaction_installer_tables.model_table(image, MODEL)[1] != ACTOR_TABLE
            or (levels(image, MODEL)[0][0], levels(image, RECEIVER)[0][0]) != (300, 180)):
        raise ValueError("type-55 exact normal actor source changed")
    for step, y in zip(STEPS, OFFSETS, strict=True):
        off = reaction_emit.STEPS+step*reaction_emit.STEP_SIZE
        if struct.unpack_from(">BBHHhhH4f", attack, off) != (0, 0, 1, 26, 0, -1, 0, 0, y, 0, 30):
            raise ValueError("type-55 normal sphere queue changed")
    return attack+body+owner_spec+target_spec, source_operands(body)["receiver"]


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-55 segment output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, body = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-55 segment AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-55 segment fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    binds, windows = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type55-segment", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "source": {"borg": "gg4e08:0704", "level_one_hp": 300, "factory": "8018EEA4", "key": 0,
                   "segment_request": "8018F384", "segment_key": 8, "reaction_index": 26, "amount_record": address(AMOUNT_RECORD),
                   "raw_power": 10, "hit_block_sha256": BLOCK_DIGEST, "actor_spec_sha256": ACTOR_DIGEST,
                   "queue": [{"descriptor": d, "local_y": y, "radius": 30} for d, y in zip(STEPS, OFFSETS, strict=True)]},
        "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180}, "bind_cases": binds, "empty_carrier_windows": windows,
        "notes": [
            "The carrier factory is called directly. The owner pose is declared and the carrier's late callback stops at the renderer write.",
            "The emission clocks are tested at the original carrier visits with no spare type-11 objects, not a live match schedule.",
            "One type-11 child binds only a prefix of its exact group. It does not retry after more shape cells become available.",
            "The narrow swept spheres share the segment token. In overlap cases the original resolver is rerun with the same registry.",
            "No live action admission, overlapping recipients, whole-move damage, or native gameplay changes are qualified.",
        ],
    }
    raw = encode(payload, expanded=("bind_cases", "empty_carrier_windows"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "bind_cases": len(binds), "empty_carrier_windows": len(windows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
