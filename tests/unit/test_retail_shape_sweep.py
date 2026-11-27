"""Exercise the original shape-sweep receipt's bounds and refusal distinctions."""
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_shape_sweep as sweep


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-shape-sweep.json"
REPORT = """\
rifle: 2 1 1 1 1 0 1 1 9 16 2 21 0
rifle: 1 1 0 1 0 300 1 0 0 0 0 0 5
rifle: 1 1 1 0 0 300 1 0 0 0 0 0 5
rifle: 0 1 1 1 0 0 0 0 0 0 0 0 0
rifle: 2 0 1 1 2 0 0 0 0 0 0 0 0
sasuke: 1 2 1 49 26 2 2
sasuke: 0 2 0 0 0 0 0
shape-sweep: 81 0 0 0 1 1 1 1 1 1
shape-sweep: 82 0 0 0 1 1 1 1 1 1
shape-sweep: 83 0 0 0 1 0 1 0 0 0
shape-sweep: 81 1 0 0 0 1 1 0 0 0
shape-sweep: 81 0 1 0 1 1 0 0 0 0
shape-sweep: 81 0 0 1 1 1 1 0 0 0
"""


class RetailShapeSweepTest(unittest.TestCase):
    def test_properly_separates_contact_gates(self):
        result = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(result["source_dol_sha256"], sweep.SOURCE_SHA256)
        self.assertEqual(result["source_afs_sha256"], sweep.AFS_SHA256)
        rows = sweep.read_sweep(REPORT)
        self.assertEqual(result["cases"], rows)
        self.assertEqual([r["distance"] for r in rows if r["damage_amount_entry_reached"]], [81, 82])
        self.assertTrue(rows[2]["attack_published"])
        self.assertFalse(rows[2]["broad_overlap"])
        self.assertTrue(rows[5]["direct_relation_helper_allows"])
        self.assertFalse(rows[5]["classifier_reached"])

    def test_refuses_incomplete_or_inconsistent_report(self):
        for report in (REPORT.replace("83 0 0 0 1 0 1 0 0 0", "83 0 0 0 1 1 1 1 1 1"),
                       REPORT.removesuffix("shape-sweep: 81 0 0 1 1 1 1 0 0 0\n"),
                       REPORT.replace("sasuke: 0 2 0", "sasuke: 0 2 1")):
            with self.subTest(report=report), self.assertRaises(ValueError):
                sweep.read_sweep(report)

    def test_receiver_body_step_and_both_spheres_are_source_checked(self):
        raw = bytearray(sweep.HIT_BYTES)
        raw[:2] = b"\x09\xff"
        struct.pack_into(">4b", raw, 0xA20 + 8, 2, -1, -1, -1)
        attack = sweep.STEPS + 2 * sweep.STEP_SIZE
        body = 32 + 9 * sweep.STEP_SIZE
        struct.pack_into(">BBHHhhH4f", raw, attack, 0, 7, 1, 2, 19, 21, 0, 30, -20, -10, 60)
        struct.pack_into(">BBHHhhH4f", raw, body, 0, 2, 2, 0, 0, -1, 0, 0, -14, 0, 22)
        operands = sweep.source_operands(raw)
        self.assertEqual(operands["attack"]["cursor_bounds"], [19, 21])
        self.assertEqual(operands["receiver"]["radius"], 22)
        raw[0] = 8
        with self.assertRaisesRegex(ValueError, "body list"):
            sweep.source_operands(raw)
        raw[0] = 9
        struct.pack_into(">f", raw, body + 24, 20)
        with self.assertRaisesRegex(ValueError, "receiver sphere"):
            sweep.source_operands(raw)

    def test_export_never_replaces_a_receipt(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "checked.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                sweep.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
