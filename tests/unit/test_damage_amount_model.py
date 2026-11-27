"""Pin the translated damage-amount chain and its two observations."""
import json
import unittest
from pathlib import Path

from tools.analyze import damage_amount_model as model

DOL = Path("/home/gadorach/Projects/GotchaForce/original/extracted/sys/main.dol")


class DamageAmountModelTest(unittest.TestCase):
    def test_stage_order(self):
        names = [stage["name"] for stage in model.STAGES]
        self.assertEqual(len(names), 19)
        order = {name: index for index, name in enumerate(names)}
        self.assertLess(order["credit_difficulty"], order["matchup"])
        self.assertLess(order["attacker_rank"], order["matchup"])
        self.assertLess(order["matchup"], order["defender_class_row"])
        self.assertLess(order["defender_class_row"], order["receiver_rank"])
        self.assertLess(order["receiver_rank"], order["tier"])

    def test_credit_split(self):
        amount, product = model.evaluate(
            20, {"credit_difficulty": 1.02, "attacker_rank": 0.85})
        self.assertAlmostEqual(product, 17.34, places=6)
        self.assertEqual(amount, 17)
        amount, _ = model.evaluate(20, {"credit_difficulty": 1.02})
        self.assertEqual(amount, 20)

    def test_kakeru_checkpoints(self):
        steps = (
            ({"credit_difficulty": 1.02, "attacker_rank": 0.85}, 17.34),
            ({"credit_difficulty": 1.02, "attacker_rank": 0.85,
              "defender_class_row": 1.13}, 19.5942),
            ({"credit_difficulty": 1.02, "attacker_rank": 0.85,
              "defender_class_row": 1.13, "receiver_rank": 1.15},
             22.533329),
        )
        for factors, expected in steps:
            _, product = model.evaluate(20, factors)
            self.assertAlmostEqual(product, expected, places=5)

    def test_kakeru_observation(self):
        amount, product = model.evaluate(
            20,
            {
                "credit_difficulty": 1.02,
                "attacker_rank": 0.85,
                "matchup": 1.0,
                "defender_class_row": 1.13,
                "receiver_rank": 1.15,
                "tier": 1.0,
                "relative": 1.0,
            },
        )
        self.assertAlmostEqual(product, 22.533329, places=5)
        self.assertEqual(amount, 22)

    def test_mana_observation(self):
        amount, product = model.evaluate(
            5,
            {
                "credit_difficulty": 0.92,
                "attacker_rank": 0.85,
                "attacker_conditional_half": 0.5,
                "defender_class_row": 1.4,
                "receiver_rank": 1.15,
            },
        )
        self.assertAlmostEqual(product, 3.14755, places=5)
        self.assertEqual(amount, 3)

    def test_x_tier_chain(self):
        base = {
            "credit_difficulty": 1.02,
            "attacker_rank": 0.85,
            "defender_class_row": 1.13,
            "receiver_rank": 1.15,
        }
        for tier, expected in ((1.0, 33), (0.8, 27), (0.5, 16)):
            amount, _ = model.evaluate(30, dict(base, tier=tier))
            self.assertEqual(amount, expected)

    def test_missing_factors_are_neutral(self):
        amount, product = model.evaluate(7, {})
        self.assertEqual(amount, 7)
        self.assertAlmostEqual(product, 7.0)

    def test_one_hp_floor_is_after_the_zero_exits(self):
        amount, _ = model.evaluate(20, {"credit_difficulty": 0.5,
                                        "attacker_rank": 0.5})
        self.assertEqual(amount, 5)
        self.assertEqual(model.evaluate(1, {"credit_difficulty": 0.5}), (1, 0.5))
        self.assertEqual(model.evaluate(0, {"receiver_rank": 1.15}), (0, 0.0))
        self.assertEqual(model.evaluate(20, {}, suppressed=True), (0, 0.0))

    def test_condition_axes_and_finalizer(self):
        by_name = {s["name"]: s for s in model.STAGES}
        self.assertIn("Settings+50 is 0 or 1", by_name["attacker_conditional_half"]["condition"])
        self.assertIn("Settings+50 is 0 or 1", by_name["defender_conditional_half"]["condition"])
        self.assertIn("!= 0", by_name["defender_1504_half"]["condition"])
        self.assertIn("actual recipient+131", by_name["scale_divisor_40"]["condition"])
        path = Path(__file__).resolve().parents[2] / "docs/api-map/damage-amount-model.json"
        finalizer = json.loads(path.read_bytes())["finalizer"]
        self.assertEqual([r["return_site"] for r in finalizer["early_zero"]], ["8003CDB4", "8003CDEC"])
        self.assertEqual((finalizer["minimum"], finalizer["truncate_toward_zero"]), (1, "8003D310"))

    def test_runtime_mirrors(self):
        artifact = (Path(__file__).resolve().parents[2]
                    / "docs/api-map/damage-amount-model.json")
        mirrors = json.loads(artifact.read_text())["runtime_mirrors"]
        self.assertEqual(mirrors["borg_hp"]["array"], "803B069C")
        self.assertEqual(mirrors["borg_hp"]["source"],
                         "actor+454 (current HP)")
        self.assertEqual(mirrors["team_force"]["array"], "803B068C")
        self.assertIn("+0x118", mirrors["team_force"]["source"])

    def test_division_stage(self):
        amount, product = model.evaluate(40, {"scale_divisor_40": 40.0})
        self.assertAlmostEqual(product, 1.0)
        self.assertEqual(amount, 1)


@unittest.skipUnless(DOL.is_file(), "original DOL not available")
class ModelIndexTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from tools.extract.battle_initial_target import SOURCE_SHA256
        from tools.extract.source_image import DolImage, ReadOnlyFile

        with ReadOnlyFile(DOL) as source:
            cls.image = DolImage(source.read(0, source.size), SOURCE_SHA256)

    def test_observed_models(self):
        self.assertEqual(model.model_index(self.image, 0x0615), 4)
        self.assertEqual(model.model_index(self.image, 0x000A), 0)

    def test_observed_matchup_cell(self):
        self.assertAlmostEqual(
            model.matchup_cell(self.image, 0x0615, 0x000A), 1.0)


if __name__ == "__main__":
    unittest.main()
