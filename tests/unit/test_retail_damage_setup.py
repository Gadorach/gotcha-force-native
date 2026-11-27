"""Check the published settings, controller and rank-setup receipt."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_damage_setup as setup


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-damage-setup.json"
REPORT = """\
setup-checks: 13 96 30
setup-amount: 2 0 3 7 7 0 0 0 0 35
setup-amount: 2 0 1 7 7 0 1 0 0 40
setup-amount: 2 0 1 7 7 0 1 3 0 47
setup-amount: 1 0 1 0 31 0 1 0 0 79
"""


class RetailDamageSetupTest(unittest.TestCase):
    def test_published_join_keeps_banks_settings_and_ranks_independent(self):
        result = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(result["source_dol_sha256"], setup.SOURCE_SHA256)
        self.assertEqual(result["source_afs_sha256"], setup.AFS_SHA256)
        self.assertEqual(result["checks"], setup.CHECKS)
        joined = setup.decode_report(REPORT)
        self.assertEqual(result["joined"], joined)
        self.assertEqual([r["native_cli_indices"] for r in joined],
                         ["7,7:0,0:0,0", "7,7:0,0:0,1", "7,7:3,0:0,1", "0,31:0,0:0,1"])
        self.assertEqual([r["amount"] for r in joined], [35, 40, 47, 79])

    def test_unqualified_report_refuses(self):
        for altered in ("", REPORT.replace("13 96 30", "13 95 30"),
                        REPORT.replace("2 0 3 7 7 0 0", "2 0 3 7 7 0 1"),
                        REPORT.replace("1 0 1 0 31", "1 0 1 31 0"),
                        REPORT.replace("0 1 3 0 47", "0 1 8 0 47")):
            with self.subTest(altered=altered), self.assertRaises(ValueError):
                setup.decode_report(altered)

    def test_output_is_never_overwritten(self):
        with TemporaryDirectory() as work:
            output = Path(work) / "fixture.json"
            output.write_text("preserved")
            with self.assertRaises(FileExistsError):
                setup.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "preserved")
