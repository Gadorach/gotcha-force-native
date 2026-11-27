"""Separate incoming carrier damage from fighter HP and outgoing child routes."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type76_incoming as incoming


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type76-incoming.json"


class RetailType76IncomingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = json.loads(ARTIFACT.read_bytes())["cases"]

    def test_contact_writes_the_carrier_and_not_either_fighter(self):
        self.assertEqual(len(self.rows), 20)
        for row in self.rows:
            self.assertEqual((row["pilot_hp_after"], row["sasuke_hp_after"]), (150, 180))
            self.assertEqual(set(row["fighter_damage_ledger_deltas"].values()), {0})
            self.assertEqual(row["carrier_outgoing_latch"], 0)
            if row["zero_hp_exit"] is not None:
                self.assertEqual((row["writer_amount"], row["carrier_hp_lost"], row["carrier_hp_after"]), (17, 10, 0))
                self.assertEqual(row["amount_record_reached"], "802C4838")
                self.assertEqual((row["carrier_history_token"], row["carrier_receive_flags"]), (2, 1))
            else:
                self.assertEqual(row["carrier_hp_after"], 10)
                self.assertIsNone(row["amount_record_reached"])

    def test_effect_refusal_does_not_keep_the_carrier_or_its_shapes(self):
        for single, miss, admitted, no_cell, no_descriptor in zip(*(self.rows[i::5] for i in range(5)), strict=True):
            self.assertFalse(single["receiver_published"])
            self.assertTrue(miss["receiver_published"])
            self.assertEqual([r["zero_hp_exit"]["allocated"] for r in (admitted, no_cell, no_descriptor)], [True, False, False])
            for row in (admitted, no_cell, no_descriptor):
                exit = row["zero_hp_exit"]
                self.assertEqual((exit["type"], exit["key"]), (4, 0))
                self.assertEqual((exit["shape_return_visit_after_hit"], exit["carrier_and_descriptor_return_visit_after_hit"]), (1, 3))

    def test_report_rejects_writer_clipping_or_cleanup_drift(self):
        lines = []
        for row in self.rows:
            scenario = next(i for i, case in enumerate(incoming.CASES) if case[0] == row["case"])
            exit = row["zero_hp_exit"] or {}
            values = (int(row["borg"][-4:], 16), row["ordinal"], scenario, row["carrier_shape_slots"],
                      row["incoming_x_offset"], row["writer_amount"], row["carrier_hp_after"], row["carrier_history_token"] or 0,
                      row["incoming_contact_flags"], row["carrier_receive_flags"], bool(exit),
                      exit.get("allocated", False), 2 if exit else 0, bool(exit))
            lines.append("type76-incoming: " + " ".join(str(int(v)) for v in values))
        report = "\n".join(lines)
        self.assertEqual(incoming.decode_report(report), self.rows)
        for old, new in (
            ("3329 0 2 2 0 17 0 2", "3329 0 2 2 0 10 0 2"),
            ("3332 0 4 2 0 17 0 2 64 1 1 0 2 1", "3332 0 4 2 0 17 0 2 64 1 1 1 2 1"),
            ("3332 0 4 2 0 17 0 2 64 1 1 0 2 1", "3332 0 4 2 0 17 0 2 64 1 1 0 0 1"),
        ):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                incoming.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "receipt.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                incoming.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
