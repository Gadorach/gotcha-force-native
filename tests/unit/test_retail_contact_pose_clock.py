"""Keep timer ownership, linked rate copying and the native pose hold separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_pose_clock as clock


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-pose-clock.json"


def body_report(rows):
    scenarios = {name: i for i, (name, _, _) in enumerate(clock.BODY_CASES)}
    return "\n".join(
        f"pose-clock: {scenarios.get(r['case'], 4)} {r['visit']} {r['timer_after']} "
        f"{r['base_rate_bits']} {r['primary_rate_bits']} {r['secondary_rate_bits']}" for r in rows
    )


def pair_report(rows):
    return "\n".join(
        "pause-pair: " + " ".join(map(str, (
            r["attack_type"], int(r["credited_borg"].split(":")[1], 16), r["attacking_object_kind"], r["source_seed"],
            r["receiver_timer_before"], r["receiver_timer_after_hit"], r["credited_actor_timer_after_hit"],
            r["slow_body_clock_visits"], r["hp_lost"], r["receiver_hp_after"],
        ))) for r in rows
    )


class RetailContactPoseClockTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_hit_arms_receiver_before_amount_and_leaves_projectile_credit_clear(self):
        rows = self.data["projectile_pairs"]
        self.assertEqual(clock.decode_pairs(pair_report(rows)), rows)
        self.assertTrue(all(r["credited_actor_timer_after_hit"] == 0 for r in rows))
        for r in rows:
            armed = max(r["source_seed"], r["receiver_timer_before"])
            self.assertEqual((r["receiver_timer_at_amount_entry"], r["receiver_timer_after_hit"],
                              r["slow_body_clock_visits"]), (armed, armed, max(0, armed-1)))
            self.assertEqual(r["visual_gate_after_clock_visits"], 4)
        saucer = [r for r in rows if r["attack_type"] == 32]
        self.assertEqual([r["slow_body_clock_visits"] for r in saucer], [3, 3, 5])

    def test_normal_clock_decrements_before_writing_rates_and_restores_product(self):
        rows = self.data["body_clock_visits"]
        self.assertEqual(clock.decode_clocks(body_report(rows)), rows)
        one = [r for r in rows if r["case"] == "one"]
        self.assertTrue(all(r["primary_rate_bits"] == clock.UNIT for r in one))
        eight = [r for r in rows if r["case"] == "eight"]
        self.assertEqual([r["timer_after"] for r in eight], [7, 6, 5, 4, 3, 2, 1, 0, 0, 0])
        self.assertEqual([r["secondary_rate_bits"] for r in eight],
                         [clock.SLOW]*7 + [clock.BOOSTED_SECONDARY]*3)

    def test_link_and_override_do_not_consume_follower_timer(self):
        rows = {r["case"]: r for r in self.data["body_clock_visits"]}
        names = tuple(row[0] for row in clock.LINK_CASES)
        self.assertEqual([rows[n]["timer_after"] for n in names], [7, 7, 7, 7, 7, 6])
        self.assertEqual([rows[n]["primary_rate_bits"] for n in names],
                         [clock.BOOST, clock.SLOW, clock.UNIT, clock.SLOW, clock.BOOST, clock.SLOW])
        self.assertEqual(rows["override-before-link"]["secondary_rate_bits"], clock.UNIT)

    def test_seed_census_is_scoped_not_whole_roster_equivalence(self):
        rows = self.data["scoped_amount_records"]
        self.assertEqual(rows["actor_amount_records"]["seed_counts"], {"0": 4, "1": 30, "2": 5, "4": 18, "8": 16})
        self.assertEqual(rows["common_amount_records"]["seed_counts"], {"0": 22})
        self.assertTrue(all(r["receiver_timer_bypass_records"] == 0 for r in rows.values()))

    def test_report_rejects_consuming_linked_timer_and_early_slow_tick(self):
        with self.assertRaises(ValueError):
            clock.decode_clocks(body_report(self.data["body_clock_visits"]).replace("4 2 7", "4 2 6", 1))
        with self.assertRaises(ValueError):
            clock.decode_pairs(pair_report(self.data["projectile_pairs"]).replace("16 1 0 1 0 0", "16 1 0 1 0 1", 1))

    def test_existing_output_is_not_replaced(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "clock.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                clock.export(None, None, None, None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
