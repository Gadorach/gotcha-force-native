"""Replay the original side placement loop across all GG4E08 setNNNN archives."""
import argparse
from collections import Counter
from hashlib import sha256
from io import BytesIO
import json
from math import isfinite
from pathlib import Path
import struct

from tools.analyze import retail_stage_prop_recovery as recovery
from tools.extract.pzz import PzzArchive
from tools.extract.retail_borg_roster import RangeFile


body = recovery.body
SIDE_SET_SHA256 = "f5c82ad47bdd9076f5d2fe2e62832965ba57efeac28c54fbb52ea48c9ebc88b3"
TYPE_TABLE_SHA256 = "1ab770d68e89a0871634794a65541d3cbfac55e78f04f3e6514dc61678511b7c"
NAMES = tuple(f"set{group:02x}{variant:02x}.arc" for group in range(16)
              for variant in ((4,) if group == 9 else range(4)))
TEXT = (
    (0x800818D0, 176, "7163a099b92a8d8fc646c10e80aaa935f17f2bc8a599dfcfae2cf3634ec36c46"),
    (0x80081D20, 120, "9dbd3611c9afb523d353d0175f3a18c49ed07b5b8eec9133bdb2538f2a07789d"),
    (0x80040EB4, 40, "809415a08cc1c8c2421222121a472454a4a95582e8dc5c44e99b0f613d658453"),
    (0x80089100, 568, "aa8525094a1010f6a9580f8d8d8413754b0204b809fb9f18536f97a640d650e5"),
    (0x80089554, 260, "14d54d2183c44456974e44418019670cfd857a373e5baced583924c4c74280df"),
    (0x80089A94, 88, "dcb8bf7edce7313e0e258ee526af82153e5e35eac0987938a42cccc6a270dde4"),
    (0x8008A28C, 48, "94cb040d0d0bd380e6ab2d81aec5d82583aaf212f52408a7050212f7e39acc08"),
    (0x80089AEC, 276, "03e4a562cd8742937f8f85f602f0f3a3a682457f8f5887accc0b15cbf24f4802"),
    (0x8008A65C, 996, "92fa6953e7aea5fe06460450ea539bf738406dc0632251b89ccba4c973b3adf4"),
)


def read_side(raw):
    if not 6144 <= len(raw) <= 1 << 18 or len(raw) % 2048:
        raise ValueError("stage side archive extent")
    sectors, placement_sectors = struct.unpack_from(">II", raw)
    if not 2 <= sectors <= 5 or placement_sectors != 1:
        raise ValueError("stage side archive sector count")
    bank_sectors = struct.unpack_from(f">{sectors-1}I", raw, 8)
    if not all(bank_sectors) or (2+sum(bank_sectors))*2048 != len(raw):
        raise ValueError("stage side archive bank extent")
    offset = 4096
    banks = []
    for sector_count in bank_sectors:
        file_size = struct.unpack_from(">I", raw, offset)[0]
        limit = offset+2048*sector_count
        if not 32 <= file_size <= limit-offset:
            raise ValueError("stage side HSD bank extent")
        banks.append(sha256(raw[offset:offset+file_size]).hexdigest())
        offset = limit
    n = struct.unpack_from(">I", raw, 2048)[0]
    if not 1 <= n <= 16:
        raise ValueError("stage side placement count")
    placements = []
    for i in range(n):
        t, record, variant, bank, *pose = struct.unpack_from(">4B6f", raw, 2052+28*i)
        if t >= 12 or variant > 1 or bank >= len(banks) or not all(map(isfinite, pose)):
            raise ValueError("stage side placement row")
        placements.append((t, record, variant, bank))
    return banks, placements


