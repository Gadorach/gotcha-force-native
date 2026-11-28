"""Compare pinned ordinary-shot and rifle observations in a native authored fixture."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.analyze import retail_projectile_contact_tail as ordinary
from tools.analyze import retail_projectile_continuation as rifle
from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


EVENT = re.compile(r"tick=(\d+) actor=(\d+) move=\S+ event=([\w-]+) frame=\d+ object=(\d+) recipient=(\d+) value=(\d+)")
HP = re.compile(r"borg-roster-state.*?actor=(\d+) hp=(\d+)")
CASES = (
    ("ordinary-default", "", False, False, False),
    ("ordinary-held-slot", "actor-contact=bolt|stop-and-hold-slot|3\n", False, False, False),
    ("ordinary-miss", "actor-contact=bolt|stop-and-hold-slot|3\n", False, True, False),
    ("rifle-continuation", "actor-contact=bolt|continue-once-per-actor\n", True, False, False),
    ("rifle-second-miss", "actor-contact=bolt|continue-once-per-actor\n", True, False, True),
)


def load_evidence(path, digest):
    raw = ordinary.read_bounded(Path(__file__).resolve().parents[2] / path, 16 << 10)
    if sha256(raw).hexdigest() != digest:
        raise ValueError(f"projectile contact source receipt changed: {path}")
    return json.loads(raw)


def compare(executable, *, baseline_only=False):
    executable = Path(executable).resolve(strict=True)
    type0 = load_evidence(ordinary.EVIDENCE, ordinary.EVIDENCE_SHA256)
    type2 = load_evidence(rifle.EVIDENCE, rifle.EVIDENCE_SHA256)
    matched_controls = [x["cases"][2] for x in type0["pilots"]]
    return_visit, = {x["carrier_and_descriptor_return_visit"] for x in matched_controls}
    ordinary_damage, = {x["requested_damage"] for x in matched_controls}
    same_carrier = type2["cases"][-1]
    rifle_damage = same_carrier["second_amount"]
    source_hp = type0["receiver"]["initial_hp"]
    rows = []
    with TemporaryDirectory(prefix="gf-native-projectile-contact-") as temp:
        root = Path(temp) / "content"
        prepare(root)
        stage = prepare_stage(root)
        with (stage / "stage.manifest").open("a", encoding="ascii") as manifest:
            manifest.write("spawn=contact-shooter|0|0|-550\nspawn=contact-first|0|0|-410\n"
                           "spawn=contact-second|0|0|-270\nspawn=contact-offset|256|0|-270\n")
        basic = (f"GFBORG-GAMEPLAY 1\nvariant=normal\nhealth={source_hp}\n"
                 "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n")
        (root / "borgs/flight-stick/gameplay.txt").write_text(basic + "movement=stationary-v1\n", encoding="ascii")
        preset = root / "contact.roster"
        for name, policy, continues, first_miss, second_miss in CASES:
            if baseline_only and name.startswith("ordinary-") and name != "ordinary-default":
                continue
            damage = rifle_damage if continues else ordinary_damage
            expected_hits = [] if first_miss else [1, 2] if continues and not second_miss else [1]
            expected_hold = return_visit if name == "ordinary-held-slot" else 0
            attack = basic + (
                "movement=grounded-v1|0.4|13|0.4\nwalk=0:1\nair=0:2\ndash=0:3\n"
                f"stock=primary|1|180\nprojectile=bolt|bullet|1|38|{100 if first_miss else 0}|0|0|8|24|{damage}|ignore-floor\n"
                "move=fire|b|10|grounded|24|0:4\nentry-stock=fire|primary|1\nrelease=fire|2|primary|1|bolt\n"
            )
            (root / "borgs/jump-stick/gameplay.txt").write_text(attack + policy, encoding="ascii")
            preset.write_text(
                "GFROSTER 1\nlocals=2\nstage=example:workshop\n"
                "fighter=0|example:jump-stick|normal|contact-shooter\n"
                "fighter=1|example:flight-stick|normal|contact-first\n"
                f"fighter=1|example:flight-stick|normal|contact-{'offset' if second_miss else 'second'}\n",
                encoding="ascii",
            )
            result = run([executable, "--battle-roster", preset, "--demo-package-battle", "--trace-package-battle",
                          "--frames", "40", "--headless", "--no-audio"], cwd=root, capture_output=True, text=True, timeout=45)
            if result.returncode:
                raise ValueError(f"native contact fixture failed: {name}\n{result.stderr[-4000:]}")
            events = [(int(t), int(a), e, int(o), int(r), int(v)) for t, a, e, o, r, v in EVENT.findall(result.stderr)]

            def events_for(kind):
                return [row for row in events if row[1] == 0 and row[2] == kind]

            emits, hits, closes, returns = map(events_for, ("emit", "hit", "projectile-contact-closed", "retire"))
            hp_after = {int(a): int(hp) for a, hp in HP.findall(result.stderr)}
            if (len(emits) != 1 or len(events_for("debit")) != 1 or len(returns) != 1 or len(closes) != bool(expected_hold)
                    or [h[4] for h in hits] != expected_hits or any(h[5] != damage for h in hits)
                    or any(row[3] != emits[0][3] for row in hits + closes + returns)
                    or any(a[0] >= b[0] for a, b in zip(hits, hits[1:]))
                    or hp_after != {i: source_hp-(damage if i in expected_hits else 0) for i in range(3)}):
                raise ValueError(f"unexpected native contact trace: {name}\n{result.stderr[-4000:]}")
            return_after_hit = returns[0][0]-hits[-1][0] if hits else None
            if ((not continues and hits and return_after_hit != expected_hold)
                    or (continues and (not hits or return_after_hit <= 0))
                    or (closes and (closes[0][0] != hits[-1][0] or closes[0][5] != expected_hold))):
                raise ValueError(f"native contact close/return order changed: {name}")
            rows.append({
                "case": name, "debits": 1, "allocations": 1, "one_object_identity": True,
                "recipients_in_order": expected_hits, "hp_removed": [damage]*len(hits),
                "final_hp_by_actor": [hp_after[i] for i in range(3)],
                "native_contact_updates_from_emission": [h[0]-emits[0][0] for h in hits],
                "native_slot_return_update_from_emission": returns[0][0]-emits[0][0],
                "native_slot_return_updates_after_last_hit": return_after_hit, "opt_in_contact_closed_events": len(closes),
            })
    return {
        "kind": "gf-native-projectile-contact-comparison", "revision": 1,
        "original_type0_sha256": ordinary.EVIDENCE_SHA256, "original_rifle_next_receiver_sha256": rifle.EVIDENCE_SHA256,
        "original_type0": {
            "routes": [{"borg": p["borg"], "key": p["key"]} for p in type0["pilots"]],
            "miss_and_seeded_duplicate_do_not_write_hp": all(not c["hp_writer_reached"] for p in type0["pilots"] for c in p["cases"][:2]),
            "requested_damage_at_declared_inputs": ordinary_damage, "attack_unlinks_on_next_visit": True,
            "carrier_and_descriptor_return_visit": return_visit,
        },
        "original_rifle": {
            "route": type2["credit"], "same_token_first_receiver_repeat_refused":
                not type2["first_receiver_control"]["same_geometry_and_token_reaches_amount"],
            "miss_and_seeded_second_receiver_duplicate_do_not_write_hp":
                all(not row["second_hp_writer_reached"] for row in type2["cases"][:2]),
            "hp_removed_per_fresh_recipient": [rifle_damage, rifle_damage],
        },
        "native_authored_cases": rows,
        "notes": [
            "Native Workshop fixtures explicitly use the two observed damage numbers. This isolates recipient and carrier custody, not the retail amount evaluator or the exact rigs and launch poses.",
            "Native default returns the carrier on the hit update. The new opt-in stops native contact immediately but delays slot return for three subsequent updates. The original attack-cell unlink and carrier descriptor return are separate; the native proxy has no claim to that original registry schedule.",
            "The rifle retains an allocation across later contacts and does not spend again. Exact retail package selection is checked separately; no launch timing, exact shape test, pose, hardware or complete moveset is inferred.",
        ],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--baseline-only", action="store_true", help="use only policies also supported by the previous native host")
    args = parser.parse_args()
    if args.output is not None and (args.output.exists() or args.output.is_symlink()):
        raise FileExistsError("projectile contact comparison output already exists")
    text = json.dumps(compare(args.binary, baseline_only=args.baseline_only), indent=2, sort_keys=True) + "\n"
    if args.output is None:
        print(text, end="")
    else:
        with args.output.open("x", encoding="ascii") as stream:
            stream.write(text)
