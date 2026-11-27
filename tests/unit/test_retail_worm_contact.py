"""Check Worm refund ownership and the distinct zero- and positive-power contacts."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_worm_contact as worm


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-worm-contact.json"
REPORT = """\
worm-release: 1281 49 0 0 2 2 0 0 00000000 0 0 0 180
worm-release: 1281 49 0 1 1 2 2 0 00000000 0 0 0 180
worm-release: 1281 49 0 2 1 1 1 1 00000000 0 0 0 180
worm-contact: 1281 2 previous=65535 ledger=0/0 cursor=0 radius=15.00000
worm-release: 1281 49 0 3 1 1 1 1 8032BA58 16 1 0 180
worm-contact: 1281 3 previous=180 ledger=0/0 cursor=1 radius=15.00000
worm-rebind: 1281 1 1 1 1 10 1 0 1 1
worm-rebind: 1281 2 1 2 0 10 1 1 2 1
worm-followup: 1281 1 1 2 2 1 179 1 1 1
worm-release: 1291 94 0 0 1 1 0 0 00000000 0 0 0 180
worm-release: 1291 94 0 1 1 1 2 0 00000000 0 0 0 180
worm-release: 1291 94 0 2 1 1 1 1 00000000 0 0 0 180
worm-contact: 1291 2 previous=65535 ledger=0/0 cursor=0 radius=100.00000
worm-release: 1291 94 0 3 1 1 1 1 8038D058 16 1 26 154
worm-contact: 1291 3 previous=180 ledger=26/26 cursor=1 radius=100.00000
worm-release: 1293 49 1 0 2 2 0 0 00000000 0 0 0 180
worm-release: 1293 49 1 1 1 2 2 0 00000000 0 0 0 180
worm-release: 1293 49 1 2 1 1 1 1 00000000 0 0 0 180
worm-contact: 1293 2 previous=65535 ledger=0/0 cursor=0 radius=7.00000
worm-release: 1293 49 1 3 1 1 1 1 8032BB18 16 1 0 180
worm-contact: 1293 3 previous=180 ledger=0/0 cursor=1 radius=7.00000
worm-rebind: 1293 1 1 1 1 10 1 0 1 1
worm-rebind: 1293 2 1 2 0 10 1 1 2 1
worm-followup: 1293 1 1 2 2 1 179 1 1 1
worm-release: 1303 94 2 0 1 1 0 0 00000000 0 0 0 180
worm-release: 1303 94 2 1 1 1 2 0 00000000 0 0 0 180
worm-release: 1303 94 2 2 1 1 1 1 00000000 0 0 0 180
worm-contact: 1303 2 previous=65535 ledger=0/0 cursor=0 radius=50.00000
worm-release: 1303 94 2 3 1 1 1 1 8038D118 16 1 13 167
worm-contact: 1303 3 previous=180 ledger=13/13 cursor=1 radius=50.00000
"""


class RetailWormContactTest(unittest.TestCase):
    def test_source_opening_and_followup_remain_separate(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], worm.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], worm.AFS_SHA256)
        pilots = data["pilots"]
        self.assertEqual([{k: v for k, v in p.items() if k != "source"} for p in pilots], worm.decode_report(REPORT))
        self.assertEqual([p["source"]["opening"]["raw_power"] for p in pilots], [0, 30, 0, 15])
        self.assertEqual([p["cases"][3]["receiver_hp_after_opening"] for p in pilots], [180, 154, 180, 167])
        for pilot in pilots:
            missing_carrier, missing_shape, miss, hit = pilot["cases"]
            self.assertFalse(miss["hp_writer_reached"])
            self.assertTrue(hit["hp_writer_reached"])
            if pilot["family"] == 49:
                self.assertEqual((missing_carrier["stock_after_release"], missing_shape["stock_after_bind"]), (2, 2))
                self.assertEqual(pilot["source"]["followup"]["group"], 7)
                self.assertEqual(pilot["followup"]["receiver_tokens"], [1, 2])
                self.assertEqual(pilot["followup"]["receiver_hp"], 179)
            else:
                self.assertEqual((missing_carrier["stock_after_release"], missing_shape["stock_after_bind"]), (1, 1))
                self.assertNotIn("followup", pilot)

    def test_parser_refuses_conflated_refunds_power_or_tokens(self):
        for altered in (
            "",
            REPORT * 2,
            REPORT.replace("49 0 1 1 2 2 0", "49 0 1 1 1 2 0"),
            REPORT.replace("94 0 1 1 1 2 0", "94 0 1 1 2 2 0"),
            REPORT.replace("8032BA58 16 1 0 180", "8032BA58 16 1 1 179"),
            REPORT.replace("1281 1 1 2 2 1 179", "1281 1 1 1 2 1 179"),
            REPORT.replace("1293 2 1 2 0 10 1 1 2 1", "1293 2 1 2 0 10 1 1 1 1"),
        ):
            with self.subTest(altered=altered), self.assertRaises(ValueError):
                worm.decode_report(altered)

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "existing.json"
            path.write_text("preserved")
            with self.assertRaises(FileExistsError):
                worm.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "preserved")
