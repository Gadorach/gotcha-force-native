"""Pin the two measured G-Red move bases against Sasuke."""
import json
import unittest
from pathlib import Path

from tools.analyze import gred_sasuke_move_bases as bases

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gred-sasuke-move-bases.json"


class MoveBasesTest(unittest.TestCase):
    def test_amounts(self):
        self.assertEqual(bases.amount_for(20)[0], 22)
        self.assertEqual(bases.amount_for(30)[0], 33)

    def test_artifact(self):
        data = json.loads(ARTIFACT.read_text())
        moves = {move["group"]: move for move in data["moves"]}
        self.assertEqual(moves["B-close/boost"]["base"], 20)
        self.assertEqual(moves["B-close/boost"]["amount"], 22)
        self.assertEqual(moves["X"]["base"], 30)
        self.assertEqual(moves["X"]["amount"], 33)
        self.assertEqual(moves["X"]["config_offset"], "590")
        self.assertTrue(data["retail_behaviour_verified"])
        self.assertFalse(data["generalises_to_other_pairs"])


if __name__ == "__main__":
    unittest.main()
