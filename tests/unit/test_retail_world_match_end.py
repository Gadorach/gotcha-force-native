"""Do not equate KO, Force exhaustion, inactive seats and match-end return."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_world_match_end as end


RECEIPT = Path(__file__).resolve().parents[2]/"docs/api-map/gg4e08-world-match-end-frontier.json"


class WorldMatchEndTest(unittest.TestCase):
    def test_same_lethal_hp_write_need_not_select_a_scene_result(self):
        report = json.loads(RECEIPT.read_bytes())
        _, no_debit, exhausts = report["original_cases"]
        for row in (no_debit, exhausts):
            self.assertEqual((row["hp_removed"], row["force_at_hp_leaf_return"], row["result_at_hp_leaf_return"]),
                             (1, 90, 0))
            self.assertEqual(row["completed_dispatches"][0]["hp_write_request"], 14)
        result = exhausts["completed_dispatches"][0]
        self.assertEqual((result["side_1_force"], result["result_mask"], result["ordinary_receiver_descriptor_mask"]),
                         (0, 1, 6))
        inactive = no_debit["completed_dispatches"][-1]
        self.assertEqual((inactive["side_1_force"], inactive["result_mask"], inactive["old_shape_cells_returned_mask"]),
                         (90, 0, 7))
        self.assertFalse(inactive["receiver_active"])
        self.assertTrue(inactive["old_archive_owner_and_wrappers_retained"])

    def test_ending_frontier_is_not_a_return(self):
        frontier = json.loads(RECEIPT.read_bytes())["ending_frontier"]
        self.assertEqual((frontier["visit_after_scene_open"], frontier["pc"], frontier["refused_external_read"]),
                         (28, "800F0FDC", "00000766"))
        self.assertFalse(frontier["ending_dispatch_return_observed"])
        self.assertFalse(frontier["match_end_return_observed"])
        self.assertFalse(frontier["save_behavior_qualified"])

    def test_incomplete_report_and_existing_output_refuse(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            end.decode("world-end-open: 174 90 90 1\n")
        with TemporaryDirectory() as tmp:
            path = Path(tmp)/"result.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                end.export(None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
