"""Keep Ultimate Cannon's raw power, requested amount, and removed HP distinct."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type71_contact as cannon


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type71-contact.json"
REPORT = """\
type71-contact: 0 1 1 1 4307 0 4307 4307 1 16 133.33334 1033.63867 3362.01221
type71-contact: 1 1 1 1 0 180 0 0 0 0 133.33334 1033.63867 3362.01221
type71-contact: 2 1 1 2 4307 0 4307 4307 1 16 133.33334 1033.63867 3362.01221
type71-contact: 3 0 0 0 0 0 0 0 0 0 0.00000 0.00000 0.00000
type71-contact: 4 1 0 0 0 0 0 0 0 0 0.00000 0.00000 0.00000
"""


class RetailType71ContactTest(unittest.TestCase):
    def test_original_writer_and_both_ledgers_keep_the_calculated_amount(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["cases"], cannon.decode_report(REPORT))
        self.assertEqual((data["source"]["raw_power"], data["source"]["raw_radius"]), (5000, 2000))
        hit, miss, closing, _, _ = data["cases"]
        self.assertEqual(hit["receiver"], {
            "hp_before": 180, "hp_after": 0, "hp_loss": 180, "hp_writer_amount": 4307,
            "credited_damage_ledger_delta": 4307, "received_damage_ledger_delta": 4307,
            "segment_token": 1, "duplicate_token_refused": True,
        })
        self.assertEqual(closing["receiver"], hit["receiver"])
        self.assertEqual(closing["parent_phase_at_second_request"], 2)
        self.assertTrue(closing["second_segment_requested"])
        self.assertEqual(miss["receiver"]["hp_after"], 180)
        self.assertIsNone(miss["receiver"]["hp_writer_amount"])
        self.assertEqual(miss["receiver"]["credited_damage_ledger_delta"], 0)

    def test_nonadmission_does_not_claim_an_hp_result(self):
        no_carrier, no_shape = cannon.decode_report(REPORT)[3:]
        self.assertFalse(no_carrier["segment_created"])
        self.assertTrue(no_shape["segment_created"])
        for row in (no_carrier, no_shape):
            self.assertTrue(row["first_segment_requested"])
            self.assertFalse(row["shape_bound"])
            self.assertIsNone(row["receiver"])
            self.assertIsNone(row["parent_phase_at_second_request"])

    def test_rejects_raw_power_as_damage_or_hp_loss_as_ledger(self):
        for old, new in (("4307 0 4307 4307", "5000 0 5000 5000"),
                         ("4307 0 4307 4307", "4307 0 180 180"),
                         ("2 1 1 2 4307", "2 1 1 1 4307"),
                         ("4 1 0 0 0", "4 1 1 0 0")):
            self.assertIn(old, REPORT)
            with self.assertRaises(ValueError):
                cannon.decode_report(REPORT.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                cannon.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
