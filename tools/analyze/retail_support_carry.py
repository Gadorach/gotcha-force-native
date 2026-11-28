"""Verify the original owner-transform carry prefix and its startup angle state."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_support_anchor as anchor


body = anchor.body
stage = anchor.stage
ANCHOR_SHA256 = "61349a2d0e69cd4615a867463bf8f7c3040c119eefb2623a74e56bc5182ba1a5"
TEXT = (
    (0x8002A3E4, 204, "f81d60fc1ac257d0009a46d9c3d7e12a011879055876880a3f136643b3ffc58f"),
    (0x8002A51C, 124, "3682376d45f718c634318f9754655fa2dd5df4e7602265a2b747817d186a9faf"),
    (0x80059068, 208, "ea22383e1d48094534b9c882b22400b794423bdea5657be04bcfab050da3716c"),
    (0x8005BAB8, 264, "d857580767ae7b19893084b31d12b3a0c9a606a7891ae5e4f3cbaa7672c0ee46"),
    (0x80067A70, 768, "7da2c876aa6504bca6967a8992b2e368d860f96b746f633e7321f2c73543e8eb"),
    (0x8008B480, 12, "c4e284ff0693779bffa3272b0381f01a3a08fbf2bc8b631a5f85a9c4b05e20ca"),
    (0x8004516C, 76, "a20064e1e263cb7b684a80a97b080cd6d4c01b127f8f4aeeef47679941196cf5"),
    (0x80045838, 224, "90bfcf864297a8889d20a2489af7de1e3441f2b62db6983b4bc0d1f6c904c595"),
    (0x800457D4, 100, "5834ba9c6bf11389b74fe25a13eaaf04a90d4aa5e6ba376d7a0ea3004613a2e5"),
)
# Name, owner translation, owner yaw before/after, scene allow mask, owner
# active/phase, current support reset, initial actor yaw, zero-GQR control.
CASES = (
    ("held", False, 0, 0, 0, True, 1, False, 0x1000, False),
    ("joint-only-translation", False, 0, 0, 0, True, 1, False, 0x1000, False),
    ("owner-translation", True, 0, 0, 0, True, 1, False, 0x1000, False),
    ("owner-quarter-turn", False, 0, 0x4000, 0, True, 1, False, 0x1000, False),
    ("owner-translation-and-turn", True, 0, 0x4000, 0, True, 1, False, 0x1000, False),
    ("allow-mask-includes-owner", True, 0, 0x4000, 2, True, 1, False, 0x1000, False),
    ("allow-mask-excludes-owner", True, 0, 0x4000, 1, True, 1, False, 0x1000, False),
    ("owner-phase-two", True, 0, 0x4000, 0, True, 2, False, 0x1000, False),
    ("owner-inactive", True, 0, 0x4000, 0, False, 1, False, 0x1000, False),
    ("previous-support-only", True, 0, 0x4000, 0, True, 1, True, 0x1000, False),
    ("receiver-yaw-wrap", False, 0, 0x4000, 0, True, 1, False, 0x7000, False),
    ("held-nonzero-owner-yaw", False, 0x4000, 0x4000, 0, True, 1, False, 0x1000, False),
    ("fixture-zero-gqr-control", False, 0, 0x4000, 0, True, 1, False, 0x1000, True),
)


def verify_source(image):
    anchor.verify_source(image)
    body.verify_text(image, TEXT)
    for site, target in ((0x8002A420, 0x8002A51C), (0x8002A52C, 0x80200318),
                         (0x8005909C, 0x8005BAB8), (0x800590A4, 0x80067A70), (0x800590AC, 0x80059E58),
                         (0x80067ACC, 0x8008B480), (0x80067B88, 0x8004516C), (0x80067C0C, 0x8020B7A8)):
        body.link(image, site, target)
    if (image.read(0x8002A414, 12, "text") != bytes.fromhex("38600007646300077C75E3A6")
            or image.read(0x800458A4, 4, "text") != bytes.fromhex("E021D00A")
            or image.read(0x80437078, 4, "data") != bytes.fromhex("38C90FDB")):
        raise ValueError("support carry angle quantization source changed")


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(CASES):
        raise ValueError("support carry report extent")
    bits = stage.walk.stage.pair.capsule.geometry.bits
    root = (130, 124, 0)
    root_bits = tuple(bits(v) for v in root)
    rows = []
    for i, (name, translated, old_yaw, new_yaw, mask, active, phase, reset, yaw, zero_gqr) in enumerate(CASES):
        allowed = active and phase == 1 and not reset and (not mask or bool(mask & 2))
        delta_yaw = new_yaw-old_yaw if allowed else 0
        offset = (32, 12, -20) if allowed and translated else (0, 0, 0)
        moved = (100, 124, -10) if delta_yaw else root
        after = tuple(bits(a+b) for a, b in zip(moved, offset))
        if zero_gqr:
            after = ("4301FF82", "42F80000", "BB7B5000")
        angle = f"{(yaw+delta_yaw)&0xFFFF:08X}"
        gqr = "00000000" if zero_gqr else "00070007"
        flags = "00100040" if allowed else "00000040"
        carry_byte = int(allowed)
        fields = (i, int(not reset), int(reset), gqr, 2, *after, angle, flags, f"{carry_byte:08X}", *root_bits, 180)
        if lines[i] != "support-carry: " + " ".join(map(str, fields)):
            raise ValueError(f"support carry response changed: {name}")
        rows.append({
            "case": name, "declared_joint_only_translation": [32, 0, -20] if i == 1 else None,
            "declared_owner_root_before": [110, 119, 10], "declared_owner_root_after": [142, 131, -10] if translated else [110, 119, 10],
            "declared_owner_yaw_before_after_hex16": [f"{old_yaw:04X}", f"{new_yaw:04X}"],
            "declared_scene_owner_mask": mask, "owner_seat_bit": 2, "declared_owner_active": active, "declared_owner_phase": phase,
            "current_support_flag": not reset, "previous_support_flag": reset, "original_support_owner_retained": True,
            "gqr5": gqr, "fixture_control_not_retail_startup": zero_gqr, "carry_admitted": allowed,
            "actor_root_before_bits": list(root_bits), "actor_root_after_bits": list(after),
            "actor_yaw_before_after_hex16": [f"{yaw:04X}", angle[4:]], "actor_flags_after": flags,
            "actor_598_after": carry_byte, "allocated_capsule_center_unchanged_bits": list(root_bits), "hp_after": 180,
        })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("support carry output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    body.read_metadata(body.MAPS / "gg4e08-support-anchor.json", ANCHOR_SHA256)
    rows = decode(body.fixture_report(fixture, dol, module, ("--support-carry",), anchor.plane.source_inputs(afs, include_actor=True)))
    raw = body.encode({
        "kind": "gf-retail-support-carry", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "support_anchor_sha256": ANCHOR_SHA256,
        "normal_actor": "gg4e08:000a", "support_plane_owner_seat": 1, "support_owner_actor_offset": "1E4",
        "actor_update": "80059068", "carry_call": "800590A4", "carry_helper": "80067A70", "stop_before": "80059E58",
        "scene_owner_mask_offset": 235, "startup_quantizer_slice": "8002A3E4", "startup_gqr5_write_site": "8002A41C",
        "cases": rows,
        "notes": [
            "Starts after original near-plane contact. The original actor carries a saved current support flag, allocated plane, and support owner. The host declares the same owner's previous/current root transforms but not the support identity.",
            "The original startup prefix writes GQR5, captured before arena/OS initialization. The scoped carry replay uses that state for angle loads. The single zero-GQR row is an intentionally uninitialized harness control, not a retail behavior.",
            "The original actor-update prefix reproduces the direct carry helper up to the next original call. Root translation follows the owner's frame and yaw adds the owner yaw delta modulo 16 bits. A joint-only translation does not move the root through this helper.",
            "For the tested actor owner, a nonzero scene mask must include its seat bit; zero imposes no mask restriction. Inactive owners, non-one phases, and previous support alone all refuse carry.",
            "The scope is the actor-owner path with declared zero pitch/roll and stance rotations. It does not execute a movement program, republish shapes, construct a moving platform, or qualify prop owners, hardware, or native gameplay.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows), "qualified_startup_cases": len(rows)-1}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
