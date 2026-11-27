"""Observe scoped type-76 debit, ordinal advance, and constructor allocation."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_type32_roster as beam, retail_type76_roster as roster
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import SOURCE_SHA256, address, encode, read_metadata
from tools.extract.source_image import DolImage


ROSTER_SHA256 = "f0224bcfb4fbf54aedf6bef4dc25551644ee0880dc35e39a1d0bed40b99afc0c"
CASES = ("two-funded", "restore-carrier-after-first-refusal", "no-descriptor", "empty-stock")
ROW = re.compile(r"type76-paid: [0-9]{4}(?: [0-3]){7}")
TEXT = (
    (0x8006DBE0, 224, "0b2e322f452d317b5f515e0ebd95f821a10cdbee5db9cdbd07678c8d251eed31"),
    (0x80088AA0, 416, "800882bf558ba7e7f9e955e1884be71e0c9aad8870807bf7dc0876f56baaeab6"),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(roster.NORMAL)*len(CASES)*3:
        raise ValueError("type-76 paid report extent")
    rows = []
    for model, _, family, _, _, _, pre, _, _, channel, site, actions in roster.NORMAL:
        parent_key = roster.SITES[site][1]
        for scenario, label in enumerate(CASES):
            visits = []
            for i in range(3):
                line = lines[3*len(rows)+i]
                if ROW.fullmatch(line) is None:
                    raise ValueError("malformed type-76 paid row")
                paid = scenario != 3 and i < 2
                made = paid and (scenario == 0 or (scenario == 1 and i == 1))
                cursor = i % 2 if scenario != 3 else 0
                next_cursor = cursor ^ int(paid)
                stock = int(paid and i == 0)
                if tuple(map(int, line.split()[1:])) != (model, scenario, i+1, cursor, paid, made, next_cursor, stock):
                    raise ValueError(f"type-76 stock, cursor or allocation changed: {model:04X}/{label}")
                visits.append({"visit": i+1, "requested_parent_ordinal": cursor if paid else None, "debit_ok": paid,
                               "carrier_allocated": made, "cursor_after": next_cursor, "stock_after": stock})
            rows.append({"borg": f"gg4e08:{model:04x}", "actions": list(actions), "family": address(family),
                         "pre_callback": address(pre), "parent_key": parent_key, "constructor_site": address(site),
                         "stock_channel": channel, "case": label, "visits": visits})
    return rows


def export(dol, census, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-76 paid output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    roster.row_index(image)
    beam.verify_text(image, TEXT)
    source = read_metadata(census, ROSTER_SHA256)
    if source["counts"]["normal_action_routes"] != 3:
        raise ValueError("type-76 paid route census changed")
    for site, target in ((0x80188F4C, beam.DEBIT), (0x800F7EF0, beam.DEBIT),
                         (0x80188F7C, roster.FACTORY), (0x800F7F20, roster.FACTORY)):
        beam.link(image, site, target)
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(args, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-76 paid fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type76-paid", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "type76_census_sha256": ROSTER_SHA256, "cases": cases,
        "notes": [
            "The pre-callback sees the declared stock threshold and clears both ordinal markers. Each action family visit is supplied phase one.",
            "Debit and allocator observations use separate passes from the same phase-one inlet. They are not consecutive game visits.",
            "A successful debit advances the ordinal after the constructor returns even when allocation was refused. Empty stock does not advance it.",
            "No carrier initializer, collider, HP, controller or real rearm timing was replayed. Alternate specs and form routes remain unqualified.",
        ],
    }
    raw = encode(payload, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "visits": sum(len(r["visits"]) for r in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "census", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.census, args.fixture, args.module, args.output), sort_keys=True))
