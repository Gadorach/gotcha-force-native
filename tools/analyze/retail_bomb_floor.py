"""Verify ordinary Ninja bomb floor crossings with actual source triangle queries."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables, retail_bomb_area
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_dragon_area import COMMON_COLLISION_SIZE
from tools.analyze.retail_small_projectile_roster import NINJA_BOMBS
from tools.analyze.retail_type0_contact import RECEIVER_BLOCK_SHA256
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


PILOTS = (0x0000, 0x000A)
SLICES = {
    0x80083244: (128, "0d12380007430d1784fc3a5d0183bdd2c9b40b58252868175125d47bbd3e48c4"),
    0x80084170: (192, "9820c165d09f46a0c71df798be6438f031c2ca9a1876c801a879562257fe9e76"),
    0x80084230: (516, "616e17f25d355f351726fd8477f741b9f996e208c76047a388747c6f997cb682"),
    0x8003E978: (124, "bfb5cd0f3daca84c72cb691c8f7396b0ad585983c7d679e074cd39c5fdd24d41"),
    0x8003E9F4: (708, "5a2d710d8d70b74297cb3ac48f9b5d07d5724f43f18d772098fc2628fcffe7e2"),
    0x80039DA4: (456, "49211bdef98464db15d88feb784a88e40f3511b5e5c00e8444c3bb299898f66c"),
    0x80039AEC: (308, "34a98991b5f94e53f4d2db6d97ac25b4da1cf082066462ffda3a14ba4e9f1607"),
}
CASES = ("empty-grid", "floor", "offset-floor", "floor-child-pool-empty", "bounds-exit")
FLOOR_ROW = re.compile(r"bomb-floor:(?: [0-9]{1,3}){11}(?: [0-9]{1,4}\.[0-9]{5}){5}")
AREA_ROW = re.compile(r"bomb-floor-area:(?: [0-9]{1,3}){9}")


def decode_report(report):
    lines = report.splitlines()
    width = len(CASES)+2
    if len(report) > 2048 or len(lines) != len(PILOTS)*width:
        raise ValueError("bomb floor report extent")
    result = []
    for i, model in enumerate(PILOTS):
        rows, areas = [], []
        section = lines[i*width:(i+1)*width]
        for case, line in enumerate(section[:len(CASES)]):
            if FLOOR_ROW.fullmatch(line) is None:
                raise ValueError("malformed bomb floor row")
            fields = line.split()[1:]
            integers = tuple(map(int, fields[:11]))
            measured = tuple(map(float, fields[11:]))
            floor, bounds_exit = case in (1, 3), case == 4
            event = 23 if bounds_exit else 35
            unlink, retire = (23, 25) if bounds_exit else (36, 37) if floor else (0, 0)
            expected_fields = (model, case, event, 600-event, int(floor), int(case == 1), int(floor),
                               int(case in (1, 2, 3)), unlink, retire, 1)
            expected_point = ((127.4, 120.65, 1215.72021, 127.4, 1176.26709) if bounds_exit else
                              (26.59997, 16.24997, 1684.49255, 25, 1651.78943) if floor else
                              (26.59997, 16.24997, 1684.49255, 16.24997, 1684.49255))
            if integers != expected_fields or measured != expected_point:
                raise ValueError(f"bomb floor crossing or disposition mismatch: {model:04X}/{CASES[case]}")
            _, _, _, timer, request, child, layer, tested, _, _, stock = integers
            before_y, proposed_y, proposed_z, final_y, final_z = measured
            rows.append({
                "case": CASES[case], "event_visit": event, "clock_at_event": timer, "previous_y": before_y,
                "proposed_point": [165, proposed_y, proposed_z], "committed_point": [165, final_y, final_z],
                "independent_segment_probe": {"grid_code": layer, "triangles_tested": tested},
                "child_requested": bool(request), "child_published": bool(child),
                "opening_shape_registered_at_event": not bounds_exit, "opening_shape_unlink_visit": unlink or None,
                "parent_carrier_and_descriptor_return_visit": retire or None,
                "flight_continued_through_visit": None if retire else 64, "stock_after": stock,
            })
        for spare, line in enumerate(section[len(CASES):]):
            if AREA_ROW.fullmatch(line) is None:
                raise ValueError("malformed bomb floor area row")
            values = tuple(map(int, line.split()[1:]))
            if values != (model, spare, 35, spare, spare, 1 if spare else 3, spare, 1, 1):
                raise ValueError(f"bomb floor same-pass area result mismatch: {model:04X}/{spare}")
            areas.append({
                "available_shape_cells_at_child_init": spare, "child_attempt_visit": 35,
                "parent_shape_registered_after_child_init": True, "child_bound_shape": bool(spare),
                "child_audio_request": 12 if spare else None, "child_attack_registered_at_init": False,
                "next_visit": {"visit": 36, "parent_shape_returned": True, "child_phase": values[5],
                               "child_bind_retry": False, "child_attack_count": values[6],
                               "child_radius": 34.1 if spare else None, "free_shape_cells": values[7]},
                "stock_after": values[8],
            })
        result.append({"borg": f"gg4e08:{model:04x}", "key": NINJA_BOMBS[model][0][1],
                       "impact_cases": rows, "same_pass_area_cases": areas})
    return result


def qualified_inlet(image, archive):
    inlet, source = retail_bomb_area.qualified_inlet(image, archive)
    for at, (size, digest) in SLICES.items():
        if sha256(image.read(at, size, "text")).hexdigest() != digest:
            raise ValueError(f"bomb floor source signature mismatch: {address(at)}")
    bounds_margin, = struct.unpack(">f", image.read(0x80437AEC, 4, "data"))
    if bounds_margin != 750:
        raise ValueError("bomb floor bounds margin changed")
    blocks, pilots = [], []
    for model in PILOTS:
        site, key = NINJA_BOMBS[model][0]
        row = image.read(0x802D6D68+56*key, 56, "data")
        block = reaction_emit.qualified_hit_block(archive, model)
        if (sha256(block).hexdigest() != RECEIVER_BLOCK_SHA256 or reaction_emit.read_group(block, 1) != [(1, 1)]
                or reaction_installer_tables.model_table(image, model)[1] != 0x802D4160
                or row[2:4] != bytes((11, 1)) or row[44:46] != bytes((0, 0))
                or struct.unpack_from(">4f", row, 4) != (55, 0, 0, 40)):
            raise ValueError(f"bomb floor source owner or launch operand mismatch: {model:04X}")
        blocks.append(block)
        pilots.append({"borg": f"gg4e08:{model:04x}", "key": key, "release_site": address(site),
                       "group": 1, "normal_hit_block_sha256": RECEIVER_BLOCK_SHA256})
    return b"".join(blocks)+inlet[-COMMON_COLLISION_SIZE:], source["common_collision"], pilots


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("bomb floor output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, area, source = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("bomb floor AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--floor"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"bomb floor fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    pilots = decode_report(result.stdout.decode("ascii"))
    receipt = {
        "kind": "gf-retail-bomb-floor", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "pilots": pilots, "source_pilots": source, "common_area_bank": area,
        "terrain": {
            "plane_y": 25, "triangle_vertices": [[-500, 25, -500], [-500, 25, 6000], [3000, 25, -500]],
            "offset_control_dx": 2500, "cell_size": 500, "bounds_control_max_z": 400, "bounds_margin": 750,
        },
        "entries": {"terrain_response": "80084170", "scene_segment_query": "8003E978",
                    "terrain_child_call": "8007E5FC", "area_callback": "80086048"},
        "notes": [
            "Original paid X leaves, no-target launch, bucket visits, and triangle tests run on declared inputs.",
            "The terrain is a synthetic rebased grid with one triangle, not an original Stage asset.",
            "The observed crossing probe does not feed a result back into the carrier.",
            "The area can initialize on the same visit; the opening shape keeps its cell through that visit on a floor hit.",
            "No actor bodies or HP writes are exercised here. Child audio stops before playback.",
            "No live scheduler, target-dependent aim, props, native gameplay, or other type-15 keys are qualified.",
        ],
    }
    raw = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(pilots), "cases": len(pilots)*(len(CASES)+2)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
