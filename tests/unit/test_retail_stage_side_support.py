from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_side_support as support


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStageSideSupportTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-side-support.json").read_bytes())
        cls.archives = {row["filename"]: row for row in cls.report["archives"]}
        cls.targets = [t for row in cls.archives.values() for t in row["targets"]]

    def test_receipt_pins_both_previous_source_joins(self):
        for stem, key in (("stage-side-census", "stage_side_census_sha256"), ("support-anchor", "support_anchor_sha256")):
            raw = (MAPS / f"gg4e08-{stem}.json").read_bytes()
            self.assertEqual(self.report[key], sha256(raw).hexdigest())
        self.assertEqual(tuple(self.archives), support.side.NAMES)
        self.assertEqual((self.report["actor"], self.report["actor_seat"], self.report["actor_hp"]),
                         ("gg4e08:000a", 0, 180))

    def test_all_published_common_cells_support_from_above_at_selected_pose(self):
        self.assertEqual(len(self.targets), 181)
        self.assertEqual(Counter(t["shape_kind"] for t in self.targets), {1: 97, 4: 84})
        self.assertEqual(self.report["targets_by_shape"], {"box": 97, "plane": 84})
        for row in self.targets:
            self.assertTrue(row["exact_cell_and_owner_acquired"])
            self.assertTrue(row["landing_followed_supported_height"])
            self.assertEqual(row["near_target_kernel_calls"], 1)
            self.assertFalse(row["above_support_acquired"])
        self.assertEqual((self.report["near_pose_above_surface"], self.report["far_pose_above_surface"]), (4, 1024))

    def test_type11_boxes_are_distinct_support_cells_under_one_owner(self):
        boxes = [r for r in self.archives["set0904.arc"]["targets"] if r["placement"] == 14]
        self.assertEqual([(r["descriptor"], r["role20_registry_index"], r["near_kernel_calls_of_same_kind"])
                          for r in boxes], [(243, 0, 2), (242, 1, 2)])

    def test_two_planes_preserve_the_float_step(self):
        rounded = [(row["filename"], t["placement"], t["descriptor"], t["height_float_steps_above_target_top"])
                   for row in self.archives.values() for t in row["targets"] if t["height_float_steps_above_target_top"]]
        self.assertEqual(rounded, [("set0b00.arc", 1, 140, 1), ("set0b00.arc", 0, 140, 1)])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            support.decode("", [], [], {})
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "side-support.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                support.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
