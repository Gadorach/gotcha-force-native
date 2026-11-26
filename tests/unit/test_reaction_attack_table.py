"""Pin the shared reaction attack table and its pointer chain."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/reaction-attack-table.json"


class ReactionAttackTableTest(unittest.TestCase):
    def test_per_borg_tables(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["revision"], 3)
        self.assertEqual(data["entry_size"], 24)
        self.assertEqual(data["count"], 64)
        self.assertEqual(len(data["models"]), 42)
        self.assertEqual(len(data["tables"]), 22)
        default = data["default_table"]
        self.assertEqual(default["table"], "80354160")
        self.assertEqual(len(default["entries"]), 64)
        models = {model["model"]: model for model in data["models"]}
        self.assertEqual(models["0615"]["table"], "80365EC0")
        self.assertEqual(models["0615"]["installer"], "8018CCFC")
        self.assertEqual(models["000A"]["table"], "802D4160")
        self.assertEqual(models["061E"]["table"], "80344C88")
        gred = {entry["index"]: entry for entry in data["tables"]["80365EC0"]}
        self.assertEqual((gred[9]["mode"], gred[9]["yaw"], gred[9]["pitch"]),
                         (4, 0, 56))
        sasuke = {entry["index"]: entry for entry in data["tables"]["802D4160"]}
        self.assertEqual((sasuke[9]["mode"], sasuke[9]["pitch"]), (4, 24))

    def test_pointer_chain(self):
        data = json.loads(ARTIFACT.read_text())
        chain = data["pointer_chain"]
        self.assertEqual(chain["actor_field"], "+0x27C")
        self.assertEqual(chain["pair_record"]["table"], "+0x2C")
        self.assertEqual(chain["pair_record"]["move"], "+0x11")
        self.assertEqual(chain["installer_table"], "802D3224")
        self.assertEqual(chain["default_setter"], "8006F5EC")
        self.assertEqual(chain["pair_allocator"], "8008AC80")
        self.assertEqual(chain["pair_update"], "8008A65C")


if __name__ == "__main__":
    unittest.main()
