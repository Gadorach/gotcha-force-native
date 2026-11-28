"""Check that the world receipt keeps publication, contact and HP separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_world_moving_receiver as moving


RECEIPT = Path(__file__).resolve().parents[2]/"docs/api-map/gg4e08-world-moving-receiver.json"


class WorldMovingReceiverTest(unittest.TestCase):
    def test_moving_hit_and_narrow_miss_share_published_attack_window(self):
        data = json.loads(RECEIPT.read_bytes())
        hit, miss = data["original_cases"]
        self.assertEqual(data["shared_full_world_visits_after_opening"], 21)
        self.assertEqual([c["attack_cell_ordinal"] for c in (hit, miss)], [6, 6])
        for case in (hit, miss):
            a, b = case["poses_and_registry"]
            self.assertEqual([a["published_attacks"], b["published_attacks"]], [0, 1])
            self.assertNotEqual(b["root_before_actor_visit"], b["root_after_actor_visit"])
            self.assertEqual([shape["current_center"] for shape in a["shapes"]],
                             [shape["previous_center"] for shape in b["shapes"]])
            self.assertEqual([shape["current_center"] for shape in b["shapes"]], case["receiver_centers_at_pair"])
            self.assertTrue(case["complete_world"][-1]["same_attack_cell_returned"])
        self.assertEqual((hit["narrow_attempted_mask"], hit["narrow_accepted_mask"]), (2, 2))
        self.assertEqual((miss["narrow_attempted_mask"], miss["narrow_accepted_mask"]), (1, 0))
        self.assertEqual([v["hp_writes"] for v in hit["complete_world"]], [1, 0, 0])
        self.assertEqual([v["hp_writes"] for v in miss["complete_world"]], [0, 0, 0])
        authored_miss = data["authored_native_controls"][1]
        self.assertTrue(authored_miss["stale_would_overlap"])
        self.assertFalse(authored_miss["current_overlaps"])

    def test_invalid_report_and_overwrite_refuse(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            moving.decode("world-receiver-open: 174 60 60\n")
        with TemporaryDirectory() as tmp:
            output = Path(tmp)/"keep.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                moving.export(None, None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
