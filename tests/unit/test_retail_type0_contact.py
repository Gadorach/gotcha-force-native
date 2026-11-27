"""Keep genuine type-0 contact distinct from an injected stop byte."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type0_contact as contact


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type0-contact.json"
REPORT = """\
type0-contact: 2048 12 256 0 0 0 0 00000000 0 180 1 0 299 1 0 0 -40.00000 0.00000
type0-contact: 2048 12 0 1 0 1 0 00000000 0 180 1 0 299 1 0 0 -40.00000 0.00000
type0-contact: 2048 12 0 0 16 1 1 802D6270 17 163 2 1 299 0 1 3 0.00000 0.00000
type0-contact: 2049 125 256 0 0 0 0 00000000 0 180 1 0 299 1 0 0 -40.00000 0.00000
type0-contact: 2049 125 0 1 0 1 0 00000000 0 180 1 0 299 1 0 0 -40.00000 0.00000
type0-contact: 2049 125 0 0 16 1 1 802F7C30 17 163 2 1 299 0 1 3 0.00000 0.00000
type0-contact: 2050 45 256 0 0 0 0 00000000 0 180 1 0 299 1 0 0 0.00000 20.00000
type0-contact: 2050 45 0 1 0 1 0 00000000 0 180 1 0 299 1 0 0 0.00000 20.00000
type0-contact: 2050 45 0 0 16 1 1 80339E18 17 163 2 1 299 0 1 3 0.00000 0.00000
"""


class RetailType0ContactTest(unittest.TestCase):
    def test_source_receipt_qualifies_three_distinct_records(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], contact.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], contact.AFS_SHA256)
        self.assertEqual(data["receiver"]["body_step"], 9)
        pilots = data["pilots"]
        self.assertEqual([{k: v for k, v in p.items() if k != "source"} for p in pilots], contact.decode_report(REPORT))
        self.assertEqual([p["source"]["descriptor"] for p in pilots], [2, 21, 0])
        self.assertTrue(all(p["source"]["raw_power"] == 20 for p in pilots))
        for pilot in pilots:
            miss, duplicate, hit = pilot["cases"]
            self.assertEqual([c["contact_flags_after_pair"] for c in pilot["cases"]], [0, 0, 16])
            self.assertEqual(duplicate["first_receiver_token"], 1)
            self.assertEqual(duplicate["receiver_token_cursor"], 0)
            self.assertEqual(miss["first_followup"]["attack_count"], 1)
            self.assertEqual(hit["first_followup"]["shape_free_count"], 1)
            self.assertEqual(hit["receiver_hp"], 163)
            self.assertEqual(hit["carrier_and_descriptor_return_visit"], 3)

    def test_parser_refuses_incorrect_damage_contact_or_cleanup(self):
        for report in (
            "",
            REPORT + REPORT,
            REPORT.replace("16 1 1 802D6270", "0 1 1 802D6270"),
            REPORT.replace("802F7C30 17 163", "802D6270 17 163"),
            REPORT.replace("80339E18 17 163", "80339E18 20 160"),
            REPORT.replace("2 1 299 0 1 3", "2 1 299 0 1 2"),
            REPORT.replace("0 1 0 00000000", "0 1 1 00000000"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                contact.decode_report(report)

    def test_refuses_to_overwrite_a_receipt(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "existing.json"
            path.write_text("preserved")
            with self.assertRaises(FileExistsError):
                contact.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "preserved")
