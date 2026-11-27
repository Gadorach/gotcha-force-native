"""Keep exact melee publication, two body timers and same-pair refusal separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_melee_pose_clock as melee


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-melee-pose-clock.json"


def report(rows):
    lines = []
    for r in rows:
        fields = (int(r["borg"].split(":")[1], 16), melee.CASES.index(r["case"]), r["published_attack_shapes"],
                  r["selected_step"] or 0, r["source_seed"], *r["timers_before"], *r["timers_after_pair"],
                  *r["slow_body_clock_visits"], r["hp_lost"], r["receiver_hp_after"], r["receiver_token"] or 0)
        lines.append("melee-clock: " + " ".join(map(str, fields)))
    return "\n".join(lines)


class RetailMeleePoseClockTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = cls.data["cases"]

    def test_lane_and_spatial_refusals_do_not_arm_both_bodies(self):
        for model, count in (("gg4e08:0807", 1), ("gg4e08:0001", 2)):
            disabled, miss = (r for r in self.rows if r["borg"] == model and r["receiver_token"] is None)
            for row, published in ((disabled, 0), (miss, count)):
                self.assertEqual((row["published_attack_shapes"], row["timers_after_pair"], row["hp_lost"]),
                                 (published, [0, 0], 0))

    def test_both_body_timers_arm_before_amount_and_keep_independent_larger_values(self):
        accepted = [r for r in self.rows if r["receiver_token"] is not None]
        self.assertEqual(len(accepted), 8)
        for row in accepted:
            expected = [max(timer, row["source_seed"]) for timer in row["timers_before"]]
            self.assertEqual(row["timers_at_amount_entry"], expected)
            self.assertEqual(row["timers_after_pair"], expected)
            self.assertEqual(row["slow_body_clock_visits"], [n-1 for n in expected])
            self.assertTrue(row["retained_pair_did_not_rearm_after_expiry"])

    def test_gred_steps_are_individually_bound_to_one_amount_record(self):
        rows = {r["case"]: r for r in self.rows if r["borg"] == "gg4e08:0001"}
        both, one, two = [rows[k] for k in ("all-shapes-overlap", "only-step-5-overlaps", "only-step-6-overlaps")]
        self.assertEqual([r["selected_step"] for r in (both, one, two)], [5, 5, 6])
        for row in (both, one, two):
            self.assertEqual((row["accepted_amount_record"], row["hp_lost"], row["receiver_hp_after"]),
                             ("8031B008", 41, 139))

    def test_decoder_refuses_shape_or_timer_regression(self):
        good = report(self.rows)
        self.assertEqual(melee.decode(good), self.rows)
        for bad in (good.replace("1 6 2 6 8", "1 6 2 5 8", 1),
                    good.replace("4 6 2 6 4 5 3", "4 6 2 4 4 3 3", 1)):
            with self.assertRaises(ValueError):
                melee.decode(bad)

    def test_static_census_does_not_extend_the_two_executed_routes(self):
        census = self.data["scoped_melee_census"]
        self.assertEqual((census["reviewed_borg_rows"], census["distinct_amount_records"]), (9, 10))
        self.assertEqual(census["seed_counts"], {"0": 2, "4": 7, "8": 1})
        self.assertEqual(len(self.data["sources"]), 2)

    def test_output_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "melee.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                melee.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
