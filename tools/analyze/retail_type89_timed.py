"""Replay the type-89 carrier's timed stage probe and root-position exit."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re

from tools.analyze import retail_type89_area as area


TERRAINS = ("empty-stage", "intersecting-wall", "offset-wall")
VISITS_PER_UNIT = (1, 2)
ROW = re.compile(r"type89-timed:(?: [0-9]{1,5}){11}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(area.PILOTS)*len(TERRAINS)*len(VISITS_PER_UNIT):
        raise ValueError("type-89 timed report extent")
    pilots = []
    line_i = 0
    for model, key, *_ in area.PILOTS:
        cases = []
        for scale in VISITS_PER_UNIT:
            for case, terrain in enumerate(TERRAINS):
                line = lines[line_i]
                line_i += 1
                if ROW.fullmatch(line) is None:
                    raise ValueError("malformed type-89 timed row")
                _, _, _, middle, probe, exit_visit, dispose, latch, z, request, area_key = observed = tuple(
                    map(int, line.split()[1:]))
                wall = terrain == "intersecting-wall"
                expected_z = 1000 if wall else 10383 if key == 11 else 10343
                if observed != (model, case, scale, 25*scale, 31*scale, 31*scale+1, 41*scale,
                                int(wall), expected_z, 31*scale+1 if wall else 0, 16 if wall else 0):
                    raise ValueError(f"type-89 timed mismatch: {model:04X}/{terrain}/{scale}")
                cases.append({
                    "stage": terrain, "owner_clock_units_per_visit": 1/scale, "middle_phase_visit": middle,
                    "stage_probe_visit": probe, "stage_latch": bool(latch), "root_z_integer_part_after_probe": z,
                    "exit_leaf_visit": exit_visit, "phase_two_visit": dispose,
                    "area_allocation": {"visit": request, "site": "8016D32C", "key": area_key,
                                        "root": [100, 200, z]} if request else None,
                })
        pilots.append({"borg": f"gg4e08:{model:04x}", "carrier_key": key, "cases": cases})
    return pilots


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-89 timed output already exists")
    report, _, schedule = area.replay(dol, afs, fixture, module, timed=True)
    pilots = decode_report(report)
    payload = {
        "kind": "gf-retail-type89-timed", "revision": 1,
        "source_dol_sha256": area.SOURCE_SHA256, "source_afs_sha256": area.AFS_SHA256,
        "schedule": schedule, "pilots": pilots,
        "notes": [
            "The original constructor is called directly with a declared owner pose. Visit 0 is the full initializer.",
            "Subsequent visits call the original carrier update. The fixture never seeds the area key, exit subphase, stage latch, or contact latch.",
            "Stage geometry is fixture-authored: empty or one rebased triangle at z=1000, with an offset triangle as a miss control.",
            "The exit leaf runs on the next update after the probe. Only the intersecting stage produces the alternate area request.",
            "These are carrier-update counts, not game frames. The replay does not schedule actors, collision passes, or the area update.",
            "Observed allocation does not qualify beam overlap, area activation, damage, disposal completion, or native admission.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(pilots),
            "cases": len(pilots)*len(TERRAINS)*len(VISITS_PER_UNIT)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
