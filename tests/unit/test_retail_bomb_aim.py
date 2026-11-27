"""Keep release point, launch-gate ownership, and flight tracking separate."""
import json
import math
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_bomb_aim as aim


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-bomb-aim.json"
REPORT = "".join(f"""\
bomb-aim: {model} 0 0 0 0 4 165 200 300 0.00000 0.00000 40.00000 0.3000000
bomb-aim: {model} 1 1 1 1 4 165 200 300 23.97356 1.87707 31.96474 0.3003559
bomb-aim: {model} 2 1 1 1 4 165 200 300 23.97356 1.87707 31.96474 0.3003559
bomb-aim: {model} 3 0 1 0 4 165 200 300 23.97356 1.87707 31.96474 0.3000000
bomb-aim: {model} 4 1 0 1 4 165 200 300 0.00000 0.00000 40.00000 0.1500000
bomb-aim: {model} 5 1 1 1 4 165 200 300 16.72096 28.69424 22.29461 0.1500000
bomb-aim: {model} 6 1 1 1 4 165 200 300 -22.24162 -15.02868 29.65550 0.2625512
bomb-aim: {model} 7 1 1 0 3 165 200 300 23.97356 1.87707 31.96474 0.3000000
bomb-aim: {model} 8 1 1 1 4 165 200 300 23.97356 1.87707 31.96474 0.1500000
bomb-aim: {model} 9 1 1 1 4 215 200 300 21.17917 1.77059 33.88668 0.3003326
""" for model in (0, 3, 10, 12))


class RetailBombAimTest(unittest.TestCase):
    def test_receipt_does_not_treat_aim_as_single_live_target_point(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], aim.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], aim.AFS_SHA256)
        self.assertEqual([r["key"] for r in data["pilots"]], [0, 1, 3, 5])
        rows = data["common_cases_verified_for_each_pilot"]
        self.assertEqual(rows, aim.decode_report(REPORT))
        fallback, ordinary, overwritten, added, removed, high, below, lost, moved, shifted = rows
        self.assertEqual(ordinary["velocity_before_first_active_visit"], overwritten["velocity_before_first_active_visit"])
        self.assertEqual(ordinary["velocity_before_first_active_visit"], added["velocity_before_first_active_visit"])
        self.assertFalse(added["target_references_copied_at_release"])
        self.assertFalse(added["fourth_active_visit"]["tracking_reference_retained"])
        self.assertEqual(fallback["velocity_before_first_active_visit"], removed["velocity_before_first_active_visit"])
        self.assertFalse(removed["live_caster_gate_at_launch"])
        self.assertTrue(removed["fourth_active_visit"]["tracking_reference_retained"])
        self.assertEqual(moved["fourth_active_visit"]["gravity"], .15)
        self.assertEqual(moved["velocity_before_first_active_visit"], ordinary["velocity_before_first_active_visit"])
        self.assertEqual(lost["fourth_active_visit"], {"tracking_reference_retained": False,
                                                      "refit_countdown_after": 3, "gravity": .3})
        vx, vy, vz = high["velocity_before_first_active_visit"]
        self.assertAlmostEqual(math.atan2(vy, math.hypot(vx, vz)), .8, places=5)
        self.assertLess(below["velocity_before_first_active_visit"][1], 0)
        self.assertEqual(shifted["point_copied_at_release"], ordinary["point_copied_at_release"])
        self.assertNotEqual(shifted["key_11_origin_at_launch"], ordinary["key_11_origin_at_launch"])
        self.assertNotEqual(shifted["velocity_before_first_active_visit"], ordinary["velocity_before_first_active_visit"])

    def test_refuses_wrong_gate_clock_or_gravity_on_one_pilot(self):
        for report in (
            REPORT*2,
            REPORT.replace("0 1 0 4 165", "0 0 0 4 165", 1),
            REPORT.replace("0 3 165 200", "1 4 165 200", 1),
            REPORT.replace("31.96474 0.1500000", "31.96474 0.3003559", 1),
            REPORT.replace("bomb-aim: 12 9", "bomb-aim: 10 9", 1),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                aim.decode_report(report)

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as temp:
            out = Path(temp) / "receipt.json"
            out.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                aim.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "unchanged")
