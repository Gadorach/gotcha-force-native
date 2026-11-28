"""Replay original scheduled contact after two side-prop rise visits."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_stage_side_rise_support as rise


side = rise.side
life = rise.life
body = rise.body
RISE_SHA256 = "7dfba8cf6bb247be17650627b3117a0e3a7661c8db92936e73a628a12f11af1f"
TEXT = (
    (0x80029434, 320, "6b951fb1955f3acb931beff0831af002d9b6ff9b108a15796a3114b5020b1b6c"),
    (0x800295E0, 104, "ea7f58f422908a1d95349dc6455e5286c96bb5d3d3622dfbe82fb74622d76d0e"),
    (0x80029708, 104, "7b2ca5fee43c5361a7bf5f3bcd932c48eaaba542f5cff70e45a2f16e2368050c"),
    (0x80053A2C, 104, "1a8719769697cc0f7a8a1a711983175c6b9823e6c5e080885fa83dd1430cb177"),
    (0x80087CF0, 60, "0a36f0211bae8654c9502654856eef93e12a2084ccdbe2ad79665ce4c6c21e72"),
    (0x80087F20, 4, "f332ea5b5437103cbb6f1508679da89eec9288ad775c96c439a17fccabe3de8e"),
    (0x8002BB14, 3712, "9aa80c3f876242ebee4cbf8afd5ede6938e90b767f8da7f4f8d9ab6d4fad1911"),
    (0x8005A580, 1896, "061ebcdfdafeca5f6a522c2dc2a792f7247ac7e6747f6413d1d862a5efad71cf"),
    (0x800466DC, 68, "72098342709fd2555748d52181032a40ebe00cc3fa80a53b88243913fbf6530a"),
)
CASES = (
    (0, 237, 11, "C456E4A4 C3659290 C2B3251E 44408000", "C456E4A4 C456E4A4 4167E2D1 C2B32520 C2B32520", (
        "C4534519 C3659290 C2B3251E C2B3251E C2B3251E C2B3251E 443CE075 C2B32520 C2B3251E C2B3251E",
        "C44FA58E C3489638 C2B3251E C2B3251E C2B3251E C27258DE 44408000 C27258E0 C27258DE C27258DE",
        "C44FA58E C3489638 C26258E0 C26258E0 C2B3251E C27258DC 44408000 C27258E0 C27258DC C27258DC",
        "C44FA58E C3489638 C2B3251E C2B3251E C2B3251E C2B3251E 00000000 00000000 00000000 C2B3251E",
        "C44FA58E C3489638 C26258E0 C26258E0 C26258E0 C27258DC 44408000 C27258E0 C27258DC C27258DC",
    )),
    (4, 238, 7, "C402BA48 C39AF490 C2B3D240 43D88000", "C402BA48 C402BA48 410E1C09 C2B3D240 C2B3D240", (
        "C40081D8 C39AF490 C2B3D240 C2B3D240 C2B3D240 C2B3D240 43D40F20 C2B3D240 C2B3D240 C2B3D240",
        "C3FC92D0 C39212D0 C2B3D240 C2B3D240 C2B3D240 C2904B40 43D88000 C2904B40 C2904B40 C2904B40",
        "C3FC92D0 C39212D0 C2884B40 C2884B40 C2B3D240 C2904B40 43D88000 C2904B40 C2904B40 C2904B40",
        "C3FC92D0 C39212D0 C2B3D240 C2B3D240 C2B3D240 C2B3D240 00000000 00000000 00000000 C2B3D240",
        "C3FC92D0 C39212D0 C2884B40 C2884B40 C2884B40 C2904B40 43D88000 C2904B40 C2904B40 C2904B40",
    )),
)
VISITS = ((1, 0, "retained-first-rise"), (2, 0, "retained-old-pose"), (2, 1, "retained-new-pose"),
          (2, 2, "fresh-old-pose"), (2, 3, "fresh-new-pose"))


def verify_source(image):
    rise.support.verify_source(image)
    life.verify_source(image)
    rise.carry.verify_source(image)
    body.verify_text(image, TEXT)
    for site, target in (
        (0x8002947C, 0x80089A94), (0x80029488, 0x80053A2C), (0x800294C8, 0x80089554),
        (0x800294F4, 0x8002BB14), (0x80029618, 0x80029708), (0x80029718, 0x80089A94),
        (0x80029724, 0x80087CF0), (0x80029728, 0x80089554), (0x8002973C, 0x8008A28C),
        (0x80029740, 0x80089AEC), (0x80029744, 0x8002BB14), (0x80029748, 0x80036E24),
        (0x8002C86C, 0x8005A580), (0x8002C888, 0x800466DC), (0x8005ACC8, 0x800677B0),
    ):
        body.link(image, site, target)
    if image.read(0x80433918, 8, "data") != bytes.fromhex("80087D2C80087F20"):
        raise ValueError("scheduled side support stage dispatch table changed")


def decode(report, members, types):
    lines = report.splitlines()
    if len(report) > 1 << 16 or len(lines) != 739:
        raise ValueError("scheduled side support report extent")
    pos = sum(3+len(m[-1])+sum(not r[2] for r in m[-1]) for m in members if m[0] < 2429)
    member = next(m for m in members if m[0] == 2429)
    pos += 4+len(member[-1])  # This archive's construction and two isolation rows.
    prior_lines = lines[:pos]
    rows = []
    scalar = rise.carry.stage.scalar
    bits = rise.support.bits

    def take(tag, fields, name):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"scheduled side support changed: {name}/{tag}")
        pos += 1

    for placement, d, cell, inlet, before_world, observed in CASES:
        take("side-scheduled-inlet", (placement, d, cell, *inlet.split()), placement)
        take("side-before-world", (placement, *before_world.split()), placement)
        base_owner, base_center, base_actor, base_anchor = inlet.split()
        _, _, rate, early_actor, early_query = before_world.split()

        def relative(value, base):
            return bits(scalar(value)-scalar(base))

        visits = []
        for (step, mode, name), dump in zip(VISITS, observed, strict=True):
            owner, center, start, sphere, previous_sphere, root, anchor, height, landed, final = dump.split()
            held = mode != 2
            previous = mode < 2
            isolated_landing = 1 if held and d == 238 else 0
            frontier = "800466DC" if held else "8005A580"
            take("side-scheduled-step", (placement, step, mode, int(held), int(previous), 1, 17+step,
                 *dump.split(), isolated_landing, frontier), f"{placement}/{name}")
            visits.append({
                "case": name, "rise_step": step, "counter_after": 17+step,
                "previous_support_latch": previous, "current_exact_support": held, "target_narrow_calls": 1,
                "owner_rise_bits": relative(owner, base_owner), "allocated_center_rise_bits": relative(center, base_center),
                "declared_actor_rise_bits": relative(start, base_actor),
                "sphere_current_y_from_initial_actor_bits": relative(sphere, base_actor),
                "sphere_previous_y_from_initial_actor_bits": relative(previous_sphere, base_actor),
                "actor_rise_after_contact_bits": relative(root, base_actor),
                "local_anchor_y_change_bits": relative(anchor, base_anchor) if held else None,
                "supported_height_from_initial_actor_bits": relative(height, base_actor) if held else None,
                "isolated_landing_return": isolated_landing if held else None,
                "isolated_landed_rise_bits": relative(landed, base_actor) if held else None,
                "actor_rise_at_cut_bits": relative(final, base_actor), "stop_before": frontier,
            })
        rows.append({
            "placement": placement, "descriptor": d, "rebound_cell_ordinal": cell, "source_rise_rate_bits": rate,
            "before_world": {"owner_root_held": True, "bucket_advanced_local_offset": True, "carry_admitted": True,
                             "actor_rise_bits": relative(early_actor, base_actor),
                             "height_from_initial_actor_bits": relative(early_query, base_actor)},
            "visits": visits,
        })
    prior = life.decode("\n".join(prior_lines+lines[pos:]), members, types)
    return rows, prior


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("scheduled side support output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    previous = body.read_metadata(body.MAPS / "gg4e08-stage-side-rise-support.json", RISE_SHA256)
    prior = body.read_metadata(body.MAPS / "gg4e08-stage-side-recovery.json", rise.LIFE_SHA256)
    payload, members, types = side.source_inputs(image, afs)
    payload[21504:21504] = rise.support.anchor.plane.source_inputs(afs, include_actor=True)
    report = body.fixture_report(fixture, dol, module, ("--scheduled-support",), payload)
    rows, (recovery_rows, isolation, _, _) = decode(report, members, types)
    if recovery_rows != prior["archives"] or isolation != prior["isolation_case"]["checkpoints"]:
        raise ValueError("scheduled side support changed the original recovery replay")
    observed = next(a for a in previous["archives"] if a["member"] == 2429)
    if any(not any((p["placement"], p["descriptor"], p["rebound_support_cell_ordinal"]) ==
                   (s["placement"], s["descriptor"], s["rebound_cell_ordinal"]) for p in observed["probes"]) for s in rows):
        raise ValueError("scheduled side support lost the verified rebound identities")
    raw = body.encode({
        "kind": "gf-retail-stage-side-scheduled-support", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "stage_side_rise_support_sha256": RISE_SHA256,
        "stage_side_recovery_sha256": rise.LIFE_SHA256, "actor": "gg4e08:000a", "side_member": 2429,
        "normal_caller": "80029434", "normal_actor_dispatch_cut": "80053A2C", "normal_world_call_site": "800294C8",
        "alternate_caller": "80029708", "alternate_parent": "800295E0", "declared_stage_phase": 1,
        "stage_dispatch_target": "80087F20", "observed_trace_sha256": sha256(report.encode("ascii")).hexdigest(),
        "cases": rows,
        "notes": [
            "Exact original set0202 side construction and declared-zero-HP break run through visit 600. Sasuke uses declared rigid joints and actual collision to acquire the rebound plane or box. Stage bounds and phase are declared; the full retail arena is not loaded.",
            "The active normal caller runs to its actor dispatcher. Group 40 has advanced the local rise offset while previous and current world roots remain equal. An independent carry-prefix probe on that state admits carry but does not lift Sasuke. The normal actor dispatcher is not resumed.",
            "The alternate original updater does not call the actor updater. It runs group 40, stage/world, counter, shape publication and collision. On its first rise the allocated center holds and fresh contact stores a lower local anchor against the raised matrix, keeping the actor at the old height.",
            "On the second rise the allocated center catches up. The retained-support actor moves up in collision even at the old pose. A fresh actor at identical current and previous sphere Y reaches the same narrow kernel but acquires no support; the fresh near-top control does acquire it.",
            "Supported visits reach the original model-refresh boundary with the same Y as an isolated landing probe. No landing helper execution is inferred from a missing observation or an unchanged root. Unsupported visits stop before actor presentation and do not exercise the absent-support floor fallback.",
            "No normal frame continuation, model refresh, physical control, full battle scheduling, original terrain, hardware or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "targets": len(rows), "visits": sum(len(row["visits"]) for row in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
