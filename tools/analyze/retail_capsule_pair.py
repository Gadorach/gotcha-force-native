"""Separate original sphere/capsule broad phase, narrow test, and HP outcome."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_body_capsules as capsule
from tools.analyze import retail_melee_pose_clock as melee


s = capsule.registry
CAPSULE_SHA256 = "cff48f15af8803230d12bbb81ff19ed10741802fc6efd98578dc0c521b804aa8"
MELEE_SHA256 = "7bb38ad95e772e1d0e3e3527a647d88319e757ef2695ce430694c1c7576af682"
LARGE_SHA256 = "d3c335c3a453b257896c960d223d892695bfc3442e526e9455361f18f69d03d1"
LARGE_SPEC_SHA256 = "93b431fb5e0a713e1add0aec3b8b96e8ffa519f9e4cbcc9774c8f3ef6f5a4e42"
TEXT = (
    (0x8002DB58, 944, "4aa097fc70080cd5b6c849d879cd20b39efebb359b80fb70b6a02c486ba38238"),
    (0x80039F6C, 240, "ea15e86d0428a669821b13e9d6131da6e5c4947d84970604cea6527186912e23"),
    (0x80030490, 112, "a395c9c1f3da5874d3ddfc9b2a710408a80620ba356a16412fc1c5aa1d8b8716"),
    (0x80030348, 104, "f62cba36d6df88ce783e1e82fd2628e345b770b4ac2d5920804dc90ee53eecb1"),
    (0x80030A3C, 500, "1ff2ec862efab07116aec009d66069576989fabac84f30bdeb2473c5b6472a63"),
    (0x8002E2A8, 5672, "3f79bf55474f2fdc4d39b4436d481b713312b1477289db13deb902ffa26c0f1e"),
    (0x8003D344, 164, "f8a7485a7bb1eb37c6a02be76336a2fef43d8a348adbe9670469e6e57d4da06f"),
    (0x8003CD5C, 1512, "7c752eea0cdf04fbe970796f80ffe1bd98ea2ef6edebea6e33cd1d3462355792"),
    (0x8002FD7C, 832, "88d85748f4401cc2a4be18a7c321b2051d29f8e8d284525a611aa91cefa71247"),
)
# Label, receiver descriptor family, axis, center distance.
CASES = (
    ("center", 0, 0, 0), ("side-inside", 0, 1, 76), ("side-tangent", 0, 1, 77), ("side-outside", 0, 1, 78),
    ("cap-inside", 0, 0, 91), ("cap-tangent", 0, 0, 92), ("cap-outside", 0, 0, 93),
    ("updated-broad-inside", 0, 0, 553), ("updated-broad-tangent", 0, 0, 554), ("updated-broad-outside", 0, 0, 555),
    ("zero-extent", 1, 0, 0), ("same-model-nondegenerate", 2, 0, 0),
)
# Model, queue ordinal, descriptor, matrix slot, flags, initialized HP, local center/extent/radius.
TARGETS = (
    (0x000A, 11, 11, 6, 2, 180, (-12, 0, 0, 15, 0, 0, 7)),
    (0x0506, 14, 6, 12, 6, 2000, (0, -50, 0, 0, 0, 0, 150)),
    (0x0506, 2, 2, 4, 2, 2000, (-190, 0, 0, 170, 0, 0, 120)),
)


def verify_source(image):
    capsule.verify_source(image)
    s.verify_text(image, (*TEXT, *melee.TEXT))
    for site, target in ((0x8004C6F0, 0x8008AC80), (0x8002DC58, 0x8002FD7C), (0x8002DCEC, 0x80039F6C),
                         (0x8002DD68, 0x80030348), (0x8002DD8C, 0x80030490), (0x8002DDA8, 0x8002E2A8),
                         (0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344)):
        s.link(image, site, target)
    if (image.read(0x80437004, 4, "data") != bytes.fromhex("3DCCCCCD")
            or image.read(0x802CFF6A, 1, "data") != b"\x02"
            or image.read(0x802CFF34, 4, "data") != bytes.fromhex("80030A3C")):
        raise ValueError("capsule pair floor or sphere-capsule dispatch changed")


def source_inputs(image, afs, prior):
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        archive = s.RetailArchive(stream)
        reviewed = s.read_metadata(s.MAPS / "reaction-melee-map.json", melee.MELEE_SHA256)
        tau, tau_row = melee.verify_route(image, archive, reviewed, melee.ROUTES[0])
        banks = {
            0x000A: melee.opening.read_normal(archive, melee.opening.RECEIVER, melee.opening.RECEIVER_BLOCK_SHA256,
                                             melee.ACTOR_SHA256[melee.opening.RECEIVER]),
            0x0506: melee.opening.read_normal(archive, 0x0506, LARGE_SHA256, LARGE_SPEC_SHA256),
        }
        if s.archive_stamp(stream) != stamp:
            raise ValueError("capsule pair source archive changed")
    if struct.unpack_from(">4f", tau, 0xAA0+2*80+12) != (0, 0, 90, 70):
        raise ValueError("capsule pair attack radius or center changed")
    sources = []
    for model, ordinal, step, joint, roles, hp, local in TARGETS:
        bank, _ = banks[model]
        start = 32+step*80
        name = f"gg4e08:{model:04x}"
        table = melee.reaction_installer_tables.model_table(image, model)[1]
        if (bank[ordinal] != step or struct.unpack_from(">BBH", bank, start) != (2, joint, roles)
                or struct.unpack_from(">7f", bank, start+12) != local or melee.levels(image, model)[0][0] != hp
                or table != (0x802D4160 if model == 0x000A else 0x802FEF08)):
            raise ValueError("capsule pair receiver source changed")
        sources.append({"borg": name, "queue_ordinal": ordinal, "descriptor": step, "matrix_selector": joint,
                        "role_word": roles, "hp": hp, "admitted_amount_bank": f"{table:08X}"})
    if prior["degenerate_capsules"] != [
        {"borg": "gg4e08:0506", "queue_ordinal": 14, "descriptor": 6, "both_receiver_lists": True}
    ]:
        raise ValueError("capsule pair zero-extent receipt changed")
    return tau+b"".join(banks[0x000A]+banks[0x0506]), tau_row, sources


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 2*len(CASES):
        raise ValueError("capsule pair report extent")
    rows = []
    for i, (name, target, axis, distance) in enumerate(CASES):
        model, _, descriptor, _, _, hp, _ = TARGETS[target]
        for updated in (0, 1):
            core = (7, 0, 120)[target]
            bound = (22, 0, 290)[target]
            if updated:
                bound *= bound
            broad = bound > .1 and distance <= 70+bound
            hit = broad and (distance < (77 if axis else 92) if target == 0 else target == 2)
            requested = (17 if target == 0 else 16) if hit else 0
            fields = (i, updated, f"{model:04X}", descriptor, axis, distance, capsule.geometry.bits(bound),
                      capsule.geometry.bits(core), int(broad), int(hit), int(hit), requested, hp-requested, int(hit))
            if lines[len(rows)] != "capsule-pair: " + " ".join(map(str, fields)):
                raise ValueError(f"capsule pair broad, narrow or HP changed: {name}/{updated}")
            rows.append({
                "case": name, "bound_update": "unchanged-second-rebuild" if updated else "initializer",
                "receiver": f"gg4e08:{model:04x}", "descriptor": descriptor, "axis": ("x", "y")[axis],
                "center_separation": distance, "receiver_core_radius": core, "receiver_broad_field": bound,
                "broad_phase_accepted": broad, "sphere_capsule_kernel_reached": broad,
                "sphere_capsule_accepted": hit if broad else None, "hp_writer_reached": hit,
                "hp_before": hp, "hp_requested": requested, "hp_removed": requested, "hp_after": hp-requested,
                "recorded_token": 1 if hit else None, "same_token_repeat_hp_writes": 0,
            })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("capsule pair output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    prior = s.read_metadata(s.MAPS / "gg4e08-body-capsules.json", CAPSULE_SHA256)
    s.read_metadata(s.MAPS / "gg4e08-melee-pose-clock.json", MELEE_SHA256)
    payload, tau, receivers = source_inputs(image, afs, prior)
    rows = decode(s.fixture_report(fixture, dol, module, payload=payload))
    raw = s.encode({
        "kind": "gf-retail-capsule-pair", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "body_capsules_sha256": CAPSULE_SHA256, "melee_pose_clock_sha256": MELEE_SHA256,
        "attack": tau, "attack_radius": 70, "receivers": receivers, "pair_loop": "8002DB58",
        "broad_phase": "80039F6C", "positive_broad_floor_bits": "3DCCCCCD", "sphere_capsule_kernel": "80030A3C",
        "cases": rows,
        "notes": [
            "The exact Tau opcode binds one sphere. Only the receiver queue is narrowed to one unedited source descriptor, and HP and rigid matrices are declared at this pair inlet.",
            "The original broad helper rejects either broad field at or below the 0.1 source floor. Otherwise it compares center distance against the raw field sum. The unchanged capsule update is not converted back to a linear radius before this test.",
            "The original narrow kernel receives the pair loop's scratch shapes. Its separate core test rejects the tested side and end-cap tangencies even when broad phase accepts them.",
            "Accepted original pairs complete their HP and request ledgers; the retained token never writes HP twice. The zero-extent receiver remains listed but is refused by broad phase even at coincident centers.",
            "This covers only ordinary stationary sphere/capsule tests in the declared geometry. No alternate pair, moving-body sweep, other contact kinds, hardware or native gameplay is qualified.",
        ],
    }, expanded=("receivers", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows),
            "narrow_reached": sum(row["sphere_capsule_kernel_reached"] for row in rows),
            "hp_writes": sum(row["hp_writer_reached"] for row in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
