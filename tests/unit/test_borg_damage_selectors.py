"""Pin the per-Borg damage selectors and their artifact."""
import json
import tempfile
import unittest
from pathlib import Path

from tools.analyze import borg_damage_selectors

ROOT = Path(__file__).resolve().parents[2]
AFS = Path("/home/gadorach/Projects/GotchaForce/original/extracted/files/afs_data.afs")
ARTIFACT = ROOT / "docs/api-map/borg-damage-selectors.json"


class SelectorArtifactTest(unittest.TestCase):
    def test_known_models(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["count"], 198)
        self.assertEqual(data["models"]["000a"], [1, 1, 0])
        self.assertEqual(data["models"]["0615"], [2, 2, 2])
        self.assertEqual(data["models"]["030a"], [1, 2, 0])

    def test_offsets(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["offsets"]["class"], 156)
        self.assertEqual(data["offsets"]["hp_157"], 157)
        self.assertEqual(data["offsets"]["hp_158"], 158)


@unittest.skipUnless(AFS.is_file(), "original AFS not available")
class SelectorReproduceTest(unittest.TestCase):
    def test_regenerates_byte_identically(self):
        with tempfile.TemporaryDirectory() as name:
            output = Path(name) / "selectors.json"
            borg_damage_selectors.export(AFS, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())


if __name__ == "__main__":
    unittest.main()
