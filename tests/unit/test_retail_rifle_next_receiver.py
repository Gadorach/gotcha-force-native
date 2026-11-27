"""Verify second-recipient damage without changing the original rifle token."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_rifle_next_receiver as rifle


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-rifle-next-receiver.json"
PREFIX = "\n".join((rifle.RELEASE, rifle.PUBLISHED, rifle.HIT)) + "\n"
REPORT = "".join(PREFIX + line + "\n" for line in (
    "rifle-next: 256 0 1 0 0 172 180 65535 0 8 8 0 1 299 445.00000 445.00000",
    "rifle-next: 0 1 1 0 0 172 180 65535 0 8 8 0 1 299 445.00000 445.00000",
    "rifle-next: 0 0 1 1 8 172 172 180 1 16 8 8 1 299 445.00000 423.00000",
))


class RetailRifleNextReceiverTest(unittest.TestCase):
    def test_receipt_separates_history_and_damage_by_receiver(self):
        receipt = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(receipt["source_dol_sha256"], rifle.SOURCE_SHA256)
        self.assertEqual(receipt["source_afs_sha256"], rifle.AFS_SHA256)
        self.assertEqual(receipt["cases"], rifle.decode_report(REPORT))
        self.assertEqual(receipt["declared"]["teams"], [0, 1, 1])
        self.assertFalse(receipt["first_receiver_control"]["same_geometry_and_token_reaches_amount"])
        self.assertTrue(receipt["first_receiver_control"]["only_history_word_cleared_reaches_amount"])
        self.assertFalse(receipt["first_receiver_control"]["control_writes_hp"])
        miss, duplicate, hit = receipt["cases"]
        self.assertEqual([r["same_carrier_token"] for r in receipt["cases"]], [1, 1, 1])
        self.assertEqual([r["first_hp"] for r in receipt["cases"]], [172, 172, 172])
        self.assertFalse(miss["second_hp_writer_reached"])
        self.assertFalse(duplicate["second_hp_writer_reached"])
        self.assertEqual((hit["second_hp"], hit["credit_ledger"], hit["stock"]), (172, 16, 1))
        self.assertEqual((hit["carrier_z_after_flight"], hit["carrier_z_at_amount_cut_or_return"]), (445, 423))

    def test_rejects_changed_contact_position_or_double_credit(self):
        for report in (
            "",
            REPORT + REPORT,
            REPORT.replace("0 0 1 1 8 172 172", "0 0 2 1 8 172 172"),
            REPORT.replace("8 172 172 180 1 16 8 8", "8 164 172 180 1 24 16 8"),
            REPORT.replace("445.00000 423.00000", "445.00000 445.00000"),
            REPORT.replace("0 1 1 0 0 172 180", "0 1 1 1 8 172 172"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                rifle.decode_report(report)

    def test_refuses_to_overwrite_output(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "receipt.json"
            path.write_text("preserved")
            with self.assertRaises(FileExistsError):
                rifle.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "preserved")
