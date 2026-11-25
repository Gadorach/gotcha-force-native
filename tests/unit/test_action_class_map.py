"""Pin the two-level action-class map."""
import json
import unittest
from pathlib import Path

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/retail-action-class-map.json"


class ActionClassMapTest(unittest.TestCase):
    def setUp(self):
        self.data = json.loads(ARTIFACT.read_text())

    def test_family_entries(self):
        families = {entry["class"]: entry for entry in self.data["family_table"]["entries"]}
        self.assertEqual(families[0]["handler"], "8006FB08")
        self.assertEqual(families[1]["handler"], "80070604")
        self.assertEqual(families[2]["handler"], "80071598")
        self.assertGreaterEqual(
            sum(1 for entry in families.values() if entry.get("status") == "matched"), 20)

    def test_subtables(self):
        names = {table["name"] for table in self.data["subtables"]}
        self.assertEqual(names, {"b-shot", "b-close-default", "b-close-model-000a",
                                 "x", "class-3", "class-4"})
        for table in self.data["subtables"]:
            self.assertTrue(table["entries"])


if __name__ == "__main__":
    unittest.main()
