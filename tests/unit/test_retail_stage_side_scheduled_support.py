from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_side_scheduled_support as scheduled


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStageSideScheduledSupportTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-side-scheduled-support.json").read_bytes())
        cls.targets = {row["descriptor"]: row for row in cls.report["cases"]}

    def test_exact_callers_and_prior_rebound_result(self):
        self.assertEqual((self.report["normal_caller"], self.report["normal_actor_dispatch_cut"],
                          self.report["normal_world_call_site"], self.report["alternate_caller"]),
                         ("80029434", "80053A2C", "800294C8", "80029708"))
        for stem in ("stage-side-rise-support", "stage-side-recovery"):
            raw = (MAPS / f"gg4e08-{stem}.json").read_bytes()
            self.assertEqual(self.report[stem.replace("-", "_")+"_sha256"], sha256(raw).hexdigest())
        self.assertEqual({d: (r["placement"], r["rebound_cell_ordinal"]) for d, r in self.targets.items()},
                         {237: (0, 11), 238: (4, 7)})

    def test_bucket_advances_local_rise_without_advancing_world_before_actor_dispatch(self):
        scalar = scheduled.rise.carry.stage.scalar
        for target in self.targets.values():
            observed = target["before_world"]
            self.assertTrue(observed["bucket_advanced_local_offset"])
            self.assertTrue(observed["owner_root_held"])
            self.assertTrue(observed["carry_admitted"])
            self.assertLess(abs(scalar(observed["actor_rise_bits"])), 0.001)

    def test_scheduled_contact_reanchors_while_allocated_center_is_held(self):
        scalar = scheduled.rise.carry.stage.scalar
        for target in self.targets.values():
            first, second = target["visits"][:2]
            self.assertEqual([first["counter_after"], second["counter_after"]], [18, 19])
            self.assertEqual(first["allocated_center_rise_bits"], "00000000")
            self.assertEqual(first["actor_rise_at_cut_bits"], "00000000")
            self.assertLess(scalar(first["local_anchor_y_change_bits"]), 0)
            self.assertEqual(second["owner_rise_bits"], second["allocated_center_rise_bits"])
            self.assertGreater(scalar(second["actor_rise_at_cut_bits"]), 0)

    def test_same_old_sphere_history_requires_prior_support_in_both_targets(self):
        for target in self.targets.values():
            _, retained_old, retained_new, fresh_old, fresh_new = target["visits"]
            for key in ("declared_actor_rise_bits", "sphere_current_y_from_initial_actor_bits",
                        "sphere_previous_y_from_initial_actor_bits", "owner_rise_bits", "allocated_center_rise_bits",
                        "target_narrow_calls"):
                self.assertEqual(retained_old[key], fresh_old[key])
            self.assertTrue(retained_old["previous_support_latch"])
            self.assertTrue(retained_old["current_exact_support"])
            self.assertFalse(fresh_old["previous_support_latch"])
            self.assertFalse(fresh_old["current_exact_support"])
            self.assertIsNone(fresh_old["supported_height_from_initial_actor_bits"])
            self.assertEqual(fresh_old["stop_before"], "8005A580")
            self.assertTrue(retained_new["current_exact_support"])
            self.assertTrue(fresh_new["current_exact_support"])
            self.assertFalse(fresh_new["previous_support_latch"])

    def test_isolated_landing_and_scheduled_model_frontier_stay_distinct(self):
        for descriptor, target in self.targets.items():
            for row in target["visits"]:
                if not row["current_exact_support"]:
                    self.assertIsNone(row["isolated_landing_return"])
                    continue
                self.assertEqual(row["stop_before"], "800466DC")
                self.assertEqual(row["actor_rise_at_cut_bits"], row["isolated_landed_rise_bits"])
                self.assertEqual(row["isolated_landing_return"], int(descriptor == 238))

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            scheduled.decode("", [], [])
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "scheduled.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                scheduled.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
