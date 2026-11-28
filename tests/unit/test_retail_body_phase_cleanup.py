"""Test distinct queue, resource, actor, and replacement boundaries."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_body_phase_cleanup as cleanup


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailBodyPhaseCleanupTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS/"gg4e08-body-phase-cleanup.json").read_bytes())
        cls.rows = {(r["area_key"], r["case"]): r for r in cls.report["cases"]}

    def test_phase3_wait_counts_only_allowed_visits(self):
        self.assertEqual(self.report["phase_exit_sha256"], sha256((MAPS/"gg4e08-body-phase-exit.json").read_bytes()).hexdigest())
        self.assertEqual(self.report["scene_allow_mask"], {"off": 0, "holds_player_1": 1, "allows_player_1": 2})
        self.assertEqual(len(self.rows), 6)
        for row in self.rows.values():
            self.assertEqual((row["phase3_wait_armed"], row["excluded_player_mask_visits"],
                              row["wait_expiry_allowed_visit"]), (60, 2, 60))
            self.assertEqual(row["after_wait"]["retained_hit_token"], 2)

    def test_normal_queue_and_fallback_stop_before_disc(self):
        for key in (19, 20):
            queue = self.rows[key, "queue-next"]["next_model_request"]
            fallback = self.rows[key, "mode1-terminal-placeholder"]["next_model_request"]
            self.assertEqual((queue["model"], queue["afs_member"], queue["cursor_at_disc_cut"]), ("0106", 504, 0))
            self.assertEqual((fallback["model"], fallback["afs_member"], fallback["cursor_at_disc_cut"]), ("0F07", 693, 1))
            self.assertTrue(queue["old_row_marked"] and queue["selected_row_marked"])
            self.assertFalse(fallback["old_row_marked"] or fallback["selected_row_marked"])
            for request in (queue, fallback):
                self.assertTrue(request["old_empty_owner_returned"] and request["archive_context_heap_returned"])
                self.assertTrue(request["actor_still_active_at_disc_cut"])
                self.assertFalse(request["disc_load_ran"])

    def test_actor_inactivity_and_its_old_resource_need_not_coincide(self):
        for key in (19, 20):
            exhausted = self.rows[key, "roster-exhausted"]
            self.assertIsNone(exhausted["next_model_request"])
            self.assertEqual(exhausted["after_wait"], {"actor_active": False, "actor_phase": 3, "subphase": 2,
                             "scene_slot_mask": 1, "scene_active_count": 1, "retained_hit_token": 2,
                             "old_empty_owner_still_bound": True})
            queued = self.rows[key, "queue-next"]
            self.assertTrue(queued["after_wait"]["actor_active"])
            self.assertEqual(queued["after_wait"]["actor_phase"], 0)
            self.assertTrue(queued["phase_zero_control"]["missing_owner_leaves_phase_zero"])
            self.assertEqual(queued["phase_zero_control"]["generation_delta_at_class_setup"], 1)
            self.assertEqual(queued["phase_zero_control"]["hp_at_class_setup"], 0)
            self.assertFalse(queued["phase_zero_control"]["class_setup_ran"])

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "changed at row"):
            cleanup.decode("body-cleanup-expiry: invalid\n"*12)
        with TemporaryDirectory() as name:
            output = Path(name) / "cleanup.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                cleanup.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
