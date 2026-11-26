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
        self.assertEqual(calls["8002F674"]["role"], "impact-effect")
        self.assertEqual(calls["8002F674"]["arguments"]["r5"]["hex"],
                         "803B0720")
        self.assertEqual(calls["8002F674"]["arguments"]["r6"]["offset"], 9)
        self.assertEqual(calls["8002F894"]["arguments"]["r6"]["offset"], 10)

    def test_effect_table(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["effects"]["8002FCBC"]["opcode"], "0x26")
        self.assertEqual(data["effects"]["8002FCBC"]["dispatch"], "800F04B4")
        self.assertEqual(data["effects"]["8002FC18"]["dispatch"], "800F088C")
        self.assertEqual(data["effects"]["80019550"]["table"], "802C7ED0")
        self.assertEqual(data["effects"]["800DB250"]["role"], "effect-values")
        self.assertIn("803B0720", data["effects"]["800DB250"]["values"])
        self.assertEqual(data["effects"]["800300BC"]["modes"]["1"],
                         "target+0x20 - attacker+0x20")
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
        self.assertIn("every branch", data["effects"]["8002FCBC"]["volume_branch"])

    def test_scaffold_reaction_direction(self):
        data = json.loads(SCAFFOLD.read_text())
        calls = {call["site"]: call for call in data["calls"]}
        self.assertEqual(calls["8002F064"]["target"], "800300BC")
        self.assertEqual(calls["8002F064"]["role"], "reaction-direction")
        self.assertIn("+0x0E", calls["8002F064"]["note"])


if __name__ == "__main__":
    unittest.main()
