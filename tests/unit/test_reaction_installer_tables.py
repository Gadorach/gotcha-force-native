"""Pin the per-model installer -> reaction table map."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/reaction-installer-tables.json"


class ReactionInstallerTablesTest(unittest.TestCase):
    def test_models(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["revision"], 1)
        self.assertEqual(data["installer_table"], "802D3224")
        self.assertEqual(len(data["models"]), 42)
        self.assertTrue(all(model["table"] for model in data["models"]))
        models = {model["model"]: model for model in data["models"]}
        self.assertEqual((models["0615"]["installer"], models["0615"]["table"]),
                         ("8018CCFC", "80365EC0"))
        self.assertEqual((models["000A"]["installer"], models["000A"]["table"]),
                         ("8006F4F8", "802D4160"))
        self.assertEqual((models["061E"]["installer"], models["061E"]["table"]),
                         ("8015A494", "80344C88"))
        self.assertEqual(models["0900"]["table"], "802D6930")


if __name__ == "__main__":
    unittest.main()
