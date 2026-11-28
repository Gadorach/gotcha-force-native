"""Keep normal dual construction, readiness, role publication, and pad routing distinct."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_dual_scene_entry as dual


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailDualSceneEntryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS/"gg4e08-dual-scene-entry.json").read_bytes())
        cls.cases = {r["seat_1_phase_dispatch_held_visits"]: r for r in cls.report["cases"]}

    def test_separate_cold_actor_assets_and_views(self):
        self.assertEqual(self.report["scene_entry_sha256"], sha256((MAPS/"gg4e08-scene-entry.json").read_bytes()).hexdigest())
        self.assertEqual([(r["seat"], r["borg"], r["afs_member"]) for r in self.report["actors"]],
                         [(0, "gg4e08:000a", 495), (1, "gg4e08:0106", 504)])
        for case in self.cases.values():
            self.assertEqual((case["both_constructors_finished"], case["source_view_roots_by_seat"],
                              case["total_view_wrappers"], case["total_bound_shapes"]), (1, [19, 4], 46, 32))

    def test_readiness_waits_for_later_actor_and_advances_on_next_scan(self):
        no_hold, hold = (self.cases[n] for n in (0, 10))
        self.assertEqual(no_hold["ready_writes_by_seat"], [81, 81])
        self.assertEqual(hold["ready_writes_by_seat"], [81, 91])
        self.assertEqual(hold["seat_1_hold_visit_range"], [2, 11])
        self.assertEqual([r["coordinator_releases_readiness_barrier"] for r in (no_hold, hold)], [82, 92])
        samples = {r["visit"]: r for r in hold["timeline"]}
        self.assertEqual((samples[81]["ready_after_actors_mask"], samples[81]["scene_substep"]), (1, 0))
        self.assertEqual((samples[91]["ready_before_scan_mask"], samples[91]["ready_after_actors_mask"],
                          samples[91]["scene_substep"]), (1, 3, 0))

    def test_both_receiving_lists_open_before_either_pad_reads(self):
        for delay, case in self.cases.items():
            opened = 174+delay
            self.assertEqual((case["scene_enters_status_four"], case["first_both_receiving_lists"],
                              case["first_both_pad_reads"]), (opened, opened, opened+1))
            samples = {r["visit"]: r for r in case["timeline"]}
            self.assertEqual(samples[opened-1]["ordinary_receiving_shape_counts"], [0, 0])
            self.assertEqual(samples[opened]["ordinary_receiving_shape_counts"], [9, 9])
            opening, following = case["processed_b_forks"]
            self.assertEqual(opening["pad_read_order"], [])
            self.assertEqual(following["pad_read_order"], [0, 1])
            self.assertEqual((following["seat_0_b_held"], following["seat_0_stock_after"],
                              following["seat_1_stock_after"]), (False, 10, 9))
            self.assertEqual(following["factory_request"], {"owner_seat": 1, "key": 2, "pose": 10, "allocation_ran": False})

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "changed at row"):
            dual.decode("dual-scene: invalid\n"*30)
        with TemporaryDirectory() as tmp:
            artifact = Path(tmp) / "dual.json"
            artifact.write_text("keep")
            with self.assertRaises(FileExistsError):
                dual.export(None, None, None, None, artifact)
            self.assertEqual(artifact.read_text(), "keep")
