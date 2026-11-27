"""Keep the type-89 contact window, HP write, and area position distinct."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type89_contact as contact


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type89-contact.json"


class RetailType89ContactTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_swept_radius_contact_threshold_uses_the_narrow_shape(self):
        self.assertEqual(len(self.data["cases"]), 16)
        self.assertEqual(self.data["receiver"]["borg"], "gg4e08:000a")
        for model in ("0503", "0506", "050f", "0512"):
            cases = [r for r in self.data["cases"] if r["borg"] == f"gg4e08:{model}"]
            self.assertEqual([r["receiver_offset_from_midpoint"] for r in cases], [30, 51, 52, 53])
            self.assertEqual([r["received_and_credited_hp_loss"] for r in cases], [17, 17, 0, 0] if model < "050f"
                             else [8, 8, 0, 0])
            self.assertEqual({r["registered_key"] for r in cases}, {10 if model in ("0503", "050f") else 3})
            self.assertEqual([r["same_registry_duplicate_refused"] for r in cases], [True, True, False, False])
            self.assertEqual([r["receiver_history_count"] for r in cases], [1, 1, 0, 0])

    def test_area_inherits_the_saved_point_not_the_end_of_the_sweep(self):
        for row in self.data["cases"]:
            area = row["area_allocation"]
            if row["receiver_hp"] == 180:
                self.assertIsNone(area)
                self.assertIsNone(row["amount_record_reached"])
                continue
            self.assertEqual((area["bucket_visit"], area["site"], area["key"]), (33, "8016D30C", 16))
            self.assertEqual(row["contact_bucket_visit"], 32)
            across = 0 if row["registered_key"] == 10 else 2
            self.assertEqual(area["saved_point_minus_carrier_root"][across],
                             row["receiver_offset_from_midpoint"]-22)
            self.assertAlmostEqual(max(map(abs, area["saved_point_minus_carrier_root"])), 5000, delta=0.01)

    def test_report_refuses_root_origin_second_hit_or_tangency_as_contact(self):
        def line(row):
            area = row["area_allocation"]
            values = (int(row["borg"][-4:], 16), row["receiver_offset_from_midpoint"], row["registered_key"],
                      row["received_and_credited_hp_loss"], row["receiver_hp"], row["receiver_token"],
                      row["receiver_history_count"], int(bool(area)))
            point = area["saved_point_minus_carrier_root"] if area else (0, 0, 0)
            return "type89-contact: " + " ".join(map(str, values)) + " " + " ".join(f"{x:.4f}" for x in point)

        report = "\n".join(line(row) for row in self.data["cases"])
        self.assertEqual(contact.decode_report(report), self.data["cases"])
        for old, new in (("8.0000 -4999.9990 0.0000", "0.0000 0.0000 0.0000"),
                         ("10 17 163 1 1 1", "10 34 146 1 2 1"),
                         ("52 10 0 180 0 0 0", "52 10 17 163 1 1 1")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                contact.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                contact.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
