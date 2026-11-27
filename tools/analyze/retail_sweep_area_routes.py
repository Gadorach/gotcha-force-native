"""Replay direction-row parents whose terrain area and type-11 segment are sibling requests."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables, retail_dragon_area as area
from tools.analyze import retail_type11_roster as roster, retail_type32_bind as segment, retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


# model, row, socket, segment key, descriptor, index, normal reaction table and block.
PILOTS = (
    (0x0E00, 0, 6, 1, 5, 1, 0x80363048, "e72edd5f977a5bacff14b2564280b24d966c8687a34ed7496f45fa1353950b9b"),
    (0x0E05, 0, 6, 1, 5, 1, 0x80363048, "e72edd5f977a5bacff14b2564280b24d966c8687a34ed7496f45fa1353950b9b"),
    (0x0E03, 2, 4, 2, 2, 2, 0x8038DE60, "84d1d9706895864f2b80aae56ad1363d822508882297ec2b963e6d3120b9e814"),
)
CASES = (("clear-stage", False, 2, 2), ("floor-both", True, 2, 2),
         ("floor-one-carrier", True, 1, 2), ("floor-no-area-descriptors", True, 2, 0))
ROW = re.compile(r"sweep-area:(?: [0-9]{1,4}){7}(?: -?[0-9]{1,5}\.[0-9]{5}){15}")
TEXT = (
    (0x801F2DBC, 720, "60c3011d041824ccdec8a525e1592b09d97290b1777d88ac34bb7c8c9870b583"),
    (0x801F392C, 572, "7d5c7924f989e08ca9edb745670c84fcd746d6f4278a25efabe1716f1bf5ede7"),
    (0x801F3580, 516, "616e17f25d355f351726fd8477f741b9f996e208c76047a388747c6f997cb682"),
    (0x801F405C, 516, "616e17f25d355f351726fd8477f741b9f996e208c76047a388747c6f997cb682"),
    (0x8003E6E4, 660, "c14a21c679159dce975a6a1141f2784f52b27928d4a4222e31b465576d3533df"),
    (0x80047AA4, 404, "d1034c09b0f68dabce25aa8dc2fd55eb99b31ee2e017cf379fad8fa7c23eb779"),
    *tuple(span for span in roster.TEXT if 0x801F2BF8 <= span[0] <= 0x801F3B68),
    *tuple(span for span in beam.TEXT if span[0] in (beam.SEGMENT, beam.SEGMENT_BIND)),
    *tuple(span for span in segment.TEXT if span[0] in (0x80088C40, 0x80139B80)),
    *tuple(span for span in area.TEXT if span[0] == 0x80085E00),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(PILOTS)*len(CASES):
        raise ValueError("sweep area report extent")
    results = []
    for model, row, _, group, *_ in PILOTS:
        reference = None
        for n, (name, floor, carriers, descriptors) in enumerate(CASES):
            line = lines[len(results)]
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed sweep area case")
            vals = line.split()[1:]
            visit = 34 if model == 0x0E03 else 35
            made_area = floor and descriptors == 2
            made_segment = carriers > int(made_area)
            if tuple(map(int, vals[:7])) != (model, n, visit, floor, made_area, made_segment, group):
                raise ValueError(f"sweep area allocation or pool order mismatch: {model:04X}/{name}")
            coords = list(map(float, vals[7:]))
            impact, probe_start, probe_end, near, far = (coords[i:i+3] for i in range(0, len(coords), 3))
            if not (probe_start[1] > 0 > probe_end[1] and probe_end[2] == 6000 and far[2] > probe_end[2]):
                raise ValueError("terrain probe and actual segment endpoints were conflated")
            if floor:
                delta = probe_start[1]/(probe_start[1]-probe_end[1])
                expected = [a+(b-a)*delta for a, b in zip(probe_start, probe_end, strict=True)]
                if any(abs(a-b) > 0.003 for a, b in zip(impact, expected, strict=True)):
                    raise ValueError("terrain point does not lie on the tested floor crossing")
            elif any(impact):
                raise ValueError("empty-stage control reported a terrain point")
            if reference is None:
                reference = coords[3:]
            elif coords[3:] != reference:
                raise ValueError("terrain or pool pressure changed the tested segment endpoints")
            results.append({
                "borg": f"gg4e08:{model:04x}", "parent_row": row, "case": name,
                "visit_including_initializer": visit, "inlet": {"carrier_cells": carriers, "descriptor_cells": descriptors},
                "terrain_probe": {"near": probe_start, "far": probe_end, "floor_crossing": impact if floor else None},
                "area": {"requested_key": 26, "allocated": made_area, "origin": impact} if floor else None,
                "segment": {"requested_key": group, "allocated": made_segment, "normal_group_bound": made_segment,
                            "near": near, "far": far},
                "allocation_order": (["area"] if made_area else [])+(["segment"] if made_segment else []),
            })
    return results


def qualified_inlet(image, archive):
    beam.verify_text(image, TEXT)
    for pc, target in ((0x801F325C, 0x8003E6E4), (0x801F3D38, 0x8003E6E4),
                       (0x801F3274, 0x80085E00), (0x801F3D50, 0x80085E00),
                       (0x801F33A4, beam.SEGMENT), (0x801F3E80, beam.SEGMENT)):
        beam.link(image, pc, target)
    if (struct.unpack(">2I", image.read(0x803A2320, 8, "data")) != (0x801F2DBC, 0x801F308C)
            or struct.unpack(">2I", image.read(0x803A2490, 8, "data")) != (0x801F392C, 0x801F3B68)):
        raise ValueError("sweep parent phase table changed")
    blocks, sources = [], []
    for model, row, socket, group, descriptor, index, table, digest in PILOTS:
        other = model == 0x0E03
        parent_row = (0x803A2330 if other else 0x803A1DD8)+44*row
        raw = image.read(parent_row, 44, "data")
        block = reaction_emit.qualified_hit_block(archive, model)
        if (struct.unpack_from(">h", raw)[0] != socket or struct.unpack_from(">i", raw, 36)[0] != group
                or reaction_installer_tables.model_table(image, model)[1] != table
                or reaction_emit.read_group(block, group) != [(descriptor, index)] or sha256(block).hexdigest() != digest):
            raise ValueError(f"sweep route normal source changed: {model:04X}")
        blocks.append(block)
        sources.append({
            "borg": f"gg4e08:{model:04x}", "parent_row_address": address(parent_row), "parent_row_index": row,
            "socket": socket, "socket_pose_pitch_degrees": 15 if other else 0,
            "normal_descriptor": descriptor, "reaction_index": index, "reaction_table": address(table),
            "hit_block_sha256": digest, "area_site": "801F3D50" if other else "801F3274",
            "segment_site": "801F3E80" if other else "801F33A4",
        })
    return b"".join(blocks), sources


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("sweep area output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("sweep area AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"sweep area fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-sweep-area-routes", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "sources": sources, "cases": cases,
        "notes": [
            "The original parent constructors are called directly with the reviewed row arguments; actor action entry is not replayed.",
            "The original parent update and pose callbacks run with declared socket matrices and empty visual records.",
            "No child cells are offered before the crossing visit. The probe and two factory observations replay the same saved inlet.",
            "The type-5 area and type-11 segment are separate allocations with the same parent and credited actor.",
            "The replay stops before area initialization and segment contact. It establishes no area or segment HP and no native behavior.",
        ],
    }
    raw = encode(payload, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
