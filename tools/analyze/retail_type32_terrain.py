"""Replay real terrain crossings through both borrowed-endpoint type-32 modes."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_type32_bind as borrowed, retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


CASES = (
    ("saucer-clear", True, False, 1, 1),
    ("saucer-floor", True, True, 1, 1),
    ("saucer-no-effect-carrier", True, True, 0, 1),
    ("saucer-no-effect-descriptor", True, True, 1, 0),
    ("segment-clear", False, False, 1, 0),
    ("segment-floor", False, True, 1, 0),
)
FIRST, SECOND, CROSSING = [100, 200, 300], [220, -200, 500], [160, 0, 400]
ROW = re.compile(r"type32-terrain:(?: [0-9]{1,4}){7}(?: -?[0-9]{1,4}\.[0-9]{2}){3}")
TEXT = (
    (0x8003E6E4, 660, "c14a21c679159dce975a6a1141f2784f52b27928d4a4222e31b465576d3533df"),
    (0x800FC6A0, 288, "4050c866c59419d4115c98a2d59cfb666b9d57191878a4e2a496f47f20eaff87"),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(CASES):
        raise ValueError("type-32 terrain report extent")
    result = []
    for i, ((case, direct, floor, carriers, descriptors), line) in enumerate(zip(CASES, lines, strict=True)):
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed type-32 terrain row")
        values = line.split()[1:]
        requested = direct and floor
        allocated = requested and carriers > 0 and descriptors > 0
        model = 0x0D03 if direct else 0x0E02
        if (tuple(map(int, values[:7])) != (model, i, floor, direct, requested, allocated, not direct)
                or list(map(float, values[7:])) != (CROSSING if requested else SECOND)):
            raise ValueError(f"type-32 terrain probe, endpoint or allocation mismatch: {case}")
        result.append({
            "case": case, "borg": f"gg4e08:{model:04x}", "carrier_key": 8 if direct else 9,
            "spare_carrier_cells": carriers, "spare_descriptor_cells": descriptors,
            "independent_stage_query_point": CROSSING if floor else None, "active_worker_queried_terrain": direct,
            "borrowed_second_endpoint_after": CROSSING if requested else SECOND,
            "copied_second_endpoint_after": CROSSING if requested else SECOND,
            "effect": {"type": 9, "key": 4, "allocated": allocated, "source_point": CROSSING} if requested else None,
            "shape_object": "carrier" if direct else "type-11-segment",
            "segment": {"key": 1, "bound_group": 1, "far_endpoint": SECOND} if not direct else None,
            "actor_contact_area_requested": False,
        })
    return result


def qualified_inlet(image, archive):
    payload, sources = borrowed.qualified_inlet(image, archive)
    beam.verify_text(image, TEXT)
    for pc, target in ((0x801DF8B4, 0x8003E6E4), (0x801DF8CC, 0x800FC6A0), (0x801DF8E8, 0x80085E00)):
        beam.link(image, pc, target)
    if beam.word(image, 0x801DF8B0) != 0x7C852378 or beam.word(image, 0x801DF8C4) != 0x38A00004:
        raise ValueError("type-32 terrain write destination or effect selector changed")
    return payload, sources


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-32 terrain output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-32 terrain AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--borrowed-terrain"]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-32 terrain fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type32-terrain", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256, "sources": sources, "cases": cases,
        "inputs": {"first_endpoint": FIRST, "second_endpoint": SECOND, "synthetic_floor_y": 0,
                   "actor_contact_latch": 0},
        "entries": {"saucer_query": "801DF8B4", "query": "8003E6E4", "terrain_effect_request": "801DF8CC",
                    "terrain_effect_factory": "800FC6A0", "segment_request": "801DF2B0", "actor_contact_area": "801DF8E8"},
        "notes": [
            "The original constructor publishes a carrier; scratch is supplied at the observation boundary and the inner phase is declared.",
            "The independent query tests the exact input points against stage geometry. Its output is not fed into the carrier.",
            "Key 8's own query writes to the borrowed second endpoint before trying the type-9 allocation.",
            "Key 9 does not call that query and its type-11 segment keeps the unclipped far endpoint in this fixture.",
            "The effect stops at allocation. No type-9 update, overlap, HP, live scheduling or native gameplay is qualified.",
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
