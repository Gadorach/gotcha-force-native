"""Replay type-89 cached area selectors through allocation and common shape bind."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables, retail_dragon_area, retail_type89
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_type5_roster import common_bank
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


# Model, carrier key, owner group, descriptor, reaction table, normal hit block.
PILOTS = (
    (0x0F03, 11, 2, (2, 2), 0x803810F8, "305d15c7c64b7208d4644ea05101c20a9dd6b7ee55a8d099c9dd0f437d56fcf4"),
    (0x062A, 16, 28, (37, 32), 0x80365EC0, "da9d7d33b1c8b66f93c0beb7572e5140f82ae120558e5f908398526fbd37e4c3"),
)
CASES = ("carrier-shape-empty", "neither-exit-gate", "contact-point", "alternate-root", "contact-wins",
         "area-carrier-empty", "area-descriptors-short")
ROW = re.compile(r"type89-area:(?: [0-9]{1,4}){9}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(PILOTS)*len(CASES):
        raise ValueError("type-89 area report extent")
    pilots = []
    for i, (model, key, group, *_) in enumerate(PILOTS):
        cases = []
        for n, case in enumerate(CASES):
            line = lines[i*len(CASES)+n]
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed type-89 area row")
            admitted, requested, made = n != 0, n >= 2, 2 <= n <= 4
            branch = 2 if n == 3 else 1 if requested else 0
            result = tuple(map(int, line.split()[1:]))
            if result != (model, n, int(admitted), 16 if admitted else 0, int(requested),
                          branch, int(made), 110 if made else 0, 5):
                raise ValueError(f"type-89 binding or cached key result mismatch: {model:04X}/{case}")
            cases.append({
                "case": case, "carrier_opening_shape_bound": admitted, "copied_area_key": 16 if admitted else None,
                "area_factory_requested": requested, "area_requested_key": 16 if requested else None,
                "origin": {"carrier_field": "0x2A0", "point": [510, 220, 630]} if branch == 1 else
                          {"carrier_field": "0x20", "point": [400, 210, 620]} if branch == 2 else None,
                "area_allocated": made, "area_bound_common_group": 110 if made else None, "owner_stock_after": 5,
            })
        pilots.append({"borg": f"gg4e08:{model:04x}", "carrier_key": key, "carrier_opening_group": group, "cases": cases})
    return pilots


def qualified_key16_bank(image, archive):
    retail_type89.verify_text(image, retail_dragon_area.TEXT)
    bank = common_bank(archive)
    if bank[4*110:4*(110+1)] != bytes([150, 255, 255, 255]):
        raise ValueError("type-89 area key-16 common group changed")
    row = 0x400+80*150
    if (struct.unpack_from(">BBHHhhH", bank, row) != (0, 0, 1, 23, 0, -1, 0)
            or struct.unpack_from(">4f", bank, row+12) != (0, 0, 0, 310)
            or image.read(0x802DA0B8+36*16+2, 2, "data") != struct.pack(">H", 110)):
        raise ValueError("type-89 area key-16 descriptor mismatch")
    return bank


def qualified_inlet(image, archive):
    rows = retail_type89.rows(image)
    bank = qualified_key16_bank(image, archive)
    blocks, sources = [], []
    for model, key, group, step, table, digest in PILOTS:
        block = reaction_emit.qualified_hit_block(archive, model)
        if (rows[key]["group"] != group or sha256(block).hexdigest() != digest
                or reaction_emit.read_group(block, group) != [step]
                or reaction_installer_tables.model_table(image, model)[1] != table):
            raise ValueError(f"type-89 normal shape source mismatch: {model:04X}")
        sources.append({"factory_row": rows[key]["row"], "hit_block_sha256": digest,
                        "opening_descriptor": step[0], "opening_reaction_index": step[1], "owner_table": address(table)})
        blocks.append(block)
    return b"".join(blocks)+bank, sources


def replay(dol, afs, fixture, module, timed=False):
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    schedule = retail_type89.timed_schedule(image) if timed else None
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-89 area AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    if timed:
        argv.append("--timed")
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-89 area fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    return result.stdout.decode("ascii"), sources, schedule


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-89 area output already exists")
    report, sources, _ = replay(dol, afs, fixture, module)
    pilots = decode_report(report)
    payload = {
        "kind": "gf-retail-type89-area", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "pilots": [{**p, "source": s} for p, s in zip(pilots, sources, strict=True)],
        "entries": {"carrier_constructor": address(retail_type89.FACTORY), "carrier_update": address(retail_type89.UPDATE),
                    "exit_leaf": address(retail_type89.EXIT), "cached_area_write": "8016CE48"},
        "area_source": {"family": "type-5", "key": 16, "common_group": 110, "descriptor": 150, "reaction_table": "802C4760"},
        "notes": [
            "The carrier factory is invoked directly. It is not a stock or controller fixture.",
            "The original initializer samples a declared socket and writes the area byte only after a successful opening bind.",
            "The original exit leaf consumes declared subphase, contact latch or alternate flag on that same carrier.",
            "The area binds a separate class-4 shape, keeps the carrier parent and credits the character. The source sound request is a cut, not playback.",
            "The observations establish no area activation timing, overlap, HP write or native admission.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(pilots), "cases": len(pilots)*len(CASES)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
