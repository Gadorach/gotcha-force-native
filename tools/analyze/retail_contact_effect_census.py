"""Read contact selectors from scoped queues and replay the effect allocator."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import contact_impact_visuals as visuals, reaction_emit, reaction_installer_tables
from tools.analyze import retail_type32_roster as beam, retail_type5_roster as area
from tools.analyze import retail_type5_sibling_exit as exits, retail_type9_roster as type9
from tools.analyze import retail_wall_effect_census as wall
from tools.analyze.retail_code_atlas import address
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, export_census, read_metadata,
)


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
VISUALS_SHA256 = "9f1579cdb80218ee4c77acb7ab2817f9f37e57e35c5d35388833266bed79e8df"
TYPE9_SHA256 = "ade49eea68755ef992f6cd00ea2718a306b3d3eb0ffe6fc16e6a22d770f5e2f3"
QUEUE_RECEIPTS = (
    ("type0", "queue", "14aa6bc3d10c9edfb7d11414a53c674dccb769e762a23ceb7c8d9718da4f8475"),
    ("type2", "queue", "060278740ca523ef6eede5e34cf2ea0db1bce28e61ecd8f291d529f0888e47ff"),
    ("small-projectile", "queue", area.PARENT_CENSUS_SHA256),
    ("type32", "normal_group_queue", type9.TYPE32_DIGEST),
    ("type11", "normal_queue", area.SWEEP_CENSUS_SHA256),
    ("type76", "normal_carrier_queue", wall.TYPE76_SHA256),
    ("type89", "opening_queue", wall.TYPE89_SHA256),
)
SELECTORS = {0: (0, 0, 1), 1: (1, 0, 1), 2: (2, 0, 3), 3: (2, 1, 3), 4: (2, 2, 3), 8: (3, 0, 8)}
CASES = ("exact-pool", "one-descriptor-short", "no-class2", "two-extra-descriptors")
ROW = re.compile(r"impact-alloc:(?: [0-9]{1,2}){7}")
TEXT = (
    *tuple(s for s in wall.TEXT if s[0] in (0x80019550, 0x8001959C, 0x8008893C)),
    (0x8008A2BC, 788, "2c811080af75624f6664c00f6e11b229fdb83bea44b41482b53b2ed14d2f0528"),
)


def amount_record(image, at, raw_power):
    record = image.read(at, 24, "data")
    if struct.unpack_from(">H", record)[0] != raw_power:
        raise ValueError(f"contact effect amount-record power changed at {address(at)}")
    return {"record": address(at), "raw_power": raw_power, "visual_selector_byte": record[9]}


def actor_records(image, archive, atlas, selections, receipts):
    picked = {row["id"]: row for row in selections["borgs"]}
    static = {row["id"]: row for row in atlas["borgs"]}
    if len(picked) != 206 or picked.keys() != static.keys():
        raise ValueError("contact effect selected profile coverage changed")
    models, records, excluded = {}, {}, {}
    for (label, queue_field, _), receipt in zip(QUEUE_RECEIPTS, receipts, strict=True):
        for row in receipt["candidates"]:
            steps = row[queue_field].get("steps", ())
            if not steps:
                continue  # Unqualified queues are not effect-selector evidence.
            model = row["borg"]
            if (picked[model]["profile"] != row["profile"] or picked[model]["spec_record"] != 0
                    or row.get("spec_record", 0) != 0):
                raise ValueError("contact effect selected normal source changed")
            if model not in models:
                number = int(model[-4:], 16)
                installer, table = reaction_installer_tables.model_table(image, number)
                if address(installer) != picked[model]["installer"] or address(table) != static[model]["reaction_table"]:
                    raise ValueError("contact effect normal actor table changed")
                models[model] = table, reaction_emit.qualified_hit_block(archive, number)
            table, block = models[model]
            for step in steps:
                desc, amount_index = step["descriptor"], step["reaction_index"]
                offset = reaction_emit.STEPS+desc*reaction_emit.STEP_SIZE
                if not 0 <= desc < 64 or not 0 <= amount_index < 256 or offset+8 > len(block):
                    raise ValueError("contact effect scoped descriptor outside its bank")
                _, _, flags, source_index = struct.unpack_from(">BBHH", block, offset)
                if source_index != amount_index:
                    raise ValueError("contact effect descriptor does not carry the scoped record index")
                at = table+24*amount_index
                source = amount_record(image, at, step["raw_power"])
                reference = model, desc, amount_index
                if not flags & 1:
                    excluded[reference] = {"borg": model, "descriptor": desc, "flags": flags, **source}
                    continue
                if at not in records:
                    records[at] = source, {}
                _, references = records[at]
                references.setdefault(reference, set()).add(label)
    rows = [{**source, "references": [
        {"borg": borg, "descriptor": desc, "reaction_index": index, "receipts": sorted(labels)}
        for (borg, desc, index), labels in sorted(references.items())
    ]} for _, (source, references) in sorted(records.items())]
    return rows, [excluded[key] for key in sorted(excluded)]


def common_records(image, archive, area_receipt, type9_receipt):
    bank = area.common_bank(archive)
    rows, groups = area.area_rows(image, bank)
    if rows != area_receipt["factory_rows"] or groups != area_receipt["common_bank"]["groups"]:
        raise ValueError("contact effect area receipt no longer matches the common collision bank")
    _, _, _, sphere, _ = type9.source_index(image, bank)
    if sphere != type9_receipt["common_collision"]:
        raise ValueError("contact effect type-9 receipt no longer matches the common collision bank")
    domains = [(5, groups), (9, [{"group": sphere["group"], "descriptor": sphere["descriptor"],
                                 "reaction_record": sphere["amount_record"], "raw_power": sphere["raw_power"]}])]
    result = []
    for kind, shapes in domains:
        for shape in shapes:
            result.append({"carrier_type": kind, "group": shape["group"], "descriptor": shape["descriptor"],
                           **amount_record(image, int(shape["reaction_record"], 16), shape["raw_power"])})
    return result


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(SELECTORS)*len(CASES):
        raise ValueError("contact effect allocation report extent")
    rows = []
    for key, (family, variant, cost) in SELECTORS.items():
        for i, name in enumerate(CASES):
            line = lines[len(rows)]
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed contact effect allocation row")
            supplied = (cost, cost-1, cost, cost+2)[i]
            made = i in (0, 3)
            remaining = supplied-cost*int(made)
            if tuple(map(int, line.split()[1:])) != (key, i, family, variant, cost, made, remaining):
                raise ValueError(f"contact effect whole-allocation contract changed: {key}/{name}")
            rows.append({"selector": key, "case": name, "family": family, "variant": variant, "descriptor_request": cost,
                         "class2_supplied": i != 2, "descriptors_supplied": supplied, "allocated": made,
                         "descriptors_left": remaining, "shape_cells_consumed": 0})
    return rows


def export(dol, afs, atlas, selections, fixture, module, output):
    def recover(image, archive, code_atlas, installed):
        beam.verify_text(image, TEXT)
        visual_map = visuals.recover(image)
        if visual_map != read_metadata(MAPS / "contact-impact-visuals.json", VISUALS_SHA256):
            raise ValueError("contact effect presentation descriptor receipt changed")
        for key, (family, variant, cost) in SELECTORS.items():
            row = visual_map["descriptors"][str(key)]
            if (row["family"], row["variant"], row["allocation_code"]) != (family, variant, cost):
                raise ValueError("contact effect admission selector changed")
        for pc, target in ((0x8002F87C, 0x80019550), (0x80019588, 0x8001959C), (0x800195D4, 0x8008893C)):
            beam.link(image, pc, target)
        receipts = [read_metadata(MAPS / f"gg4e08-{name}-roster.json", digest) for name, _, digest in QUEUE_RECEIPTS]
        actor, omitted = actor_records(image, archive, code_atlas, installed, receipts)
        common = common_records(image, archive, read_metadata(MAPS / "gg4e08-type5-roster.json", exits.CENSUS_SHA256),
                                read_metadata(MAPS / "gg4e08-type9-roster.json", TYPE9_SHA256))
        histogram = Counter(r["visual_selector_byte"] for r in actor)
        if histogram != {1: 69, 8: 4} or len(common) != 22 or any(r["visual_selector_byte"] != 0 for r in common):
            raise ValueError("contact effect selector coverage changed")
        args = [str(Path(fixture).resolve(strict=True)), str(Path(dol).resolve(strict=True)),
                str(Path(module).resolve(strict=True)), "--selectors"]
        proc = run(args, capture_output=True, timeout=45)
        if proc.returncode or proc.stderr:
            raise ValueError(f"contact effect fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
        cases = decode_report(proc.stdout.decode("ascii"))
        return {
            "kind": "gf-retail-contact-effect-census", "revision": 1,
            "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
            "code_atlas_sha256": ATLAS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
            "visual_descriptor_sha256": VISUALS_SHA256, "type5_census_sha256": exits.CENSUS_SHA256,
            "type9_census_sha256": TYPE9_SHA256, "queue_receipt_sha256": {name: digest for name, _, digest in QUEUE_RECEIPTS},
            "actor_amount_records": actor, "actor_selector_histogram": dict(histogram), "omitted_nonattack_steps": omitted,
            "common_amount_records": common, "cases": cases,
            "counts": {"actor_records": len(actor), "actor_references": sum(len(r["references"]) for r in actor),
                       "actor_models": len({source["borg"] for r in actor for source in r["references"]}),
                       "common_records": len(common), "nonattack_steps": len(omitted), "allocation_cases": len(cases)},
            "notes": [
                "Bytes at amount-record +9 are inventoried only for previously scoped queues. An attack descriptor has source flag bit 0.",
                "The selector is used only if the original contact gate reaches 8002F87C. Counts are not observed hit frequencies or action bindings.",
                "Type-76 receiver-only steps are not outgoing selector evidence. Common area and type-9 rows use the separate common amount table.",
                "Direct helper runs check exact, short, missing-class-2 and surplus pools without executing initialization or rendering.",
                "The replay does not bind a live contact tail or change the native contact-visual support scope.",
            ],
        }

    return export_census(dol, afs, atlas, selections, output, recover)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "atlas", "selections", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.atlas, args.selections, args.fixture, args.module, args.output), sort_keys=True))
