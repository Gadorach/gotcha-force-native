"""Bind the exact active normal receiver data records and exercise their lookups."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import damage_amount_model as amount
from tools.analyze import damage_content_row as tables
from tools.analyze import retail_stock_startup as startup
from tools.analyze.retail_stock_lifecycle import STARTUP_SHA256


TIER_SHA256 = "884e506221acc59f3a8cb9013bbd3b9444c57dfeda1772b39a296729d67ae072"
TEXT = (0x8003CD5C, 1512, "7c752eea0cdf04fbe970796f80ffe1bd98ea2ef6edebea6e33cd1d3462355792")
ROW = re.compile(r"receiver-factors: [0-9A-F]{4} [0-7](?: [0-7]){3} [0-9]{1,5}(?: [0-9]{1,3}){4}")


def f32(value):
    return struct.unpack(">f", struct.pack(">f", value))[0]


def reference(image, selectors):
    """Receiver-only probe with fixed raw power, team, setting, rank and snapshots."""
    class_id, hp_id, force_id = selectors
    out = []
    for bank in (0, 1):
        class_row = tables._u32(image, tables._row(image, tables.CLASS_ROOT, bank)+4*class_id)
        class_factor = tables._f32(image, class_row+7*4)
        hp_row = tables._u32(image, tables._row(image, amount.ROOTS["defender_hp_row_157"][0], bank)+4*hp_id)
        force_row = tables._u32(image, tables._row(image, amount.ROOTS["defender_force_row_158"][0], bank)+4*force_id)
        rank = tables._f32(image, tables._row(image, tables.RECEIVER_RANK_ROOT, bank))
        for low in (False, True):
            value = f32(20*class_factor)
            if low:
                value = f32(value*tables._f32(image, hp_row+16*4))
            value = f32(value*tables._f32(image, force_row+(26 if low else 0)*4))
            out.append(int(max(1, f32(value*rank))))
    return out


def decode(image, report, sources, gauges):
    lines = report.splitlines()
    if len(report) > 16384 or len(lines) != len(sources):
        raise ValueError("normal receiver amount report extent")
    by_borg = {r["borg"]: r for r in gauges["normal_receivers"]}
    entries = []
    for line, row in zip(lines, sources, strict=True):
        triple = list(row.spec[156:159])
        cap = struct.unpack_from(">h", row.spec)[0]
        if len(triple) != 3 or any(value >= 8 for value in triple) or cap <= 0:
            raise ValueError("normal damage operands outside supported domain")
        factors = reference(image, triple)
        prior = by_borg[row.id]
        if (ROW.fullmatch(line) is None or tuple(line.split()[1:]) != (
                f"{row.model:04X}", *map(str, (row.spec_index, *triple, cap, *factors))) or
            (prior["spec_record"], prior["capacity"]) != (row.spec_index, cap)):
            raise ValueError(f"{row.id}: selected normal damage record or amount changed")
        entries.append({
            "borg": row.id, "installer": row.installer, "spec_record": row.spec_index,
            "record_sha256": sha256(row.spec).hexdigest(), "selectors": triple, "reset_capacity": cap,
            "receiver_only_amounts": factors,
        })
    counts = Counter(r["spec_record"] for r in entries)
    if counts != {0: 200, 1: 5, 2: 1}:
        raise ValueError("normal receiving selected-record coverage changed")
    return entries, dict(sorted(counts.items()))


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal receiver factor output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = startup.DolImage(startup.read_bounded(dol, 8 << 20), startup.SOURCE_SHA256)
    startup.verify_text(image, (*startup.TEXT, TEXT))
    if image.read(0x802C49D0, 2, "data") != b"\x00\x14":
        raise ValueError("normal receiver factor base source changed")
    installed = startup.read_metadata(startup.MAPS / "gg4e08-standalone-installers.json", startup.INSTALLERS_SHA256)
    startup.read_metadata(startup.MAPS / "gg4e08-stock-startup.json", STARTUP_SHA256)
    gauges = startup.read_metadata(startup.MAPS / "gg4e08-damage-tier-clock.json", TIER_SHA256)
    with startup.open_regular(afs) as stream:
        stamp = startup.archive_stamp(stream)
        startup.verify_archive(stream)
        plan, sources = startup.source_inputs(image, startup.RetailArchive(stream), installed)
        if startup.archive_stamp(stream) != stamp:
            raise ValueError("normal receiver factor archive input changed")
    entries, counts = decode(image, startup.fixture_report(fixture, dol, module, ("--receiver-factors",), plan), sources, gauges)
    raw = startup.encode({
        "kind": "gf-retail-normal-receiver-factors", "revision": 1, "source_dol_sha256": startup.SOURCE_SHA256,
        "source_afs_sha256": startup.AFS_SHA256, "stock_startup_sha256": STARTUP_SHA256, "tier_clock_sha256": TIER_SHA256,
        "records_by_selected_ordinal": counts, "records": entries,
        "notes": [
            "One normal level-1 installer and constructor prefix selects the exact archived receiving record.",
            "The amount helper uses raw area power 20 and an inactive credit owner to skip outgoing ladders and matchup.",
            "Amounts are ordered by receiver bank, then high/low probe. Team 1, setting 7, rank 0, neutral size and tier zero are declared.",
            "High probe skips the HP ladder at mirrored HP 1000 and uses force index 0; low probe uses mirrored HP 120, max HP 200 and force index 26.",
            "This does not classify combined-form ownership, admit new actors, or establish live damage settings.",
        ],
    }, expanded=("records",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "records": len(entries), "amount_probes": 4*len(entries)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
