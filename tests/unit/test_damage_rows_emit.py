"""Pin the recovered test-battle damage row emitted for basic shots."""
import unittest
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_roster import basic_shot_damage_lines
from tools.extract.source_image import DolImage, ReadOnlyFile

DOL = Path("/home/gadorach/Projects/GotchaForce/original/extracted/sys/main.dol")
SELECTORS = Path(__file__).resolve().parents[2] / "docs/api-map/borg-damage-selectors.json"


@unittest.skipUnless(DOL.is_file() and SELECTORS.is_file(), "original DOL/selectors not available")
class DamageRowsEmitTest(unittest.TestCase):
    def test_pilot_rows(self):
        with ReadOnlyFile(DOL) as source:
            image = DolImage(source.read(0, source.size), SOURCE_SHA256)
            expected = [
                "# authored test damage-vs: attacker settings 0, defender settings 16; not universal battle state",
                "damage-factors=1.0|1.0|1.0|1.0",
                "damage-vs=10|0.867|1.13|1.15|1.0",
            ]
            for model in (0x0615, 0x000A, 0x061E):
                self.assertEqual(basic_shot_damage_lines(image, model), expected)


if __name__ == "__main__":
    unittest.main()
