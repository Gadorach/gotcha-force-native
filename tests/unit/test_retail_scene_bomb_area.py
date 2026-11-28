"""Keep terminal allocation, publication, damage credit, and return separate."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_scene_bomb_area as area


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSceneBombAreaTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-scene-bomb-area.json").read_bytes())
        cls.cases = {(r["entry_hold_visits"], r["shape_cells_offered"]): r for r in cls.report["cases"]}

    def test_source_scope(self):
        for name in ("scene_bomb_world", "type41_area_hp", "contact_effect_source_art"):
            prior = MAPS / f"gg4e08-{name.replace('_', '-')}.json"
            self.assertEqual(self.report[f"{name}_sha256"], sha256(prior.read_bytes()).hexdigest())
        common = self.report["common_collision"]
        self.assertEqual((common["group"], common["descriptor"], common["amount_record"], common["raw_power"]),
                         (179, 246, "802C49D0", 20))
        self.assertEqual(set(self.cases), {(d, s) for d in (0, 10) for s in (1, 2)})
        self.assertEqual(sum(r["completed_coordinator_visits"] for r in self.cases.values()), 828)

    def test_one_shape_is_not_reused_during_child_initialization(self):
        for delay in (0, 10):
            row = self.cases[delay, 1]
            samples = {s["visit_from_press"]: s for s in row["samples"]}
            self.assertTrue(row["parent_shape_active_and_marked_at_area_initializer"])
            self.assertEqual(row["free_shape_cells_at_area_initializer"], 0)
            self.assertTrue(samples[181]["area_active"])
            self.assertEqual(samples[181]["area_phase_field"], 2)
            self.assertIsNone(row["area_first_publish_visit"])
            self.assertIsNone(row["area_first_hit_visit"])
            self.assertEqual((row["parent_shape_return_visit"], row["area_carrier_return_visit"]), (181, 183))
            self.assertTrue(all(not s["area_shape_bound"] and s["hp_by_seat"] == [180, 220] for s in samples.values()))

    def test_spare_shape_binds_before_publication_and_survives_parent_return(self):
        for delay in (0, 10):
            row = self.cases[delay, 2]
            samples = {s["visit_from_press"]: s for s in row["samples"]}
            self.assertEqual((row["free_shape_cells_at_area_initializer"], row["area_initializer_visit"]), (1, 181))
            self.assertTrue(samples[181]["area_shape_bound"])
            self.assertFalse(samples[181]["area_shape_published"])
            self.assertEqual((row["area_first_publish_visit"], row["parent_stock_return_visit"]), (182, 182))
            self.assertEqual((samples[182]["attack_shapes"], samples[182]["ordinary_receivers"]), (1, 18))
            self.assertFalse(samples[183]["parent_active"])
            self.assertTrue(samples[183]["area_active"])
            self.assertEqual((row["area_shape_return_visit"], row["area_carrier_return_visit"]), (198, 231))
            self.assertTrue(samples[198]["area_active"])
            self.assertFalse(samples[198]["area_shape_bound"])
            self.assertFalse(samples[231]["area_active"])

    def test_self_hit_is_received_damage_not_ally_credit(self):
        for delay in (0, 10):
            row = self.cases[delay, 2]
            self.assertEqual((row["parent_token"], row["area_first_hit_visit"]), (1, 185))
            hit = next(s for s in row["samples"] if s["visit_from_press"] == 185)
            self.assertEqual(hit["hp_writes_in_order"], [
                {"receiver_seat": 0, "relation": "opponent", "requested_hp": 17, "hp_removed": 17, "accepted_token": 2},
                {"receiver_seat": 1, "relation": "self", "requested_hp": 4, "hp_removed": 4, "accepted_token": 2},
            ])
            self.assertEqual((hit["hp_by_seat"], hit["received_damage_ledgers_by_seat"]), ([163, 216], [17, 4]))
            self.assertEqual((hit["owner_opponent_damage_credit"], hit["owner_ally_damage_credit"]), (17, 0))
            self.assertEqual(sum(len(s["hp_writes_in_order"]) for s in row["samples"]), 2)

    def test_rejects_malformed_report_and_output_overwrite(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            area.decode("dual-world-pay: 0 1 9\n")
        with self.assertRaisesRegex(ValueError, "changed at row 2"):
            area.decode("dual-world-pay: 0 1 9\n" + "dual-area: invalid\n"*45)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "area.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                area.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
