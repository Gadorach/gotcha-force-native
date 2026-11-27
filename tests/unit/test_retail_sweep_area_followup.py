"""Verify that sibling carrier, shape and HP order are not interchangeable."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_sweep_area_followup as followup


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-sweep-area-followup.json"


class RetailSweepAreaFollowupTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_the_first_carrier_and_first_shape_go_to_different_children(self):
        self.assertEqual(len(self.data["cases"]), 12)
        for model in ("0e00", "0e05", "0e03"):
            empty, single, both, area_only = [r for r in self.data["cases"] if r["borg"] == f"gg4e08:{model}"]
            self.assertEqual(single["allocation_order"], ["area", "segment"])
            self.assertEqual(single["shape_admission_order"], ["segment"])
            self.assertEqual(both["allocation_order"], ["area", "segment"])
            self.assertEqual(both["shape_admission_order"], ["segment", "area"])
            self.assertEqual(both["next_registry_order"], ["area", "segment"])
            self.assertEqual(area_only["allocation_order"], ["area"])
            self.assertEqual(area_only["shape_admission_order"], ["area"])
            self.assertEqual(empty["shape_admission_order"], [])
            self.assertEqual(empty["receiver_hp_after"], 180)

    def test_sibling_tokens_and_amount_records_write_separately(self):
        data = self.data
        self.assertEqual((data["area_source"]["group"], data["area_source"]["raw_power"]), (3, 20))
        for row in data["cases"]:
            if row["case"] != "two-shapes":
                continue
            self.assertEqual(row["opening_attack_registry_count"], 1)
            self.assertEqual(row["area"]["radii"], [3.1, 34.1])
            self.assertEqual(row["hp_writer_order"], ["area", "segment"])
            self.assertEqual((row["area"]["token"], row["segment"]["token"]), (1, 2))
            self.assertEqual((row["area"]["hp_loss"], row["segment"]["hp_loss"], row["receiver_hp_after"]), (17, 26, 137))
            self.assertNotEqual(row["area"]["common_amount_record"], row["segment"]["actor_amount_record"])
            self.assertTrue(row["same_registry_refused_after_hits"])

    def test_report_refuses_inverting_tokens_or_sharing_damage(self):
        def line(row):
            case = next(i for i, args in enumerate(followup.CASES) if args[0] == row["case"])
            area, segment = row["area"], row["segment"]
            numbers = (int(row["borg"][-4:], 16), case, int(segment["shape_bound"]), int(area["shape_bound"]),
                       row["opening_attack_registry_count"], len(row["next_registry_order"]),
                       segment["token"] or 0, area["token"] or 0, segment["hp_loss"], area["hp_loss"],
                       row["receiver_hp_after"])
            return ("sweep-followup: " + " ".join(map(str, numbers)) + " "
                    + " ".join(f"{r:.5f}" for r in area["radii"] or (0, 0)))

        report = "\n".join(map(line, self.data["cases"]))
        self.assertEqual(followup.decode_report(report), self.data["cases"])
        for old, new in (("3584 2 1 1 1 2 2 1 26 17 137", "3584 2 1 1 1 2 1 2 26 17 137"),
                         ("3584 2 1 1 1 2 2 1 26 17 137", "3584 2 1 1 1 2 2 1 43 0 137"),
                         ("3584 1 1 0 1 1 1 0 26 0 154", "3584 1 0 1 0 1 0 1 0 17 163")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                followup.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                followup.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
