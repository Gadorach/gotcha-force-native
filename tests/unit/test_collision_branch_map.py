"""Pin the collision-walk conditional branch map."""
import json
import unittest
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/collision-branch-map.json"


class CollisionBranchMapTest(unittest.TestCase):
    def test_phases(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(len(data["branches"]), 98)
        self.assertEqual(data["phases"],
                         {"broad-phase": 10, "geometry": 14,
                          "apply": 27, "post": 47})

    def test_known_branch(self):
        data = json.loads(ARTIFACT.read_text())
        calls = {b["site"]: b for b in data["branches"]}
        branch = calls["8002BBA0"]
        self.assertEqual(branch["cond"], "eq")
        self.assertEqual(branch["sense"], "true")
        self.assertEqual(branch["condition"]["kind"], "extsb.")
        self.assertEqual(branch["target"], "8002BD2C")

    def test_all_conditions_decoded(self):
        data = json.loads(ARTIFACT.read_text())
        kinds = Counter(b["condition"]["kind"] for b in data["branches"])
        self.assertNotIn("unknown", kinds)
        self.assertEqual(kinds["fcmpo"], 28)
        self.assertEqual(kinds["cmpi"], 29)


if __name__ == "__main__":
    unittest.main()
