"""The three published contact artifacts must regenerate byte-identically.

The original DOL is a private local input, so the test skips when it is absent.
When present, it pins the exact bytes that were committed.
"""
import tempfile
import unittest
from pathlib import Path

from tools.analyze import (
    collision_walk_map,
    contact_scalar_ladder,
    contact_side_rows,
    damage_amount_model,
    gred_sasuke_contact_damage,
    gred_sasuke_move_bases,
    reaction_effect_map,
    reaction_scaffold_map,
)

ROOT = Path(__file__).resolve().parents[2]
DOL = Path("/home/gadorach/Projects/GotchaForce/original/extracted/sys/main.dol")
ARTIFACTS = (
    ("gred-sasuke-contact-damage.json", gred_sasuke_contact_damage.export),
    ("contact-scalar-ladder.json", contact_scalar_ladder.export),
    ("gred-sasuke-move-bases.json", gred_sasuke_move_bases.export),
    ("contact-side-rows.json", contact_side_rows.export),
    ("damage-amount-model.json", damage_amount_model.export),
    ("reaction-scaffold-map.json", reaction_scaffold_map.export),
    ("collision-walk-map.json", collision_walk_map.export),
    ("reaction-effect-map.json", reaction_effect_map.export),
)


@unittest.skipUnless(DOL.is_file(), "original DOL not available")
class PublishedArtifactReproduceTest(unittest.TestCase):
    def test_artifacts_regenerate_byte_identically(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            for artifact, export in ARTIFACTS:
                output = root / artifact
                export(DOL, output)
                self.assertEqual(
                    output.read_bytes(),
                    (ROOT / "docs/api-map" / artifact).read_bytes(),
                    artifact,
                )


if __name__ == "__main__":
    unittest.main()
