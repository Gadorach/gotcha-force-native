"""Keep team eligibility, geometric overlap and HP requests independent."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_world_team_pair as team


RECEIPT = Path(__file__).resolve().parents[2]/"docs/api-map/gg4e08-world-team-pair.json"


class WorldTeamPairTest(unittest.TestCase):
    def test_ally_can_miss_geometrically_or_take_damage(self):
        miss, hit = json.loads(RECEIPT.read_bytes())["original_cases"]
        case = miss["pair_controls"][0]
        self.assertEqual(case["eligible_mask"] & 12, 12)
        self.assertEqual(case["geometry_overlap_mask"] & 12, 0)
        first, next_visit, returned = hit["completed_world_observations"]
        self.assertEqual([(row["recipient"], row["removed"]) for row in first["hp_writes"]], [(1, 17), (2, 4)])
        self.assertEqual(first["accepted_token_by_seat"], [0, 1, 1])
        self.assertEqual(next_visit["hp_writes"], [])
        self.assertEqual(returned["published_attacks"], 0)
        self.assertEqual(hit["same_strike_cell_returned_visit"], 32)

    def test_team_edit_does_not_change_geometry(self):
        case = json.loads(RECEIPT.read_bytes())["original_cases"][1]
        stock, both_enemies, both_allies, excluded = case["pair_controls"]
        self.assertTrue(all(c["geometry_overlap_mask"] == stock["geometry_overlap_mask"] for c in case["pair_controls"]))
        self.assertEqual((both_enemies["attacker_opponent_damage_credit"], both_allies["attacker_ally_damage_credit"]),
                         (34, 8))
        self.assertEqual((excluded["eligible_mask"], excluded["narrow_attempt_mask"]), (12, 8))
        self.assertEqual(excluded["hp_by_seat"], [60, 43, 60])

    def test_partial_report_and_existing_output_refuse(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            team.decode("world-team-open: 174 1 240 3 6\n")
        with TemporaryDirectory() as tmp:
            out = Path(tmp)/"already.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                team.export(None, None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
