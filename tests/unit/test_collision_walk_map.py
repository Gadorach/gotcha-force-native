"""Pin the collision walk call map and its pair-loop boundary."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/collision-walk-map.json"


class CollisionWalkTest(unittest.TestCase):
    def test_phases(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["phases"],
                         {"broad-phase": 14, "geometry": 7, "apply": 24,
                          "post": 23})
        self.assertEqual(len(data["calls"]), 68)

    def test_pair_loop_boundary(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["pair_loop"]["call"], "8002C4CC")
        self.assertEqual(data["pair_loop"]["target"], "8002DB58")
        self.assertEqual(data["pair_loop"]["reaction"], "8002E2A8")
        calls = {call["site"]: call for call in data["calls"]}
        self.assertEqual(calls["8002C4CC"]["role"], "pair-loop")
        self.assertEqual(calls["8002C4CC"]["phase"], "apply")

    def test_pair_loop_targeting(self):
        data = json.loads(ARTIFACT.read_text())
        loop = data["pair_loop"]
        self.assertEqual(loop["outer"]["array"], "803C477C")
        self.assertEqual(loop["inner"]["array"], "803C417C")
        self.assertIn("400000000.0", loop["mode"]["nearest"])
        self.assertEqual(len(loop["filters"]), 6)
        self.assertEqual(loop["nearest_distance"], "8020BC88")

    def test_post_band_has_no_pair_loop(self):
        data = json.loads(ARTIFACT.read_text())
        roles = {call["role"] for call in data["calls"]
                 if call["phase"] == "post"}
        self.assertNotIn("pair-loop", roles)


if __name__ == "__main__":
    unittest.main()