def read_types(image, common):
    raw = image.read(0x802D7990, 12*20, "data")
    if sha256(raw).hexdigest() != TYPE_TABLE_SHA256:
        raise ValueError("stage side original type table changed")
    types = []
    for i in range(12):
        hp, selector, cull, radius, program, duration = struct.unpack_from(">HHb3xIIh2x", raw, 20*i)
        keys = common[4*selector:4*selector+4]
        descriptors = tuple(keys.split(b"\xff", 1)[0])
        if not descriptors or keys[len(descriptors):] != b"\xff"*(4-len(descriptors)):
            raise ValueError("stage side common selector changed")
        shapes = []
        for key in descriptors:
            kind, _, role = struct.unpack_from(">BBH", common, 0x400+80*key)
            if role not in (0x06, 0x26):
                raise ValueError("stage side common descriptor role changed")
            shapes.append({"descriptor": key, "shape_kind": kind, "descriptor_role": role})
        types.append({
            "type": i, "collision_owner_hp": hp, "common_selector": selector, "model_cull_operand": cull,
            "radius_bits": f"{radius:08X}", "program_key": f"{program:08X}", "break_duration_operand": duration,
            "collision_shapes": shapes,
        })
    return types


def source_inputs(image, afs):
    recovery.verify_source(image)
    body.verify_text(image, TEXT)
    for site, target in ((0x80081904, 0x80081980), (0x800819A0, 0x80081D20),
                         (0x80081D48, 0x8008893C), (0x80081D58, 0x800819B4), (0x80081D74, 0x80047AA4)):
        body.link(image, site, target)
    if image.read(0x802D7AD0, 16, "data") != bytes.fromhex("80081DD480081DE480081FF880082040"):
        raise ValueError("stage side update table changed")
    with body.open_regular(afs) as stream:
        stamp = body.archive_stamp(stream)
        body.verify_archive(stream)
        archive = body.RetailArchive(stream)
        if archive.names[696] != b"cmn_data.pzz" or tuple(archive.names[2419:2480]) != tuple(x.encode() for x in NAMES):
            raise ValueError("stage side AFS member names changed")
        packed = archive.payload(696, 2 << 20)
        if sha256(packed).hexdigest() != recovery.prop.COMMON_SHA256:
            raise ValueError("stage side common archive changed")
        common, _ = PzzArchive(RangeFile(BytesIO(packed), 0, len(packed))).read(0, 21504)
        if len(common) != 21504 or sha256(common).hexdigest() != recovery.prop.BANK_SHA256:
            raise ValueError("stage side common collision bank changed")
        members = []
        payload = bytearray(common)
        for member, name in enumerate(NAMES, 2419):
            raw = archive.payload(member, 1 << 18)
            banks, placements = read_side(raw)
            members.append((member, name, sha256(raw).hexdigest(), banks, placements))
            payload.extend(struct.pack(">II", member, len(raw)))
            payload.extend(raw)
        if body.archive_stamp(stream) != stamp:
            raise ValueError("stage side archive changed")
    if sha256(memoryview(payload)[len(common):]).hexdigest() != SIDE_SET_SHA256:
        raise ValueError("stage side retained payload set changed")
    return payload, members, read_types(image, common)


