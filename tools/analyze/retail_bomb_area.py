"""Verify Sasuke's type-5/key-0 area from the original direct bomb hit."""
import argparse
from hashlib import sha256
from io import BytesIO
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_bomb_contact as contact, retail_dragon_area as area
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_type0_contact import RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import MEMBER_LIMIT, PzzArchive, RangeFile, RetailArchive
from tools.extract.source_image import DolImage


SASUKE = 0x000A
PHASES = (*area.PHASES, 0x800864E4)
SLICES = {
    **{at: (size, digest) for at, size, digest in area.TEXT if at in (0x80086048, 0x80086084, 0x8008AE60, 0x80040F74)},
    0x8008AE3C: (36, "4a5238df2bbfcfc97ddc6e932511a41407c18a16b307bcfa3eb7827b46ecd02f"),
    0x80086268: (80, "164154efe400bd3ddf82fb6ac13bf79fe2d85958bd6a2377a27a4e5ca616515b"),
    0x800862B8: (152, "375e4c6c4edbe572e062666f083c96e75a54e2dc539c8ee70e66593203d2a991"),
    0x80086350: (236, "77b8a014f1e649b7f61c89136bb96c285e7953c114dfb1499b4d8f74c800bfdc"),
    0x8008643C: (148, "bad891ff84b634baa35dbb901bf5c083debb18ef1282bb49fbabc3e656544466"),
    0x800864D0: (20, "9212e9caa3a25e54a968e0a58a25f7946023df4677e5701fd0ee94e4396d8819"),
    0x800864E4: (32, "3a1f67e030b60f5a034b1475777e047d9dd71efc025f953dd4b7af7f8677fc51"),
    0x8008AA40: (240, "48b0c49de83b28a396e02f233ec8298596003ab69f3746744c69ab3acb14d682"),
}
COMMON = 696
GROUP = 3
STEP = 4
REACTION_TABLE = 0x802C4760
AMOUNT = REACTION_TABLE+24
ROW = re.compile(r"bomb-area:(?: [0-9]){3}(?: [0-9]{1,3}\.[0-9]{4}){3}(?: [0-9]{1,3}){6}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 384 or len(lines) != 2:
        raise ValueError("bomb area report extent")
    results = []
    for free, line in enumerate(lines):
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed bomb area report row")
        fields = line.split()[1:]
        values = tuple(map(int, fields[:3]))+tuple(map(float, fields[3:6]))+tuple(map(int, fields[6:]))
        expected = ((1, 1, 2, 3.1, 34.1, 387.5, 17, 2, 155, 17, 50, 1)
                    if free else (0, 0, 0, 0, 0, 0, 0, 0, 172, 0, 2, 1))
        if values != expected:
            raise ValueError("bomb area bind, hit or lifetime mismatch")
        _, cue, token, radius, grown, peak, damage, duplicates, hp, unlinked, returned, stock = values
        results.append({
            "case": "group-3-binds" if free else "empty-shape-pool-at-bind",
            "free_shape_cells_at_init": free, "bound_shape": bool(free),
            "one_shape_becomes_free_after_refused_bind": not free,
            "initial_attack_registry_count": 0, "child_audio_request": 12 if cue else None,
            "child_token": token or None, "initial_bound_radius": radius if free else None,
            "growth_visit_one": {"radius": grown, "attack_registry_count": 1,
                                 "received_record": address(AMOUNT)} if free else None,
            "peak_registered_radius": peak if free else None, "child_amount": damage,
            "duplicate_checks_without_another_write": duplicates,
            "receiver_hp_after_direct_hit": 172, "receiver_hp_after_child": hp,
            "receiver_hit_tokens": [1, token] if free else [1], "credited_and_received_damage": 180-hp,
            "shape_unlink_visit": unlinked or None, "carrier_and_both_descriptors_return_visit": returned,
            "stock_after": stock,
        })
    return results


def qualified_inlet(image, archive):
    contact.qualify(image, archive)  # Pin the predecessor release, direct hit and child allocation.
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"bomb area source signature mismatch: {address(entry)}")
    if (struct.unpack(">4I", image.read(area.PHASE_TABLE, 16, "data")) != PHASES
            or struct.unpack(">3I", image.read(area.PHASE_TABLE+16, 12, "data")) != (0x800862B8, 0x80086350, 0x8008643C)):
        raise ValueError("bomb area phase or active subphase table mismatch")
    table = image.read(area.TABLE, 27*36, "data")
    if sha256(table).hexdigest() != area.TABLE_SHA:
        raise ValueError("bomb area row table identity")
    record, group, hold, material, initial, limit, gain, _, _, _, cue = struct.unpack_from(">4h6fi", table)
    threshold, = struct.unpack(">f", image.read(0x80437C44, 4, "data"))
    if (record, group, hold, material, limit, cue) != (22, GROUP, 4, 0, 1.25, 12) or initial != threshold:
        raise ValueError("bomb area key-0 operands")

    if len(archive.names) <= COMMON or archive.names[COMMON] != b"cmn_data.pzz" or archive.names.count(b"cmn_data.pzz") != 1:
        raise ValueError("bomb area common member identity")
    payload = archive.payload(COMMON, MEMBER_LIMIT)
    bank, _ = PzzArchive(RangeFile(BytesIO(payload), 0, len(payload))).read(0, area.COMMON_COLLISION_SIZE)
    if len(bank) != area.COMMON_COLLISION_SIZE or sha256(bank).hexdigest() != area.COMMON_COLLISION_SHA:
        raise ValueError("bomb area common collision block identity")
    if bank[4*GROUP:4*(GROUP+1)] != bytes([STEP, 255, 255, 255]):
        raise ValueError("bomb area common group mismatch")
    off = 0x400+STEP*80
    if (struct.unpack_from(">BBHHhhH", bank, off) != (0, 0, 1, 1, 0, -1, 0)
            or struct.unpack_from(">4f", bank, off+12) != (0, 0, 0, 310)
            or struct.unpack(">H", image.read(AMOUNT, 2, "data")) != (20,)):
        raise ValueError("bomb area descriptor or common reaction mismatch")
    hit, spec = read_normal(archive, SASUKE, RECEIVER_BLOCK_SHA256, contact.ACTOR_SHA256[SASUKE])
    return hit+spec+bank, {
        "common_collision": {"member": COMMON, "block": 0, "sha256": area.COMMON_COLLISION_SHA, "group": GROUP,
                             "descriptor": STEP, "center": [0, 0, 0], "raw_radius": 310,
                             "context_assignment": "80040FC0"},
        "reaction": {"table": address(REACTION_TABLE), "index": 1, "record": address(AMOUNT), "raw_power": 20},
        "key_zero_operands": {"row": address(area.TABLE), "visual_record": record, "cue": cue, "scale_limit": limit,
                              "initial_scale": initial, "growth_increment": gain, "scale_gate_scalar": threshold},
    }


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("bomb area output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, source = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("bomb area AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--sasuke-area"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"bomb area fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    receipt = {
        "kind": "gf-retail-bomb-area", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "owner": "gg4e08:000a", "opening": {"type": 15, "key": 3, "mode": "ordinary-X"},
        "area": {"class": 3, "type": 5, "key": 0, "descriptor_cells": 2},
        "receiver": {"borg": "gg4e08:000a", "initial_hp": 180, "body_step": 9, "declared_center_offset_x": 30},
        "source": source, "cases": cases,
        "notes": [
            "Starts at the paid release leaf; the parent hits the single declared body before the area is allocated.",
            "The area initializer stops at the sound request after a successful bind. No sound result or playback is supplied.",
            "Initial shape bind and attack registry admission are separate. The original scale gate refuses the initial shape.",
            "The same receiver admits the new area token after one growth visit, then refuses both rechecks of that token.",
            "Visits are isolated bucket/shape passes counted from the first area initializer, not live battle frame counts.",
            "Only this Sasuke child path is verified. Other owners, terrain triggering, visual playback, and native gameplay are unchanged.",
        ],
    }
    raw = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
