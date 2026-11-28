from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_prop_recovery as recovery


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStagePropRecoveryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-prop-recovery.json").read_bytes())
        cls.rows = {r["checkpoint"]: r for r in cls.report["checkpoints"]}

    def test_exact_prop_and_declared_break_trigger(self):
        prior = (MAPS / "gg4e08-stage-prop-support.json").read_bytes()
        self.assertEqual(self.report["stage_prop_support_sha256"], sha256(prior).hexdigest())
        self.assertEqual((self.report["declared_destruction_hp"], self.report["destruction_cue"]), (0, 115))
        self.assertFalse(self.report["audible_playback_qualified"])
        self.assertEqual(len(self.rows), 16)

    def test_collision_lifetime_and_support_flag_clear_on_distinct_passes(self):
        before, broken, expired, rebound = [self.rows[name] for name in (
            "supported-before-break", "break", "collision-after-release", "collision-rebound",
        )]
        self.assertEqual([r["live_plane_cell"] for r in (before, broken, expired, rebound)], [1, None, None, 0])
        self.assertEqual([r["shape_free_count"] for r in (before, broken, expired, rebound)], [0, 2, 2, 0])
        self.assertEqual([r["actor_active_support"] for r in (before, broken, expired, rebound)],
                         [True, True, False, False])
        self.assertTrue(expired["actor_previous_support"])
        self.assertEqual(rebound["actor_retained_shape_cell"], 1)
        self.assertEqual(self.report["rising_carry_fork"]["reused_old_plane_cell_descriptor"], 236)

    def test_original_wait_and_rise_edges_are_independent(self):
        self.assertEqual(self.report["rebind_visit_after_break"], 600)
        self.assertEqual([self.rows[n]["timer"] for n in ("break", "first-wait", "last-wait", "collision-rebound")],
                         [600, 599, 1, 0])
        rebound, first, second = [self.rows[n] for n in ("collision-rebound", "first-rise", "second-rise")]
        self.assertNotEqual(first["world_y_bits"], rebound["world_y_bits"])
        self.assertEqual(first["allocated_plane_y_bits"], rebound["allocated_plane_y_bits"])
        self.assertNotEqual(second["allocated_plane_y_bits"], first["allocated_plane_y_bits"])
        self.assertEqual(self.report["rise_steps_to_settle"], 61)
        self.assertNotEqual(self.rows["rise-60"]["rise_offset_bits"], "00000000")
        self.assertEqual(self.rows["settled"]["world_y_bits"], "41200000")
        self.assertEqual(self.rows["settled"]["rise_offset_bits"], "00000000")
        self.assertEqual(self.rows["settled"]["type4_placement_mask"], 0)

    def test_actual_rise_carries_reacquired_support_before_plane_center_updates(self):
        f = self.report["rising_carry_fork"]
        self.assertEqual((f["retired_plane_cell"], f["reacquired_plane_cell"]), (1, 0))
        self.assertEqual(f["plane_center_y_before_after_bits"], ["C3659290", "C3659290"])
        self.assertEqual(f["actor_y_after_carry_bits"], f["support_query_y_after_first_rise_bits"])
        self.assertAlmostEqual(
            recovery.stage.scalar(f["actor_y_after_carry_bits"])-recovery.stage.scalar(f["actor_y_before_carry_bits"]),
            14.493, places=3,
        )

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            recovery.decode("")
        with self.assertRaisesRegex(ValueError, "supported-before-break"):
            recovery.decode("stage-prop-recovery: invalid\n"*17)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "stage-prop-recovery.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                recovery.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
