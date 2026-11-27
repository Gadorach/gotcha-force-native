"""Keep accepted HP, visual gate, allocation and repeat refusal independent."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_effect_tail as tail


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-effect-tail.json"


class RetailContactEffectTailTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = json.loads(ARTIFACT.read_bytes())["cases"]

    def test_held_gate_suppresses_only_the_visual(self):
        self.assertEqual(len(self.rows), 20)
        for clear, held, no_cell, short, miss in zip(*(self.rows[i::5] for i in range(5)), strict=True):
            self.assertEqual(held["gate_before"], 4)
            self.assertIsNone(held["effect_request"])
            self.assertEqual([r["hp_lost"] for r in (held, no_cell, short)], [clear["hp_lost"]]*3)
            for row in (clear, held, no_cell, short):
                self.assertEqual(row["gate_after"], 4)
                self.assertEqual(row["damage_ledger_deltas"], [row["hp_lost"]]*2)
                self.assertTrue(row["reopened_gate_duplicate_refused"])
            self.assertEqual((miss["receiver_hp_after"], miss["gate_after"]), (180, 0))
            self.assertIsNone(miss["effect_request"])
            self.assertIsNone(miss["accepted_token"])

    def test_refused_allocator_keeps_the_request_and_effect_origin(self):
        for clear, no_cell, short in zip(self.rows[::5], self.rows[2::5], self.rows[3::5], strict=True):
            self.assertTrue(clear["effect_request"]["allocated"])
            for row in (no_cell, short):
                self.assertEqual(row["effect_request"], {**clear["effect_request"], "allocated": False})
                self.assertEqual(row["descriptors_left"], row["descriptors_supplied"])
        self.assertEqual({r["effect_request"]["selector"] for r in self.rows[::5]}, {0, 1, 8})

    def test_report_rejects_hp_rollback_or_allocation_retry(self):
        lines = []
        for row, pair in zip(self.rows, [p for p in tail.PAIRS for _ in tail.CASES], strict=True):
            effect = row["effect_request"] or {}
            values = (row["attack_type"], int(row["credited_borg"][-4:], 16), tail.CASES.index(row["case"]),
                      pair[4], row["hp_lost"], row["receiver_hp_after"], bool(effect), effect.get("allocated", False),
                      row["gate_after"], row["descriptors_left"], row["accepted_token"] or 0)
            lines.append("impact-tail: " + " ".join(str(int(v)) for v in values)
                         + " " + " ".join(f"{x:.5f}" for x in effect.get("position", (0, 0, 0))))
        report = "\n".join(lines)
        self.assertEqual(tail.decode_report(report), self.rows)
        for old, new in (("32 3331 2 8 26 154", "32 3331 2 8 0 180"),
                         ("32 3331 3 8 26 154 1 0 4 7", "32 3331 3 8 26 154 1 1 4 0"),
                         ("76 3329 1 1 35 145 0 0 4", "76 3329 1 1 35 145 1 1 4")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                tail.decode_report(report.replace(old, new, 1))

    def test_overwrite_is_refused_before_source_lookup(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "receipt.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                tail.export(None, None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
