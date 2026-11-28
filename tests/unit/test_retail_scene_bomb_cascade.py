"""Keep the second bomb's zero-HP branch, nested area, and credit separate."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_scene_bomb_cascade as cascade


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSceneBombCascadeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-scene-bomb-cascade.json").read_bytes())
        cls.cases = {(r["entry_hold_visits"], r["case"]): r for r in cls.report["cases"]}

    def test_source_scope(self):
        prior = MAPS / "gg4e08-scene-bomb-reaction.json"
        self.assertEqual(self.report["scene_bomb_reaction_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual((self.report["zero_hp_terminal_site"], self.report["area_key"],
                          self.report["common_group"], self.report["amount_record"]),
                         ("800E0B18", 19, 179, "802C49D0"))
        self.assertEqual((self.report["old_area_token"], self.report["second_bomb_token"],
                          self.report["new_area_token_if_admitted"]), (2, 3, 4))
        self.assertEqual(sum(r["completed_fork_visits"] for r in self.cases.values()), 288)

    def test_spare_carrier_is_not_enough_when_both_prior_shapes_are_still_bound(self):
        for delay in (0, 10):
            missing_carrier, missing_shape, admitted = [self.cases[delay, k] for k in cascade.CASES]
            self.assertFalse(missing_carrier["new_area_carrier_allocated"])
            self.assertIsNone(missing_carrier["free_shape_cells_at_initializer"])
            self.assertTrue(missing_shape["new_area_carrier_allocated"])
            self.assertEqual(missing_shape["free_shape_cells_at_initializer"], 0)
            self.assertIsNone(missing_shape["new_area_first_publish_visit"])
            self.assertEqual(missing_shape["new_area_carrier_return_visit"], 189)
            self.assertEqual(admitted["free_shape_cells_at_initializer"], 1)
            self.assertEqual(admitted["new_area_first_publish_visit"], 188)
            for row in (missing_carrier, missing_shape, admitted):
                self.assertEqual((row["second_press_visit"], row["request_visit"],
                                  row["second_bomb_lifetime_at_request"]), (186, 187, 180))
                self.assertEqual((row["second_bomb_stock_return_visit"], row["second_bomb_carrier_return_visit"]),
                                 (188, 189))

    def test_carrier_loses_one_hp_before_fresh_area_hits_both_receivers(self):
        for delay, opponent_visit in ((0, 190), (10, 191)):
            row = self.cases[delay, cascade.CASES[2]]
            samples = {s["visit_from_first_press"]: s for s in row["samples"]}
            self.assertEqual(samples[186]["hp_write"], {
                "source_token": 2, "receiver": "second-bomb", "receiver_tier_at_amount": None,
                "hp_before": 1, "hp_requested": 4, "hp_removed": 1,
            })
            self.assertEqual(samples[188]["hp_write"]["receiver"], "seat-1")
            self.assertEqual(samples[opponent_visit]["hp_write"]["receiver"], "seat-0")
            for visit, loss in ((188, 4), (opponent_visit, 17)):
                self.assertEqual((samples[visit]["hp_write"]["source_token"],
                                  samples[visit]["hp_write"]["hp_removed"]), (4, loss))
                self.assertTrue(samples[visit]["old_area_active"])
                self.assertEqual(samples[visit]["total_attacks"], 2)
            hit = samples[opponent_visit]
            self.assertEqual((hit["accepted_tokens_by_seat"], hit["hp_by_seat"]), ([[2, 4], [2, 4]], [146, 212]))
            self.assertEqual((hit["owner_opponent_credit"], hit["owner_ally_credit"],
                              hit["received_damage_ledgers_by_seat"]), (34, 0, [34, 8]))

    def test_shapes_and_carriers_return_separately(self):
        for delay in (0, 10):
            row = self.cases[delay, cascade.CASES[2]]
            self.assertEqual((row["old_area_shape_return_visit"], row["new_area_shape_return_visit"]), (198, 204))
            self.assertEqual((row["old_area_carrier_return_visit"], row["new_area_carrier_return_visit"]), (231, 237))
            final = row["samples"][-1]
            self.assertEqual((final["free_carrier_cells"], final["free_shape_cells"], final["free_view_cells"]),
                             (3, 3, 18))
            self.assertFalse(final["old_area_active"] or final["new_area_active"])
            self.assertEqual(final["stock"], 10)

    def test_rejects_malformed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            cascade.decode("cascade: 0\n")
        with self.assertRaisesRegex(ValueError, "changed at row 1"):
            cascade.decode("cascade: invalid\n"*66)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "cascade.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                cascade.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
