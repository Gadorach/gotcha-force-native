"""Keep the type-76 attack, receiver, and hit-latched child separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type76_opening as opening


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type76-opening.json"


class RetailType76OpeningTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_the_mixed_queue_is_not_two_attacks(self):
        self.assertEqual(len(self.data["cases"]), 24)
        for source in self.data["sources"]:
            self.assertEqual([(r["source_flags"], r["raw_power"], r["published_role"], r["published_radius"])
                              for r in source["shapes"]], [(1, 40, "attack", 30), (2, 10, "receiver", 55)])
        for empty, single, both, tail, miss, refused in zip(*(self.data["cases"][i::6] for i in range(6)), strict=True):
            self.assertIsNone(empty["attack_descriptor"])
            self.assertIsNone(single["carrier_receiver_descriptor"])
            self.assertEqual(single["attack_descriptor"], both["attack_descriptor"])
            for row in (single, both, refused):
                self.assertEqual((row["hp_loss"], row["receiver_hp_after"]), (35, 145))
                self.assertTrue(row["duplicate_token_refused"])
                self.assertEqual(row["accepted_amount_record"], single["accepted_amount_record"])
            for row in (empty, tail, miss):
                self.assertEqual(row["receiver_hp_after"], 180)
                self.assertIsNone(row["accepted_amount_record"])
            self.assertIsNotNone(tail["carrier_receiver_descriptor"])

    def test_hit_latch_closes_parent_even_if_child_refuses(self):
        for admitted, refused in zip(self.data["cases"][2::6], self.data["cases"][5::6], strict=True):
            self.assertEqual(admitted["hit_latched_exit"], {
                "type": 8, "key": 0, "spare_carriers": 1, "allocated": True, "returned_parent_shapes": 2,
            })
            self.assertEqual(refused["hit_latched_exit"], {
                "type": 8, "key": 0, "spare_carriers": 0, "allocated": False, "returned_parent_shapes": 2,
            })
            self.assertEqual(admitted["accepted_token"], refused["accepted_token"])

    def test_report_rejects_a_receiver_becoming_an_attacker(self):
        def line(row):
            scenario = next(i for i, case in enumerate(opening.CASES) if row["case"] == case[0])
            tail = row["hit_latched_exit"] or {}
            values = (int(row["borg"][-4:], 16), row["ordinal"], scenario, row["socket"], row["carrier_group"],
                      row["shape_slots"], row["attack_descriptor"] is not None,
                      1+int(row["carrier_receiver_descriptor"] is not None), row["hp_loss"], row["receiver_hp_after"],
                      row["accepted_token"] or 0, bool(tail), tail.get("allocated", False))
            record = row["accepted_amount_record"] or "00000000"
            return ("type76-opening: " + " ".join(str(int(v)) for v in values)
                    + f" {record} " + " ".join(f"{v:.4f}" for v in row["parent_origin"]))

        report = "\n".join(map(line, self.data["cases"]))
        self.assertEqual(opening.decode_report(report), self.data["cases"])
        for old, new in (
            ("3329 0 2 2 2 2 1 2 35 145", "3329 0 2 2 2 2 2 1 35 145"),
            ("1 1 1 80362920", "1 1 1 803628F0"),
            ("3332 0 5 8 1 2 1 2 35 145 1 1 0", "3332 0 5 8 1 2 1 2 35 145 1 1 1"),
        ):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                opening.decode_report(report.replace(old, new, 1))

    def test_export_refuses_existing_output(self):
        with TemporaryDirectory() as tmp:
            target = Path(tmp) / "opening.json"
            target.write_text("keep")
            with self.assertRaises(FileExistsError):
                opening.export(None, None, None, None, target)
            self.assertEqual(target.read_text(), "keep")
