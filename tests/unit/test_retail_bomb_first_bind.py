"""Distinguish release, shape priority, and attack registration orders."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_bomb_first_bind as bind


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-bomb-first-bind.json"
REPORT = """\
bomb-bind: 257 4 4 0 4 7698 0 0 15 8 1 0 0 3
bomb-bind: 257 4 4 1 4 7698 1 7 15 8 1 1 7 0
bomb-bind: 257 4 4 2 4 7698 2 76 15 8 1 2 67 0
bomb-bind: 257 4 4 4 4 7698 4 7698 15 8 1 4 8967 0
bomb-bind: 257 2 4 1 2 98 1 9 15 8 1 1 9 0
bomb-bind: 257 4 2 1 2 98 1 9 15 8 1 1 9 0
bomb-bind: 10 1 1 0 1 3 0 0 1 1 1 0 0 3
bomb-bind: 10 1 1 1 1 3 1 3 1 1 1 1 3 0
"""


class RetailBombFirstBindTest(unittest.TestCase):
    def test_receipt_preserves_shape_priority_separately_from_registry_order(self):
        receipt = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(receipt["source_dol_sha256"], bind.SOURCE_SHA256)
        self.assertEqual(receipt["source_afs_sha256"], bind.AFS_SHA256)
        self.assertEqual(receipt["cases"], bind.decode_report(REPORT))
        empty, one, two, full, carrier_short, descriptor_short, sasuke_empty, _ = receipt["cases"]
        self.assertEqual(two["bound_in_visit_order"], [7, 6])
        self.assertEqual(two["attack_registry_order"], [6, 7])
        self.assertEqual(full["attack_registry_order"], [8, 9, 6, 7])
        self.assertEqual(one["refused_in_visit_order"], [6, 9, 8])
        for row in (carrier_short, descriptor_short):
            self.assertEqual(row["published_in_request_order"], [8, 9])
            self.assertEqual(row["bound_in_visit_order"], [9])
        for row in (empty, sasuke_empty):
            self.assertEqual(row["refused_carriers_control"], {
                "new_free_shapes_before_visit_two": 1, "bind_attempts_after_visit_one": 0,
                "phase_on_visit_two": 3, "carrier_and_descriptor_return_visit": 3,
            })
        self.assertEqual([row["raw_power"] for row in receipt["normal_opening_records"]], [20, 10])

    def test_refuses_wrong_priority_wrong_registry_or_failed_disposal(self):
        for report in (
            REPORT+REPORT,
            REPORT.replace("4 7698 2 76", "4 7698 2 89"),
            REPORT.replace("2 67 0", "2 76 0"),
            REPORT.replace("1 0 0 3\n", "1 0 0 2\n"),
            REPORT.replace("15 8 1 4", "15 8 0 4"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                bind.decode_report(report)

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as temp:
            out = Path(temp) / "receipt.json"
            out.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                bind.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "unchanged")