def decode(report, members, types):
    lines = report.splitlines()
    if len(report) > 1 << 16 or len(lines) != sum(2+len(m[-1]) for m in members):
        raise ValueError("stage side census report extent")
    pos = 0
    observed_banks = {}
    cases = []
    type_counts = Counter()

    def take(tag, fields, member):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"stage side census changed: {member}/{tag}")
        pos += 1

    for member, name, digest, banks, placements in members:
        prefix = f"side-banks: {member} {len(banks)} "
        suffix = lines[pos].removeprefix(prefix).split()
        if not lines[pos].startswith(prefix) or len(suffix) != len(banks) or not all(
            v.isascii() and v.isdecimal() and len(v) <= 3 and 1 < int(v) <= 255 for v in suffix
        ):
            raise ValueError(f"stage side census changed: {member}/side-banks")
        counts = list(map(int, suffix))
        take("side-banks", (member, len(banks), *counts), member)
        for b, c in zip(banks, counts, strict=True):
            if observed_banks.setdefault(b, c) != c:
                raise ValueError(f"stage side bank record count changed: {member}")
        n3 = sum(not row[2] for row in placements)
        n2 = len(placements)-n3
        shapes_n = common_n = 0
        rows = []
        for t, record, variant, bank in placements:
            type_counts[t, variant] += 1
            join = types[t]
            shapes = join["collision_shapes"] if not variant else []
            shape_fields = [(s["descriptor"], 1+2*bool(s["descriptor_role"] & 0x20)) for s in shapes]
            fields = (t, record, variant, bank, 2 if variant else 3,
                      0 if variant else join["collision_owner_hp"],
                      0 if variant else join["break_duration_operand"] & 0xFFFF,
                      0 if variant else join["common_selector"], *(f"{key}/{roles}" for key, roles in shape_fields))
            rows.append(fields)
            shapes_n += len(shapes)
            common_n += sum(roles == 3 for _, roles in shape_fields)
            if variant >= counts[bank]:
                raise ValueError(f"stage side model record outside loaded bank: {member}")
        take("side-census", (member, len(placements), n2, n3, n2+2*n3, shapes_n, shapes_n, common_n), member)
        for ordinal, fields in enumerate(rows):
            take("side-owner", (member, ordinal, *fields), member)
        cases.append({
            "member": member, "filename": name, "sha256": digest, "banks": counts,
            "class2_model_only": n2, "class3_collision_owners": n3, "model_roots": n2+2*n3,
            "published_ordinary_receiver_entries": shapes_n, "published_common_role20_entries": common_n,
            "placement_rows": placements,
        })
    counts = [21, 22]*4+[2, 3, 2, 1]
    if len(cases) != 61 or type_counts != Counter({(t, v): n for t, n in enumerate(counts) for v in (0, 1)}):
        raise ValueError("stage side type/variant coverage changed")
    for t in types:
        t["class2_visits"] = t["class3_visits"] = counts[t["type"]]
    return cases


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stage side census output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    body.read_metadata(body.MAPS / "gg4e08-stage-prop-support.json", recovery.PROP_SHA256)
    payload, members, types = source_inputs(image, afs)
    cases = decode(body.fixture_report(fixture, dol, module, payload=payload), members, types)
    totals = {key: sum(row[key] for row in cases) for key in (
        "class2_model_only", "class3_collision_owners", "model_roots",
        "published_ordinary_receiver_entries", "published_common_role20_entries",
    )}
    raw = body.encode({
        "kind": "gf-retail-stage-side-census", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "side_input_set_sha256": SIDE_SET_SHA256,
        "stage_prop_support_sha256": recovery.PROP_SHA256, "type_table_sha256": TYPE_TABLE_SHA256,
        "common_collision_bank_sha256": recovery.prop.BANK_SHA256, "archive_loader": "8004279C",
        "placement_loop": "800818D0", "owner_kind": 22, "owner_bucket": 40,
        "placement_row_columns": ["type", "group_mask_bit_index", "model_variant", "loaded_bank_index"],
        "totals": totals, "types": types, "archives": cases,
        "notes": [
            "All 61 named setNNNN side archives are replayed individually. The separate set00_mdl archive is not a side placement table.",
            "The original loader and placement loop consume each retained member inside a declared heap and pool setup. Rows with nonzero byte 2 create class-2 model-only objects; zero creates class-3 collision owners with two separate model roots.",
            "The original bucket update activates each owner. Only class-3 owners register the world callback; the model-only constructor evaluates its own initial world. Both paths reach the authored positions, then the original shape walk publishes the common cells.",
            "Collision shape lists have distinct cells even for repeated descriptors. Common role-0x20 membership includes non-plane shapes and does not by itself qualify support acquisition.",
            "The type table is source data, not a recovery replay on every variant. The -1 operand is distinct from the 600-visit types. No live stage selection, full arena scheduling, support on other variants, drawing, audio, hardware or native gameplay is qualified.",
        ],
    }, expanded=("types", "archives"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "archives": len(cases), **totals}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
