"""Replay four accepted pairs through the original impact-effect gate."""
import argparse
from hashlib import sha256
import json
import math
from pathlib import Path
import re
from subprocess import run

from tools.analyze import contact_impact_visuals as visuals, retail_saucer_contact as saucer
from tools.analyze import retail_type32_roster as beam, retail_type76_opening as opening, retail_type9_roster as type9
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_contact_effect_census import SELECTORS, TEXT as EFFECT_TEXT
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
CENSUS_SHA256 = "0eb904daecf3a7188209b6ab8d5804460dc78e5cc2da9a690def380e4f22271c"
# Attack type, model, descriptor, amount record, visual selector, HP loss, effect origin.
PAIRS = (
    (76, 0x0D01, 3, "80362920", 1, 35, (200, 157.6, 325)),
    (76, 0x0D04, 1, "8037A848", 1, 35, (800, 169.5, 325)),
    (32, 0x0D03, 1, "80362C40", 8, 26, (160, 200, 900)),
    (9, 0x0D03, 122, "802C4838", 0, 17, (160, 0, 400)),
)
CASES = ("open-gate", "held-gate", "no-class2", "one-descriptor-short", "miss")
ROW = re.compile(r"impact-tail:(?: [0-9]{1,4}){11}(?: [0-9]{1,4}\.[0-9]{5}){3}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(PAIRS)*len(CASES):
        raise ValueError("contact effect tail report extent")
    result = []
    for kind, model, step, record, selector, hp_loss, origin in PAIRS:
        _, _, cost = SELECTORS[selector]
        for i, case in enumerate(CASES):
            line = lines[len(result)]
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed contact effect tail row")
            hit, held, class2 = case != "miss", case == "held-gate", case != "no-class2"
            asked = hit and not held
            offered = cost-1 if case == "one-descriptor-short" else cost
            allocated = asked and class2 and offered == cost
            damage, gate = hp_loss if hit else 0, 4 if held or asked else 0
            fields = line.split()[1:]
            point = list(map(float, fields[11:]))
            if (tuple(map(int, fields[:11])) != (kind, model, i, selector, damage, 180-damage, asked, allocated,
                                                 gate, offered-cost*allocated, hit)
                    or any(not math.isclose(x, want, abs_tol=0.001)
                           for x, want in zip(point, origin if asked else (0, 0, 0), strict=True))):
                raise ValueError(f"contact effect gate, credit or allocation changed: {kind}/{model:04X}/{case}")
            result.append({
                "attack_type": kind, "credited_borg": f"gg4e08:{model:04x}", "attack_descriptor": step, "source_record": record,
                "case": case, "hp_lost": damage, "receiver_hp_after": 180-damage,
                "damage_ledger_deltas": [damage, damage], "accepted_token": 1 if hit else None,
                "gate_before": 4 if held else 0, "gate_after": gate, "class2_supplied": class2,
                "descriptors_supplied": offered, "descriptors_left": offered-cost*allocated,
                "effect_request": {"selector": selector, "allocated": allocated, "position": point} if asked else None,
                "reopened_gate_duplicate_refused": hit,
            })
    return result


def check_census(census):
    actor = {r["record"]: r for r in census["actor_amount_records"]}
    common = {r["record"]: r for r in census["common_amount_records"]}
    for kind, model, step, record, selector, _, _ in PAIRS:
        if kind == 9:
            row = common[record]
            if (row["carrier_type"], row["descriptor"], row["visual_selector_byte"]) != (kind, step, selector):
                raise ValueError("contact-effect tail common amount record changed")
        else:
            row = actor[record]
            if row["visual_selector_byte"] != selector or not any(
                (r["borg"], r["descriptor"]) == (f"gg4e08:{model:04x}", step) for r in row["references"]
            ):
                raise ValueError("contact-effect tail actor amount record changed")


def export(dol, afs, type76_fixture, type32_fixture, type9_fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact effect tail output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, EFFECT_TEXT+visuals.TEXT+type9.CONTACT_TEXT)
    for site, target in ((0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344), (0x8002F87C, 0x80019550),
                         (0x80019588, 0x8001959C), (0x800195D4, 0x8008893C)):
        beam.link(image, site, target)
    check_census(read_metadata(MAPS / "gg4e08-contact-effect-census.json", CENSUS_SHA256))
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        carrier, sources, body = opening.qualified_inlet(image, archive)
        saucer_input, saucer_source, _ = saucer.qualified_inlet(image, archive)
        common_input, _ = type9.contact_inlet(image, archive)
        common_input = type9.common_bank(archive)+common_input
        if stamp != archive_stamp(stream):
            raise ValueError("contact effect tail AFS changed during extraction")
    chunks = []
    for fixture, option, payload in ((type76_fixture, "--impact", carrier), (type32_fixture, "--saucer-impact", saucer_input),
                                     (type9_fixture, "--impact", common_input)):
        args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), option]
        proc = run(args, input=payload, capture_output=True, timeout=45)
        if proc.returncode or proc.stderr:
            raise ValueError(f"contact effect tail failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
        chunks.append(proc.stdout.decode("ascii"))
    cases = decode_report("".join(chunks))
    data = {
        "kind": "gf-retail-contact-effect-tail", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "contact_effect_census_sha256": CENSUS_SHA256, "cases": cases,
        "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180},
        "sources": sources+[saucer_source, {"type": 9, "key": 4, "common_amount_record": "802C4838",
                                           "credited_borg": "gg4e08:0d03", "constructor_supplied": True}],
        "entries": {"pair_pass": "8002DB58", "amount": "8003CD5C", "hp_writer": "8003D344", "effect_helper_call": "8002F87C"},
        "notes": [
            "The original published attack and stationary body are replayed from a declared inlet. The type-9 effect is standalone, not a live floor follow-up.",
            "Amount entry, pre-allocation helper and complete pair return are forks from that inlet, not three hits.",
            "The visual gate is 4 before the allocator runs. Seeding it held prevents the visual request, not the HP write.",
            "The same accepted token is refused even after the fixture explicitly reopens the visual gate. Live gate countdown is not measured.",
            "Empty render/audio support, one normal receiver, and admitted carrier fixtures bound the replay. The new effect is not initialized and native gameplay does not run.",
        ],
    }
    raw = encode(data, expanded=("cases", "sources"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "type76_fixture", "type32_fixture", "type9_fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.type76_fixture, args.type32_fixture, args.type9_fixture,
                            args.module, args.output), sort_keys=True))
