"""Separate scoped contact selector bytes from runtime effect allocation."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_effect_census as impact


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-effect-census.json"


class RetailContactEffectCensusTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_static_counts_deduplicate_records_without_losing_scope(self):
        self.assertEqual(self.data["counts"], {"actor_records": 73, "actor_references": 106, "actor_models": 86,
                                               "common_records": 22, "nonattack_steps": 2, "allocation_cases": 24})
        self.assertEqual(self.data["actor_selector_histogram"], {"1": 69, "8": 4})
        records = self.data["actor_amount_records"]
        self.assertEqual(len({r["record"] for r in records}), 73)
        self.assertTrue(any(len(s["receipts"]) > 1 for r in records for s in r["references"]))
        for row in self.data["common_amount_records"]:
            self.assertEqual(row["visual_selector_byte"], 0)
            self.assertIn(row["carrier_type"], (5, 9))
        omitted = self.data["omitted_nonattack_steps"]
        self.assertEqual({(r["borg"], r["descriptor"], r["flags"], r["raw_power"]) for r in omitted},
                         {("gg4e08:0d01", 4, 2, 10), ("gg4e08:0d04", 2, 2, 10)})

    def test_full_descriptor_admission_is_atomic(self):
        rows = self.data["cases"]
        for exact, short, no_cell, spare in zip(*(rows[i::4] for i in range(4)), strict=True):
            cost = exact["descriptor_request"]
            self.assertEqual([r["allocated"] for r in (exact, short, no_cell, spare)], [True, False, False, True])
            self.assertEqual([r["descriptors_left"] for r in (exact, short, no_cell, spare)], [0, cost-1, cost, 2])
            self.assertTrue(all(r["shape_cells_consumed"] == 0 for r in (exact, short, no_cell, spare)))
        self.assertEqual({r["selector"]: r["descriptor_request"] for r in rows}, {0: 1, 1: 1, 2: 3, 3: 3, 4: 3, 8: 8})

    def test_report_refuses_partial_or_over_budget_allocations(self):
        lines = []
        for row in self.data["cases"]:
            values = (row["selector"], impact.CASES.index(row["case"]), row["family"], row["variant"],
                      row["descriptor_request"], row["allocated"], row["descriptors_left"])
            lines.append("impact-alloc: " + " ".join(str(int(v)) for v in values))
        report = "\n".join(lines)
        self.assertEqual(impact.decode_report(report), self.data["cases"])
        for old, new in (("8 1 3 0 8 0 7", "8 1 3 0 8 1 0"), ("4 3 2 2 3 1 2", "4 3 2 2 5 1 0")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                impact.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_is_not_replaced(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "receipt.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                impact.export(None, None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
