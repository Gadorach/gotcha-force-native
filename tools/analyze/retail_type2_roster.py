"""Census exact type-2 carrier operands and static roster release candidates."""
import argparse
from collections import Counter
import json
from math import isfinite
from pathlib import Path
import struct

from tools.analyze import reaction_emit
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, address, candidate_census, export_census,
)
from tools.analyze.retail_projectile_origins import TABLES, origin_tables

FACTORY, DISPATCH = 0x800C3BE0, 0x803051E4
PHASES = (0x800C3D2C, 0x800C4088, 0x800C426C, 0x800C4448)
EXIT_PHASES = (0x800C42A8, 0x800C42BC, 0x800C4308)
BASE, STRIDE, COUNT, _, _ = TABLES["type-2"]


def carrier_rows(image, origins):
    if struct.unpack(">7I", image.read(DISPATCH, 28, "data")) != PHASES + EXIT_PHASES:
        raise ValueError("type-2 phase tables disagree")
    result = []
    for key, origin in enumerate(origins):
        start = BASE + STRIDE * key
        raw = image.read(start, STRIDE, "data")
        baked = reaction_emit.signed_byte(raw[2])
        bearing, flip = raw[28], reaction_emit.signed_byte(raw[29])
        speed, delta, cap = struct.unpack_from(">3f", raw, 44)
        if (baked >= reaction_emit.GROUPS or flip not in (-1, 1)
                or any(not isfinite(v) or not 0 <= v <= 1e5 for v in (speed, delta, cap)) or speed > cap):
            raise ValueError(f"type-2 unsupported operand at key {key}")
        helper = {0x00: "never", 0x80: "when-target-present", 0xC0: "even-without-target"}.get(bearing & 0xC0)
        axis = None if helper == "even-without-target" else (bearing & 0x7F)
        if helper is None or (axis is not None and axis >= 3):
            raise ValueError(f"type-2 unknown direction operands at key {key}")
        result.append({
            **origin, "row": address(start), "collision_group_selector": baked,
            "bearing_control": bearing, "bearing_helper": helper, "matrix_axis_if_no_helper": axis,
            "reverse_matrix_axis": flip < 0 if axis is not None else None,
            "initial_speed": speed, "speed_add_per_active_visit": delta, "speed_cap": cap,
        })
    if len(result) != COUNT:
        raise ValueError("type-2 table extent")
    return result


def recover(image, archive, atlas, selections):
    origins = origin_tables(image)
    rows = carrier_rows(image, origins["type-2"])
    counts, sites, candidates = candidate_census(image, archive, atlas, selections, origins, "type-2", FACTORY, rows)
    counts.update({
        "speed_triplets": len({(r["initial_speed"], r["speed_add_per_active_visit"], r["speed_cap"]) for r in rows}),
        "bearing_helpers": dict(Counter(r["bearing_helper"] for r in rows)),
    })
    return {
        "kind": "gf-retail-type2-roster", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
        "factory": address(FACTORY), "phase_table": address(DISPATCH), "phase_entries": list(map(address, PHASES)),
        "exit_subphase_entries": list(map(address, EXIT_PHASES)), "counts": counts,
        "factory_rows": rows, "call_sites": sites, "candidates": candidates,
        "notes": [
            "Selected profiles come from bounded standalone installer execution; the calls associated with those profiles are static candidates.",
            "An adjacent li is a local source observation, not argument proof at every branch predecessor.",
            "Normal owner descriptors describe raw power only. Combined-form and alternate-spec joins are withheld; no route is admitted to gameplay.",
            "Direction classification describes the bearing branch after any target validation. It does not qualify a camera, aim target, or rendered muzzle.",
            "Only Machine Red key 21 has the accompanying joined lifetime execution. Other keys do not inherit its result.",
        ],
    }


def export(dol, afs, atlas, selections, output):
    return export_census(dol, afs, atlas, selections, output, recover)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "atlas", "selections", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.atlas, args.selections, args.output), sort_keys=True))
