"""Keep scheduled type-11 carrier pressure separate from shape pressure and hit tokens."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type32_stream as stream


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type32-stream.json"


class RetailType32StreamTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_fewer_carriers_and_fewer_shapes_block_different_steps(self):
        self.assertEqual(len(self.data["cases"]), 20)
        for model in ("0104", "0505", "0511", "0615"):
            single, three, one_shape, two_shapes, ample = [
                r for r in self.data["cases"] if r["borg"] == f"gg4e08:{model}"
            ]
            self.assertEqual(single["landed_visits"], [2, 6])
            self.assertEqual(three["landed_visits"], [2, 3, 4, 6, 7])
            self.assertEqual(one_shape["landed_visits"], [2, 5])
            self.assertEqual(two_shapes["landed_visits"], [2, 3, 5, 6])
            self.assertEqual(ample["landed_visits"], [2, 3, 4, 5, 6, 7])
            self.assertFalse(three["visits"][3]["segment_allocated"])
            self.assertEqual(three["visits"][3]["free_carrier_cells_after"], 1)
            self.assertTrue(two_shapes["visits"][2]["segment_allocated"])
            self.assertFalse(two_shapes["visits"][2]["opening_shape_bound"])
            self.assertEqual(two_shapes["visits"][2]["free_shape_cells_after"], 1)

    def test_gate_closure_stops_requests_but_not_an_admitted_sweep(self):
        for row in self.data["cases"]:
            final = row["visits"][-1]
            self.assertEqual(final["parent_phase"], 2)
            self.assertFalse(final["request"])
            self.assertFalse(final["segment_allocated"])
            if (row["spare_carriers_at_start"], row["attacker_shapes_at_start"]) == (4, 3):
                self.assertEqual(final["landed_segment_token"], 6)
                self.assertGreater(final["hp_loss"], 0)
                self.assertEqual(row["hp_lost"], 6*final["hp_loss"])

    def test_report_refuses_reclaiming_same_visit_or_reusing_token(self):
        def line(row, visit):
            scenario = stream.POOLS.index((row["spare_carriers_at_start"], row["attacker_shapes_at_start"]))
            values = (int(row["borg"][-4:], 16), scenario, visit["active_visit"], int(visit["request"]),
                      int(visit["segment_allocated"]), int(visit["opening_shape_bound"]), visit["registry_count"],
                      visit["landed_segment_token"] or 0, visit["hp_loss"], visit["receiver_hp"],
                      visit["free_carrier_cells_after"], visit["free_shape_cells_after"], visit["parent_phase"])
            return "type32-stream: " + " ".join(map(str, values))

        report = "\n".join(line(row, visit) for row in self.data["cases"] for visit in row["visits"])
        self.assertEqual(stream.decode_report(report), self.data["cases"])
        for old, new in (("260 1 4 1 0 0", "260 1 4 1 1 1"),
                         ("260 3 3 1 1 0", "260 3 3 1 1 1"),
                         ("260 4 7 0 0 0 1 6 2 168", "260 4 7 0 0 0 1 5 2 168")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                stream.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                stream.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
