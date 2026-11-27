"""Join type-60 contact to the original receiver size queue and expiry."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_type60_contact as cast


vm = cast.vm
CONTACT_SHA256 = "9f2a1a2a0ae13faa8b2d82a759850bc4fef14a83213fb7d4f968a1886c584b9d"
LARGE = (0x0602, "3a52950b9cb3e5fef91747380f59c68e267ca68b84ef794b28172a50a3bd4a90",
         "41256e7cf7c80f78a33d5649c69cc6222d892eab985b457ce178c3abfd6f0dba")
CASES = ("opponent", "opponent-no-effect-cell", "receiver-mask-refused", "at-upper-size-bound", "ally")
TEXT = (
    (0x8002BB14, 3712, "9aa80c3f876242ebee4cbf8afd5ede6938e90b767f8da7f4f8d9ab6d4fad1911"),
    (0x800562B8, 1444, "f2cd0805aa1b4be0c29b96adf31b92c0f289b243496d863c0262909316ad7e95"),
    (0x80059068, 208, "ea22383e1d48094534b9c882b22400b794423bdea5657be04bcfab050da3716c"),
    (0x8005BAB8, 264, "d857580767ae7b19893084b31d12b3a0c9a606a7891ae5e4f3cbaa7672c0ee46"),
    (0x8006817C, 104, "cc7a1791c3441a7767e2999ffe19791d625384942e34ec13b0ff5d0639063f2d"),
    (0x800681E4, 136, "f1ba318aa385e06154b97172910182b31e8584fcae256cb104c0b115c9bba8ae"),
    (0x8006826C, 140, "b5984640399d2952aedfaa554a6459035788415792189757db695fe6c0b4a7bf"),
)
ROW = re.compile(r"type60-scale:(?: [0-9]{1,4}){4} -?[0-9] [01](?: -?[0-9]{1,2}){3} [0-9]{1,3}(?: [0-9A-F]{8}){8}")


def steps(image):
    table = vm.beam.word(image, 0x804339E0, "data")
    ramp = struct.unpack(">f", image.read(0x8043EA20-30208, 4, "data"))[0]
    if table != 0x802DD5A0 or struct.pack(">f", ramp) != bytes.fromhex("3d4ccccd"):
        raise ValueError("type-60 original scale table or step changed")
    return {i: struct.unpack(">3f", image.read(table+i*12, 12, "data"))
            for i in (14, 15, 16, 17, 18, 29, 30, 31)}, ramp


def decode(report, table, ramp):
    lines = report.splitlines()
    if len(report) > 3072 or len(lines) != len(CASES)*len(cast.ROUTES):
        raise ValueError("type-60 scale report extent")
    rows = []
    for r in cast.ROUTES:
        for i, name in enumerate(CASES):
            blocked = i == 2
            hp = 1050 if blocked else 180
            fx = 15 if blocked else 0 if i == 1 else 33
            start = 31 if i == 3 else 16
            delta = 0 if blocked else (2 if r.key >= 2 else 1)*(-1 if r.key % 2 else 1)
            level = start+delta
            effective = min(level, 31)
            start_scale = table[start][0]
            target, rate, derived = table[effective]
            first = min(target, start_scale+ramp) if target >= start_scale else max(target, start_scale-ramp)
            expire = min(start_scale, target+ramp) if start_scale >= target else max(start_scale, target-ramp)
            scalars = (start_scale, target, rate, derived, first, target, expire, start_scale)
            words = tuple(struct.pack(">f", value).hex().upper() for value in scalars)
            line = lines[len(rows)]
            integers = (r.model, i, hp, fx, delta, not blocked, level, effective, delta, 0 if blocked else 900)
            if ROW.fullmatch(line) is None:
                raise ValueError("invalid scale report row")
            fields = line.split()[1:]
            if tuple(map(int, fields[:10])) != integers or tuple(fields[10:]) != words:
                raise ValueError(f"type-60 scale changed: {r.model:04X}/{name}")
            rows.append({
                "borg": f"gg4e08:{r.model:04x}", "case": name, "receiver": "gg4e08:0602" if blocked else "gg4e08:000a",
                "receiver_hp_after": hp, "effect_type_allocated": fx or None, "queued_signed_delta": delta,
                "scene_size_consumer_called": not blocked, "raw_size_index_before": start,
                "raw_size_index_after": level, "clamped_size_index_after": effective,
                "temporary_size_offset_after": delta, "timer_after_scene": integers[-1],
                "float_bits": dict(zip(("applied_before_and_after_scene", "target_after_scene", "rate_operand_after_scene",
                                        "derived_operand_after_scene", "applied_on_first_tick", "applied_before_expiry",
                                        "applied_on_expiry_tick", "applied_after_return"), words, strict=True)),
            })
    return rows


def check_recipients(image, archive, installed):
    hurt, sasuke = vm.opening.read_normal(archive, vm.opening.RECEIVER, vm.opening.RECEIVER_BLOCK_SHA256,
                                         vm.ACTOR_SHA256[vm.opening.RECEIVER])
    large_hit, large = vm.opening.read_normal(archive, *LARGE)
    if (sasuke[0xA1:0xA4] != bytes((31, 8, 0)) or sasuke[0xA8:0xAC] != bytes(4) or
        large[0xA1:0xA4] != bytes((24, 4, 0)) or large[0xA8:0xAC] != bytes.fromhex("1f1c3c2c") or
        vm.reaction_installer_tables.model_table(image, LARGE[0])[1] != 0x8030C570 or
        vm.levels(image, LARGE[0])[0][0] != 1050 or
        struct.unpack_from(">BBH", large_hit, 32+9*80) != (0, 2, 6) or
        struct.unpack_from(">4f", large_hit, 32+9*80+12) != (0, -40, 20, 80) or
        9 not in large_hit[:large_hit[:32].index(255)]):
        raise ValueError("type-60 scale exact receiver source changed")
    counts, blocked = Counter(), []
    for row in installed["borgs"]:
        model = int(row["id"][-4:], 16)
        member = archive.names.index(f"pl{model:04x}.pzz".encode())
        sections, _ = archive.member(member, model)
        records = sections.read(0, 432*8)[0]
        off = row["spec_record"]*432+0xA8
        mask = struct.unpack_from(">H", records, off)[0]
        counts[mask] += 1
        if mask & 12:
            blocked.append({"borg": row["id"], "spec_record": row["spec_record"], "mask": f"{mask:04X}"})
    if counts != {0: 181, 0x1000: 8, 0x0F1C: 1, 0x1F1C: 15, 0x1F1E: 1} or len(blocked) != 17:
        raise ValueError("type-60 installed scale mask census changed")
    return hurt+sasuke+large_hit+large, blocked, {f"{mask:04X}": n for mask, n in sorted(counts.items())}


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-60 scale output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = vm.DolImage(vm.read_bounded(dol, 8 << 20), vm.SOURCE_SHA256)
    vm.beam.verify_text(image, (*vm.TEXT, *cast.TEXT, *TEXT))
    for site, target in ((0x8002C4CC, 0x8002DB58), (0x8002C5D0, 0x800681E4), (0x8005909C, 0x8005BAB8),
                         (0x8005BAF4, 0x8006817C), (0x800681D0, 0x8006826C), (0x8002F698, 0x800DB250)):
        vm.beam.link(image, site, target)
    vm.read_metadata(vm.MAPS / "gg4e08-type60-contact.json", CONTACT_SHA256)
    atlas = vm.read_metadata(vm.MAPS / "gg4e08-code-atlas.json", vm.ATLAS_SHA256)
    installed = vm.read_metadata(vm.MAPS / "gg4e08-standalone-installers.json", vm.alternate.INSTALLERS_SHA256)
    with vm.open_regular(afs) as stream:
        stamp = vm.archive_stamp(stream)
        vm.verify_archive(stream)
        archive = vm.RetailArchive(stream)
        defenders, blocked, masks = check_recipients(image, archive, installed)
        _, casters = cast.source_routes(image, archive, atlas)
        if vm.archive_stamp(stream) != stamp:
            raise ValueError("type-60 scale archive input changed")
    table, ramp = steps(image)
    rows = decode(vm.clock.fixture_report(fixture, dol, module, ("--scale",), defenders+casters), table, ramp)
    raw = vm.encode({
        "kind": "gf-retail-type60-scale", "revision": 1, "source_dol_sha256": vm.SOURCE_SHA256,
        "source_afs_sha256": vm.AFS_SHA256, "type60_contact_sha256": CONTACT_SHA256,
        "standalone_installers_sha256": vm.alternate.INSTALLERS_SHA256,
        "entries": {"pair": "8002DB58", "scene_prelude": "8002BB14", "size_apply": "800681E4",
                    "size_table_lookup": "8006826C", "size_tick": "8005BAB8", "size_delta_bank": "8043612C"},
        "normal_spec_mask_counts": masks, "normal_size_mask_exclusions": blocked, "cases": rows,
        "notes": [
            "Spec +0xA8 is copied to actor +0x59C by the original initializer. Either set bit is an exclusion for its polarity, not an enable bit.",
            "The scene fork runs the original prelude from before contact to its next reaction or callback frontier. It has one active receiver record, not a live match.",
            "A separate pair-only fork checks the per-slot signed delta before the scene applies it. Receiver pose, initial size index and resources are declared.",
            "The timer is measured in isolated size-callback visits with no contact refresh or full-frame scheduler. The type-33 effect is not ticked.",
            "The two other target-table operands are recorded as fields, not qualified locomotion or animation changes. No native gameplay is enabled.",
        ],
    }, expanded=("normal_size_mask_exclusions", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows), "normal_mask_exclusions": len(blocked)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
