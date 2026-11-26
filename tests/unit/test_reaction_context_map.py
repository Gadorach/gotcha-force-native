"""Pin the pilot move-context -> reaction-group map."""
import json
import tempfile
import unittest
from pathlib import Path

from tools.analyze import reaction_context_map

ROOT = Path(__file__).resolve().parents[2]
DOL = Path("/home/gadorach/Projects/GotchaForce/original/extracted/sys/main.dol")
AFS = Path("/home/gadorach/Projects/GotchaForce/original/extracted/files/afs_data.afs")
ROSTER = ROOT / "local/retail-roster-basic-shot-test-v1"
ARTIFACT = ROOT / "docs/api-map/reaction-context-map.json"


class ReactionContextMapTest(unittest.TestCase):
    def test_gred_b_shot(self):
        data = json.loads(ARTIFACT.read_text())
        models = data["models"]
        gred = models["0615"]
        self.assertEqual(gred["table"], "80365EC0")
        b_shot = next(action for action in gred["actions"]
                      if action["group"] == "B-shot")
        context = next(entry for entry in b_shot["contexts"]
                       if entry["context"] == [1, 0])
        self.assertEqual(context["group"], 1)
        self.assertEqual([(step["table"], step["mode"], step["pitch"])
                          for step in context["steps"]], [(2, 4, 24)])
        sasuke = models["000a"]
        close = next(action for action in sasuke["actions"]
                     if action["group"] == "B-shot")
        context = next(entry for entry in close["contexts"]
                       if entry["context"] == [2, 0])
        self.assertEqual([(step["table"], step["mode"], step["pitch"])
                          for step in context["steps"]], [(2, 4, 24)])

    @unittest.skipUnless(DOL.is_file() and AFS.is_file() and ROSTER.is_dir(),
                         "original DOL/AFS or roster not available")
    def test_regenerates_byte_identically(self):
        with tempfile.TemporaryDirectory() as name:
            output = Path(name) / "reaction-context-map.json"
            reaction_context_map.export(DOL, AFS, ROSTER, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())


if __name__ == "__main__":
    unittest.main()
