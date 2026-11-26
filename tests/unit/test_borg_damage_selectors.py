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

    def test_combined_models(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["combined_count"], 13)
        self.assertEqual(data["combined_models"]["0604"],
                         [[3, 2, 2], [3, 2, 2]])
        self.assertEqual(data["combined_models"]["0802"],
                         [[0, 2, 2], [0, 2, 2]])

    def test_models_without_block(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(data["models_without_block"],
                         ["0619", "061b", "061f", "0620", "0628",
                          "0803", "080b", "0f07"])

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
