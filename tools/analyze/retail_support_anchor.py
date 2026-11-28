"""Verify original queries and landing against a retained local support anchor."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_plane_reference as plane


body = plane.body
stage = plane.stage
PLANE_SHA256 = "6fb6b26c0da43d222b646485cf48d39660eb868e563420239611697de2f4a31a"
TEXT = (
    (0x8003D6E4, 640, "3788a13d2282f6b5ae753eb0d7f29c13289f305f35665a0122ce54e8e1fca7b3"),
    (0x8003E0A8, 844, "4310809c29ad48750521ad16c4e2f47192b2c69b1d3dc357d613870af21d1708"),
    (0x800677B0, 544, "09ab80886742547a9862056d729a3b23984007549f8cd636ea2d5cf3e63bf246"),
    (0x8006F4F8, 276, "9bedc00559917b9e5fa76d02977c59be604bed473605fc394e3df7af750ee800"),
)
# Name, matrix translation change, tilted, geometry rebuilt before query, reset,
# query height bits, original landing return value.
CASES = (
    ("held-support", {}, False, False, False, "42F80000", 1),
    ("raise-12", {"delta_y": 12}, False, False, False, "43080000", 1),
    ("translate-xz", {"delta_x": 32, "delta_z": -20}, False, False, False, "42F80000", 1),
    ("lower-12", {"delta_y": -12}, False, False, False, "42E00000", 0),
    ("tilt-and-raise", {"set_y": 150}, True, False, False, "43160000", 1),
    ("raise-after-reset", {"delta_y": 12}, False, False, True, "C60CA000", 0),
    ("raise-matched-untilted", {"set_y": 150}, False, False, False, "433B3333", 1),
    ("tilt-raise-and-rebuild", {"set_y": 150}, True, True, False, "43160000", 1),
)


def verify_source(image):
    plane.verify_source(image)
    body.verify_text(image, TEXT)
    for site, target in ((0x80035890, 0x8020B078), (0x800358A0, 0x8020B7A8), (0x8003D734, 0x8020B7A8),
                         (0x8003E100, 0x8020B7A8), (0x800677F0, 0x8003E0A8)):
        body.link(image, site, target)


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(CASES):
        raise ValueError("support anchor report extent")
    bits = stage.walk.stage.pair.capsule.geometry.bits
    anchor = ("00000000", "4214CCCC", "00000000")
    saved = ("00000000", "3F7FFFFF", "00000000")
    up = ("00000000", "3F800000", "00000000")
    left = ("BF800000", "00000000", "00000000")
    initial = tuple(bits(n) for n in (130, 124, 0))
    rows = []
    for i, (name, transform, tilt, rebuilt, reset, height, landed) in enumerate(CASES):
        normal = up if reset else saved
        shape_normal = left if rebuilt else up
        after_rebuild_normal = left if tilt else up
        center = tuple(bits(n) for n in (130, 128 if rebuilt else 124, 0))
        root_after = (initial[0], height if landed else initial[1], initial[2])
        fields = (i, int(not reset), int(reset), *anchor, *saved, height, height, *normal,
                  *initial, *root_after, *center, *shape_normal, *after_rebuild_normal, landed, 180)
        if lines[i] != "support-anchor: " + " ".join(map(str, fields)):
            raise ValueError(f"support anchor changed: {name}")
        rows.append({
            "case": name, "declared_plane_translation_change": transform, "declared_plane_rotation_z_degrees": 90 if tilt else 0,
            "geometry_rebuilt_before_query": rebuilt, "current_support_flag": not reset, "previous_support_flag": reset,
            "original_plane_cell_retained": True, "saved_local_anchor_bits": list(anchor), "saved_support_normal_bits": list(saved),
            "ground_height_query_bits": height, "surface_height_query_bits": height, "surface_normal_query_bits": list(normal),
            "plane_normal_at_query_bits": list(shape_normal), "plane_normal_after_optional_rebuild_bits": list(after_rebuild_normal),
            "actor_root_before_bits": list(initial), "actor_root_after_landing_bits": list(root_after),
            "allocated_capsule_center_held_during_landing_bits": list(center), "original_landing_return": landed,
            "hp_after": 180,
        })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("support anchor output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    body.read_metadata(body.MAPS / "gg4e08-plane-reference.json", PLANE_SHA256)
    rows = decode(body.fixture_report(fixture, dol, module, ("--support-anchor",), plane.source_inputs(afs, include_actor=True)))
    raw = body.encode({
        "kind": "gf-retail-support-anchor", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "plane_reference_sha256": PLANE_SHA256,
        "actor": "gg4e08:000a", "plane_descriptor": 3, "plane_matrix_slot": 12, "plane_seat": 1,
        "support_anchor_save": "800358A0", "anchor_actor_offset": "1F8", "support_shape_actor_offset": "1E0",
        "current_support_actor_offset": "1DB", "previous_support_actor_offset": "1DE",
        "height_query": "8003D6E4", "surface_query": "8003E0A8", "landing_helper": "800677B0",
        "declared_empty_floor_fallback": -9000, "cases": rows,
        "notes": [
            "The original near-only collision walk produces support and saves the local anchor on the same allocated foreign plane. No support flag, pointer or anchor is seeded by the host.",
            "Declared changes affect the original joint matrix read through the plane cell. Both original queries return the reprojected Y. The surface query returns the saved normal, even after the original shape rebuild publishes the rotated plane normal.",
            "The normal Sasuke installer supplies the actor configuration. The original landing helper follows an upward support height, keeps X/Z, refuses a lowered support while above it, and leaves the already allocated capsule center unchanged.",
            "The original reset copies current support to the previous flag and clears current support. The plane pointer and anchor remain, but neither query uses them; the declared empty floor supplies the fallback.",
            "Each case is a fresh bounded fork after the original near contact. There is no full actor tick, platform driver, lateral carrying, floor integration, live stage, hardware or native gameplay qualification.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows),
            "landing_returns": sum(r["original_landing_return"] for r in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
