"""Keep timed requests independent of type-89 carrier or descriptor refusal."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type89_cast as cast


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailType89CastTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads((MAPS / "gg4e08-type89-cast.json").read_bytes())
        cls.selector = json.loads((MAPS / "gg4e08-type89-roster.json").read_bytes())["pair_selector"]

    def test_partial_pool_preserves_the_request_pair_order(self):
        self.assertEqual(len(self.data["cases"]), 24)
        self.assertEqual(self.data["timed_step_clock_reset"], 10)
        for row in self.data["cases"]:
            first = row["casts"][0]
            pair = row["bank"] == 0
            requested = [9, 10] if pair else [3]
            self.assertEqual(first["visit"], 4)
            self.assertEqual(first["requested_keys"], requested)
            self.assertEqual(row["sentinel_visit"], 81 if pair else 70)
            if row["case"] == "ample":
                self.assertEqual(first["allocated_keys"], requested)
            elif row["case"] in ("one-carrier", "one-descriptor"):
                self.assertEqual(first["allocated_keys"], requested[:1])
                self.assertFalse(any(step["allocated_keys"] for step in row["casts"][1:]))
            else:
                self.assertEqual(first["allocated_keys"], [])
            if row["case"] == "refill-after-first-request":
                self.assertEqual(row["refill_at_visit"], 5)
                self.assertEqual(row["casts"][1]["visit"], 15)
                self.assertEqual(row["casts"][1]["allocated_keys"], [3] if pair else [6])

    def test_report_refuses_reordered_pairs_retries_and_timer_changes(self):
        def report_line(model, case, visit, cursor, requested, allocated):
            keys = (requested+[0, 0])[:2]
            return f"type89-cast: {model} {case} {visit} {cursor} {len(requested)} {keys[0]} {keys[1]} {len(allocated)}"

        lines = []
        for row in self.data["cases"]:
            case = next(i for i, (key, *_) in enumerate(cast.CASES) if key == row["case"])
            model = int(row["borg"][-4:], 16)
            lines.extend(report_line(model, case, r["visit"], r["cursor"], r["requested_keys"], r["allocated_keys"])
                         for r in row["casts"])
            lines.append(report_line(model, case, row["sentinel_visit"], row["cursor_at_sentinel"], [], []))
        report = "\n".join(lines)
        pilots = cast.decode_report(report, self.selector)
        self.assertEqual([{"borg": p["borg"], "bank": p["bank"], **row} for p in pilots for row in p["cases"]],
                         self.data["cases"])
        for old, new in (("0 4 0 2 9 10 2", "0 4 0 2 10 9 2"),
                         ("5 15 1 1 3 0 1", "5 15 1 1 10 0 1"),
                         ("0 15 1 1 3 0 1", "0 14 1 1 3 0 1")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                cast.decode_report(report.replace(old, new, 1), self.selector)

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                cast.export(None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
