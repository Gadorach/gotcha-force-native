"""Pin the pilot action-group -> step -> reaction-table map."""
import json
import tempfile
import unittest
from pathlib import Path

from tools.analyze import reaction_action_map

ROOT = Path(__file__).resolve().parents[2]
DOL = Path("/home/gadorach/Projects/GotchaForce/original/extracted/sys/main.dol")
AFS = Path("/home/gadorach/Projects/GotchaForce/original/extracted/files/afs_data.afs")
ARTIFACT = ROOT / "docs/api-map/reaction-action-map.json"


class ReactionActionMapTest(unittest.TestCase):
    def test_pilot_groups(self):
        data = json.loads(ARTIFACT.read_text())
        models = {model["model"]: model for model in data["models"]}
        self.assertEqual(set(models), {"0615", "000a", "061e"})
        gred = models["0615"]
        self.assertEqual(gred["groups"][9]["steps"], [8, 48])
        self.assertEqual(gred["steps"]["8"]["table"], 9)
        self.assertEqual(gred["steps"]["8"]["mode"], 2)
        self.assertEqual(gred["steps"]["8"]["yaw"], 2)
        self.assertEqual(gred["steps"]["8"]["pitch"], 24)
        sasuke = models["000a"]
        self.assertEqual(sasuke["groups"][9]["steps"], [10])
        self.assertEqual(sasuke["steps"]["10"]["table"], 9)
        self.assertEqual(sasuke["steps"]["10"]["mode"], 2)

    @unittest.skipUnless(DOL.is_file() and AFS.is_file(),
                         "original DOL/AFS not available")
    def test_regenerates_byte_identically(self):
        with tempfile.TemporaryDirectory() as name:
            output = Path(name) / "reaction-action-map.json"
            reaction_action_map.export(DOL, AFS, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())


if __name__ == "__main__":
    unittest.main()
