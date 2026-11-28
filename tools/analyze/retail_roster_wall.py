"""Verify the original wall pass on all normal body queues under declared poses."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_body_registry as body
from tools.analyze import retail_st42_wall as stage


BODY_SHA256 = "63682c1f85334bcdb5565a69191676fba916c7071f95230d340f77e220efac92"
ST42_SHA256 = "591dc9578179deb9a5caa2fecc42523119654e8af805b2ae7ecba83c51d8f9c6"
VARIANTS = (
    ("overlap", 1280, 0, False, False),
    ("role08-masked", 1280, 0, True, False),
    ("empty-secondary", 1280, 0, False, True),
    ("tangent", 1280, 0, False, False),
    ("one-step-out", 1280, 0, False, False),
    ("low-reference-zero", 128, 0, False, False),
    ("low-reference-reset", 128, -10000, False, False),
)
FIELDS = ("bounds_visits_ordinal_and_incoming_x", "output_correction_x", "stage_contact", "shifted_centers")


def verify_source(image):
    body.verify_source(image)
    stage.verify_source(image)
    if image.read(0x80436F6C, 4, "data") != bytes.fromhex("C61C4000"):
        raise ValueError("roster wall reference reset constant changed")


def plans(payload, sources, reviewed):
    stride = 2+body.reaction_emit.HIT_BYTES
    if len(payload) != stride*len(sources) or len(sources) != 206 or len(reviewed["models"]) != len(sources):
        raise ValueError("roster wall source extent")
    rows = []
    for i, ((model, digest, queue), cached) in enumerate(zip(sources, reviewed["models"], strict=True)):
        bank = payload[i*stride+2:(i+1)*stride]
        if payload[i*stride:i*stride+2] != struct.pack(">H", model) or sha256(bank).hexdigest() != digest:
            raise ValueError(f"roster wall source bank: {model:04X}")
        stage_ordinals, skipped_kinds, spheres = [], [], []
        receiver_bits = 0
        for ordinal, shape in reversed(list(enumerate(queue))):
            if shape.roles & 2:
                receiver_bits |= 1 << ordinal
            if not shape.roles & 8:
                continue
            stage_ordinals.append(ordinal)
            if shape.kind == 1:
                skipped_kinds.append(ordinal)
                continue
            dx, dy, dz, radius = struct.unpack_from(">4f", bank, 44+80*shape.descriptor)
            if (shape.kind or dx not in (0, -10) or not -5 <= dy <= 850 or not -310 <= dz <= 510
                    or not 40 <= radius <= 980 or int(radius) != radius):
                raise ValueError(f"roster wall sphere scope: {model:04X}/{ordinal}")
            spheres.append((ordinal, int(dx), dy, int(radius)))
        if (cached["borg"] != f"gg4e08:{model:04x}" or cached["hit_bank_sha256"] != digest or not stage_ordinals
                or cached["observed_capacity_cases"]["32"]["published_queue_ordinals"]["open"]["role-08"] != stage_ordinals
                or len(spheres) > 2):
            raise ValueError(f"roster wall publication disagrees with prior census: {model:04X}")
        rows.append((model, len(queue), stage_ordinals, skipped_kinds, spheres, receiver_bits))
    return rows


def decode(report, rows):
    lines = report.splitlines()
    if len(report) > 512000 or len(lines) != len(rows)*len(VARIANTS):
        raise ValueError("roster wall report extent")
    bits = stage.walk.stage.pair.capsule.geometry.bits
    pos = 0
    models = []
    for model, count, stage_ordinals, skipped, spheres, receiver_bits in rows:
        tangent = 100+max(radius-dx for _, dx, _, radius in spheres) if spheres else None
        stage_mask = sum(1<<n for n in stage_ordinals)
        sphere_mask = sum(1<<n for n, *_ in spheres)
        results = {}
        for i, (name, y, reference, masked, empty) in enumerate(VARIANTS):
            if tangent is None or i not in (3, 4):
                x = 130
            else:
                x = tangent if i == 3 else stage.scalar(f"{int(bits(tangent), 16)+1:08X}")
            visits, pushed, contact = [], 0, False
            for ordinal, dx, dy, radius in spheres:
                if masked or stage.f32(y+dy)-radius < reference:
                    continue
                visits.append((ordinal, pushed))
                separation = stage.f32(stage.f32(stage.f32(x+dx)+pushed)-100)
                if not empty and 0 <= separation <= radius:
                    contact = True
                    delta = radius-separation
                    if int(delta) != delta:
                        raise ValueError("roster wall expected correction outside integer capture")
                    pushed += int(delta)
            admitted = sum(1<<n for n, _ in visits)
            packed_visits = [(n+1)*65536+before for n, before in visits]
            packed_visits += [0]*(2-len(packed_visits))
            shifted = count if pushed else 0
            fields = (f"{model:04X}", i, count, f"{0 if masked else stage_mask:08X}", f"{sphere_mask:08X}",
                      f"{admitted:08X}", f"{receiver_bits:08X}", len(visits), *(f"{p:08X}" for p in packed_visits),
                      bits(x), bits(x+pushed), bits(pushed), int(contact), int(contact), shifted)
            if lines[pos] != "roster-wall: " + " ".join(map(str, fields)):
                raise ValueError(f"roster wall response changed: {model:04X}/{name}")
            pos += 1
            results[name] = [visits, pushed, contact, shifted]
        models.append({
            "borg": f"gg4e08:{model:04x}", "bound_normal_cells": count, "stage_queue_ordinals": stage_ordinals,
            "published_kind1_stage_ordinals": skipped, "sphere_fields_ordinal_local_x_local_y_radius": spheres,
            "tangent_root_x": tangent, "observations": results,
        })
    return models


def totals(models):
    def observation(row, name):
        return row["observations"][name]

    high = [observation(row, "overlap") for row in models]
    low = [observation(row, "low-reference-zero") for row in models]
    pushed = [row[1] for row in high if row[1]]
    return {
        "models": len(models), "probes": len(models)*len(VARIANTS),
        "role08_published": sum(len(row["stage_queue_ordinals"]) for row in models),
        "kind0_solver_visits_in_overlap": sum(len(visits) for visits, *_ in high),
        "kind1_published_not_dispatched": sum(len(row["published_kind1_stage_ordinals"]) for row in models),
        "models_with_overlap_correction": len(pushed),
        "overlap_correction_range_x": [min(pushed), max(pushed)],
        "models_with_two_increases": [row["borg"] for row, (visits, distance, *_) in zip(models, high, strict=True)
                                      if len(visits) == 2 and 0 < visits[1][1] < distance],
        "sphere_visits_excluded_at_low_reference_zero": sum(len(a[0])-len(b[0]) for a, b in zip(high, low, strict=True)),
        "models_with_reduced_correction_at_low_reference_zero": [
            row["borg"] for row, a, b in zip(models, high, low, strict=True) if a[1] != b[1]
        ],
    }


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("roster wall output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    body.read_metadata(body.MAPS / "gg4e08-st42-wall.json", ST42_SHA256)
    reviewed = body.read_metadata(body.MAPS / "gg4e08-body-registry.json", BODY_SHA256)
    installed = body.read_metadata(body.MAPS / "gg4e08-standalone-installers.json", body.INSTALLERS_SHA256)
    payload, sources = body.source_inputs(afs, installed)
    rows = plans(payload, sources, reviewed)
    models = decode(body.fixture_report(fixture, dol, module, ("--wall",), payload), rows)
    summary = totals(models)
    raw = body.encode({
        "kind": "gf-retail-roster-wall", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "body_registry_sha256": BODY_SHA256, "st42_wall_sha256": ST42_SHA256,
        "stage_pass": "8002E1C8", "solver": "80035E58", "observed_bounds_call": "80035EC8", "broad_centers": "8002CE44",
        "declared_wall_plane": [1, 0, 0, -100], "declared_pose_z": -64, "declared_actor_scale": 1,
        "vertex_leg_yz_range": [-2000, 6000], "seat": 0, "reference_slot": "803B0708",
        "variants": {name: {"pose_y": y, "reference": ref, "role08_masked": mask, "empty_secondary": empty}
                     for name, y, ref, mask, empty in VARIANTS},
        "observation_fields": FIELDS, "summary": summary, "models": models,
        "notes": [
            "Full normal body queues bind unclipped at seat 0. Root and joint matrices are identical declared rigid translations, not retail actor animation. A single declared wall avoids unrelated arena triangles.",
            "Original publication supplies the role-08 registry; the original wall walk dispatches kind-0 shapes in reverse queue order. Observations at its first bounds call identify the exact allocated queue ordinal and incoming correction. The later bounds path is not counted as an additional visit.",
            "Every qualifying front overlap ends at the greatest offset required by an admitted sphere, not the sum of its isolated offsets. Five models increase the correction on both visits. Kind-1 stage entries publish but do not dispatch through the sphere solver.",
            "Masking role 08 removes only stage entries from the tested registry, not ordinary or alternate receiving. An empty secondary grid removes correction while retaining the solver visits.",
            "A low declared reference of zero excludes a published sphere if center Y minus radius is below zero. Keeping the low pose but restoring -10000 (the coordinator's reset constant, supplied here) restores those visits.",
            "The center pass consumes the original vector for all bound shapes without changing allocated endpoint fields, HP or registry membership. No attack, full scene tick, live pose, real arena response for the roster, or native gameplay is qualified.",
        ],
    }, expanded=("models",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **summary}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
