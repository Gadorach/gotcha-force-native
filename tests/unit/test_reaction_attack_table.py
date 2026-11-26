"""Pin the shared reaction attack table and its pointer chain."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/reaction-attack-table.json"


class ReactionAttackTableTest(unittest.TestCase):
    def test_prefix_and_entries(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["table"], "80354160")
        self.assertEqual(data["entry_size"], 24)
        self.assertEqual(data["count"], 64)
        self.assertEqual(data["validated_prefix"], 38)
        entries = {entry["index"]: entry for entry in data["entries"]}
        first = entries[0]
        self.assertEqual((first["mode"], first["flags"], first["yaw"],
                          first["pitch"], first["repeat"]), (0, 46, 1, 44, 0))
        self.assertEqual(entries[19]["mode"], 1)
        self.assertEqual(entries[22]["mode"], 3)
        self.assertTrue(all(entry["validated"] == (entry["index"] < 38)
                            for entry in data["entries"]))

    def test_pointer_chain(self):
        data = json.loads(ARTIFACT.read_text())
        chain = data["pointer_chain"]
        self.assertEqual(chain["actor_field"], "+0x27C")
        self.assertEqual(chain["pair_record"]["table"], "+0x2C")
        self.assertEqual(chain["pair_record"]["move"], "+0x11")
        self.assertEqual(chain["setters"]["8006F5EC"], "80354160")
        self.assertEqual(chain["pair_allocator"], "8008AC80")
        self.assertEqual(chain["pair_update"], "8008A65C")


if __name__ == "__main__":
    unittest.main()
