"""Keep Emperor placement, later pin, registration and native contrast separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_emperor_pin as emperor


RECEIPT = Path(__file__).resolve().parents[2]/"docs/api-map/gg4e08-emperor-pin.json"


class EmperorPinReceiptTest(unittest.TestCase):
    def test_loaded_placement_is_not_the_pin(self):
        data = json.loads(RECEIPT.read_bytes())
        self.assertEqual((data["borg"], data["source_member"], data["source_stage_member"]),
                         ("gg4e08:0e04", 684, 473))
        self.assertEqual(data["lane_setter_control"],
                         {"admitted": [0], "refused": [1, 2, 3, 4, 7, 27, 28], "both_lanes": True})
        for case, position in zip(data["original_cases"], ([0, 1000, 0], [-3648, 1985, -3800]), strict=True):
            rows = {r["edge"]: r for r in case["poses"]}
            self.assertEqual(rows["placement-inlet"]["xyz"], [0, 2000, 0])
            self.assertEqual(rows["constructed"]["xyz"], position)
            self.assertEqual(rows["first-pre"]["xyz"], position)
            self.assertEqual((rows["first-pin"]["visit"], rows["first-pin"]["xyz"]), (22, [0, 2000, 0]))
            self.assertTrue(case["scheduled_steering"]["same_pose_as_neutral_fork"])
            self.assertEqual(case["receiver_rebuild"]["emperor_count"], 25)
            self.assertFalse(case["receiver_rebuild"]["emperor_saved_support_owner"])
        cruise, stationary = data["native_contrasts"]
        self.assertEqual(cruise["spawn_y"]-cruise["floor"], 50)
        self.assertTrue(cruise["steering_changes_heading"])
        self.assertTrue(stationary["grounded"])
        self.assertFalse(stationary["allows_moves"])

    def test_overwrite_and_incomplete_report_refuse(self):
        with TemporaryDirectory() as temp:
            existing = Path(temp)/"keep.json"
            existing.write_text("keep")
            with self.assertRaises(FileExistsError):
                emperor.export(None, None, None, None, None, existing)
            self.assertEqual(existing.read_text(), "keep")
        with self.assertRaisesRegex(ValueError, "report extent"):
            emperor.decode("emperor-active: incomplete")
