"""Keep stage-wrapper operands, effect selectors and combat carriers distinct."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_wall_effect_census as wall


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-wall-effect-census.json"
REPORT = "\n".join(f"wall-effect: {i} 1 21 22 23 1.00000 2.00000 3.00000" if i < 4
                   else f"wall-effect: {i} 0 0 0 0 0.00000 0.00000 0.00000" for i in range(6))


class RetailWallEffectCensusTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_only_the_effect_helper_uses_a_selector(self):
        self.assertEqual(self.data["wrapper_argument_histogram"], {"0": 1, "1": 26, "8": 3, "255": 25})
        sites = self.data["call_sites"]
        stage = [r for r in sites if r["target"] == "80084170"]
        helper = [r for r in sites if r["target"] == "80019550"]
        self.assertEqual((len(stage), len(helper)), (55, 11))
        self.assertTrue(all(r["operand"]["kind"] == "unused-wrapper-r4" for r in stage))
        dynamic = [r for r in helper if r["operand"]["kind"] == "dynamic-contact-descriptor-byte"]
        self.assertEqual([r["site"] for r in dynamic], ["8002F87C"])
        self.assertIsNone(dynamic[0]["operand"]["selector_values"])
        zero = [r for r in helper if r["operand"]["kind"] == "literal-effect-selector"]
        self.assertEqual(len(zero), 10)
        self.assertEqual({r["operand"]["value"] for r in zero}, {0})

    def test_inherited_roster_scope_does_not_bind_unjoined_calls(self):
        self.assertEqual((self.data["counts"]["candidate_rows"], self.data["counts"]["candidate_models"]), (72, 35))
        rows = self.data["candidates"]
        chains = {r["parent_type"]: r for r in self.data["parent_dispatch_links"]}
        self.assertEqual(len(chains), 10)
        self.assertEqual(chains[14]["site"], "8006F3C8")
        self.assertEqual(chains[89]["dispatch_slots"], ["80350628", "80350634"])
        for row in rows:
            self.assertEqual(row["callback_site"], chains[row["parent"]["type"]]["site"])
            self.assertEqual(row["effect_selector"], 0)
            self.assertNotIn("hp_loss", row)
        conditional = next(r for r in rows if r["parent"].get("additional_guard"))
        self.assertEqual(conditional["parent"]["additional_guard"]["required"], "nonzero")

    def test_selector_zero_never_publishes_an_attack_and_returns_both_cells(self):
        shape = self.data["selector_zero"]
        self.assertEqual((shape["class"], shape["bucket"], shape["visual_record"], shape["descriptor_cells"]), (2, 31, 21, 1))
        rows = self.data["cases"]
        self.assertEqual(rows, wall.decode_report(REPORT))
        for row in rows:
            self.assertEqual(row["attack_registry_entries"], 0)
            if row["allocated"]:
                self.assertEqual(row["return_visit"], 23)
                self.assertEqual(row["scale_samples_at_visits_1_5_21"], [1, 2, 3])
            else:
                self.assertIsNone(row["return_visit"])

    def test_report_rejects_early_return_or_refused_allocation(self):
        for old, new in (("1 1 21 22 23", "1 1 20 21 22"), ("4 0 0 0 0", "4 1 21 22 23")):
            self.assertIn(old, REPORT)
            with self.assertRaises(ValueError):
                wall.decode_report(REPORT.replace(old, new, 1))

    def test_existing_output_is_preserved(self):
        with TemporaryDirectory() as tmp:
            target = Path(tmp) / "receipt.json"
            target.write_text("keep")
            with self.assertRaises(FileExistsError):
                wall.export(None, None, None, None, None, None, target)
            self.assertEqual(target.read_text(), "keep")
