"""Pin the pair-loop conditional branch map."""
import json
import unittest
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/pair-loop-branch-map.json"


class PairLoopBranchMapTest(unittest.TestCase):
    def test_phases(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(len(data["branches"]), 32)
        self.assertEqual(data["phases"],
                         {"prepare": 2, "inner": 20, "select": 4,
                          "publish": 6})

    def test_mode_and_carrier_flags(self):
        data = json.loads(ARTIFACT.read_text())
        calls = {b["site"]: b for b in data["branches"]}
        self.assertEqual(calls["8002DBC8"]["condition"]["text"], "r0 cmp 2")
        self.assertEqual(calls["8002DBC8"]["sense"], "false")
        self.assertEqual(calls["8002DC04"]["condition"]["text"],
                         "(r4 & 0x00000800)")

    def test_ctr_copies(self):
        data = json.loads(ARTIFACT.read_text())
        kinds = Counter(b["condition"]["kind"] for b in data["branches"])
        self.assertEqual(kinds["bdnz"], 2)
        self.assertNotIn("unknown", kinds)


if __name__ == "__main__":
    unittest.main()
