"""Cover the measured type-11 sweep, wall control, and ownership boundaries."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type32_contact as contact
from tools.analyze import retail_type32_shared as shared


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type32-contact.json"


class RetailType32ContactTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_triangle_crossing_does_not_change_tested_overlap_or_hp(self):
        self.assertEqual(len(self.data["cases"]), 16)
        for pilot, loss in zip(shared.PILOTS, (2, 3, 1, 1), strict=True):
            model = f"gg4e08:{pilot.model:04x}"
            clear_hit, clear_miss, wall_hit, wall_miss = [r for r in self.data["cases"] if r["borg"] == model]
            self.assertEqual([r["stage_query_hit_z"] for r in (clear_hit, clear_miss, wall_hit, wall_miss)],
                             [None, None, 1000, 1000])
            self.assertEqual([r["hp_loss"] for r in (clear_hit, clear_miss, wall_hit, wall_miss)], [loss, 0, loss, 0])
            for clear, wall in ((clear_hit, wall_hit), (clear_miss, wall_miss)):
                self.assertEqual({k: v for k, v in clear.items() if k not in ("case", "stage_query_hit_z")},
                                 {k: v for k, v in wall.items() if k not in ("case", "stage_query_hit_z")})
                self.assertGreater(wall["receiver_center_z"], wall["stage_query_hit_z"])

    def test_segment_owns_contact_and_retires_separately(self):
        for row in self.data["cases"]:
            hit = bool(row["hp_loss"])
            self.assertEqual(row["segment_contact_flags"], 16 if hit else 0)
            self.assertEqual(row["carrier_contact_flags"], 0)
            self.assertEqual(row["same_token_refused_on_repeat"], hit)
            self.assertTrue(row["segment_shape_unlinked"])
            self.assertTrue(row["segment_retired_before_carrier"])

    def test_refuses_truncated_wall_sweep_and_false_hp(self):
        def line(row):
            ident = int(row["borg"][-4:], 16)
            values = (ident, row["carrier_key"], contact.CASES.index(row["case"]), row["hp_loss"], row["receiver_hp"],
                      row["receiver_token"], row["segment_contact_flags"], int(bool(row["stage_query_hit_z"])))
            coords = (row["registered_narrow_radius"], row["receiver_offset_x"], row["first_sample_z"],
                      row["refreshed_second_sample_z"], row["receiver_center_z"])
            return "type32-contact: " + " ".join(map(str, values)) + " " + " ".join(f"{x:.5f}" for x in coords)

        report = "\n".join(map(line, self.data["cases"]))
        self.assertEqual(contact.decode_report(report), self.data["cases"])
        for old, new in (("260 1 2 2 178 1 16 1", "260 1 2 0 180 0 0 1"),
                         ("6694.00049", "1000.00000"),
                         ("1557 15 3 0 180 0 0 1", "1557 15 3 1 179 1 16 1")):
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
