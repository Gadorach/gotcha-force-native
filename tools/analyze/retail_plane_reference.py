"""Verify foreign plane support and a later rejected pair's wall-reference refresh."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_wall_reference as reference


body = reference.body
stage = reference.stage
REFERENCE_SHA256 = "f0dfbb0fd8a9d61a610cd4915c8ef93bb49edd3655c5c984f5900b3ee064d786"
TEXT = (
    (0x8003A05C, 108, "556c958bf405c202365a70c5b23b0b6cd24f833ec35514703b8db5bc55ee3a69"),
    (0x8003C3D8, 572, "ec544172b5914f77aa13c810ec3fbe09c3ce0cf72e82968ecc86b1449493ade3"),
    (0x8003C614, 672, "2ad1d418ba06ee67bbe059d4f1dd5bf9feb1a26f99bb002e60a64d8be0969458"),
    (0x80030348, 104, "f62cba36d6df88ce783e1e82fd2628e345b770b4ac2d5920804dc90ee53eecb1"),
    (0x800303B0, 112, "c205f8323cc475a1e4921738199739a357b739147253ee2e64be5db94c3efd2a"),
    (0x80035698, 604, "0380afd489e67dc402091fe2635bad43fd044346c655e225385d0cae0420cb24"),
    (0x80035B3C, 360, "454f6ceb390d0e9d0bb5b2fa6f2cec9f2db47c228aaae14d432c62ede099bd81"),
    (0x80039DA4, 456, "49211bdef98464db15d88feb784a88e40f3511b5e5c00e8444c3bb299898f66c"),
)
# Name, order of the original role-20 plane cells in the registry, stage-sphere joint Y.
CASES = (
    ("no-plane", (), 160), ("near-only", ("near",), 160), ("far-only", ("far",), 160),
    ("near-then-far", ("near", "far"), 160), ("far-then-near", ("far", "near"), 160),
    ("refreshed-equality", ("near", "far"), 159), ("self-plane", ("self",), 160),
    ("refreshed-one-step-below", ("near", "far"), stage.scalar("431EFFFF")),
)


def verify_source(image):
    reference.verify_source(image)
    body.verify_text(image, TEXT)
    for site, target in ((0x8002D080, 0x8003A05C), (0x8002D0CC, 0x80039F6C), (0x8002D134, 0x80030348),
                         (0x8002D158, 0x800303B0), (0x80035800, 0x80039DA4), (0x80035844, 0x80035B3C),
                         (0x80035C78, 0x80035CA4)):
        body.link(image, site, target)
    if (image.read(0x802CFEB0, 1, "data") != b"\x02"
            or image.read(0x802CFEA8, 4, "data") != bytes.fromhex("80035698")):
        raise ValueError("sphere/plane type-pair dispatcher changed")


def source_inputs(afs):
    melee = stage.walk.stage.melee
    with body.open_regular(afs) as stream:
        stamp = body.archive_stamp(stream)
        body.verify_archive(stream)
        archive = body.RetailArchive(stream)
        model = melee.opening.RECEIVER
        hit, _ = melee.opening.read_normal(archive, model, melee.opening.RECEIVER_BLOCK_SHA256, melee.ACTOR_SHA256[model])
        if body.archive_stamp(stream) != stamp:
            raise ValueError("plane reference source archive changed")
    queue = body.body_queue(hit)
    for d, kind, matrix, roles, expected in (
        (0, 0, 128, 0x10, (0, 0, 0, 5)),
        (2, 0, 3, 0x08, (0, 10, 0, 50)),
        (3, 4, 12, 0x20, (0, stage.f32(32.2), 0, 5, 20, 0, 0, 0)),
        (11, 2, 6, 0x02, (-12, 0, 0, 15, 0, 0, 7)),
    ):
        if (body.Shape(d, kind, matrix, roles) not in queue
                or struct.unpack_from(f">{len(expected)}f", hit, 44+80*d) != expected):
            raise ValueError(f"plane reference lost exact normal descriptor: {d}")
    return hit


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 2*len(CASES):
        raise ValueError("plane reference report extent")
    bits = stage.walk.stage.pair.capsule.geometry.bits
    rows = []
    for i, (name, order, jy) in enumerate(CASES):
        reference_y = 123
        support_cell = None
        checks = []
        for index, label in enumerate(order):
            if label == "self":
                continue
            reference_y = 119 if support_cell is not None else 123
            checks.append({"plane": label, "cell": 3+index, "reference_before_broad": reference_y,
                           "broad_accepted": label == "near"})
            if label == "near":
                support_cell = 3+index
        broad = sum(1 << index for index, row in enumerate(checks) if row["broad_accepted"])
        bottom = stage.f32(stage.f32(jy+10)-50)
        wall = bottom >= reference_y
        x = 150 if wall else 130
        y = 128 if support_cell is None else 124
        xdelta = 20 if wall else 0
        ydelta = "00000000" if support_cell is None else "C07FFFFF"
        hit = int(support_cell is not None)
        cells = [row["cell"] for row in checks] + [255]*(2-len(checks))
        heights = [row["reference_before_broad"] for row in checks] + [0]*(2-len(checks))
        fields = (i, len(order), len(checks), *(bits(v) for v in (*heights, reference_y)),
                  *cells, broad, hit, support_cell or 255, support_cell or 255, bits(y), ydelta,
                  bits(bottom), bits(xdelta), int(wall), hit)
        final = (i, bits(x), bits(y), bits(xdelta), ydelta, bits(x), bits(y), bits(x), bits(y),
                 int(wall or hit), int(wall), hit, int(wall), 180)
        if (lines[2*i] != "plane-reference: " + " ".join(map(str, fields))
                or lines[2*i+1] != "plane-walk: " + " ".join(map(str, final))):
            raise ValueError(f"plane reference response changed: {name}")
        rows.append({
            "case": name, "published_planes": list(order), "ordered_foreign_checks": checks,
            "narrow_support_cell": support_cell, "support_owner_latch": bool(hit),
            "support_root_y": y, "support_correction_y_bits": ydelta,
            "wall_reference_after_plane_pass": reference_y, "raw_role08_bottom_bits": bits(bottom),
            "role08_solver_reached": wall, "wall_correction_x": xdelta,
            "root_xy_after_walk": [x, y], "capsule_broad_center_xy": [x, y], "allocated_capsule_endpoints_unchanged": True,
            "receiver_hp_after": 180,
        })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("plane reference output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    body.read_metadata(body.MAPS / "gg4e08-wall-reference.json", REFERENCE_SHA256)
    rows = decode(body.fixture_report(fixture, dol, module, payload=source_inputs(afs)))
    raw = body.encode({
        "kind": "gf-retail-plane-reference", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "wall_reference_sha256": REFERENCE_SHA256,
        "normal_body": "gg4e08:000a", "role10_sphere_descriptor": 0, "role20_plane_descriptor": 3,
        "role08_wall_sphere_descriptor": 2, "ordinary_capsule_descriptor": 11,
        "pass": "8002CF80", "reference_slot": "803B0708", "initial_sphere_write": "8002CFE0",
        "candidate_refresh": "8002D0B8", "broad_call": "8002D0CC", "narrow_kernel": "80035698",
        "support_writer": "80035B3C", "correction_writer": "80035CA4",
        "declared_wall_plane": [1, 0, 0, -100], "declared_receiver_root": [130, 128, 0],
        "declared_near_plane_center": [130, 119, 0], "declared_far_plane_center": [600, 119, 0],
        "stop_before_presentation": "8005A580", "cases": rows,
        "notes": [
            "Each plane is an unedited normal Sasuke role-20 descriptor bound by original code to a declared owner and rigid matrix. The two foreign planes have separate active seats; the same-owner control publishes but is not a foreign candidate.",
            "The near plane reaches the original narrow and support path, producing the captured negative Y vector and root Y 124. The far plane fails the original broad helper when replayed from its exact captured scratch operands.",
            "A far test after near support refreshes the reference with the adjusted role-10 sphere bottom before broad rejection. A far test before support, or no second plane, leaves the earlier value unchanged. The far test is not a support contact.",
            "The following wall pass compares the published role-08 bottom against that reference, admitting equality. Only the tested order with a refresh and an admitted sphere produces +20 X.",
            "Both an isolated pass sequence and the original enclosing walk produce the same state through the presentation frontier. This does not establish retail prop construction, animated platform support, live arena behavior or native gameplay.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows),
            "cases_with_support": sum(r["support_owner_latch"] for r in rows),
            "cases_admitted_to_wall": sum(r["role08_solver_reached"] for r in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
