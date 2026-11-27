"""Keep Nurse stock, alternate shape publication and signed HP independent."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_nurse_setup as nurse


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-nurse-setup.json"
REPORT = """nurse-setup: 0 1 1 0 1 120 1 4 2 -200 180
nurse-setup: 1 1 0 0 1 120 1 4 2 0 90
nurse-setup: 2 1 1 0 7 17 0 8 2 -200 180
nurse-setup: 3 1 0 0 7 17 0 8 2 0 90
nurse-setup: 4 0 0 0 1 120 1 0 2 0 90
nurse-setup: 5 0 0 0 1 120 1 0 2 0 90
nurse-setup: 6 0 0 0 7 17 0 8 2 0 90
nurse-setup: 7 0 0 0 7 17 0 8 2 0 90
"""


class RetailNurseSetupTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = json.loads(ARTIFACT.read_bytes())["cases"]

    def test_shape_refusal_does_not_cancel_debit(self):
        paid, refused = self.rows[:2]
        for row in (paid, refused):
            self.assertTrue(row["group_3_request_before_debit"])
            self.assertEqual((row["channel_2_stock_after"], row["channel_2_clock_after"]), (0, 120))
        self.assertFalse(refused["shape_admitted_before_debit"])
        self.assertIsNone(refused["signed_hp_writer_argument"])

    def test_stock_refusal_does_not_retract_shape(self):
        row = self.rows[2]
        self.assertTrue(row["shape_admitted_before_debit"])
        self.assertEqual((row["stock_latch_after"], row["cue_before_debit"], row["cue_after_debit"]), (0, 4, 8))
        self.assertTrue(row["shape_retained_after_one_followup"])
        self.assertFalse(row["published_at_supplied_frame_21"])
        self.assertTrue(row["published_at_supplied_frame_22"])
        self.assertEqual((row["signed_hp_writer_argument"], row["ally_hp_after"]), (-200, 180))

    def test_masked_shape_lane_still_allows_debit(self):
        row = self.rows[4]
        self.assertTrue(row["shape_cell_available"])
        self.assertFalse(row["group_3_request_before_debit"])
        self.assertEqual(row["stock_latch_after"], 1)
        self.assertEqual(row["ally_hp_after"], 90)

    def test_report_rejects_joined_stock_and_shape_gates(self):
        self.assertEqual(nurse.decode(REPORT), self.rows)
        for old, new in (("1 120 1 4 2 0 90", "7 17 0 4 2 0 90"),
                         ("7 17 0 8 2 -200 180", "7 17 0 8 2 0 90")):
            with self.assertRaises(ValueError):
                nurse.decode(REPORT.replace(old, new, 1))

    def test_never_replaces_output(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "nurse-setup.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                nurse.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
