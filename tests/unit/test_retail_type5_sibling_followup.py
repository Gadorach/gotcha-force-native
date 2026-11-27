"""Preserve distinct sibling area timing, credit and return boundaries."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type5_sibling_followup as follow


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type5-sibling-followup.json"


class RetailType5SiblingFollowupTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = json.loads(ARTIFACT.read_bytes())["cases"]

    def test_area_keys_keep_their_own_growth_and_record(self):
        self.assertEqual(len(self.rows), 30)
        for refused, near, middle, wide, far in zip(*(self.rows[i::5] for i in range(5)), strict=True):
            key = near["area_key"]
            fast = key == 7
            self.assertEqual([r["first_hit_visit"] for r in (near, middle, wide, far)],
                             [2, 2, 3, 8] if fast else [2, 3, 4, None])
            self.assertEqual(near["opening_radius"], 3.1)
            self.assertAlmostEqual(near["first_published_radius"], 65.1 if fast else 34.1, places=3)
            self.assertEqual(far["peak_radius"], 1240 if fast else 387.5)
            self.assertEqual((near["hp_loss"], near["receiver_hp_after"], near["accepted_token"]), (17, 163, 1))
            self.assertEqual(far["hp_loss"], 17 if fast else 0)
            self.assertEqual(near["area_origin"], [200, 100, 500] if fast else [260, 120, 580])
            self.assertEqual(near["common_amount_record"], f"{0x802C4760+24*follow.GROUPS[key][2]:08X}")
            self.assertIsNone(refused["first_published_visit"])

    def test_shape_and_area_outlive_parent_but_refused_shape_does_not_retry(self):
        for row in self.rows:
            self.assertEqual(row["parent_return_visit"], 2)
            if row["shape_bound"]:
                self.assertEqual(row["first_published_visit"], 2)
                self.assertEqual((row["area_shape_return_visit"], row["area_carrier_return_visit"]),
                                 (22, 55) if row["area_key"] == 7 else (18, 51))
            else:
                self.assertEqual(row["area_carrier_return_visit"], 3)
                self.assertIsNone(row["area_shape_return_visit"])
                self.assertIsNone(row["observed_area_token"])
                self.assertEqual(row["receiver_hp_after"], 180)

    def test_decoder_refuses_shape_lifetime_alias_or_hitting_beyond_the_cap(self):
        lines = []
        for row in self.rows:
            index = next(i for i, case in enumerate(follow.CASES) if case[0] == row["case"])
            values = (int(row["borg"][-4:], 16), index, row["area_key"], row["target_x_offset"],
                      row["first_published_visit"], row["first_hit_visit"], row["hp_loss"], row["receiver_hp_after"],
                      row["observed_area_token"], row["parent_return_visit"], row["area_shape_return_visit"],
                      row["area_carrier_return_visit"])
            radii = (row["opening_radius"], row["first_published_radius"], row["radius_at_first_hit"], row["peak_radius"])
            lines.append("area-followup: " + " ".join(str(v or 0) for v in values)
                         + " " + " ".join(f"{v or 0:.5f}" for v in radii))
        report = "\n".join(lines)
        self.assertEqual(follow.decode_report(report), self.rows)
        for old, new in (("262 4 19 420 2 0 0 180", "262 4 19 420 2 14 17 163"),
                         ("3329 2 7 70 2 2 17 163 1 2 22 55", "3329 2 7 70 2 3 17 163 1 2 18 51")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                follow.decode_report(report.replace(old, new, 1))

    def test_existing_output_remains_unmodified(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "receipt.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                follow.export(None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
