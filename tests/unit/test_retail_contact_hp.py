"""Pin scope, controls, and amount-versus-HP distinctions in the HP receipt."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_hp as contact


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-hp.json"
REPORT = """\
rifle: 2 1 1 1 1 0 1 1 9 16 2 21 0
rifle: 1 1 0 1 0 300 1 0 0 0 0 0 5
rifle: 1 1 1 0 0 300 1 0 0 0 0 0 5
rifle: 0 1 1 1 0 0 0 0 0 0 0 0 0
rifle: 2 0 1 1 2 0 0 0 0 0 0 0 0
sasuke: 1 2 1 49 26 2 2
sasuke: 0 2 0 0 0 0 0
contact-hp: 81 0 0 0 0 180 0 0 1 35 145 180 35 35 0
contact-hp: 82 0 0 0 0 180 0 0 1 35 145 180 35 35 0
contact-hp: 83 0 0 0 0 180 0 0 0 0 180 0 0 0 0
contact-hp: 81 1 0 0 0 180 0 0 0 0 180 0 0 0 0
contact-hp: 81 0 1 0 0 180 0 0 0 0 180 0 0 0 0
contact-hp: 81 0 0 1 0 180 0 0 0 0 180 0 0 0 0
contact-hp: 81 0 0 0 1 180 0 0 1 35 180 0 0 0 0
contact-hp: 81 0 0 0 0 1 0 0 1 34 0 1 34 34 1
contact-hp: 81 0 0 0 0 180 7 0 1 35 145 180 35 35 0
contact-hp: 81 0 0 0 0 180 7 1 1 40 140 180 40 40 0
"""


class RetailContactHpTest(unittest.TestCase):
    def test_exact_results_preserve_amount_guard_and_overkill_distinctions(self):
        result = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(result["source_dol_sha256"], contact.SOURCE_SHA256)
        self.assertEqual(result["source_afs_sha256"], contact.AFS_SHA256)
        self.assertEqual(result["source"]["raw_attack_power"], 40)
        cases = contact.read_contact_hp(REPORT)
        self.assertEqual(result["cases"], cases)
        self.assertEqual(sum(case["hp_writer_reached"] for case in cases), 6)
        self.assertEqual(sum(case["hp_removed"] > 0 for case in cases), 5)
        self.assertEqual(cases[6]["requested_damage"], 35)
        self.assertEqual(cases[6]["hp_removed"], 0)
        self.assertEqual(cases[7]["hp_removed"], 1)
        self.assertEqual(cases[7]["attacker_ledger_delta"], 34)
        self.assertEqual([case["requested_damage"] for case in cases[-2:]], [35, 40])
        self.assertEqual(result["settings"]["controller_source_rows_at_setting_7"], [
            {"controller_source": 0, "receiver_class_factor": 0.92},
            {"controller_source": 1, "receiver_class_factor": 1.04},
        ])

    def test_refuses_missing_controls_and_inconsistent_hp_writes(self):
        for report in (REPORT.replace("35 180 0 0 0 0", "35 145 180 35 35 0"),
                       REPORT.replace("34 0 1 34 34 1", "34 0 1 1 1 1"),
                       REPORT.replace("7 1 1 40 140 180 40 40 0", "7 1 1 35 145 180 35 35 0"),
                       REPORT.removesuffix("contact-hp: 81 0 0 0 0 180 7 1 1 40 140 180 40 40 0\n")):
            with self.subTest(report=report), self.assertRaises(ValueError):
                contact.read_contact_hp(report)

    def test_export_refuses_existing_output(self):
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "preserved.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                contact.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
