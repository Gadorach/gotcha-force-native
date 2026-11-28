"""Replay archive delivery and normal replacement startup in the retained body seat."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_body_phase_cleanup as cleanup


s, area = cleanup.s, cleanup.area
CLEANUP_SHA256 = "f8a1ecf077357ae98927c55ea99066ce445aa4ac54e25eb900ebed078dc01310"
MEMBER_SHA256 = "115a623e6f06c7edb5b9e03004247e3f6abc43af2f44b66742c01f4fff80caab"
ASSETS = (
    (0, 432, "69919a580073d32e8db9bdbccaed60fa81687a5fc7c58af87fda3ea6bb0ae26f"),
    (2, 7840, "8aa36502a67ebb5a77d0f6812470b8d0bb244dd0592198805bb126e0b2cc3e8d"),
    (3, 435104, "aaa8e47ff59c992dc9681e98ad310709e3340c64c2621435b7e8d2ca8568565d"),
    (4, 171236, "c75e27aeb31b33ba55777b2de181443b2ec84ac8e78390fd043bd6323830951b"),
)
TEXT = (
    (0x800412C4, 160, "3e98fd4062de07be013cfad0ad621c3fcab2f6d0e7edbcbf6fb4661edb13797f"),
    (0x80056CE0, 72, "b07773b3d078dd35918cbd9697bc5fa43d55dd7926cbccdae108cd962e2d92b5"),
    (0x800419EC, 308, "792a8a7f3020c2e0c2e518a49580fc47b4b8888fd703fb8bbc27e6377b30e01c"),
    (0x8005380C, 284, "0581d5c5455483e6a83cc578da47715685be52f9ae02743aa5d8e710880953e8"),
    (0x80007030, 180, "ad0b95338662fb97c000b4675cdf22a4c4dd3643018e8725a13ecc66eaff2580"),
    (0x80006C88, 196, "afac18ada3931593eb2f6090d92d6574481630aa9b34345f8b72de0623dfd030"),
    (0x80254288, 68, "68434a34c000cfde2763eca6914bdf238278837116e7e7fb69faac3cb2af5606"),
    (0x8003CC24, 312, "7fcf39fdb005df817b0de481e6860f17a2a8bb8c0d9fc7b82cb11882b2c9b415"),
    (0x8008AB30, 336, "9af045cf898354aadd281a64834e3cdde963848553ea25260df7d4f3de9b439b"),
    (0x80089AEC, 276, "03e4a562cd8742937f8f85f602f0f3a3a682457f8f5887accc0b15cbf24f4802"),
    (0x8008A65C, 968, "49c1e62d33c50709ec0dc16c6c0306b07ee5f6714feb14ac3f7871ede07c864a"),
)
RECEIVING_STEPS = (4, 5, 6, 7, 9, 10, 11, 12, 13)


def decode(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != 4:
        raise ValueError("normal body replacement report extent")
    rows = []
    for caster, _, _, _ in area.bucket.MODELS:
        for offered in (0, 16):
            steps = RECEIVING_STEPS if offered else ()
            mask = sum(1 << i for i in steps)
            expected = (caster & 255, offered, "0106", 1, 1, 220, 10, 3, "00EF", 4, 8,
                        0, 0, 0, 1, 0, 16, len(steps), f"{mask:04X}")
            if lines[len(rows)] != "body-replacement: " + " ".join(map(str, expected)):
                raise ValueError(f"normal replacement actor, resource or receive list changed: {caster:04X}/{offered}")
            rows.append({
                "initial_attacker": f"gg4e08:{caster:04x}", "area_key": 19+int(caster == 0x0107), "seat": 1,
                "replaced_borg": "gg4e08:000a", "replacement_borg": "gg4e08:0106", "generation_delta": 1,
                "old_receiving_shape_returned_before_reentry": True, "unallocated_archive_owner_refused": True,
                "actor_phase": 1, "hp": 220, "stock_channels": {"0": 10, "2": 3}, "initial_role_mask": "00EF",
                "old_damage_ledger": 16, "receiving_force": 0, "collision_history_cleared_entries": 48,
                "source_view_records": 4, "view_wrappers": 8, "shape_cells_offered": offered,
                "body_shapes_bound": offered, "ordinary_receiving_shapes_published": 0,
                "receiving_steps_with_only_bit_02_cleared": list(steps),
            })
    return rows


def verify_source(image):
    cleanup.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x80041F00, 0x8000A154), (0x80041338, 0x80197A0C), (0x80054324, 0x80056CE0),
                         (0x80054378, 0x800419EC), (0x80054380, 0x8005380C), (0x80054414, 0x80055F90),
                         (0x8005441C, 0x8005614C), (0x80054444, 0x8003CC24), (0x800544A8, 0x8008AB30),
                         (0x8008A69C, 0x80088E50)):
        s.link(image, site, target)
    for address, size, ctor, dtor in ((0x8036F5B8, 44, 0x80006F60, 0x80006ED8),
                                     (0x8036F5F0, 16, 0x80006E3C, 0x80006E8C)):
        if image.read(address+16, 12, "data") != struct.pack(">III", size << 16, ctor, dtor):
            raise ValueError("normal replacement archive pool operands changed")


def payload(image, afs):
    base = area.bucket.qualified_payload(image, afs, (area.RECEIVER, area.RECEIVER_BLOCK_SHA256,
                                                      area.ACTOR_SHA256[area.RECEIVER]))
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        member, digest = s.RetailArchive(stream).member(504, 0x0106)
        if digest != MEMBER_SHA256 or member.entries[1]["size"]:
            raise ValueError("normal Time Bomber archive changed")
        parts = []
        for role, size, checksum in ASSETS:
            record, _ = member.read(role, size)
            if len(record) != size or sha256(record).hexdigest() != checksum:
                raise ValueError(f"normal Time Bomber archive role {role} changed")
            parts.append(record)
        if s.archive_stamp(stream) != stamp:
            raise ValueError("normal replacement archive input changed")
    spec, hit, motion, view = parts
    if spec != base[21504+7840:21504+7840+432] or hit != base[21504:21504+7840]:
        raise ValueError("replacement role data does not match the qualified Time Bomber inlet")
    if list(hit[:32]) != list(range(16))+[255]*16:
        raise ValueError("normal replacement body descriptor selection changed")
    receiving = tuple(i for i in range(16) if hit[32+i*80+3] & 2)
    if receiving != RECEIVING_STEPS:
        raise ValueError("normal replacement receiving role bits changed")
    return base+motion+view


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal body replacement output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-body-phase-cleanup.json", CLEANUP_SHA256)
    cases = decode(s.fixture_report(fixture, dol, module, ("--replacement",), payload(image, afs)))
    raw = s.encode({
        "kind": "gf-retail-body-replacement", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "phase_cleanup_sha256": CLEANUP_SHA256,
        "replacement_member": 504, "member_sha256": MEMBER_SHA256,
        "delivered_roles": [{"role": role, "bytes": size, "sha256": checksum} for role, size, checksum in ASSETS],
        "entries": {"loader_disc_cut": "8000B07C", "delivery_callback": "800412C4", "phase_zero": "800541AC",
                    "normal_installer": "801301F8", "body_view_bind": "800419EC", "collision_reset": "8003CC24",
                    "body_shapes": "8008AB30", "body_update": "80054070"},
        "cases": cases,
        "notes": [
            "The old normal body reaches the established phase-3 queue path. The collision list walk returns its marked receiving shape before replacement startup.",
            "The host supplies only exact read-only decompressed role bytes at the original request destinations. The original completion callback is invoked with its declared seat and color; no disc I/O, asynchronous completion or actual callback scheduling is replayed.",
            "Exact raw roles without an allocated archive owner leave phase zero refused. A source-allocated owner, view pools and initialized SDK heap let the original normal actor wrapper return.",
            "The constructor builds the view records, writes level-1 HP and stock, and resets collision history. Neither the old 16-point damage ledger nor the zero Force snapshot is reset by this scoped visit.",
            "With 16 offered cells all body descriptors bind, but the initial role mask suppresses ordinary receiving publication. Changing only bit 02 in a discarded fork exposes the exact receiving shapes; this does not qualify when the game opens that mask.",
            "No subsequent input or match frame, loaded original Sasuke rig teardown, GX drawing, scene transition or native gameplay is qualified.",
        ],
    }, expanded=("delivered_roles", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
