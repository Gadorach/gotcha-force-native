"""Check the source menu-to-enrollment and ordinary-bank join."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_menu_port_input as menu


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-menu-port-input.json"
REPORT = """\
first-match: 0 256 256 0 2 1 255 1 3 2 12 4 9
held-ignored: 0 0 256 0 2 2 255 2 5 2 10 4 9
status-zero: 0 0 256 0 2 2 255 2 5 2 10 4 9
status-nonzero: 0 256 256 0 2 1 255 1 3 2 12 4 9
current-excluded: 0 0 0 0 0 255 255 0 1 1 14 4 8
held-no-edge: 0 0 0 0 0 255 255 0 1 1 14 4 8
alternate-bit: 0 0 128 0 3 2 255 2 5 2 10 4 10
edge-priority: 0 128 256 0 2 2 255 2 5 2 10 4 9
upper-bit-priority: 0 256 512 128 1 255 2 0 1 1 14 5 0
single-port: 256 64 64 64 2 255 255 0 1 1 14 4 9
single-port-alternate: 128 64 64 64 3 255 255 0 1 1 14 4 10
nonzero-current: 256 256 0 0 2 0 255 0 5 2 10 4 9
already-enrolled: 256 256 0 0 2 0 255 0 5 2 10 4 9
"""


class RetailMenuPortInputTest(unittest.TestCase):
    def test_receipt_joins_observed_menu_and_bank_outputs(self):
        result = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(result["source_dol_sha256"], menu.SOURCE_SHA256)
        self.assertEqual(result["cases"], menu.decode_report(REPORT))
        self.assertEqual(result["reviewed_enrollment_calls"], {"801901E0": 0x100, "80190250": 0x80})
        cases = {r["case"]: r for r in result["cases"]}
        self.assertEqual(cases["first-match"]["ordinary_banks"], [0, 0, 1, 1])
        self.assertEqual(cases["held-ignored"]["enrolled_port"], 2)
        self.assertEqual(cases["edge-priority"]["enrolled_port"], 2)
        self.assertEqual(cases["upper-bit-priority"]["branch_0x200_port"], 2)
        self.assertIsNone(cases["upper-bit-priority"]["enrolled_port"])
        self.assertEqual(cases["single-port"]["mode_input_mask"], 1)
        self.assertEqual(cases["already-enrolled"]["mode_input_port_count"], 2)

    def test_refuses_missing_and_disagreeing_reports(self):
        for altered in (
            "",
            REPORT + REPORT,
            REPORT.replace("held-ignored: 0 0 256", "held-ignored: 0 256 256"),
            REPORT.replace("status-zero: 0 0 256 0 2 2", "status-zero: 0 0 256 0 2 1"),
            REPORT.replace("0 5 2 10 4 9\n", "0 5 3 10 4 9\n"),
            REPORT.replace("2 1 255 1 3 2 12", "2 1 255 1 3 2 10"),
            REPORT.replace("1 255 2 0 1 1 14 5 0", "2 1 255 1 3 2 12 4 9"),
        ):
            with self.subTest(altered=altered), self.assertRaises(ValueError):
                menu.decode_report(altered)

    def test_existing_output_and_dangling_symlink_are_never_overwritten(self):
        with TemporaryDirectory() as temp:
            output = Path(temp) / "receipt.json"
            output.write_text("preserved")
            with self.assertRaises(FileExistsError):
                menu.export(None, None, None, output)
            self.assertEqual(output.read_text(), "preserved")
            link = Path(temp) / "dangling.json"
            link.symlink_to("absent.json")
            with self.assertRaises(FileExistsError):
                menu.export(None, None, None, link)
            self.assertFalse(link.exists())
            self.assertTrue(link.is_symlink())
