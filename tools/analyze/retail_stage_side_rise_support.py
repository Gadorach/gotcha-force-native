"""Join original side prop rebind and automatic rise to Sasuke support carry."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_stage_side_support as support


life = support.life
side = support.side
body = support.body
carry = side.recovery.prop.carry
SUPPORT_SHA256 = "bcc4123f1e9f250fed5315223f5ad10beb4ae4aea2ef823d1ca15ded9bf169e1"
LIFE_SHA256 = "9a20065e42df380a544526cd0e230b53aa1b764ec7e162ce248928859f779b36"
TRACE_SHA256 = "e907f34e29ddfff84b99586c20f93f9a7f4d77ed1256756f26a355f0d8d31294"


def source_poses(payload, members):
    poses = {}
    start = 21504
    for member, _, _, _, placements in members:
        key, size = struct.unpack_from(">II", payload, start)
        if key != member or not 6144 <= size <= 1 << 18 or start+8+size > len(payload):
            raise ValueError("side rise support payload boundary")
        start += 8
        for i in range(len(placements)):
            y, _, pitch, yaw, roll = struct.unpack_from(">5f", payload, start+2052+28*i+8)
            if pitch or roll:
                raise ValueError("side rise support placement is not upright")
            poses[member, i] = y, yaw
        start += size
    if start != len(payload):
        raise ValueError("side rise support payload suffix")
    return poses


def decode(report, members, types, poses, collision):
    lines = report.splitlines()
    if len(report) > 1 << 18 or len(lines) != 1241:
        raise ValueError("side rise support report extent")
    life_lines = []
    archive_rows = []
    pos = 0
    kinds = Counter()
    lateral = 0
    bits = support.bits
    scalar = carry.stage.scalar
    f32 = carry.stage.f32

    def take(tag, fields, key):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"side rise support changed: {key}/{tag}")
        pos += 1

    for member, name, _, _, placements in members:
        end = pos+2+len(placements)+2*(member == 2429)
        life_lines.extend(lines[pos:end])
        pos = end
        owners = [(i, t, types[t]["collision_shapes"]) for i, (t, _, variant, _) in enumerate(placements) if not variant]
        allotted = sum(len(shapes) for _, _, shapes in owners)
        used = 0
        rows = []
        for i, t, shapes in owners:
            start = used
            used += len(shapes)
            if types[t]["break_duration_operand"] != 600:
                continue
            # The initial pass allocates by placement order. The timed rebind
            # walks the bucket in reverse and consumes the remaining unused cells.
            index, shape = next((j, s) for j, s in enumerate(shapes) if s["descriptor_role"] & 0x20)
            old_cell, fresh_cell = start+index, allotted+(allotted-used)+index
            d, kind = shape["descriptor"], shape["shape_kind"]
            kinds[kind] += 1
            key = member, i, d
            y, yaw = poses[member, i]
            off = 0x400+80*d
            center_y = struct.unpack_from(">f", collision, off+16)[0]
            surface_depth = struct.unpack_from(">f", collision, off+(24 if kind == 4 else 28))[0]
            anchor_y = f32(center_y+surface_depth)
            offset = -f32(100+scalar(types[t]["radius_bits"]))
            rate = f32(-offset/60)
            owner_y = f32(y+offset)
            center = bits(owner_y+center_y)
            surface = f32(owner_y+anchor_y)
            root_offset = 2**-16 if kind == 4 else 0
            actor = f32(surface+root_offset)
            take("side-ride", (*key, old_cell, fresh_cell, 255 if kind == 4 else 0, 1, bits(actor), bits(surface),
                              bits(anchor_y), bits(owner_y), bits(owner_y), center, bits(rate)), key)
            xzs = []
            for step in (1, 2):
                previous_y, previous_actor = owner_y, actor
                offset = f32(offset+rate)
                owner_y = f32(y+offset)
                actor = f32(f32(previous_actor-previous_y)+owner_y)
                queried = f32(anchor_y+owner_y)
                if actor != queried:
                    raise ValueError(f"side rise carry and anchor predictions diverged: {key}/{step}")
                drift = "B8800000" if key == (2443, 1, 1) and step == 1 and yaw else "00000000"
                lateral += drift != "00000000"
                xzs.append([drift, "00000000"])
                mask = (bits(previous_actor), bits(queried), bits(queried), 1) if step == 1 else (
                    "00000000", "00000000", "00000000", 0,
                )
                take("side-rise-step", (member, i, step, bits(owner_y), bits(previous_y), bits(owner_y),
                     center if step == 1 else bits(owner_y+center_y), bits(previous_actor), bits(actor), bits(queried),
                     *mask, *xzs[-1]), key)
            rows.append({
                "placement": i, "type": t, "descriptor": d, "shape_kind": kind,
                "retired_support_cell_ordinal": old_cell, "rebound_support_cell_ordinal": fresh_cell,
                "initial_collision_root_above_query_bits": bits(root_offset),
                "source_rise_rate_bits": bits(rate), "unmasked_actor_lateral_delta_bits": xzs,
            })
        end = pos+1+len(owners)
        life_lines.extend(lines[pos:end])
        pos = end
        archive_rows.append({"member": member, "filename": name, "probes": rows})
    if pos != len(lines) or kinds != {1: 88, 4: 84} or lateral != 1:
        raise ValueError("side rise support coverage changed")
    prior = life.decode("\n".join(life_lines), members, types)
    return archive_rows, kinds, prior


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("side rise support output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    support.verify_source(image)
    life.verify_source(image)
    carry.verify_source(image)
    prior = body.read_metadata(body.MAPS / "gg4e08-stage-side-recovery.json", LIFE_SHA256)
    spatial = body.read_metadata(body.MAPS / "gg4e08-stage-side-support.json", SUPPORT_SHA256)
    body.read_metadata(body.MAPS / "gg4e08-support-carry.json", side.recovery.prop.CARRY_SHA256)
    payload, members, types = side.source_inputs(image, afs)
    poses = source_poses(payload, members)
    collision = bytes(payload[:21504])
    payload[21504:21504] = support.anchor.plane.source_inputs(afs, include_actor=True)
    report = body.fixture_report(fixture, dol, module, ("--rise-support",), payload)
    rows, kinds, (recovery_rows, isolation, _, _) = decode(report, members, types, poses, collision)
    if recovery_rows != prior["archives"] or isolation != prior["isolation_case"]["checkpoints"]:
        raise ValueError("side rise support changed the original recovery replay")
    for a, s in zip(rows, spatial["archives"], strict=True):
        support_keys = {(t["placement"], t["descriptor"]) for t in s["targets"]}
        if a["member"] != s["member"] or any((t["placement"], t["descriptor"]) not in support_keys for t in a["probes"]):
            raise ValueError("side rise support changed the prior static target identity")
    trace = sha256(report.encode("ascii")).hexdigest()
    if trace != TRACE_SHA256:
        raise ValueError("side rise support original trace changed")
    raw = body.encode({
        "kind": "gf-retail-stage-side-rise-support", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "stage_side_support_sha256": SUPPORT_SHA256,
        "stage_side_recovery_sha256": LIFE_SHA256, "support_carry_sha256": side.recovery.prop.CARRY_SHA256,
        "observed_trace_sha256": trace, "declared_break_hp": 0, "rebind_visit": 600,
        "actor": "gg4e08:000a", "actor_seat": 0, "actor_update_stop_before": "80059E58",
        "probe_counts": {"box": kinds[1], "plane": kinds[4], "no_rebind_terminal_owners": 8},
        "unmasked_rise_steps_checked": 2*sum(kinds.values()), "masked_first_steps_checked": sum(kinds.values()),
        "first_rise": {"allocated_center_held": True, "owner_matrix_moves": True, "carry_matches_height": True,
                       "masked_carry_root_held": True, "masked_landing_matches_height": True},
        "second_rise": {"allocated_center_catches_up": True, "carry_matches_height": True},
        "archives": rows,
        "notes": [
            "The original recovery replay constructs and breaks each full archive cohort with declared zero HP. A separate fork at rebind poses Sasuke above one fresh support cell at a time; original collision, not the harness, writes the supported cell, owner and local anchor.",
            "The 32-cell test pool has unused cells available before rebind, so every tested timed owner receives a different address in this setup. This is not a general rule about recycled addresses or ownership.",
            "The first prop-only rise visit advances the original owner root and matrix while the allocated support center holds. The second visit catches that center up. The actor update prefix carries both geometry kinds on each visit without moving the allocated receiving capsule during the prefix.",
            "On the first visit a declared nonzero scene mask blocks carry for the unsponsored prop. A separate masked height and landing fork still follows the raised matrix. Both full-carry steps match the supported-height query exactly.",
            "At rebind, the collision-corrected root on the tested planes starts 2^-16 units above the anchor query, unlike the tested boxes. One yawed box exhibits a tiny X rounding delta during the original transform; actor yaw stays unchanged.",
            "The fork does not rerun collision between the two rise steps or continue a full actor or scene tick. No damage admission, physical controls, actual retail terrain, drawing, audio, hardware or native gameplay is qualified.",
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
