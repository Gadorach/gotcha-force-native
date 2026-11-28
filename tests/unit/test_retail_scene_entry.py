"""Pin the source coordinator and its handoff to the loaded actor."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_scene_entry as scene


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSceneEntryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads((MAPS / "gg4e08-scene-entry.json").read_bytes())

    def test_every_active_unready_physical_seat_blocks(self):
        self.assertEqual(self.data["replacement_entry_sha256"],
                         sha256((MAPS / "gg4e08-replacement-entry.json").read_bytes()).hexdigest())
        self.assertEqual([r["seat"] for r in self.data["readiness_seats"]], list(range(6)))
        for seat in self.data["readiness_seats"]:
            self.assertEqual((seat["active_unready_leaves_substep"], seat["inactive_unready_allows_substep"]), (0, 1))
            self.assertTrue(seat["ready_actors_have_zero_hp"])

    def test_countdown_reaches_substep_three_before_status_four(self):
        clocks = {r["case"]: r for r in self.data["scene_clocks"]}
        self.assertEqual([clocks[k]["scene_status_four_visit"] for k in ("short", "extended", "skip-delays")],
                         [93, 212, 2])
        for schedule in clocks.values():
            first, release = schedule["samples"][-2:]
            self.assertEqual((first["scene_substep"], first["timer"], first["status_before_actor_pass"]), (3, 0, 3))
            self.assertEqual((release["handler"], release["status_before_actor_pass"], release["hud_byte_13"]),
                             ("80029408", 4, 0))
        self.assertEqual(self.data["ending_control"]["status_before_actor_pass"], 5)

    def test_ready_write_handoff_and_first_paid_request_are_distinct(self):
        self.assertEqual([r["area_key"] for r in self.data["replacement_handoffs"]], [19, 20])
        for row in self.data["replacement_handoffs"]:
            self.assertEqual((row["actor_marks_ready"], row["scene_observes_ready"], row["scene_writes_four"],
                              row["ordinary_receiving_publishes"], row["first_processed_pad_read"]),
                             (80, 81, 173, 173, 174))
            samples = {s["visit"]: s for s in row["samples"]}
            self.assertFalse(samples[80]["ready_before_coordinator"])
            self.assertTrue(samples[80]["ready_after_actor"])
            self.assertFalse(samples[173]["processed_pad_read"])
            self.assertEqual(samples[173]["ordinary_receiving_shapes_after_rebuild"], 9)
            self.assertEqual([b["channel_0_stock_after"] for b in row["fresh_b_forks"]], [10, 10, 9])
            self.assertEqual(row["fresh_b_forks"][-1]["factory_request"], {"key": 2, "pose": 10, "allocation_ran": False})

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "changed at row"):
            scene.decode_scene("scene-seat: invalid\n"*27)
        with self.assertRaises(ValueError):
            scene.decode_bridge("scene-bridge: invalid")
        with TemporaryDirectory() as name:
            out = Path(name) / "scene-entry.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                scene.export(None, None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
