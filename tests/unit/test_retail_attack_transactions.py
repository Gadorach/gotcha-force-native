"""Validate metadata-only release and contact-pair receipts."""
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_attack_transactions as transactions


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-attack-transactions.json"
REPORT = """\
rifle: 2 1 1 1 1 0 1 1 9 16 2 21 0
rifle: 1 1 0 1 0 300 1 0 0 0 0 0 5
rifle: 1 1 1 0 0 300 1 0 0 0 0 0 5
rifle: 0 1 1 1 0 0 0 0 0 0 0 0 0
rifle: 2 0 1 1 2 0 0 0 0 0 0 0 0
sasuke: 1 2 1 49 26 2 2
sasuke: 0 2 0 0 0 0 0
"""


class RetailAttackTransactionsTest(unittest.TestCase):
    def test_fixture_rejects_partial_outcomes_and_impossible_publication(self):
        for altered in ("", REPORT + REPORT,
                        REPORT.replace("1 1 0 1 0 300", "1 1 0 1 1 0"),
                        REPORT.replace("sasuke: 0 2 0", "sasuke: 0 2 1"),
                        REPORT.replace("0 300", "0 300.0")):
            with self.subTest(altered=altered), self.assertRaises(ValueError):
                transactions.decode_report(altered)

    def test_published_transactions_keep_attempts_distinct_from_publication(self):
        result = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(result["source_dol_sha256"], transactions.SOURCE_SHA256)
        self.assertEqual(result["source_afs_sha256"], transactions.AFS_SHA256)
        rifle, pairs = transactions.decode_report(REPORT)
        self.assertEqual(result["machine_red"]["cases"], rifle)
        self.assertEqual(result["sasuke"]["cases"], pairs)
        self.assertEqual([row["allocation_attempts"] for row in rifle], [1, 1, 1, 0, 0])
        self.assertEqual([row["published"] for row in rifle], [True, False, False, False, False])
        self.assertEqual(pairs[1]["queue_byte_after_attempt"], 2)
        self.assertFalse(pairs[1]["published"])

    def test_sasuke_queue_pointer_and_attack_index_are_checked(self):
        raw = bytearray(transactions.HIT_BYTES)
        queue = transactions.QUEUE + 8
        step = transactions.STEPS + 2 * transactions.STEP_SIZE
        struct.pack_into(">4b", raw, queue, 2, -1, -1, -1)
        struct.pack_into(">BBHH", raw, step, 0, 7, 1, 2)
        out = transactions.sasuke_source(raw)
        self.assertEqual(out["matrix_selector"], 7)
        self.assertEqual(out["attack_table_index"], 2)
        raw[step + 5] = 1
        with self.assertRaisesRegex(ValueError, "step mismatch"):
            transactions.sasuke_source(raw)
        with self.assertRaisesRegex(ValueError, "extent"):
            transactions.sasuke_source(raw[:-1])

    def test_export_refuses_to_overwrite(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "retained.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                transactions.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
