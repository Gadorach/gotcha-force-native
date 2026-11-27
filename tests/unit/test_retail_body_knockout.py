"""Keep original damage, Force, reserve and actor credit records separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_body_knockout as knockout


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-body-knockout.json"


class RetailBodyKnockoutTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads(ARTIFACT.read_bytes())
        cls.rows = {(r["area_key"], r["case"]): r for r in cls.report["cases"]}

    def test_hp_request_clipping_and_cost_are_independent(self):
        self.assertEqual(self.report["receiver_cached_force_cost"], 150)
        self.assertEqual(len(self.rows), 16)
        for key in (19, 20):
            survived = self.rows[key, "one-hp-survives"]
            exact = self.rows[key, "exact-zero"]
            overkill = self.rows[key, "overkill-force-clamp"]
            self.assertEqual((survived["hp_after"], survived["meter_after"], survived["force_after"]), (1, 50, 200))
            self.assertEqual((exact["hp_after"], exact["meter_after"], exact["force_after"]), (0, 0, 50))
            self.assertEqual((overkill["hp_request"], overkill["hp_removed"], overkill["damage_ledger_delta"],
                              overkill["receiver_loss_cost"], overkill["force_after"]), (17, 7, 17, 150, 0))

    def test_force_and_reserves_can_request_the_same_scene_pair_independently(self):
        for key in (19, 20):
            reserved = self.rows[key, "reserve-without-force-debit"]
            ineligible = self.rows[key, "reserve-recipient-ineligible"]
            depleted = self.rows[key, "exact-force-zero-empty-reserve"]
            self.assertEqual((reserved["force_after"], reserved["reserves_after"]), (200, 1))
            self.assertTrue(reserved["scene_actor_pair_written"])
            self.assertEqual((ineligible["force_after"], ineligible["reserves_after"]), (200, 2))
            self.assertFalse(ineligible["scene_actor_pair_written"])
            self.assertEqual((depleted["force_after"], depleted["reserves_after"]), (0, 0))
            self.assertTrue(depleted["scene_actor_pair_written"])

    def test_same_player_debit_has_no_opponent_credit_and_held_hp_has_no_ko(self):
        for key in (19, 20):
            same = self.rows[key, "same-player-slot-zero"]
            self.assertEqual((same["force_after"], same["owner_credit_count"], same["receiver_loss_count"]), (50, 0, 1))
            self.assertEqual(same["owner_ring_relations"], [2, 4])
            self.assertFalse(same["receiver_last_knockout_owner_written"])
            held = self.rows[key, "scene-holds-hp"]
            self.assertEqual((held["hp_after"], held["damage_ledger_delta"], held["force_after"]), (7, 0, 140))
            self.assertEqual((held["owner_ring_relations"], held["owner_slot_tally_delta"]), ([1], 50))
            self.assertFalse(held["knockout"])

    def test_bad_report_and_existing_output_refuse(self):
        with self.assertRaises(ValueError):
            knockout.decode("body-knockout: 6")
        with TemporaryDirectory() as name:
            out = Path(name) / "knockout.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                knockout.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
