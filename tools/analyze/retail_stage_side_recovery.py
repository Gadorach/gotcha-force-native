"""Verify break, timed return and terminal retirement across all retail side props."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_stage_side_census as side


body = side.body
CENSUS_SHA256 = "6e4e1137edd8ba111bcf2014668978f26885a5c0f2feb3ea04b89a011b42ae10"
PROP_RECOVERY_SHA256 = "65007c5b48ea9144c8be213e69afe7bd5c0f3b6edc1db4bddb6540b020167b4d"
POST_BREAK_VISITS = 664
TEXT = (
    (0x80081FF8, 72, "c70ccbd26a564fe5294b00a20d97b1009de69478833cf0fc292fa0f223342c5b"),
    (0x80082040, 32, "018352f1e20af5404299323cdafe2cf6302258f23ed5d5f28d9342f17adf4703"),
    (0x800827F4, 48, "993df5a765663cf6902c5c89c4e8b917b4af1e80a3e62470a2898bf377729ae4"),
    (0x80088E50, 328, "c747b919f60eaad0d8b255db2b1fdc9ecb0bd22cea33c4930d1085dcdf34d3f4"),
    (0x80089444, 272, "662fe9d92fabf618cf21cc81fdcc266d090bda7b674bc9ad28266a89be6e2bf2"),
    (0x80089814, 132, "1bb689f99364d17b27d89cf9d15ceb89ffde7be29dc90d4bba30b495975710dd"),
    (0x80006D80, 72, "af3ef03d2ff4dfbd26b5e33794ffac72543f7b6b70aaa2a0c7f7af14cdf13768"),
    (0x8008AFF0, 32, "8ea8cac06c446278fad05cfae1dbe3ee6b3a321f4bfd1168269ce88283bef951"),
    (0x80197AD8, 188, "e3650544ce98694f5c3a5ca208c4dfe1479fbc02f46712a3a8a75d8d210c9082"),
)


def verify_source(image):
    body.verify_text(image, TEXT)
    for site, target in ((0x80081FC4, 0x800827F4), (0x80082020, 0x8008AFF0), (0x8008204C, 0x80088E50),
                         (0x80088F3C, 0x80089444), (0x80088F58, 0x80006D80), (0x80088F78, 0x80089814),
                         (0x80006DA0, 0x8024FD74), (0x80006DB0, 0x80197AD8)):
        body.link(image, site, target)


def rise(t):
    """Expected single-precision rate and first settled step from pinned operands."""
    if t["break_duration_operand"] == -1:
        return 0, "00000000"
    f32 = side.recovery.stage.f32
    offset = -f32(100 + side.recovery.stage.scalar(t["radius_bits"]))
    rate = f32(-offset/60)
    for step in range(1, 64):
        offset = f32(offset+rate)
        if offset >= 0:
            return step, struct.pack(">f", rate).hex().upper()
    raise ValueError("stage side recovery rise extent")


def decode(report, members, types):
    lines = report.splitlines()
    if len(report) > 1 << 16 or len(lines) != 725:
        raise ValueError("stage side recovery report extent")
    pos = 0
    census_lines = []
    rows = []
    settle_counts = Counter()
    isolation = None
    rise_by_type = [rise(t) for t in types]

    def take(tag, fields, label):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"stage side recovery changed: {label}/{tag}")
        pos += 1

    for member, name, _, _, placements in members:
        end = pos+2+len(placements)
        census_lines.extend(lines[pos:end])
        pos = end
        if member == 2429:
            isolation = []
            for visit in (0, 1):
                take("side-isolation", (visit, 1, 1-visit, 1, 100, 4), name)
                isolation.append({
                    "visits_after_break": visit, "type4_mask": 1, "broken_companion_enabled": visit == 0,
                    "unbroken_companion_enabled": True, "unbroken_owner_hp": 100, "ordinary_receivers": 4,
                })
        timed = [types[t] for t, _, variant, _ in placements if not variant and types[t]["break_duration_operand"] >= 0]
        retired = sum(not variant for _, _, variant, _ in placements)-len(timed)
        bound = [s for t in timed for s in t["collision_shapes"]]
        common = sum(bool(s["descriptor_role"] & 0x20) for s in bound)
        take("side-recovery", (member, len(timed), retired, len(bound), common), name)
        owners = []
        for i, (t, bit, variant, _) in enumerate(placements):
            if variant:
                continue
            companions = [j for j, r in enumerate(placements) if r[:3] == (t, bit, 1) and j > i]
            if len(companions) != 1:
                raise ValueError(f"stage side recovery companion mapping changed: {name}/{i}")
            companion, = companions
            duration = types[t]["break_duration_operand"]
            count = len(types[t]["collision_shapes"])
            steps, rate = rise_by_type[t]
            if duration >= 0:
                rebind, settle, show, retirement = duration, duration+steps, duration+steps+1, 0
                settle_counts[steps] += 1
            else:
                rebind = settle = show = 0
                retirement = 2
            take("side-life", (member, i, companion, count, rebind, settle, 1, show, rate, retirement), f"{name}/{i}")
            owners.append({
                "placement": i, "type": t, "companion_placement": companion, "mask_bit_index": bit,
                "shape_cells_returned_on_break": count, "break_phase": 1 if rebind else 2,
                "companion_disable_visit": 1, "collision_rebind_visit": rebind or None,
                "rise_rate_bits": rate, "rise_steps": steps, "settled_visit": settle or None,
                "companion_enable_visit": show or None, "owner_return_visit": retirement or None,
                "model_descriptors_returned": 2 if retirement else 0,
            })
        rows.append({
            "member": member, "filename": name, "rebound_owners": len(timed), "retired_owners": retired,
            "rebound_shape_cells": len(bound), "owners": owners,
        })
    if pos != len(lines) or settle_counts != {60: 64, 61: 108} or isolation is None:
        raise ValueError("stage side recovery coverage changed")
    original = side.decode("\n".join(census_lines), members, types)
    return rows, isolation, settle_counts, original


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stage side recovery output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    prior = body.read_metadata(body.MAPS / "gg4e08-stage-side-census.json", CENSUS_SHA256)
    body.read_metadata(body.MAPS / "gg4e08-stage-prop-recovery.json", PROP_RECOVERY_SHA256)
    payload, members, types = side.source_inputs(image, afs)
    report = body.fixture_report(fixture, dol, module, ("--recovery",), payload)
    rows, isolation, settle_counts, initial = decode(report, members, types)
    if json.loads(json.dumps(initial)) != prior["archives"] or types != prior["types"]:
        raise ValueError("stage side recovery initial census changed")
    summary = {
        "rebound_owners": sum(row["rebound_owners"] for row in rows),
        "retired_owners": sum(row["retired_owners"] for row in rows),
        "rebound_shape_cells": sum(row["rebound_shape_cells"] for row in rows),
        "returned_model_descriptors": sum(o["model_descriptors_returned"] for row in rows for o in row["owners"]),
        "rise_step_counts": dict(settle_counts),
    }
    raw = body.encode({
        "kind": "gf-retail-stage-side-recovery", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "stage_side_census_sha256": CENSUS_SHA256,
        "stage_prop_recovery_sha256": PROP_RECOVERY_SHA256, "declared_break_hp": 0, "break_sound_request": 115,
        "post_break_bucket_visits_per_archive": POST_BREAK_VISITS, "break_handler": "80081DE4",
        "terminal_phase_advance": "80081FF8", "terminal_retire_handler": "80082040",
        "companion_mask_reader": "800827F4", "summary": summary, "archives": rows,
        "isolation_case": {"member": 2429, "broken_owner_placement": 0, "unbroken_same_type_placement": 1,
                           "checkpoints": isolation},
        "notes": [
            "Each full archive cohort starts from the previously verified original side construction and publication. Only collision-owner HP is set to zero; the original bucket, world, and shape walks own all later writes.",
            "All original prop collision cells close on the break visit. The 600-duration rows restore 100 HP and rebind on the 600th later visit, then settle at the authored position in 60 or 61 single-precision rise steps.",
            "The -1-duration rows enter phase 2 on break, phase 3 on the first later visit, and return the original owner and both model descriptors on the second. Callback-list removal, free-list identities, cleared model-root fields and no rebind through visit 664 are checked separately.",
            "Model-only objects are not retired or rebound. The earlier traversal of their matching type/mask bit delays observation by one visit. Terminal companions remain disabled in the replay. The single-break fork leaves a same-type sibling and its different mask bit unchanged.",
            "The break cue is a request, not playback. There is no battle damage admission, draw, live stage selection, full scene scheduling, other-variant support, hardware or native gameplay in this census.",
        ],
    }, expanded=("archives",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "archives": len(rows), **summary}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
