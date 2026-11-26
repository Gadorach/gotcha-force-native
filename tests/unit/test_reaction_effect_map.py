"""Pin the reaction effect calls and their decoded arguments."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/reaction-effect-map.json"


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


if __name__ == "__main__":
    unittest.main()
