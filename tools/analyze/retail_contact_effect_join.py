"""Join accepted original hits to their own source-archive visual updates."""
import argparse
from hashlib import sha256
import json
import math
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_contact_effect_source_art as art, retail_contact_effect_tail as tail
from tools.analyze import retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
TAIL_SHA256 = "c3da79a83106c076b9ce91d2a36c344455243a90b2a5c64b0dd4370454716250"
ART_SHA256 = "025c745717b4c40844049721831a40d402bc82ad31d07fddc451187efb8eb86e"
MODEL_IDS = {0: (21,), 1: (2,), 8: (53, 54)}
ROW = re.compile(r"impact-art:(?: [0-9]{1,6}){17}(?: [0-9]{1,4}\.[0-9]{5}){3}")


def archive_records(slot):
    return {r["record"]: r["root_offset"] for c in slot["cases"] for r in c["first_update_models"]}


def decode(report, slot, roots):
    lines = report.splitlines()
    if slot not in (1, 2) or len(report) > 2048 or len(lines) != 2*len(tail.PAIRS):
        raise ValueError("joined contact report extent")
    rows = []
    for attack_type, model, step, record, selector, hp_lost, point in tail.PAIRS:
        for type3_supplied in (True, False):
            initialized = type3_supplied or selector == 0
            models = MODEL_IDS[selector] if initialized else ()
            offsets = [roots[r] for r in models]
            padded = offsets+[0]*(2-len(offsets))
            returned = 0 if initialized else 3
            acquired = selector != 0 and initialized
            expected = (slot, attack_type, model, type3_supplied, selector, hp_lost, 180-hp_lost,
                        1, 4, 0, 0, 1 if initialized else 2, len(models), *padded, returned, acquired)
            line = lines[len(rows)]
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed joined contact row")
            fields = line.split()[1:]
            observed_point = list(map(float, fields[17:]))
            if tuple(map(int, fields[:17])) != expected or any(
                not math.isclose(a, b, abs_tol=0.001) for a, b in zip(observed_point, point, strict=True)
            ):
                raise ValueError(f"joined contact point, HP, or model route changed: {slot}/{attack_type}/{model:04X}")
            rows.append({
                "attack_type": attack_type, "credited_borg": f"gg4e08:{model:04x}", "attack_descriptor": step,
                "source_record": record, "accepted_token": 1, "hp_lost": hp_lost, "receiver_hp_after": 180-hp_lost,
                "damage_ledger_deltas": [hp_lost, hp_lost], "hit_point": observed_point, "visual_gate_after": 4,
                "selector": selector, "parent_lane_byte": 0, "scene_mode_byte": 0, "first_update_phase": 1 if initialized else 2,
                "type3_initially_supplied": type3_supplied, "type3_claimed": acquired,
                "first_update_models": [{"record": r, "root_offset": at} for r, at in zip(models, offsets, strict=True)],
                "lane_5_descriptor_count": len(models), "refused_emitter_return_visit": returned or None,
                "late_type3_offer_did_not_retry": not initialized,
            })
    return rows


def input_pairs(image, afs):
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        type76, _, _ = tail.opening.qualified_inlet(image, archive)
        type32, _, _ = tail.saucer.qualified_inlet(image, archive)
        type9, _ = tail.type9.contact_inlet(image, archive)
        type9 = tail.type9.common_bank(archive)+type9
        if stamp != archive_stamp(stream):
            raise ValueError("joined contact AFS changed during extraction")
    return type76, type32, type9


def export(dol, afs, type76_fixture, type32_fixture, type9_fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("joined contact output already exists")
    dol, type76_fixture, type32_fixture, type9_fixture, module = (
        Path(x).resolve(strict=True) for x in (dol, type76_fixture, type32_fixture, type9_fixture, module)
    )
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    art.verify_source(image)
    beam.verify_text(image, tail.EFFECT_TEXT+tail.visuals.TEXT+tail.type9.CONTACT_TEXT)
    for pc, target in ((0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344), (0x8002F87C, 0x80019550)):
        beam.link(image, pc, target)
    tail.check_census(read_metadata(MAPS / "gg4e08-contact-effect-census.json", tail.CENSUS_SHA256))
    read_metadata(MAPS / "gg4e08-contact-effect-tail.json", TAIL_SHA256)
    source = read_metadata(MAPS / "gg4e08-contact-effect-source-art.json", ART_SHA256)
    inputs = input_pairs(image, afs)
    slots = []
    for slot, raw in art.source_blocks(afs):
        prior = source["slots"][slot-1]
        if (prior["pzz_slot"], prior["archive_sha256"]) != (slot, sha256(raw).hexdigest()):
            raise ValueError("joined contact archive digest changed")
        chunks = []
        for fixture, opt, pair in zip((type76_fixture, type32_fixture, type9_fixture),
                                      ("--impact-art", "--saucer-impact-art", "--impact-art"), inputs, strict=True):
            proc = run([str(fixture), str(dol), str(module), opt], input=raw+pair, capture_output=True, timeout=45)
            if proc.returncode or proc.stderr:
                raise ValueError(f"joined contact fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
            chunks.append(proc.stdout.decode("ascii"))
        slots.append({"pzz_slot": slot, "archive_sha256": prior["archive_sha256"],
                      "cases": decode("".join(chunks), slot, archive_records(prior))})
    data = {
        "kind": "gf-retail-contact-effect-join", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "contact_effect_tail_sha256": TAIL_SHA256, "source_art_sha256": ART_SHA256,
        "entries": {"pair": "8002DB58", "effect_call": "8002F87C", "effect_bucket": 31,
                    "bucket_update": "80089A94", "joint_load": "8024F834", "view_zero_enlist": "80089EB4"},
        "slots": slots,
        "notes": [
            "An amount-entry cut qualifies the inlet; the accepted hit and its effect update share one uninterrupted guest memory state.",
            "After the hit the fixture supplies the qualified archive and SDK inlet, then calls the original bucket visit. It never calls the effect helper again.",
            "The collision scratch is overwritten as a control; the allocated effect keeps the saved hit point. The HP, ledgers, token and shape occupancy do not change.",
            "Selector 1 has one declared scene entry remaining. Selectors 1 and 8 need type-3 storage, whereas selector 0 queues record 21 without it.",
            "After type-3 refusal a late cell offer does not retry models. The original effect returns its class-2 cell and all descriptors on visit 3.",
            "The type-9 hit source is standalone, not the result of a live Saucer floor callback.",
            "This joins accepted contacts to draw-list preparation, not GX submission, a full frame scheduler, live hardware or native gameplay.",
        ],
    }
    raw = encode(data, expanded=("slots",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    cases = [r for s in slots for r in s["cases"]]
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "queued_cases": sum(bool(c["lane_5_descriptor_count"]) for c in cases),
            "late_offers_refused": sum(c["late_type3_offer_did_not_retry"] for c in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "type76_fixture", "type32_fixture", "type9_fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.type76_fixture, args.type32_fixture, args.type9_fixture,
                            args.module, args.output), sort_keys=True))
