"""Verify projectile contact timer ownership and the original linked body clocks."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import contact_pose_pause as pose, retail_contact_effect_tail as tail
from tools.analyze import retail_type32_roster as beam
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_contact_effect_join import TAIL_SHA256, input_pairs
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.analyze.retail_rifle_contact_hp import SLICES
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
POSE_SHA256 = "396def83f1736fd55e625cd6336552b7e9390173fe75f3a73a7c4f408e5a02f0"
UNIT, BOOST, SLOW = "3F800000", "3FC00000", "3CF5C28F"
SECONDARY, BOOSTED_SECONDARY = "3EC00000", "3F100000"
BODY_CASES = (("zero", 0, False), ("one", 1, False), ("two", 2, True), ("eight", 8, True))
LINK_CASES = (
    ("linked-before-owner-visit", 7, BOOST, BOOST, BOOSTED_SECONDARY),
    ("linked-after-owner-visit", 7, BOOST, SLOW, SLOW),
    ("override-before-link", 7, UNIT, UNIT, UNIT),
    ("link-resumed", 7, BOOST, SLOW, SLOW),
    ("linked-owner-expired", 7, BOOST, BOOST, BOOSTED_SECONDARY),
    ("unlinked-own-timer-resumes", 6, UNIT, SLOW, SLOW),
)
BODY_ROW = re.compile(r"pose-clock:(?: [0-9]{1,2}){3}(?: [0-9A-F]{8}){3}")
PAIR_ROW = re.compile(r"pause-pair:(?: [0-9]{1,4}){10}")
TEXT = (
    (0x8005A378, 520, "b6a3dfe9d8bf1b34fb4d470444385f1bbdafe4ba0596786d67699ca9e92ecd2f"),
    (0x80059068, 208, "ea22383e1d48094534b9c882b22400b794423bdea5657be04bcfab050da3716c"),
    (0x8002E2A8, *SLICES[0x8002E2A8]),
)
SEEDS = (1, 1, 4, 0)


def decode_clocks(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 25:
        raise ValueError("body clock report extent")
    rows = []

    def row(scenario, visit, name, remaining, base, primary, secondary, **fields):
        text = lines[len(rows)]
        result = [str(v) for v in (scenario, visit, remaining, base, primary, secondary)]
        if BODY_ROW.fullmatch(text) is None or text.split()[1:] != result:
            raise ValueError(f"body clock rate or countdown changed: {name}/{visit}")
        rows.append({
            "case": name, "visit": visit, "timer_after": remaining,
            "base_rate_bits": base, "primary_rate_bits": primary, "secondary_rate_bits": secondary, **fields,
        })

    for scenario, (name, seed, boosted) in enumerate(BODY_CASES):
        base, scaled = (BOOST, BOOSTED_SECONDARY) if boosted else (UNIT, SECONDARY)
        for visit in range(1, seed+3):
            remaining = max(0, seed-visit)
            row(scenario, visit, name, remaining, base, SLOW if remaining else base, SLOW if remaining else scaled,
                declared_seed=seed, declared_base_rate_flag=int(boosted))
    for visit, (name, count, base, primary, secondary) in enumerate(LINK_CASES, 1):
        row(4, visit, name, count, base, primary, secondary)
    return rows


def decode_pairs(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 12:
        raise ValueError("contact clock paired report extent")
    rows = []
    for (kind, model, step, record, _, lost, _), seed in zip(tail.PAIRS, SEEDS, strict=True):
        for prior in (0, 2, 6):
            armed = max(prior, seed)
            visits = max(0, armed-1)
            line = lines[len(rows)]
            expected = (kind, model, 16, seed, prior, armed, 0, visits, lost, 180-lost)
            if PAIR_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"contact clock amount, ownership or expiry changed: {model:04X}/{prior}")
            rows.append({
                "attack_type": kind, "credited_borg": f"gg4e08:{model:04x}", "attack_descriptor": step,
                "source_amount_record": record, "attacking_object_kind": 16, "source_seed": seed,
                "receiver_timer_before": prior, "receiver_timer_at_amount_entry": armed, "receiver_timer_after_hit": armed,
                "credited_actor_timer_after_hit": 0, "slow_body_clock_visits": visits, "normal_rates_restored": True,
                "hp_lost": lost, "receiver_hp_after": 180-lost, "visual_gate_after_clock_visits": 4,
            })
    return rows


def census_seeds(image, census):
    result = {}
    for collection, expected in (
        ("actor_amount_records", {0: 4, 1: 30, 2: 5, 4: 18, 8: 16}),
        ("common_amount_records", {0: 22}),
    ):
        counts = Counter()
        bypass = 0
        for row in census[collection]:
            record = image.read(int(row["record"], 16), 24, "data")
            if struct.unpack_from(">H", record)[0] != row["raw_power"] or record[9] != row["visual_selector_byte"]:
                raise ValueError("contact clock census amount record changed")
            counts[record[8]] += 1
            bypass += bool(struct.unpack_from(">H", record, 18)[0] & pose.RECEIVER_SKIP_FLAGS)
        if counts != expected or bypass:
            raise ValueError("scoped outgoing contact seed or flag census changed")
        result[collection] = {"seed_counts": dict(counts), "receiver_timer_bypass_records": bypass}
    return result


def verify_source(image):
    beam.verify_text(image, TEXT)
    if pose.recover(image) != read_metadata(MAPS / "contact-pose-pause.json", POSE_SHA256):
        raise ValueError("contact clock static operands changed")
    beam.link(image, 0x80059118, 0x8005A378)
    beam.link(image, 0x8002F108, 0x8003CD5C)
    for pair, seed in zip(tail.PAIRS, SEEDS, strict=True):
        record = image.read(int(pair[3], 16), 24, "data")
        if record[8] != seed or struct.unpack_from(">H", record, 18)[0] & pose.RECEIVER_SKIP_FLAGS:
            raise ValueError("contact clock replay seed or flag changed")


def fixture_report(fixture, dol, module, options=(), payload=b""):
    proc = run([str(Path(fixture).resolve(strict=True)), str(dol), str(module), *options],
               input=payload, capture_output=True, timeout=45)
    if proc.returncode or proc.stderr:
        raise ValueError(f"contact clock fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
    return proc.stdout.decode("ascii")


def export(dol, afs, body_fixture, type76_fixture, type32_fixture, type9_fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact clock output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    verify_source(image)
    read_metadata(MAPS / "gg4e08-contact-effect-tail.json", TAIL_SHA256)
    census = census_seeds(image, read_metadata(MAPS / "gg4e08-contact-effect-census.json", tail.CENSUS_SHA256))
    body = decode_clocks(fixture_report(body_fixture, dol, module))
    parts = []
    for fixture, option, payload in zip((type76_fixture, type32_fixture, type9_fixture),
                                        ("--pose-clock", "--saucer-pose-clock", "--pose-clock"),
                                        input_pairs(image, afs), strict=True):
        parts.append(fixture_report(fixture, dol, module, (option,), payload))
    pairs = decode_pairs("".join(parts))
    data = {
        "kind": "gf-retail-contact-pose-clock", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "contact_effect_tail_sha256": TAIL_SHA256,
        "contact_effect_census_sha256": tail.CENSUS_SHA256, "contact_pose_pause_sha256": POSE_SHA256,
        "entries": {"attacker_write": "8002E628", "receiver_write": "8002E6A8", "amount": "8003CD5C",
                    "body_clock_call": "80059118", "body_clock": "8005A378", "decrement_store": "8005A520"},
        "scoped_amount_records": census, "body_clock_visits": body, "projectile_pairs": pairs,
        "notes": [
            "Receiver timer histories 0, 2 and 6 are declared before accepted hits. Source seeds come from the exact unmodified amount records.",
            "An amount-entry fork observes the armed timer before HP or rate writes; the complete pair and body clock visits use a separate retained fork.",
            "These projectile objects are not bodies. Their credited fighter does not acquire the receiver timer.",
            "The linked and override branches use declared body fields. The follower copies the current linked rates without ticking its own timer.",
            "Visit counts do not establish frame or pause scheduling. Melee timer playback, live linked forms, pose progression and native gameplay are outside this receipt.",
        ],
    }
    raw = encode(data, expanded=("body_clock_visits", "projectile_pairs"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "body_clock_visits": len(body), "projectile_pairs": len(pairs)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "body_fixture", "type76_fixture", "type32_fixture", "type9_fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.body_fixture, args.type76_fixture, args.type32_fixture,
                            args.type9_fixture, args.module, args.output), sort_keys=True))
