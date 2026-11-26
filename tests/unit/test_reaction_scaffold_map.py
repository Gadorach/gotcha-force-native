"""Pin the reaction scaffold call map and its decoder."""
import json
import struct
import unittest
from pathlib import Path

from tools.analyze import reaction_scaffold_map as map_tool

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/reaction-scaffold-map.json"


class ReactionScaffoldTest(unittest.TestCase):
    def test_direct_call_decoder(self):
        code = struct.pack(">I", 0x48001001)
        self.assertEqual(map_tool.decode_calls(code, 0x80001000),
                         [{"site": "80001000", "target": "80002000"}])

    def test_force_ledger_note(self):
        data = json.loads(ARTIFACT.read_text())
        calls = {call["site"]: call for call in data["calls"]}
        self.assertEqual(calls["8002F200"]["role"], "force-ledger-update")
        self.assertIn("team force", calls["8002F200"]["note"])

    def test_artifact_phases(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["phases"],
                         {"pre-amount": 16, "amount": 2, "reaction": 29})
        self.assertEqual(len(data["calls"]), 47)

    def test_amount_boundary(self):
        data = json.loads(ARTIFACT.read_text())
        calls = {call["site"]: call for call in data["calls"]}
        self.assertEqual(calls["8002F108"]["role"], "amount")
        self.assertEqual(calls["8002F1D8"]["role"], "hp-write")
        self.assertEqual(data["boundary"]["amount_call"], "8002F108")
        self.assertEqual(data["boundary"]["hp_write"], "8002F1D8")

    def test_reaction_band_is_presentation(self):
        data = json.loads(ARTIFACT.read_text())
        roles = {call["role"] for call in data["calls"]
                 if call["phase"] == "reaction"}
        self.assertIn("impact-effect", roles)
        self.assertIn("object-spawn", roles)
        self.assertNotIn("amount", roles)


if __name__ == "__main__":
    unittest.main()
