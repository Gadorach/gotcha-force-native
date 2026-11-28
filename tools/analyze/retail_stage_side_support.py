"""Verify original support acquisition on the complete stage-side role-0x20 census."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_stage_side_recovery as life


side = life.side
body = side.body
anchor = side.recovery.prop.carry.anchor
ANCHOR_SHA256 = "61349a2d0e69cd4615a867463bf8f7c3040c119eefb2623a74e56bc5182ba1a5"
TEXT = (
    (0x800350F8, 844, "6b529d4d35804ff73ed41d6e571caf4b3bc43ab761fecfbe20d4a9b83f3dd6fd"),
    (0x8002CF80, 536, "e983f60ae94672202ce930e0db112632eb3ed9b4f6e6a1624e61191e4f760668"),
)
HEX = re.compile(r"[0-9A-F]{8}")


def verify_source(image):
    anchor.verify_source(image)
    body.verify_text(image, TEXT)
    for site, target in ((0x80035288, 0x80039DA4), (0x80035354, 0x80039C20), (0x80035378, 0x80035B3C),
                         (0x800353D4, 0x8020B7A8), (0x800353F4, 0x8020BA78)):
        body.link(image, site, target)
    if (image.read(0x802CFEA0, 8, "data") != bytes.fromhex("800350F880035444")
            or image.read(0x802CFEAD, 1, "data") != b"\0"
            or image.read(0x802CFFC8, 24, "data") != bytes.fromhex("000203010103070502060703040507060602000405040001")):
        raise ValueError("stage side original box dispatch or faces changed")


def bits(value):
    return struct.pack(">f", value).hex().upper()


def read_tops(payload, members, types):
    """Source-height cross-check independent of the runtime cells used to pose the actor."""
    start = 21504
    tops = {}
    for member, _, _, _, placements in members:
        key, size = struct.unpack_from(">II", payload, start)
        if key != member or not 6144 <= size <= 1 << 18 or start+8+size > len(payload):
            raise ValueError("stage side support payload boundary")
        start += 8
        for i, (t, _, variant, _) in enumerate(placements):
            if variant:
                continue
            y = struct.unpack_from(">f", payload, start+2052+28*i+8)[0]
            for shape in types[t]["collision_shapes"]:
                if not shape["descriptor_role"] & 0x20:
                    continue
                d, kind = shape["descriptor"], shape["shape_kind"]
                off = 0x400+80*d
                local_y = struct.unpack_from(">f", payload, off+16)[0]
                extra = struct.unpack_from(">f", payload, off+(24 if kind == 4 else 28))[0]
                tops[member, i, d] = bits(y+local_y+extra)
        start += size
    if start != len(payload):
        raise ValueError("stage side support unexpected payload suffix")
    return tops


def decode(report, members, types, tops):
    lines = report.splitlines()
    if len(report) > 1 << 17 or len(lines) != 844:
        raise ValueError("stage side support report extent")
    pos = 0
    census_lines = []
    rows = []
    kinds = Counter()
    rounded = set()
    double_narrow = set()
    scalar = anchor.stage.scalar
    for member, name, _, _, placements in members:
        end = pos+2+len(placements)
        census_lines.extend(lines[pos:end])
        pos = end
        case = []
        for i, (t, _, variant, _) in reversed(list(enumerate(placements))):
            if variant:
                continue
            for shape in reversed(types[t]["collision_shapes"]):
                if not shape["descriptor_role"] & 0x20:
                    continue
                d, kind = shape["descriptor"], shape["shape_kind"]
                kinds[kind] += 1
                key = member, i, d
                extra_narrow = member == 2455 and i == 14
                rounding = member == 2460 and i in (0, 1) and d == 140
                # Positioning starts from the original cell, not a host copy of
                # the world matrix. Require the independent source top to agree.
                near = lines[pos].split()
                if len(near) != 16 or not all(HEX.fullmatch(v) for v in near[10:15]):
                    raise ValueError(f"stage side support numeric report changed: {key}")
                top = near[10]
                if abs(int(top, 16)-int(tops[key], 16)) > 1:
                    raise ValueError(f"stage side support surface changed: {key}")
                expected_surface = f"{int(top, 16)+int(rounding):08X}"
                face = 255 if kind == 4 else 0
                for far in (False, True):
                    pose = bits(scalar(top)+(1024 if far else 4))
                    expected = ("side-support:", *key[:2], d, len(case), face, int(far),
                                0 if far else 1+int(extra_narrow), int(not far), 0 if far else 7, top, pose,
                                pose if far else expected_surface, "00000000" if far else expected_surface,
                                "00000000" if far else expected_surface, int(not far))
                    if lines[pos] != " ".join(map(str, expected)):
                        raise ValueError(f"stage side support changed: {key}/{'above' if far else 'near'}")
                    pos += 1
                if rounding:
                    rounded.add(key)
                if extra_narrow:
                    double_narrow.add(key)
                case.append({
                    "placement": i, "descriptor": d, "shape_kind": kind, "role20_registry_index": len(case),
                    "near_kernel_calls_of_same_kind": 1+int(extra_narrow), "near_target_kernel_calls": 1,
                    "exact_cell_and_owner_acquired": True, "above_support_acquired": False,
                    "landing_followed_supported_height": True, "height_float_steps_above_target_top": int(rounding),
                })
        rows.append({"member": member, "filename": name, "targets": case})
    if pos != len(lines) or kinds != {4: 84, 1: 97} or len(rounded) != 2 or len(double_narrow) != 2:
        raise ValueError("stage side support coverage changed")
    original = side.decode("\n".join(census_lines), members, types)
    return rows, kinds, original


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stage side support output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    prior = body.read_metadata(body.MAPS / "gg4e08-stage-side-census.json", life.CENSUS_SHA256)
    body.read_metadata(body.MAPS / "gg4e08-support-anchor.json", ANCHOR_SHA256)
    payload, members, types = side.source_inputs(image, afs)
    tops = read_tops(payload, members, types)
    actor = anchor.plane.source_inputs(afs, include_actor=True)
    payload[21504:21504] = actor
    report = body.fixture_report(fixture, dol, module, ("--support",), payload)
    rows, kinds, original = decode(report, members, types, tops)
    if json.loads(json.dumps(original)) != prior["archives"] or types != prior["types"]:
        raise ValueError("stage side support initial census changed")
    raw = body.encode({
        "kind": "gf-retail-stage-side-support", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "stage_side_census_sha256": life.CENSUS_SHA256,
        "support_anchor_sha256": ANCHOR_SHA256, "actor": "gg4e08:000a", "actor_seat": 0, "actor_hp": 180,
        "near_pose_above_surface": 4, "far_pose_above_surface": 1024, "collision_stop_before": "8005A580",
        "near_exact_acquisitions": sum(kinds.values()), "above_controls_without_support": sum(kinds.values()),
        "targets_by_shape": {"box": kinds[1], "plane": kinds[4]}, "archives": rows,
        "notes": [
            "All side geometry and collision queues come from the original archive loader, placement loop, callbacks and shape registries. Each independent probe retains every side object and supplied no support state. Terrain is a declared empty grid.",
            "Sasuke uses the exact normal role-0x10 sphere and receiving capsule with declared rigid joint poses. The near root starts four units above the plane's top surface or the upward box face; the above control is 1024 units above the same point.",
            "Both original narrow kernels write an exact supporting cell and owner when tested from above. Both kind-1 boxes on type 11 have the same owner, but each low/high probe selects its own cell even though both cells reach narrow.",
            "The supported height query and landing helper agree with the collision-corrected root. Two set0b00 rows land one float step above the target top used to place the actor. The above controls do not enter the absent-support floor fallback.",
            "This sweep does not exercise moving-stage carry, other actor rigs, actual arena terrain, live scheduling, drawing, hardware or native gameplay.",
        ],
    }, expanded=("archives",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "archives": len(rows), "box_targets": kinds[1], "plane_targets": kinds[4]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
