"""Replay construction and support on one original class-22 stage side object."""
import argparse
from hashlib import sha256
from io import BytesIO
import json
from pathlib import Path
import struct

from tools.analyze import retail_support_carry as carry
from tools.extract.pzz import PzzArchive
from tools.extract.retail_borg_roster import RangeFile


body = carry.body
stage = carry.stage
CARRY_SHA256 = "c6549651548a5d27de30f3bcd81497567305b84de4893199f34acc69796f7498"
SIDE_SHA256 = "1e8bc4b55516764638b7ffa5021f18c9cfbab927a26fd859adb491002f35868c"
COMMON_SHA256 = "21b0994e76f97435e2c1bd7e34c39d7a33ec19a8f1d0d4ebc9207bf697448c1e"
BANK_SHA256 = "e6a41d20e919e0fcf77898af7800f2beaffc459e2267bf4c227879d38a40c0a1"
PLANE_SHA256 = "fede8a60410f7e586cfd13aa876fda875c828035c29582b52d7d0607ab5d7adb"
ROW = bytes.fromhex("04000000C509B99441200000C4E09EE8000000000000000000000000")
TEXT = (
    (0x8004279C, 216, "7f6e28a788efa58185dd2a5878fb96bf2a7d6ebbd8897ef75c2640b05ebacb65"),
    (0x80006C88, 196, "afac18ada3931593eb2f6090d92d6574481630aa9b34345f8b72de0623dfd030"),
    (0x80081980, 52, "07d1b0a3ea04d1d3e014a645458537f131f37536b3b5604f5830d1ed59869e94"),
    (0x800819B4, 488, "66c702b9d9ffd12a6e07c65a2deaa7575a117fc233c5a37b72bc5f4b9cdf6bec"),
    (0x80081B9C, 388, "5afae4d86c8cff84478705655d5c981e9eebb248d47fe658cfd56104b60e8d64"),
    (0x80081D98, 60, "10a8c0601337077ba0ef287eeec6c06905bda71f25aff0ff0c1e9f6b21e442db"),
    (0x80081DD4, 16, "5df183e493359951d4d5dc1c414d6e67bbaf062e0c5f0169b3fd44547f07e1a3"),
    (0x80082060, 104, "fa01301a7286ab7da15e4e2f357c14d7df8baeb7c05683829a91db45468a77e9"),
    (0x8008893C, 356, "ca5d566a4630c0e0846ee4c3fed9ab52590fc334db7c2d539a4eb2d5a473f276"),
    (0x8008AE10, 44, "889d493955cfddbef9abf4689532aa277427c73011aeab4a4fe22e6e75e4053f"),
    (0x8008AE60, 400, "ed773c3f3f4973120e9f352561f67629417f7a374b39e2215c65901ffba46c62"),
)
CASES = ("original-support", "outside-plane", "moved-owner", "moved-owner-scene-masked")


def verify_source(image):
    carry.verify_source(image)
    body.verify_text(image, TEXT)
    for site, target in ((0x80042840, 0x80040EB4), (0x80081998, 0x80081B9C), (0x80081BC4, 0x8008893C),
                         (0x80081BD4, 0x800819B4), (0x80081C1C, 0x80089100), (0x80081CD0, 0x8008AE10),
                         (0x8008AE28, 0x8008AE60), (0x80082074, 0x80047AA4), (0x80082098, 0x8020B7A8)):
        body.link(image, site, target)
    if (image.read(0x802D79E0, 20, "data") != bytes.fromhex("006400AB00000000444064A4802D748002580000")
            or image.read(0x802D7AD0, 4, "data") != bytes.fromhex("80081DD4")
            or image.read(0x802DA700+40, 1, "data") != b"\x16"):
        raise ValueError("stage prop original type 4 operands changed")


