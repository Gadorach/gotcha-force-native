"""Check the normal-level stock channel correction and its original-code receipt."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stock_startup as startup


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-stock-startup.json"


class RetailStockStartupTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.examples = {(r["borg"], r["level"]): r["channels"] for r in cls.data["examples"]}

    def test_all_normal_records_and_levels_match_channel_boundary(self):
        self.assertEqual(self.data["level_row_channels"], [0, 2])
        self.assertEqual(self.data["untouched_channel"], 1)
        totals = self.data["summary"]
        self.assertEqual((totals["models"], totals["model_level_visits"], totals["checked_channel_rows"]), (206, 2060, 4120))
        self.assertEqual((totals["neutral_size_1_0_visits"], totals["untouched_channel_1_visits"]), (2060, 2060))
        self.assertTrue(all(r["initial_size_index"] == 16 and r["checked_levels"] == 10 for r in self.data["models"]))

    def test_support_stock_uses_second_row_and_original_preset(self):
        for model, current, preset in (("0900", 1, 180), ("0902", 2, 180), ("0628", 3, 300)):
            _, channel = self.examples[f"gg4e08:{model}", 1]
            self.assertEqual((channel["channel"], channel["current"], channel["mode"], channel["preset"]),
                             (2, current, 0, preset))
            self.assertEqual(channel["initial_clock"], 0)

    def test_preserves_limit_disagreement_and_mode_specific_clock(self):
        first = self.examples["gg4e08:0c06", 10][0]
        self.assertEqual((first["current"], first["previous"], first["limit"]), (23, 22, 22))
        mode_one = self.examples["gg4e08:0705", 1][0]
        self.assertEqual((mode_one["mode"], mode_one["current"], mode_one["initial_clock"]), (1, 0, 300))
        mode_two = self.examples["gg4e08:0206", 1][1]
        self.assertEqual((mode_two["mode"], mode_two["current"], mode_two["initial_clock"]), (2, 0, 0))

    def test_refuses_malformed_receipt_and_existing_output(self):
        with self.assertRaises(ValueError):
            startup.decode("stock-start: 1", [])
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "startup.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                startup.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
