"""Run scoped original release and pair-publication fixtures, exporting metadata only."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze.reaction_emit import GROUPS, HIT_BYTES, QUEUE, STEPS, STEP_SIZE, qualified_hit_block
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_stock_operands import stock_rows, stock_schema
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage

RIFLE_CASES = ("paid-and-published", "carrier-pool-empty", "descriptor-pool-empty",
               "stock-empty", "cue-absent")
RIFLE_INPUTS = ((2, 1, 1, 1), (1, 1, 0, 1), (1, 1, 1, 0), (0, 1, 1, 1), (2, 0, 1, 1))
RIFLE_RECORD = re.compile(r"rifle:(?: [0-9]{1,3}){13}")
SASUKE_RECORD = re.compile(r"sasuke:(?: [0-9]{1,3}){7}")


def decode_report(report):
    if len(report) > 4096:
        raise ValueError("attack fixture report extent")
    rifle, sasuke = [], []
    for line in report.splitlines():
        if line.startswith("rifle:"):
            if not RIFLE_RECORD.fullmatch(line):
                raise ValueError("malformed rifle case")
            rifle.append(tuple(map(int, line.split()[1:])))
        if line.startswith("sasuke:"):
            if not SASUKE_RECORD.fullmatch(line):
                raise ValueError("malformed Sasuke case")
            sasuke.append(tuple(map(int, line.split()[1:])))
    if tuple(r[:4] for r in rifle) != RIFLE_INPUTS or [x[0] for x in sasuke] != [1, 0]:
        raise ValueError("missing or reordered attack fixture cases")
    visits = []
    for name, row in zip(RIFLE_CASES, rifle, strict=True):
        initial, cue, free_cell, free_descriptor, remaining, clock, attempts, published, bucket, tag, kind, key, restore = row
        paid = bool(cue and initial)
        emitted = bool(paid and free_cell and free_descriptor)
        if (remaining != initial - paid or clock != (300 if paid and not remaining else 0)
                or attempts != paid or published != emitted
                or (bucket, tag, kind, key) != ((9, 0x10, 2, 21) if emitted else (0, 0, 0, 0))
                or restore != (5 if clock else 0)):
            raise ValueError("inconsistent rifle transaction report")
        result = {"case": name, "initial_stock": initial, "cue": bool(cue), "free_carrier_cell": bool(free_cell),
                  "free_descriptor_cell": bool(free_descriptor), "stock_after_attempt": remaining,
                  "refill_clock_after_attempt": clock, "allocation_attempts": attempts, "published": bool(published)}
        if emitted:
            result["carrier"] = {"class": 3, "bucket": bucket, "physical_tag": tag, "type": kind, "key": key}
        if clock:
            result["refill_control"] = {"inhibited_visit_keeps_clock": clock,
                                        "unit_step_visits": 300, "restored_count": restore}
        visits.append(result)
    pairs = []
    for free_cell, queue, published, bucket, tag, step, tuning_index in sasuke:
        if queue != 2 or published != free_cell or (
                bucket, tag, step, tuning_index) != ((49, 0x1A, 2, 2) if published else (0, 0, 0, 0)):
            raise ValueError("inconsistent Sasuke transaction report")
        item = {"free_shape_cell": bool(free_cell), "queue_byte_after_attempt": queue,
                "published": bool(published)}
        if published:
            item["pair"] = {"class": 4, "bucket": bucket, "physical_tag": tag,
                            "step": step, "attack_table_index": tuning_index}
        pairs.append(item)
    return visits, pairs


def sasuke_source(block):
    queue = 2
    if len(block) != HIT_BYTES or queue >= GROUPS:
        raise ValueError("Sasuke source queue extent")
    group = struct.unpack_from(">4b", block, QUEUE + 4 * queue)
    if group != (2, -1, -1, -1):
        raise ValueError("Sasuke source queue mismatch")
    shape, matrix, _, tuning = struct.unpack_from(">BBHH", block, STEPS + group[0] * STEP_SIZE)
    if (shape, matrix, tuning) != (0, 7, 2):
        raise ValueError("Sasuke source step mismatch")
    return {"hit_block_sha256": sha256(block).hexdigest(), "queue": queue, "step": group[0],
            "shape_kind": shape, "matrix_selector": matrix, "attack_table_index": tuning}


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("attack transaction output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    stock_schema(image)
    common_stock = {"initial": 5, "limit": 5, "mode": 0, "preset": 300}
    if any({k: row[k] for k in common_stock} != common_stock for row in stock_rows(levels(image, 0x0600))[0]["levels"]):
        raise ValueError("Machine Red standard stock operand mismatch")
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        block = qualified_hit_block(RetailArchive(stream), 0x000A)
        if stamp != archive_stamp(stream):
            raise ValueError("attack source AFS changed during extraction")
    source = sasuke_source(block)
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)),
            "--sasuke-hit-block"]
    result = run(argv, input=block, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"attack fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    rifle, pairs = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-attack-transactions", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "machine_red": {"id": "gg4e08:0600", "program": "8030903E", "cue": "8030906A",
                        "release": "800C96C0", "debit_site": "800C9F78", "allocator_site": "800C3C10",
                        "world_query_cut": "8003E0A8", "normal_channel_0": common_stock, "cases": rifle},
        "sasuke": {"id": "gg4e08:000a", "program": "802BE3B6", "op10_allocator_call": "8004C6F0",
                   "pair_allocator_call": "8008ACE8", "pair_callback": "8008A65C",
                   "source": source, "cases": pairs},
        "notes": [
            "Fighter and scene inlets are synthetic; pulse, debit, original allocator writes and constructor execute.",
            "No allocation result, physical tag, active list entry or pair record is supplied by the harness.",
            "The fixture does not resume the stopped world query, visit the objects, or establish contact, damage or lifetime.",
            "Rifle stock starts at a declared count and uses a supplied unit refill step. The 300 visits are not live frames.",
            "The Sasuke hit block is checked against the matching archive, passed via stdin and never exported.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "rifle_cases": len(rifle), "sasuke_cases": len(pairs)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
