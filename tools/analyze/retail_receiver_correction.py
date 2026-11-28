"""Verify original body correction, center publication, and narrow scratch offsets."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_moving_receiver as moving


s = moving.s
MOTION_SHA256 = "9a160963eb3dfab7d57f15aa216e1b741a3b0e733d785a314e56802518164892"
TEXT = (
    (0x80035CA4, 436, "eb751ef6a9fc320545b707c87bef69b1e814c6a581ee69033bd92e8b66815461"),
    (0x8002CE44, 76, "18b249a49dc819bc3e9d29548241e69cba1aa2a3eced9dc011c807dac7a325bd"),
    (0x8002CE90, 240, "59dfbede2aa96fbce042cfd535cc233fb16df309ad4c7d5f30f8ac50afcb567e"),
)
CASES = (
    ("capsule-outside-control", 1, 78, ()),
    ("capsule-inward", 1, 78, (-2,)),
    ("capsule-outward", 1, 76, (2,)),
    ("capsule-inward-past-broad-gate", 1, 100, (-10, -14)),
    ("sphere-outward", 0, 91, (2,)),
    ("sphere-inward", 0, 93, (-2,)),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 2*len(CASES):
        raise ValueError("receiver correction report extent")
    bits = moving.sweep.pair.capsule.geometry.bits
    rows = []
    for i, (name, capsule, distance, steps) in enumerate(CASES):
        correction = sum(steps)
        root = 150+distance+correction
        for apply_center in (0, 1):
            center = 150+distance+apply_center*correction
            broad = center-150 <= 92
            narrow_separation = distance+correction if capsule else center-150
            accepted = broad and narrow_separation < (77 if capsule else 92)
            damage = 17 if accepted else 0
            fields = (i, apply_center, capsule, distance, correction, int(bool(steps)),
                      *(bits(x) for x in (root, correction, center, 22)),
                      int(broad), int(accepted), int(accepted), damage, 180-damage, int(accepted))
            if lines[len(rows)] != "correction-pair: " + " ".join(map(str, fields)):
                raise ValueError(f"receiver correction geometry, narrow test or HP changed: {name}/{apply_center}")
            rows.append({
                "case": name, "receiver_shape": "capsule" if capsule else "sphere", "receiver_descriptor": 11 if capsule else 9,
                "declared_writer_y_operands": list(steps), "body_center_pass_run": bool(apply_center),
                "root_y": root, "owner_correction_y": correction, "original_correction_flag": bool(steps),
                "broad_center_y": center, "broad_field": 22, "broad_phase_accepted": broad,
                "allocated_capsule_endpoint_y": 150+distance if capsule else None,
                "scratch_capsule_endpoint_y": root if broad and capsule else None,
                "scratch_center_y": center if broad else None,
                "narrow_accepted": accepted if broad else None, "hp_removed": damage,
                "hp_after": 180-damage, "accepted_token": 1 if accepted else None,
            })
    return rows


def verify_source(image):
    moving.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x80035CC8, 0x8020BA78), (0x80035CD8, 0x8020BA54), (0x80035CE8, 0x8020BA54),
                         (0x8002CE58, 0x8002CE90), (0x8002CF14, 0x8020BA54)):
        s.link(image, site, target)


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("receiver correction output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-moving-receiver.json", MOTION_SHA256)
    payload, tau = moving.sweep.source_inputs(image, afs)
    cases = decode(s.fixture_report(fixture, dol, module, ("--receiver-correction",), payload))
    raw = s.encode({
        "kind": "gf-retail-receiver-correction", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "moving_receiver_sha256": MOTION_SHA256,
        "attack": tau, "receiver": "gg4e08:000a", "attack_center_y": 150,
        "original_correction_writer": "80035CA4", "receiver_center_pass": "8002CE90", "center_bucket": 50,
        "capsule_scratch_preparation": "8003A260", "cases": cases,
        "notes": [
            "The original writer consumes declared Y displacements after one original registry rebuild. It updates the receiver's root and the accumulated +0x1CC vector. No linked actor propagation is admitted.",
            "The targeted bucket-50 center pass consumes the original correction flag and vector. It adjusts the broad center without changing the allocated capsule endpoints or broad field.",
            "If broad phase admits a capsule, its narrow scratch preparation adds the receiver offset to the endpoints only. The ordinary pair does not store that scratch adjustment back into the allocated cell.",
            "Skipping the bucket pass deliberately leaves broad phase on the old center. This is a scoped control, not an inferred original frame ordering.",
            "These stationary matrices and declared corrections do not qualify how stage or body collisions choose the vector, linked actors, hardware, or native gameplay.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "hp_writes": sum(row["accepted_token"] is not None for row in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
