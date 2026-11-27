"""Distinguish carrier, shape allocation, and timed registry order."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type89_opening as opening


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type89-opening.json"


class RetailType89OpeningTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_scarce_carrier_and_scarce_shape_choose_different_keys(self):
        self.assertEqual(len(self.data["cases"]), 20)
        self.assertEqual(self.data["opening_source"], {"group": 3, "descriptor": 3, "reaction_index": 3, "raw_radius": 30})
        for model in ("0503", "050f"):
            cases = {row["case"]: row for row in self.data["cases"] if row["borg"] == f"gg4e08:{model}"}
            first = cases["one-shape"]
            self.assertEqual(first["request_order"], [9, 10])
            self.assertEqual(first["carrier_allocation_order"], [9, 10])
            self.assertEqual(first["bind_attempt_order"], [10, 9])
            self.assertEqual(first["bound"], [{"key": 10, "opening_sphere_center": [1200, 600, 300]}])
            self.assertEqual(first["attack_registry"], {"bucket_visit": 32, "keys": [10]})
            self.assertEqual(cases["two-shapes"]["attack_registry"], {"bucket_visit": 32, "keys": [9, 10]})
            for name in ("one-carrier", "one-descriptor"):
                self.assertEqual(cases[name]["bind_attempt_order"], [9])
                self.assertEqual(cases[name]["bound"], [{"key": 9, "opening_sphere_center": [800, 600, 300]}])

    def test_no_slot_and_unregistered_opening_are_explicit(self):
        for case in self.data["cases"]:
            self.assertEqual(case["opening_register_count"], 0)
            if case["case"] == "no-shapes":
                self.assertFalse(case["bound"])
                self.assertIsNone(case["attack_registry"])
                self.assertIsNone(case["shape_slots_returned_at_bucket_visit"])
            else:
                self.assertEqual(case["shape_slots_returned_at_bucket_visit"], 33)
                self.assertEqual(case["attack_registry"]["bucket_visit"], 32)

    def test_rejects_request_order_as_bind_order_or_opening_registration(self):
        def line(i, row):
            registry = row["attack_registry"] or {"bucket_visit": 0, "keys": []}
            values = (int(row["borg"][-4:], 16), i % len(opening.CASES),
                      len(row["request_order"]), len(row["carrier_allocation_order"]),
                      *(row["bind_attempt_order"]+[0, 0])[:2], len(row["bound"]), row["opening_register_count"],
                      registry["bucket_visit"], *(registry["keys"]+[0, 0])[:2])
            return "type89-opening: " + " ".join(map(str, values))

        report = "\n".join(line(i, row) for i, row in enumerate(self.data["cases"]))
        self.assertEqual(opening.decode_report(report), self.data["cases"])
        for old, new in (("2 2 10 9 1 0 32 10 0", "2 2 9 10 1 0 32 9 0"),
                         ("2 2 10 9 2 0 32 9 10", "2 2 10 9 2 0 32 10 9"),
                         ("1 1 3 0 1 0 32 3 0", "1 1 3 0 1 1 1 3 0")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                opening.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                opening.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
