"""Check that an opening bomb hit and a later allocation stay separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_bomb_contact as contact


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-bomb-contact.json"
REPORT = """\
bomb-contact: 257 0 8 0 0 00000000 0 0 180 1 598 1 0 0 0 0 1
bomb-contact: 257 1 8 0 1 00000000 0 0 180 1 598 1 0 0 0 0 1
bomb-contact: 257 2 8 16 1 8034EB10 1 17 163 2 598 0 3 1 4 0 1
bomb-contact: 257 3 8 16 1 8034EB10 1 17 163 2 598 0 3 1 0 0 1
bomb-contact: 257 4 8 16 1 8034EB10 1 17 163 2 598 0 3 1 0 0 1
bomb-contact: 10 0 3 0 0 00000000 0 0 180 1 598 1 0 0 0 0 1
bomb-contact: 10 1 3 0 1 00000000 0 0 180 1 598 1 0 0 0 0 1
bomb-contact: 10 2 3 16 1 802D4178 1 8 172 2 598 0 3 1 5 0 1
bomb-contact: 10 3 3 16 1 802D4178 1 8 172 2 598 0 3 1 0 0 1
bomb-contact: 10 4 3 16 1 802D4178 1 8 172 2 598 0 3 1 0 0 1
"""


class RetailBombContactTest(unittest.TestCase):
    def test_receipt_keeps_child_type_cost_and_refusals_separate_from_hp(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], contact.first_bind.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], contact.first_bind.AFS_SHA256)
        self.assertEqual(data["receiver"]["body_step"], 9)
        self.assertEqual(data["damage_inlets"]["actor_banks"], [0, 0])
        for pilot, expected, (cls, kind, bucket, cost) in zip(
            data["pilots"], contact.decode_report(REPORT), ((2, 4, 25, 1), (3, 5, 9, 2)), strict=True,
        ):
            self.assertEqual(pilot["cases"], expected["cases"])
            self.assertEqual(pilot["key"], expected["key"])
            miss, duplicate, published, empty, short = pilot["cases"]
            self.assertEqual((miss["receiver_token"], duplicate["receiver_token"]), (0, 1))
            self.assertEqual(duplicate["receiver_token_cursor"], 0)
            for row in (miss, duplicate):
                self.assertFalse(row["hp_writer_reached"])
                self.assertFalse(row["followup"]["child_request"])
                self.assertEqual(row["followup"]["remaining_registered_attack_count"], 1)
            self.assertEqual(published["followup"]["child_published"], {
                "class": cls, "type": kind, "key": 0, "active_bucket": bucket,
                "descriptor_cells": cost, "bound_shapes": 0,
            })
            self.assertEqual(empty["followup"]["declared_free_child_cells"], 0)
            self.assertEqual(short["followup"]["declared_free_child_descriptors"], cost-1)
            for row in (published, empty, short):
                self.assertTrue(row["hp_writer_reached"])
                self.assertEqual(row["receiver_hp"], published["receiver_hp"])
                self.assertTrue(row["followup"]["child_request"])
                self.assertEqual(row["parent_carrier_and_descriptor_return_followup_visit"], 3)
            for row in (empty, short):
                self.assertIsNone(row["followup"]["child_published"])

    def test_refuses_wrong_amount_record_spurious_child_or_hypothetical_refund(self):
        for report in (
            REPORT[:-1]+REPORT,
            REPORT.replace("8034EB10", "802D4178"),
            REPORT.replace(" 0 3 1 4 0 1\n", " 0 3 1 5 0 1\n"),
            REPORT.replace(" 0 3 1 0 0 1\n", " 0 3 1 4 0 1\n"),
            REPORT.replace(" 0 3 1 5 0 1\n", " 0 3 1 5 0 2\n"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                contact.decode_report(report)

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as temp:
            out = Path(temp) / "receipt.json"
            out.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                contact.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "unchanged")