def source_inputs(afs):
    with body.open_regular(afs) as stream:
        stamp = body.archive_stamp(stream)
        body.verify_archive(stream)
        member = body.RetailArchive(stream)
        if member.names[2429] != b"set0202.arc" or member.names[696] != b"cmn_data.pzz":
            raise ValueError("stage prop source member names changed")
        side = member.payload(2429, 1 << 20)
        packed = member.payload(696, 2 << 20)
        if sha256(side).hexdigest() != SIDE_SHA256 or sha256(packed).hexdigest() != COMMON_SHA256:
            raise ValueError("stage prop source archive digest")
        pzz = PzzArchive(RangeFile(BytesIO(packed), 0, len(packed)))
        common, _ = pzz.read(0, 21504)
        if body.archive_stamp(stream) != stamp:
            raise ValueError("stage prop archive changed")
    if (len(side) != 122880 or struct.unpack_from(">4I", side) != (3, 1, 28, 30)
            or struct.unpack_from(">I", side, 2048)[0] != 8 or side[2052:2080] != ROW):
        raise ValueError("stage prop original sector/placement row changed")
    d = 0x400+237*80
    if (len(common) != 21504 or sha256(common).hexdigest() != BANK_SHA256
            or common[4*171:4*172] != bytes.fromhex("ECEDFFFF")
            or sha256(common[d:d+80]).hexdigest() != PLANE_SHA256
            or common[d:d+12] != bytes.fromhex("0400002600000000FFFF0000")
            or struct.unpack_from(">8f", common, d+12) != (0, 630, 0, 140, 220, 0, 1, 0)):
        raise ValueError("stage prop original selector or plane geometry changed")
    return side+common+carry.anchor.plane.source_inputs(afs, include_actor=True)


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(CASES):
        raise ValueError("stage prop support report extent")
    bits = stage.walk.stage.pair.capsule.geometry.bits
    x, _, z = struct.unpack_from(">3f", ROW, 4)
    rows = []
    for i, name in enumerate(CASES):
        held = i != 1
        moved = i >= 2
        carries = held and i != 3
        y = 780 if held else 784
        root = (x if held else stage.f32(x+600), y, z)
        plane = (x, 640, z)
        local = (0, 770 if held else 0, 0)
        owner_now = (stage.f32(x+32), 22, stage.f32(z-20)) if moved else (x, 10, z)
        final = (owner_now[0], y+12, owner_now[2]) if moved and carries else root
        height = 792 if moved else 780 if held else 0
        fields = (i, 22, 1, int(held), int(held), 9, 8,
                  *(bits(v) for v in (*plane, *root, *local, *final)), "00100000" if carries else "00000000",
                  "00000000", *(bits(v) for v in (*root, *owner_now, height, height)), int(held), 180, 100)
        if lines[i] != "stage-prop-support: " + " ".join(map(str, fields)):
            raise ValueError(f"stage prop support changed: {name}")
        rows.append({
            "case": name, "prop_owner_kind": 22, "prop_phase": 1, "support_acquired": held,
            "plane_narrow_calls": int(held), "support_owner_is_original_prop": held,
            "declared_translation": [32, 12, -20] if moved else None, "declared_scene_owner_mask": int(i == 3),
            "prop_root_after_world_update_bits": [bits(v) for v in owner_now],
            "plane_center_held_during_carry_bits": [bits(v) for v in plane],
            "saved_local_anchor_bits": [bits(v) for v in local] if held else None,
            "actor_root_after_original_collision_bits": [bits(v) for v in root],
            "actor_root_after_carry_prefix_bits": [bits(v) for v in final],
            "capsule_center_held_during_carry_bits": [bits(v) for v in root], "carry_admitted": carries,
            "separate_supported_surface_height": height if held else None,
            "separate_supported_landing_root_y": height if held else None,
            "separate_supported_landing_return": 1 if held else None,
            "actor_hp_after": 180, "prop_hp_after": 100,
        })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stage prop support output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    body.read_metadata(body.MAPS / "gg4e08-support-carry.json", CARRY_SHA256)
    rows = decode(body.fixture_report(fixture, dol, module, payload=source_inputs(afs)))
    raw = body.encode({
        "kind": "gf-retail-stage-prop-support", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "support_carry_sha256": CARRY_SHA256,
        "side_member": 2429, "side_file": "set0202.arc", "side_sha256": SIDE_SHA256, "placement_ordinal": 0,
        "loaded_side_bank_record_counts": [9, 8], "archive_loader": "8004279C", "placement_entry": "80081980",
        "constructor": "80081B9C", "owner_object_class": 3, "owner_bucket": 40, "owner_kind": 22,
        "world_callback": "80082060", "collision_selector": 171, "common_collision_bank_sha256": BANK_SHA256,
        "collision_descriptors": [236, 237], "role20_plane_descriptor": 237, "role20_plane_sha256": PLANE_SHA256,
        "plane_normal_y_bits": "3F7FFFFF", "plane_core_thickness": 140, "plane_radius": 220,
        "cases": rows,
        "notes": [
            "The exact retained side payload starts at the original side archive loader. A single original placement row reaches the original class-3 constructor with two real model roots and original common collision bindings. No game assets are included in this receipt.",
            "The original bucket-40 update admits phase one. The registered world callback resolves the authored placement. A declared local translation in the moved cases goes through that same callback; no autonomous stage motion or scene loader is qualified.",
            "Original collision records the exact prop and its published common role-20 plane as support. The horizontally separated control does not acquire it and does not enter the unqualified absent-support floor query.",
            "The actual class-22 owner has no actor sponsor. The tested nonzero scene mask therefore prevents root carry, but a separate supported height and landing probe still observes the translated plane matrix and follows it vertically.",
            "The original actor-update carry prefix matches the direct helper, while allocated receiving geometry and HP stay fixed. No complete scene tick, original audio, rendering, hardware, or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows), "support_cases": sum(r["support_acquired"] for r in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
