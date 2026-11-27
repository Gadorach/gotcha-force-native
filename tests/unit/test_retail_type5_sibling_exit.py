"""Separate the actor-contact area from zero-HP, expiry and bounds exits."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type5_sibling_exit as replay


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type5-sibling-exits.json"


class RetailType5SiblingExitTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases = json.loads(ARTIFACT.read_bytes())["cases"]

    def test_latch_wins_over_terminal_conditions_without_fallback(self):
        self.assertEqual(len(self.cases), 36)
        for latch, terminal, refused, zero, expired, outside in zip(*(self.cases[i::6] for i in range(6)), strict=True):
            self.assertEqual(latch["request"], terminal["request"])
            self.assertEqual(terminal["request"]["type"], 5)
            self.assertTrue(terminal["zero_hp"] and terminal["expired"])
            self.assertEqual(refused["request"], {**terminal["request"], "allocated": False})
            self.assertEqual(refused["free_descriptors_after"], 1)
            for row in (zero, expired):
                self.assertEqual((row["request"]["type"], row["request"]["key"], row["request"]["position"]),
                                 (4, 0, [200, 100, 500]))
                self.assertEqual(row["free_descriptors_after"], 1)
            self.assertIsNone(outside["request"])
            self.assertEqual(outside["parent_phase_after"], 2)
            self.assertEqual(outside["free_descriptors_after"], 2)

    def test_type77_uses_root_despite_contact_latch(self):
        for row in self.cases[::6]:
            use_root = row["parent_type"] == 77
            self.assertEqual(row["request"]["position_offset"], "0x20" if use_root else "0x2A0")
            self.assertEqual(row["request"]["position"], [200, 100, 500] if use_root else [260, 120, 580])

    def test_decoder_refuses_invented_fallback_or_universal_hit_point(self):
        lines = []
        for row in self.cases:
            scenario = next(i for i, case in enumerate(replay.CASES) if case[0] == row["case"])
            request = row["request"] or {}
            values = (int(row["borg"][-4:], 16), scenario, row["parent_type"], row["parent_row"], row["declared_ordinal"],
                      request.get("type", 0), request.get("key", 0), request.get("allocated", False),
                      row["free_descriptors_after"], row["parent_phase_after"], int(request.get("position_offset", "0"), 16))
            lines.append("area-exit: " + " ".join(str(int(v)) for v in values))
        report = "\n".join(lines)
        self.assertEqual(replay.decode_report(report), self.cases)
        for old, new in (("262 2 33 2 2 5 19 0 1 2 672", "262 2 33 2 2 4 0 1 0 2 32"),
                         ("3329 0 77 0 0 5 7 1 0 2 32", "3329 0 77 0 0 5 7 1 0 2 672")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                replay.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_remains_unmodified(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "receipt.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                replay.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
