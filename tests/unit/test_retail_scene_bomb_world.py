"""Pin payment, world scheduling, and resource return as separate boundaries."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_scene_bomb_world as bomb


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSceneBombWorldTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-scene-bomb-world.json").read_bytes())
        cls.rows = {(r["entry_hold_visits"], r["case"]): r for r in cls.report["cases"]}

    def test_payment_precedes_allocation_and_the_two_refusals_return_differently(self):
        self.assertEqual(self.report["dual_scene_entry_sha256"],
                         sha256((MAPS / "gg4e08-dual-scene-entry.json").read_bytes()).hexdigest())
        for delay in (0, 10):
            no_carrier, no_shape = [self.rows[delay, tag] for tag in ("no-carrier", "no-shape")]
            self.assertEqual([r["channel_0_stock_at_factory_entry"] for r in (no_carrier, no_shape)], [9, 9])
            self.assertEqual([r["stock_return_visit"] for r in (no_carrier, no_shape)], [1, 2])
            self.assertEqual([s["stock_after_world"] for s in no_carrier["samples"]], [10, 10, 10])
            self.assertEqual([s["stock_after_world"] for s in no_shape["samples"]], [9, 10, 10])
            self.assertTrue(all(not s["parent_shape_published_both_roles"] for s in no_shape["samples"]))

    def test_first_successful_world_visit_publishes_one_shape_in_both_lists(self):
        self.assertEqual((self.report["owner_seat"], self.report["parent_bucket"],
                          self.report["view_record"], self.report["parent_shape_group"]), (1, 10, 3, 15))
        for delay in (0, 10):
            first = self.rows[delay, "natural-expiry"]["samples"][0]
            self.assertEqual((first["visit_from_press"], first["total_attack_shapes"],
                              first["total_ordinary_receivers"]), (1, 1, 19))
            self.assertEqual((first["stock_after_world"], first["recorded_parent_token"],
                              first["parent_lifetime_field"]), (9, 1, 180))
            self.assertTrue(first["parent_shape_published_both_roles"])

    def test_natural_expiry_request_precedes_stock_and_carrier_return(self):
        self.assertEqual(sum(r["completed_coordinator_visits"] for r in self.rows.values()), 384)
        for delay in (0, 10):
            row = self.rows[delay, "natural-expiry"]
            self.assertEqual((row["press_in_entry_visit"], row["stock_return_visit"], row["carrier_retired_visit"]),
                             (175+delay, 182, 183))
            visits = {s["visit_from_press"]: s for s in row["samples"]}
            self.assertEqual([visits[n]["parent_lifetime_field"] for n in (178, 179, 180, 181)], [3, 2, 1, 0])
            self.assertEqual(visits[181]["terminal_request"],
                             {"key": 19, "caller": "800E0B4C", "area_carrier_allocated": False})
            self.assertFalse(visits[181]["parent_shape_published_both_roles"])
            self.assertTrue(visits[182]["parent_carrier_active"])
            self.assertFalse(visits[183]["parent_carrier_active"])
            self.assertTrue(all(s["hp_by_seat"] == [180, 220] for s in visits.values()))
            self.assertFalse(row["actor_damage_ledger_changed"])

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "changed at row"):
            bomb.decode("dual-world-bomb: invalid\n"*44)
        with TemporaryDirectory() as tmp:
            artifact = Path(tmp) / "bomb.json"
            artifact.write_text("keep")
            with self.assertRaises(FileExistsError):
                bomb.export(None, None, None, None, artifact)
            self.assertEqual(artifact.read_text(), "keep")
