"""Keep bomb request order, list order and debit ownership distinct."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_bomb_release as bombs


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-bomb-release.json"
REPORT = """\
bomb: 257 3 0 1 4 4 4 8967 4 8967 1 0 1
bomb: 257 3 0 0 4 4 4 8967 4 8967 0 0 1
bomb: 257 3 0 1 2 4 4 8967 2 89 1 0 1
bomb: 257 3 0 1 4 2 4 8967 2 89 1 0 1
bomb: 257 3 0 1 0 4 4 8967 0 0 1 0 1
bomb: 257 3 0 1 4 0 4 8967 0 0 1 0 1
bomb: 257 2 0 1 4 4 0 0 0 0 0 120 2
bomb: 257 2 0 0 4 4 0 0 0 0 0 0 3
bomb: 0 2 0 1 1 1 1 0 1 0 0 300 0
bomb: 0 2 0 0 1 1 0 0 0 0 0 0 0
bomb: 3 2 0 1 2 2 1 1 1 1 0 300 0
bomb: 3 2 1 1 2 1 2 12 1 1 0 300 0
bomb: 3 2 1 1 2 2 2 12 2 12 0 300 0
bomb: 10 2 0 1 0 1 1 3 0 0 0 300 0
bomb: 10 2 0 2 1 1 1 3 1 3 1 0 0
bomb: 12 2 0 2 1 1 1 5 1 5 1 0 0
bomb-wrong-parent: 10 8 0 1 1
"""


class RetailBombReleaseTest(unittest.TestCase):
    def test_published_transaction_keeps_request_and_link_order_separate(self):
        receipt = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(receipt["source_dol_sha256"], bombs.SOURCE_SHA256)
        self.assertEqual(receipt["cases"], bombs.decode_report(REPORT))
        rows = {row["case"]: row for row in receipt["cases"]}
        full, short = (rows[name] for name in ("charged-full", "charged-descriptor-short"))
        self.assertEqual(full["factory_requests"], [8, 9, 6, 7])
        self.assertEqual(full["list_head_to_tail"], [7, 6, 9, 8])
        self.assertEqual(short["factory_requests"], full["factory_requests"])
        self.assertEqual(short["published_in_request_order"], [8, 9])
        self.assertEqual(short["list_head_to_tail"], [9, 8])
        self.assertEqual(short["remaining_free_carriers"], 2)
        self.assertEqual(rows["ordinary-other-request"]["factory_requests"], [])
        self.assertEqual(rows["ordinary-other-request"]["refill_clock"], 120)
        self.assertEqual(rows["double-extra-short"]["refill_clock"], 300)
        self.assertEqual(rows["double-extra-short"]["stock_after"], 0)
        self.assertFalse(receipt["wrong_parent"]["published"])

    def test_refuses_changed_call_sequence_or_conflated_stock(self):
        for report in (
            "",
            REPORT * 2,
            REPORT.replace("4 8967", "4 8976"),
            REPORT.replace("2 89 1 0 1", "2 98 1 0 1"),
            REPORT.replace("1 1 0 300 0", "1 1 1 0 0"),
            REPORT.replace("1 3 0 0 0 300 0", "1 3 0 0 1 0 0"),
            REPORT.replace("bomb-wrong-parent: 10 8 0 1 1", "bomb-wrong-parent: 10 8 1 0 0"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                bombs.decode_report(report)

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as temp:
            out = Path(temp) / "receipt.json"
            out.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                bombs.export(None, None, None, out)
            self.assertEqual(out.read_text(), "unchanged")
