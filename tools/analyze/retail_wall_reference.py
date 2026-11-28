"""Join the original per-seat height producer to normal roster wall admission."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_roster_wall as roster


body = roster.body
stage = roster.stage
ROSTER_SHA256 = "104eb2d5c17cc0188908fd48e9958a7d792a554d7de19e85fdee5f60d7293313"
VIEWS = ("open", "role10-masked", "role08-masked")
FIELDS = ("reference_y", "bounds_visits_ordinal_and_incoming_x", "output_correction_x", "wall_contact")


def verify_source(image):
    roster.verify_source(image)
    body.verify_text(image, ((0x8002CF80, 536, "e983f60ae94672202ce930e0db112632eb3ed9b4f6e6a1624e61191e4f760668"),))
    body.link(image, 0x8002BD98, 0x8002CF80)


def plans(payload, sources, reviewed):
    rows = roster.plans(payload, sources, reviewed)
    stride = 2+body.reaction_emit.HIT_BYTES
    result = []
    for i, (row, (_, _, queue)) in enumerate(zip(rows, sources, strict=True)):
        bank = payload[i*stride+2:(i+1)*stride]
        bindings = []
        for ordinal, shape in enumerate(queue):
            if shape.roles & 0x10:
                _, y, _, radius = struct.unpack_from(">4f", bank, 44+80*shape.descriptor)
                if shape.kind != 0 or y not in (0, -60) or radius != 5:
                    raise ValueError(f"wall reference source sphere changed: {row[0]:04X}/{ordinal}")
                bindings.append((ordinal, shape.descriptor, shape.matrix, int(y), int(radius)))
        if len(bindings) > 1:
            raise ValueError(f"wall reference source role count: {row[0]:04X}")
        result.append((*row, bindings[0] if bindings else None))
    return result


def decode(report, rows):
    lines = report.splitlines()
    if len(report) > 131072 or len(lines) != len(rows)*len(VIEWS):
        raise ValueError("roster wall reference report extent")
    bits = stage.walk.stage.pair.capsule.geometry.bits
    results = []
    pos = 0
    for model, count, role08, _, spheres, receiving, source in rows:
        observations = {}
        for i, name in enumerate(VIEWS):
            ref_publishes = source is not None and i != 1
            ref_bits = 1 << source[0] if ref_publishes else 0
            reference = 128+source[3]-source[4] if ref_publishes else -10000
            stage_mask = sum(1<<n for n in role08) if i != 2 else 0
            visits, corrected = [], 0
            for n, local_x, local_y, radius in spheres:
                if i == 2 or stage.f32(128+local_y)-radius < reference:
                    continue
                visits.append((n, corrected))
                corrected = max(corrected, radius-local_x-30)
            packed = [(n+1)*65536+incoming for n, incoming in visits] + [0]*(2-len(visits))
            fields = (f"{model:04X}", i, count, f"{ref_bits:08X}", f"{stage_mask:08X}", f"{receiving:08X}",
                      bits(reference), bits(corrected), int(bool(corrected)), int(bool(corrected)), len(visits),
                      *(f"{p:08X}" for p in packed))
            if lines[pos] != "roster-wall-reference: " + " ".join(map(str, fields)):
                raise ValueError(f"roster wall reference changed: {model:04X}/{name}")
            pos += 1
            observations[name] = [reference, visits, corrected, bool(corrected)]
        results.append({
            "borg": f"gg4e08:{model:04x}",
            "role10_reference_sphere": dict(zip(("queue_ordinal", "descriptor", "matrix", "local_y", "core_radius"),
                                               source, strict=True)) if source else None,
            "role08_ordinals": role08, "role08_kind0_ordinals": [n for n, *_ in spheres],
            "ordinary_receiving_count": receiving.bit_count(), "observations": observations,
        })
    return results


def summarize(models):
    open_rows = [m["observations"]["open"] for m in models]
    masked = [m["observations"]["role10-masked"] for m in models]
    correction_changes = [m["borg"] for m, a, b in zip(models, open_rows, masked, strict=True) if a[2] != b[2]]
    return {
        "models": len(models), "physical_seats_each": 6, "collision_prefixes": len(models)*6*len(VIEWS),
        "reference_distribution_open": dict(Counter(str(r[0]) for r in open_rows)),
        "role08_solver_visits_open_per_seat": sum(len(r[1]) for r in open_rows),
        "role08_solver_visits_without_role10_per_seat": sum(len(r[1]) for r in masked),
        "models_with_wall_correction_changed_by_role10": correction_changes,
        "models_with_wall_correction_prevented_by_role10": [
            m["borg"] for m, a, b in zip(models, open_rows, masked, strict=True) if a[2] == 0 < b[2]
        ],
    }


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("roster wall reference output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    body.read_metadata(body.MAPS / "gg4e08-roster-wall.json", ROSTER_SHA256)
    reviewed = body.read_metadata(body.MAPS / "gg4e08-body-registry.json", roster.BODY_SHA256)
    installed = body.read_metadata(body.MAPS / "gg4e08-standalone-installers.json", body.INSTALLERS_SHA256)
    payload, sources = body.source_inputs(afs, installed)
    models = decode(body.fixture_report(fixture, dol, module, ("--wall-reference",), payload), plans(payload, sources, reviewed))
    summary = summarize(models)
    raw = body.encode({
        "kind": "gf-retail-wall-reference", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "roster_wall_sha256": ROSTER_SHA256,
        "body_registry_sha256": roster.BODY_SHA256, "collision_walk": "8002BB14",
        "reference_reset_site": "8002BD28", "reference_producer": "8002CF80",
        "reference_sphere_write_site": "8002CFE0", "reference_consumer": "8002E1C8",
        "reference_slots_base": "803B0708", "stop_before_presentation": "8005A580",
        "declared_rigid_translation": [130, 128, -64], "declared_wall_plane": [1, 0, 0, -100],
        "stale_reference_base_plus_seat": 5000, "reset_reference": -10000,
        "views": dict(zip(VIEWS, [0, 0x10, 0x08], strict=True)), "observation_fields": FIELDS,
        "summary": summary, "models": models,
        "notes": [
            "The fixture binds the complete normal body queue at each seat separately. Other seats are inactive, with distinct stale reference values. Poses, role masks, and the single secondary wall are declared.",
            "The original enclosing walk clears the active slot to -10000. With role 10 published, its first pass overwrites that slot with the current source sphere bottom before the role-08 gate. With role 10 masked or absent only the reset remains.",
            "Closing role 08 does not stop the earlier reference write. Closing role 10 does not remove a role-08 list entry, but changes which of its spheres reaches the solver. The ordinary receiver lists remain intact.",
            "There are no foreign planes or cross-actor contacts in these probes; the secondary reference write when a foreign plane is considered is not tested. The recorded scalar is not a recovered live floor height.",
            "The original walk reaches the presentation frontier without changing HP. It does not run presentation, a full scene tick, actor animation, live stage geometry, combined forms, or native gameplay.",
        ],
    }, expanded=("models",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "models": len(models), "prefixes": summary["collision_prefixes"],
            "models_with_changed_correction": len(summary["models_with_wall_correction_changed_by_role10"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
