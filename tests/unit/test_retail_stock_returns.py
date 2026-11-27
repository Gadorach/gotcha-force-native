"""Separate stock writer census, actor callbacks and observed carrier exits."""
from collections import Counter
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stock_returns as returns


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-stock-returns.json"


class RetailStockReturnTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.wire = {r["case"]: r for r in cls.data["type43_cases"]}

    def test_dynamic_type41_selector_stays_separate_from_literal_callers(self):
        self.assertEqual(Counter(r["literal_r4"] for r in self.data["indexed_return_calls"]),
                         {0: 17, 2: 36, None: 1})
        self.assertEqual({r["key"]: r["signed_return_selector"] for r in self.data["type41_derived_selector_scope"]},
                         {0: -1, 1: -1, 2: 0, 3: 0})

    def test_owner_callbacks_return_without_a_carrier_and_only_once(self):
        rows = self.data["actor_callback_cases"]
        self.assertEqual(len(rows), 13)
        for r in rows:
            self.assertEqual(r["channel"], 0 if r["borg"] == "gg4e08:0403" else 2)
            self.assertEqual(r["writer_calls_across_two_visits"], int(r["case"] == "depleted-open"))
            self.assertEqual(r["declared_clock_preserved"], 17)

    def test_wire_resource_refusals_are_separate_boundaries(self):
        for reason in ("carrier-cell-refused", "descriptor-refused"):
            r = self.wire[reason]
            self.assertTrue(r["initial_debit_succeeded"])
            self.assertFalse(r["carrier_allocated"])
            self.assertEqual((r["return_site"], r["stock_after_release"]), ("800DA0AC", 1))
        r = self.wire["opening-shape-refused"]
        self.assertTrue(r["carrier_allocated"])
        self.assertFalse(r["shape_published"])
        self.assertEqual((r["return_site"], r["stock_after_release"], r["stock_after_case"]), ("800DA274", 0, 1))

    def test_wire_retirement_does_not_imply_a_return(self):
        outside = self.wire["outside-stage-bounds"]
        self.assertEqual((outside["return_site"], outside["stock_after_case"]), ("800DA4CC", 1))
        for reason in ("owner-phase-invalid", "owner-model-changed"):
            r = self.wire[reason]
            self.assertTrue(r["carrier_and_descriptor_returned"])
            self.assertEqual(r["shape_rebuild_visits_to_return"], 1)
            self.assertIsNone(r["return_site"])
            self.assertEqual(r["stock_after_case"], 0)

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaises(ValueError):
            returns.decode("stock-wire-return: 0")
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "returns.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                returns.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
