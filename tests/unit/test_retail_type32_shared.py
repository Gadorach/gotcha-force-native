"""Hold the copied aim, live emission gate, and segment samples apart."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type32_shared as shared


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type32-shared.json"


class RetailType32SharedTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_moving_copy_and_outside_gate(self):
        self.assertEqual(len(self.data["cases"]), 16)
        for pilot in shared.PILOTS:
            ident = f"gg4e08:{pilot.model:04x}"
            none, moves, outside, half = [r for r in self.data["cases"] if r["borg"] == ident]
            self.assertEqual(outside["first_request"], none["first_request"])
            self.assertEqual(outside["second_request"], none["second_request"])
            self.assertEqual(moves["copied_target_changes_direction"], pilot.aim)
            if pilot.aim:
                self.assertGreater(moves["first_request"]["far"][0], 100)
                self.assertLess(moves["second_request"]["far"][0], 100)
            else:
                self.assertEqual(moves["first_request"], none["first_request"])
                self.assertEqual(moves["second_request"], none["second_request"])
            self.assertTrue(all(r["empty_scratch_request"] == (pilot.key != 1) for r in (none, moves, outside, half)))

    def test_active_clock_and_child_reads_are_separate_from_second_allocation(self):
        for row in self.data["cases"]:
            half = row["case"] == "half-rate"
            self.assertEqual(row["emission_active_visits"], [1, 3] if half else [1, 2])
            self.assertEqual(row["gate_closed_on_active_visit"], 4 if half else 3)
            self.assertFalse(row["second_request"]["carrier_available"])
            self.assertEqual(row["child"]["first_sample"], row["first_request"]["near"])
            self.assertEqual(row["child"]["next_sample"], row["second_request"]["far"])
            self.assertNotEqual(row["child"]["next_sample"], row["first_request"]["far"])

    def test_report_refuses_unqualified_aim_freezing_or_second_sample(self):
        def line(row):
            ident = int(row["borg"][-4:], 16)
            head = (ident, row["carrier_key"], shared.CASES.index(row["case"]), row["child"]["group"], 2,
                    int(row["empty_scratch_request"]))
            points = [row[request][side] for request in ("first_request", "second_request") for side in ("near", "far")]
            return "type32-shared: " + " ".join(map(str, head)) + " " + " ".join(f"{x:.5f}" for p in points for x in p)

        report = "\n".join(map(line, self.data["cases"]))
        self.assertEqual(shared.decode_report(report), self.data["cases"])
        for old, new in (("260 1 1 10 2 0", "260 1 1 10 3 0"),
                         ("-796.56964", "796.56964"),
                         ("9716.00000", "6694.00049")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                shared.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                shared.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
