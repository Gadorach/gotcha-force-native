"""Keep terrain probes, area allocations and segment binds independent."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_sweep_area_routes as sweep


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-sweep-area-routes.json"


class RetailSweepAreaRoutesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_allocation_order_depends_on_two_different_resources(self):
        self.assertEqual(len(self.data["cases"]), 12)
        for borg, expected_group in (("0e00", 1), ("0e05", 1), ("0e03", 2)):
            clear, both, one, no_descriptors = [r for r in self.data["cases"] if r["borg"] == f"gg4e08:{borg}"]
            self.assertEqual([r["allocation_order"] for r in (clear, both, one, no_descriptors)],
                             [["segment"], ["area", "segment"], ["area"], ["segment"]])
            self.assertIsNone(clear["area"])
            self.assertFalse(one["segment"]["allocated"])
            self.assertFalse(one["segment"]["normal_group_bound"])
            self.assertEqual(one["segment"]["requested_key"], expected_group)
            self.assertEqual(no_descriptors["area"]["requested_key"], 26)
            self.assertFalse(no_descriptors["area"]["allocated"])
            self.assertTrue(no_descriptors["segment"]["normal_group_bound"])

    def test_floor_point_does_not_trim_segment(self):
        for row in self.data["cases"]:
            self.assertEqual(row["terrain_probe"]["far"][2], 6000)
            self.assertGreater(row["segment"]["far"][2], 6000)
            self.assertEqual(row["visit_including_initializer"], 34 if row["borg"] == "gg4e08:0e03" else 35)
            if row["area"] is not None:
                self.assertEqual(row["area"]["origin"], row["terrain_probe"]["floor_crossing"])
                self.assertEqual(row["area"]["origin"][1], 0)

    def test_report_refuses_resource_aliasing_or_emitted_endpoint_trimming(self):
        def line(row):
            scenario = next(i for i, case in enumerate(sweep.CASES) if row["case"] == case[0])
            area = row["area"] or {}
            segment = row["segment"]
            counts = (int(row["borg"][-4:], 16), scenario, row["visit_including_initializer"], bool(area),
                      area.get("allocated", False), segment["allocated"], segment["requested_key"])
            points = (area.get("origin", (0, 0, 0)), row["terrain_probe"]["near"], row["terrain_probe"]["far"],
                      segment["near"], segment["far"])
            return ("sweep-area: " + " ".join(str(int(v)) for v in counts)
                    + " " + " ".join(f"{x:.5f}" for p in points for x in p))

        report = "\n".join(map(line, self.data["cases"]))
        self.assertEqual(sweep.decode_report(report), self.data["cases"])
        for old, new in (("3584 2 35 1 1 0 1", "3584 2 35 1 1 1 1"),
                         ("3587 3 34 1 0 1 2", "3587 3 34 1 1 0 2"),
                         ("14026.48633", "6000.00000")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                sweep.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                sweep.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
