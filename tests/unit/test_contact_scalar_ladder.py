"""Pin the published contact scalar ladder and its observed indices."""
import json
import unittest
from pathlib import Path

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/contact-scalar-ladder.json"


class ContactScalarLadderTest(unittest.TestCase):
    def setUp(self):
        self.data = json.loads(ARTIFACT.read_text())

    def test_ladder_shape(self):
        entries = self.data["entries"]
        self.assertEqual(self.data["count"], 32)
        self.assertEqual(len(entries), 32)
        self.assertAlmostEqual(entries[0], 0.97, places=5)
        self.assertAlmostEqual(entries[31], 2.0, places=5)

    def test_observed_indices(self):
        entries = self.data["entries"]
        self.assertAlmostEqual(entries[16], 1.13, places=5)
        self.assertAlmostEqual(entries[30], 1.4, places=5)

    def test_observations_are_marked(self):
        observations = {item["state"]: item for item in self.data["settings"]["observations"]}
        self.assertTrue(observations["kakeru_battle"]["verified"])
        self.assertEqual(observations["kakeru_battle"]["hit"]["amount"], 22)
        self.assertTrue(observations["mana_combat_hud"]["verified"])
        self.assertEqual(observations["mana_combat_hud"]["hit"]["amount"], 3)


if __name__ == "__main__":
    unittest.main()
