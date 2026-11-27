"""Verify Saucer endpoint correction separately from the type-9 allocation."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type32_terrain as terrain


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type32-terrain.json"
REPORT = """\
type32-terrain: 3331 0 0 1 0 0 0 220.00 -200.00 500.00
type32-terrain: 3331 1 1 1 1 1 0 160.00 0.00 400.00
type32-terrain: 3331 2 1 1 1 0 0 160.00 0.00 400.00
type32-terrain: 3331 3 1 1 1 0 0 160.00 0.00 400.00
type32-terrain: 3586 4 0 0 0 0 1 220.00 -200.00 500.00
type32-terrain: 3586 5 1 0 0 0 1 220.00 -200.00 500.00
"""


class RetailType32TerrainTest(unittest.TestCase):
    def test_correction_survives_either_type9_allocation_refusal(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["cases"], terrain.decode_report(REPORT))
        clear, floor, no_carrier, no_descriptor, _, _ = data["cases"]
        self.assertTrue(clear["active_worker_queried_terrain"])
        self.assertIsNone(clear["effect"])
        self.assertEqual(clear["borrowed_second_endpoint_after"], terrain.SECOND)
        for row in (floor, no_carrier, no_descriptor):
            self.assertEqual(row["borrowed_second_endpoint_after"], terrain.CROSSING)
            self.assertEqual(row["copied_second_endpoint_after"], terrain.CROSSING)
            self.assertEqual(row["effect"]["key"], 4)
            self.assertEqual(row["effect"]["source_point"], terrain.CROSSING)
            self.assertFalse(row["actor_contact_area_requested"])
        self.assertTrue(floor["effect"]["allocated"])
        self.assertFalse(no_carrier["effect"]["allocated"])
        self.assertFalse(no_descriptor["effect"]["allocated"])

    def test_positive_mode_does_not_take_the_carrier_query(self):
        clear, floor = terrain.decode_report(REPORT)[4:]
        self.assertIsNone(clear["independent_stage_query_point"])
        self.assertEqual(floor["independent_stage_query_point"], terrain.CROSSING)
        for row in (clear, floor):
            self.assertFalse(row["active_worker_queried_terrain"])
            self.assertEqual(row["borrowed_second_endpoint_after"], terrain.SECOND)
            self.assertEqual(row["segment"], {"key": 1, "bound_group": 1, "far_endpoint": terrain.SECOND})

    def test_refuses_clip_lost_on_pool_failure_or_borrowed_mode_alias(self):
        for old, new in (("3331 2 1 1 1 0 0 160.00 0.00 400.00", "3331 2 1 1 1 0 0 220.00 -200.00 500.00"),
                         ("3586 5 1 0 0 0 1 220.00 -200.00 500.00", "3586 5 1 1 0 0 1 160.00 0.00 400.00")):
            self.assertIn(old, REPORT)
            with self.assertRaises(ValueError):
                terrain.decode_report(REPORT.replace(old, new, 1))

    def test_existing_output_stays_unmodified(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "terrain.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                terrain.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
