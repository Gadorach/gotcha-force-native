"""Replay the original collision walk from entry to its presentation call."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_stage_correction as stage


s = stage.s
STAGE_SHA256 = "f713164be040a48570eebebdf3236f0b5507fd9a5049b6c78c3a5249c4ac2a19"
TEXT = (
    (0x8002BB14, 3712, "9aa80c3f876242ebee4cbf8afd5ede6938e90b767f8da7f4f8d9ab6d4fad1911"),
    (0x8002CDA4, 160, "2dcecd916811217d411f21faca6a1ca97774fa3c65c5f3756bf92d230096a693"),
    (0x8006F4F8, 276, "9bedc00559917b9e5fa76d02977c59be604bed473605fc394e3df7af750ee800"),
)


def verify_source(image):
    stage.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x8002BBE4, 0x8002CDA4), (0x8002C4C0, 0x8002CE44),
                         (0x8002C4CC, 0x8002DB58), (0x8002C5A0, 0x8005C290),
                         (0x8002C86C, 0x8005A580)):
        s.link(image, site, target)


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 16:
        raise ValueError("stage collision walk report extent")
    bits = stage.pair.capsule.geometry.bits
    pos = 0
    cases = []
    for i, (name, wall, begin, _, attack, root, latch) in enumerate(stage.CASES):
        if attack is None:
            continue
        # Frozen observations for these two attack placements, not generalized
        # distance/guard rules. Probe the original kernel on a separate fork.
        first_hit = i in (7, 10)
        for stale in (0, -64):
            visits = []
            for visit in (1, 2):
                first = visit == 1
                pose_x = begin if first else root
                correction = root - pose_x
                input_vector = stale if first else root - begin
                bound = 22 if first else 484
                blocked = not first and first_hit
                role_mask, guard = (0x43, 60) if blocked else (0, 0)
                narrow = first_hit if first else not blocked
                wrote = first and first_hit
                hp, token = (163, 1) if first_hit else (180, 0)
                endpoints = [pose_x-15, pose_x+15]
                scratch = [x+correction for x in endpoints] if narrow else None
                fields = (i, int(bool(stale)), visit, "8005A580", "8002C870",
                          *(bits(x) for x in (input_vector, bound, root, correction, root, root, *endpoints)),
                          latch, latch, f"{role_mask:04X}", bits(guard), 1, int(not blocked), int(narrow),
                          int(wrote), int(wrote), hp, token, *(bits(x) for x in (scratch or [0, 0])))
                if lines[pos] != "stage-walk: " + " ".join(map(str, fields)):
                    raise ValueError(f"stage collision walk changed: {name}/{stale}/{visit}")
                pos += 1
                visits.append({
                    "visit": visit, "declared_pose_x": pose_x, "pre_walk_correction_x": input_vector,
                    "post_walk_root_x": root, "post_walk_correction_x": correction,
                    "owner_correction_flag": bool(latch), "stage_contact_flag": bool(latch),
                    "stage_sphere_center_x": root, "receiver_broad_center_x": root, "capsule_broad_field": bound,
                    "allocated_capsule_endpoints_x": endpoints, "pre_walk_receiver_role_mask": role_mask,
                    "pre_walk_receiver_guard": guard, "ordinary_attack_entries_on_rebuild": 1,
                    "ordinary_receiver_entries_on_rebuild": int(not blocked),
                    "narrow_capsule_kernel": "80030A3C" if narrow else None,
                    "narrow_scratch_capsule_endpoints_x": scratch,
                    "narrow_overlap": wrote if narrow else None,
                    "hp_write_count": int(wrote), "hp_removed": 17 if wrote else 0,
                    "hp_after": hp, "last_accepted_token": token or None,
                })
            cases.append({
                "case": name, "wall_grid_supplied": bool(wall), "tau_center_x": attack,
                "seeded_stale_correction_x": stale if stale else None, "visits": visits,
            })
    return cases


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stage collision walk output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-stage-correction.json", STAGE_SHA256)
    payload, tau = stage.source_inputs(image, afs)
    cases = decode(s.fixture_report(fixture, dol, module, ("--collision-walk",), payload))
    raw = s.encode({
        "kind": "gf-retail-stage-collision-walk", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "stage_correction_sha256": STAGE_SHA256, "tau_pair_attack": tau,
        "receiver": "gg4e08:000a", "entry": "8002BB14", "actor_reset": "8002CDA4",
        "stage_contact_pass": "8002E1C8", "broad_center_pass": "8002CE44", "ordinary_pair_pass": "8002DB58",
        "hp_writer": "8003D344", "hp_writer_call": "8002F1D8",
        "stop_before": {"call_site": "8002C86C", "entry": "8005A580"},
        "cases": cases,
        "notes": [
            "Uses the previous declared wall or empty grids and the same normal Sasuke shapes and Tau command. The declared caster participates in the shape rebuild, but not the enclosing walk's actor loop.",
            "The original walk resets the incoming correction before contact. The seeded -64 and flags do not change the first-visit result; the wall regenerates +20.",
            "Visit two declares rigid matrices at the root from visit one, then runs the original shape rebuild. The stale +20 is reset, and the wall reports zero displacement at tangent contact. This is not a second original actor tick.",
            "The updated capsule's broad field is 484 instead of 22. Both first-visit misses reach narrow on visit two, but their current endpoints still miss. Scratch and overlap are observed on a fork; HP and the accepted token come from the original walk run to the presentation frontier.",
            "After a first-visit hit, the original reaction leaves mask 0x43 and guard 60 for the next rebuild. Tau's attack remains published, but the ordinary receiver is absent; this is not a test of repeated-token rejection.",
            "The walk stops before presentation. No live stage, full scene update, original hardware, or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "visits": 2*len(cases),
            "hp_writes": sum(v["hp_write_count"] for row in cases for v in row["visits"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
