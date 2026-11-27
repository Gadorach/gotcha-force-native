"""Keep bounded source refill behavior separate from native stock admission."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stock_lifecycle as lifecycle


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-stock-lifecycle.json"


class RetailStockLifecycleTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.timelines = {r["borg"]: {s["checkpoint"]: s for s in r["cases"]} for r in cls.data["timelines"]}

    def test_all_source_rows_keep_check_debit_clock_boundaries(self):
        self.assertEqual(len(self.data["models"]), 206)
        self.assertEqual(sum(r["checked_channel_rows"] for r in self.data["models"]), 4120)
        totals = self.data["summary"]
        self.assertEqual((totals["accepted_checks_and_debits"], totals["refill_clock_changes"],
                          totals["alternate_step_changes"]), (2920, 1140, 1140))

    def test_mode_one_restores_one_then_retains_negative_overshoot(self):
        rows = self.timelines["gg4e08:0800"]
        self.assertEqual((rows[0]["current"], rows[0]["clock"]), (3, 599))
        self.assertEqual((rows[19]["current"], rows[20]["current"]), (3, 4))
        self.assertEqual((rows[40]["current"], rows[40]["clock"], rows[41]["clock"]), (5, -1, -1))
        mode_zero = self.timelines["gg4e08:0900"]
        self.assertEqual((mode_zero[179]["clock"], mode_zero[179]["current"]), (1, 0))
        self.assertEqual((mode_zero[180]["clock"], mode_zero[180]["current"]), (0, 1))

    def test_zero_limit_negative_is_not_clamped_in_original_receipt(self):
        self.assertEqual(len(self.data["negative_temporary_stock"]), 6)
        self.assertEqual(sum(len(r["levels"]) for r in self.data["negative_temporary_stock"]), 60)
        rows = self.timelines["gg4e08:0705"]
        self.assertEqual((rows[1]["current"], rows[1]["previous"]), (-1, -1))
        self.assertEqual((rows[300]["current"], rows[300]["previous"], rows[300]["clock"]), (0, 0, 0))

    def test_modes_two_and_three_ignore_timer_and_use_separate_return(self):
        for model, cap in (("0103", 4), ("010a", 1)):
            rows = self.timelines[f"gg4e08:{model}"]
            self.assertEqual((rows[0]["clock"], rows[1]["clock"]), (17, 17))
            self.assertEqual((rows[2]["clock"], rows[3]["clock"]), (0, 0))
            self.assertEqual((rows[2]["current"], rows[3]["current"], rows[4]["current"]), (cap-1, cap-1, cap))

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaises(ValueError):
            lifecycle.decode("stock-cycle: 0000", [])
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "lifecycle.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                lifecycle.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
