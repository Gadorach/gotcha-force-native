"""Trace retail size rows into published hurt radius and isolated movement clocks."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_type60_refresh as refresh


scale, cast, vm = refresh.scale, refresh.cast, refresh.vm
DRAGON = (0x0502, "0f8ac385a01d317f0f80d666a6bf88fcacc4e7ef0c12940ff2f4494e43f830be",
          "dd9cc8a759e46ca791593db53275ddd8e059be7dc23a6de07b824002ff0723db")
STAGES = ("before-contact", "after-scene-size-write", "first-size-tick", "settled-before-expiry", "expiry-tick", "restored")
TEXT = (
    (0x8003A4B8, 300, "b3238a9a5df814d1f999a4007e720d90c7c090d74a263d9542e138a21044aea2"),
    (0x80039F6C, 240, "ea15e86d0428a669821b13e9d6131da6e5c4947d84970604cea6527186912e23"),
    (0x8005A378, 520, "b6a3dfe9d8bf1b34fb4d470444385f1bbdafe4ba0596786d67699ca9e92ecd2f"),
    (0x80067610, 200, "20b7a06e35736f9a925830866a3e0a51f1689dbbdd2a2a536bbe203413a3f451"),
    (0x8008A65C, 996, "92fa6953e7aea5fe06460450ea539bf738406dc0632251b89ccba4c973b3adf4"),
)
ROW = re.compile(r"size-consumer:(?: [0-9]{1,4}){6}(?: [0-9A-F]{8}){8}")
PATTERNS = {
    (0, 4, 24): 28, (0, 8, 16): 5, (0, 8, 18): 1, (0, 8, 20): 1, (0, 8, 23): 1, (0, 8, 25): 1,
    (0, 8, 27): 5, (0, 8, 28): 15, (0, 8, 29): 3, (0, 8, 31): 125, (0, 10, 28): 1,
    (1, 4, 18): 1, (1, 4, 21): 13, (1, 6, 21): 2, (1, 8, 21): 4,
}


def f32(value):
    return struct.unpack(">f", struct.pack(">f", value))[0]


def decode(report, tables, ramp):
    lines = report.splitlines()
    if len(report) > 6144 or len(lines) != len(cast.ROUTES)*2*len(STAGES):
        raise ValueError("size consumers report extent")
    rows = []
    timers = (0, 900, 899, 1, 0, 0)
    normal_operand = struct.unpack(">f", bytes.fromhex("3f7fffff"))[0]
    for route, delta in zip(cast.ROUTES, (1, 2, -1, -2), strict=True):
        for model, radius, family in ((0x000A, 22, 0), (0x0502, 40, 1)):
            table = tables[family]
            size, rate, movement = table[16+delta]
            applied = (1.0, 1.0, refresh.advance(1.0, size, ramp),
                       size, refresh.advance(size, 1.0, ramp), 1.0)
            for i, name in enumerate(STAGES):
                nominal, secondary, distance = (size, rate, movement) if 1 <= i <= 3 else (1.0, 1.0, 1.0)
                current_radius = f32(radius*applied[i])
                last_pass = int(f32(50+current_radius))
                hold = 4 if i else 0
                first_rate = 0.03 if hold else 1.0
                expected = (route.model, model, i, hold, last_pass, timers[i])
                bits = tuple(map(refresh.float_word, (applied[i], nominal, current_radius, first_rate, first_rate,
                                                       1.0, secondary, normal_operand*f32(10*distance))))
                line = lines[len(rows)]
                if ROW.fullmatch(line) is None:
                    raise ValueError("invalid size consumer row")
                fields = line.split()[1:]
                if tuple(map(int, fields[:6])) != expected or tuple(fields[6:]) != bits:
                    raise ValueError(f"size consumers changed: {route.model:04X}/{model:04X}/{name}")
                rows.append({
                    "caster": f"gg4e08:{route.model:04x}", "receiver": f"gg4e08:{model:04x}",
                    "size_family": family, "stage": name, "size_timer": timers[i], "hit_hold_inlet": hold,
                    "coarse_axis_pair_pass_distance": last_pass, "coarse_axis_pair_fail_distance": last_pass+1,
                    "float_bits": dict(zip(("applied_scale", "target_scale", "published_receiver_radius",
                                            "first_primary_rate", "first_secondary_rate", "primary_after_hold",
                                            "secondary_after_hold", "x_displacement_after_hold"), bits, strict=True)),
                })
    return rows


def source_inputs(image, archive, installed):
    hurt, spec = vm.opening.read_normal(archive, vm.opening.RECEIVER, vm.opening.RECEIVER_BLOCK_SHA256,
                                       vm.ACTOR_SHA256[vm.opening.RECEIVER])
    dragon_hit, dragon = vm.opening.read_normal(archive, *DRAGON)
    if (spec[0xA1:0xA4] != bytes((31, 8, 0)) or dragon[0xA1:0xA4] != bytes((21, 6, 1)) or
        (struct.unpack_from(">H", dragon, 0xA8)[0] & 12) or
        vm.reaction_installer_tables.model_table(image, DRAGON[0])[1] != 0x8033B870 or
        vm.levels(image, DRAGON[0])[0][0] != 500 or
        struct.unpack_from(">BBHH", dragon_hit, 32+80) != (0, 23, 2, 0) or
        struct.unpack_from(">4f", dragon_hit, 32+80+12) != (0, -10, 30, 40) or dragon_hit[1] != 1):
        raise ValueError("size consumer exact receiver changed")
    counts, exclusion = Counter(), Counter()
    for row in installed["borgs"]:
        model = int(row["id"][-4:], 16)
        sections, _ = archive.member(archive.names.index(f"pl{model:04x}.pzz".encode()), model)
        records = sections.read(0, 432*8)[0]
        off = row["spec_record"]*432
        hi, lo, table = records[off+0xA1:off+0xA4]
        mask = struct.unpack_from(">H", records, off+0xA8)[0]
        counts[table, lo, hi] += 1
        exclusion[table] += bool(mask & 12)
    if counts != PATTERNS or exclusion != {0: 10, 1: 7}:
        raise ValueError("size roster family, clamp or exclusion census changed")
    return hurt+spec+dragon_hit+dragon, [
        {"family": fam, "minimum": lo, "maximum": hi, "normal_records": count}
        for (fam, lo, hi), count in sorted(counts.items())
    ]


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("size consumers output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = vm.DolImage(vm.read_bounded(dol, 8 << 20), vm.SOURCE_SHA256)
    vm.beam.verify_text(image, (*vm.TEXT, *cast.TEXT, *scale.TEXT, *refresh.TEXT, *TEXT))
    for site, target in ((0x8002DCEC, 0x80039F6C), (0x80059118, 0x8005A378), (0x8006766C, 0x8020BAB8)):
        vm.beam.link(image, site, target)
    vm.read_metadata(vm.MAPS / "gg4e08-type60-scale.json", refresh.SCALE_SHA256)
    atlas = vm.read_metadata(vm.MAPS / "gg4e08-code-atlas.json", vm.ATLAS_SHA256)
    installed = vm.read_metadata(vm.MAPS / "gg4e08-standalone-installers.json", vm.alternate.INSTALLERS_SHA256)
    with vm.open_regular(afs) as stream:
        stamp = vm.archive_stamp(stream)
        vm.verify_archive(stream)
        archive = vm.RetailArchive(stream)
        defenders, census = source_inputs(image, archive, installed)
        _, casters = cast.source_routes(image, archive, atlas)
        if vm.archive_stamp(stream) != stamp:
            raise ValueError("size consumers archive input changed")
    first, ramp = scale.steps(image)
    if vm.beam.word(image, 0x804339E4, "data") != 0x802DD720 or vm.beam.word(image, 0x802DA758, "data") != 0x8003A4B8:
        raise ValueError("size consumers second table or sphere handler changed")
    second = {i: struct.unpack(">3f", image.read(0x802DD720+12*i, 12, "data")) for i in range(14, 19)}
    rows = decode(vm.clock.fixture_report(fixture, dol, module, ("--consumers",), defenders+casters), (first, second), ramp)
    raw = vm.encode({
        "kind": "gf-retail-size-consumers", "revision": 1, "source_dol_sha256": vm.SOURCE_SHA256,
        "source_afs_sha256": vm.AFS_SHA256, "type60_scale_sha256": refresh.SCALE_SHA256,
        "standalone_installers_sha256": vm.alternate.INSTALLERS_SHA256,
        "table_roots": ["802DD5A0", "802DD720"], "normal_records_by_family": [186, 20],
        "normal_size_exclusions_by_family": [10, 7], "normal_clamp_profiles": census, "cases": rows,
        "notes": [
            "One exact type-60 contact is joined to a stationary receiver from each size family. Each rig and socket is declared.",
            "The original sphere publisher reads applied actor scale at +0xC0. Detached coarse-axis queries use copies of the published sphere fields, not later damage or exact overlap dispatch.",
            "Each stage forks for the clock test, with a declared unit lane factor. The hit timer observed in the main branch is not advanced by isolated size ticks.",
            "The clock fork expires any hit hold before the isolated movement helper consumes a declared axial direction and speed 10.",
            "This does not qualify live input scheduling, full joint poses or moving-body geometry, rendering, or native gameplay.",
        ],
    }, expanded=("normal_clamp_profiles", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows), "normal_clamp_profiles": len(census)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
