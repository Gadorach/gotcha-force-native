"""Pin the actual geometry crossing and floor-specific shape timing."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_bomb_floor as floor


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-bomb-floor.json"
REPORT = "".join(f"""\
bomb-floor: {model} 0 35 565 0 0 0 0 0 0 1 26.59997 16.24997 1684.49255 16.24997 1684.49255
bomb-floor: {model} 1 35 565 1 1 1 1 36 37 1 26.59997 16.24997 1684.49255 25.00000 1651.78943
bomb-floor: {model} 2 35 565 0 0 0 1 0 0 1 26.59997 16.24997 1684.49255 16.24997 1684.49255
bomb-floor: {model} 3 35 565 1 0 1 1 36 37 1 26.59997 16.24997 1684.49255 25.00000 1651.78943
bomb-floor: {model} 4 23 577 0 0 0 0 23 25 1 127.40000 120.65000 1215.72021 127.40000 1176.26709
bomb-floor-area: {model} 0 35 0 0 3 0 1 1
bomb-floor-area: {model} 1 35 1 1 1 1 1 1
""" for model in (0, 10))


class RetailBombFloorTest(unittest.TestCase):
    def test_receipt_separates_floor_bounds_and_same_pass_shape_pressure(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], floor.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], floor.AFS_SHA256)
        self.assertEqual(data["pilots"], floor.decode_report(REPORT))
        self.assertEqual([(r["borg"], r["key"]) for r in data["pilots"]], [("gg4e08:0000", 0), ("gg4e08:000a", 3)])
        for pilot in data["pilots"]:
            empty, landed, offset, unavailable, bounds = pilot["impact_cases"]
            self.assertEqual(landed["committed_point"], [165, 25, 1651.78943])
            self.assertEqual(landed["independent_segment_probe"], {"grid_code": 1, "triangles_tested": 1})
            self.assertEqual(offset["independent_segment_probe"], {"grid_code": 0, "triangles_tested": 1})
            self.assertEqual(offset["committed_point"], empty["committed_point"])
            self.assertEqual(empty["flight_continued_through_visit"], 64)
            self.assertTrue(unavailable["child_requested"])
            self.assertFalse(unavailable["child_published"])
            self.assertEqual(unavailable["opening_shape_unlink_visit"], 36)
            self.assertEqual(bounds["opening_shape_unlink_visit"], bounds["event_visit"])
            self.assertFalse(bounds["child_requested"])
            refused, admitted = pilot["same_pass_area_cases"]
            self.assertTrue(refused["parent_shape_registered_after_child_init"])
            self.assertFalse(refused["child_bound_shape"])
            self.assertTrue(refused["next_visit"]["parent_shape_returned"])
            self.assertFalse(refused["next_visit"]["child_bind_retry"])
            self.assertEqual(refused["next_visit"]["child_attack_count"], 0)
            self.assertTrue(admitted["child_bound_shape"])
            self.assertFalse(admitted["child_attack_registered_at_init"])
            self.assertEqual(admitted["next_visit"]["child_radius"], 34.1)

    def test_refuses_wrong_origin_early_unlink_or_fabricated_retry(self):
        for report in (
            REPORT*2,
            REPORT.replace("25.00000 1651.78943", "16.24997 1684.49255"),
            REPORT.replace("1 1 36 37", "1 1 35 37"),
            REPORT.replace("0 35 0 0 3 0 1 1", "0 35 0 0 1 1 0 1"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                floor.decode_report(report)

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as temp:
            out = Path(temp) / "receipt.json"
            out.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                floor.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "unchanged")
