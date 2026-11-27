"""Pin the reaction effect calls and their decoded arguments."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/reaction-effect-map.json"
SCAFFOLD = ROOT / "docs/api-map/reaction-scaffold-map.json"


class ReactionEffectMapTest(unittest.TestCase):
    def test_calls(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(len(data["calls"]), 29)
        calls = {call["site"]: call for call in data["calls"]}
        self.assertEqual(calls["8002F674"]["role"], "impact-sound")
        self.assertEqual(calls["8002F674"]["arguments"]["r5"]["hex"],
                         "803B0720")
        self.assertEqual(calls["8002F674"]["arguments"]["r6"]["offset"], 9)
        self.assertEqual(calls["8002F894"]["arguments"]["r6"]["offset"], 10)

    def test_effect_table(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["effects"]["8002FCBC"]["sound_id"], 38)
        self.assertEqual(data["effects"]["8002FCBC"]["dispatch"], "800F04B4")
        self.assertEqual(data["effects"]["8002FC18"]["dispatch"], "800F088C")
        self.assertEqual(data["effects"]["80019550"]["table"], "802C7ED0")
        self.assertEqual(data["effects"]["800DB250"]["role"], "effect-values")
        self.assertIn("803B0720", data["effects"]["800DB250"]["values"])
        self.assertEqual(data["effects"]["800300BC"]["modes"]["1"],
                         "receiver object +0x20 - attacker object +0x20")
        self.assertEqual(data["effects"]["8018214C"]["role"], "reaction-id")

    def test_constants_and_fallback(self):
        data = json.loads(ARTIFACT.read_text())
        constants = data["constants"]
        self.assertEqual(constants["80436F68"], 0.0)
        self.assertEqual(constants["80436F74"], -1.0)
        self.assertEqual(constants["80436FB8"], 1.5)
        self.assertEqual(constants["80436F94"], 2.5)
        self.assertEqual(constants["80436FBC"], 3.5)
        self.assertAlmostEqual(constants["80436FC0"], 0.01, places=6)
        self.assertIn("(0,0,-1)", data["effects"]["800300BC"]["fallback"])
        self.assertEqual(data["effects"]["8002FC18"]["thresholds"]["80436FBC"], 3.5)
        self.assertIn("803D3AE8", data["effects"]["800F088C"]["counter_row"])
        self.assertEqual(data["effects"]["8002F8DC"]["fields"][0], "actor+0x4AA")
        self.assertIn("every branch", data["effects"]["8002FCBC"]["size_branch"])

    def test_mode_four_reads_the_world_root_column(self):
        body = json.loads(ARTIFACT.read_text())["mode_4_body_root"]
        self.assertEqual(body["producer"], "80046BD0")
        self.assertEqual((body["matrix_offset"], body["copy_offset"]), (0x8D4, 0x114))
        self.assertEqual(body["positive_z_offsets"], [0x8D4 + i for i in (8, 24, 40)])
        self.assertEqual(body["calls"]["80046C68"], "80045838")

    def test_modes_zero_and_two_have_separate_motion_samples(self):
        motion = json.loads(ARTIFACT.read_text())["motion_inputs"]
        self.assertEqual((motion["object"]["current_offset"], motion["object"]["previous_offset"]), (0x20, 0x2C))
        self.assertEqual((motion["pair"]["current_offset"], motion["pair"]["previous_offset"]), (0x30, 0x3C))
        self.assertEqual(motion["pair"]["update_point"], "8008A888")
        self.assertTrue(motion["selection"]["object_vector_only_on_zero"])

    def test_modes_one_and_three_do_not_share_an_origin(self):
        positions = json.loads(ARTIFACT.read_text())["position_inputs"]
        self.assertFalse(positions["mode_1"]["uses_owner_link"])
        self.assertEqual(positions["mode_1"]["origin_offset"], 0x20)
        self.assertEqual(positions["mode_3"]["pair_centre_offset"], 0x64)
        self.assertEqual(positions["mode_3"]["ordinary_sphere_centre_update"], "8003A634")
        self.assertEqual(positions["nearest_contact_correction"]["pair_centre_add"], "8002DEB0")

    def test_effect_value_table(self):
        data = json.loads(ARTIFACT.read_text())
        values = data["effect_values"]
        self.assertEqual(values["table"], "8031227C")
        entries = values["entries"]
        self.assertEqual(len(entries), 9)
        self.assertEqual(entries[0]["definition"], 2)
        self.assertEqual(entries[0]["pointer"], "80312210")
        self.assertEqual(entries[1]["definition"], 1)
        self.assertEqual(entries[7]["pointer"], "80312270")
        self.assertEqual(entries[8]["definition"], 2)
        self.assertEqual(entries[8]["pointer"], "80312210")

    def test_scaffold_reaction_direction(self):
        data = json.loads(SCAFFOLD.read_text())
        calls = {call["site"]: call for call in data["calls"]}
        self.assertEqual(calls["8002F064"]["target"], "800300BC")
        self.assertEqual(calls["8002F064"]["role"], "reaction-direction")
        self.assertIn("+0x0E", calls["8002F064"]["note"])


if __name__ == "__main__":
    unittest.main()
