"""Keep the timed stage latch distinct from the next-visit area allocation."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type89_timed as timed


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type89-timed.json"
REPORT = """\
type89-timed: 3843 0 1 25 31 32 41 0 10383 0 0
type89-timed: 3843 1 1 25 31 32 41 1 1000 32 16
type89-timed: 3843 2 1 25 31 32 41 0 10383 0 0
type89-timed: 3843 0 2 50 62 63 82 0 10383 0 0
type89-timed: 3843 1 2 50 62 63 82 1 1000 63 16
type89-timed: 3843 2 2 50 62 63 82 0 10383 0 0
type89-timed: 1578 0 1 25 31 32 41 0 10343 0 0
type89-timed: 1578 1 1 25 31 32 41 1 1000 32 16
type89-timed: 1578 2 1 25 31 32 41 0 10343 0 0
type89-timed: 1578 0 2 50 62 63 82 0 10343 0 0
type89-timed: 1578 1 2 50 62 63 82 1 1000 63 16
type89-timed: 1578 2 2 50 62 63 82 0 10343 0 0
"""


class RetailType89TimedTest(unittest.TestCase):
    def test_only_intersecting_stage_can_produce_area_in_timed_replay(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["pilots"], timed.decode_report(REPORT))
        self.assertEqual(data["source_dol_sha256"], timed.area.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], timed.area.AFS_SHA256)
        self.assertEqual(data["schedule"]["duration_units"], [25, 6, 10])
        self.assertEqual(data["schedule"]["stage_latch_offset"], "0x17C")
        for pilot in data["pilots"]:
            for row in pilot["cases"]:
                self.assertEqual(row["exit_leaf_visit"] - row["stage_probe_visit"], 1)
                allocation = row["area_allocation"]
                if row["stage"] == "intersecting-wall":
                    self.assertTrue(row["stage_latch"])
                    self.assertEqual(allocation, {"visit": row["exit_leaf_visit"], "key": 16,
                                                  "site": "8016D32C", "root": [100, 200, 1000]})
                else:
                    self.assertFalse(row["stage_latch"])
                    self.assertIsNone(allocation)

    def test_refuses_same_visit_allocation_or_false_stage_latch(self):
        for report in (REPORT + REPORT[:100],
                       REPORT.replace("1 1000 32 16", "1 1000 31 16"),
                       REPORT.replace("0 10343 0 0", "1 10343 32 16"),
                       REPORT.replace("50 62 63 82", "50 62 64 82")):
            with self.assertRaises(ValueError):
                timed.decode_report(report)

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                timed.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
