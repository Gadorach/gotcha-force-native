"""Check the setting-7 damage content snapshot and exact receiver selection."""
import json
import os
import tempfile
import unittest
from pathlib import Path

from tools.analyze import damage_contact_roles
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/damage-contact-roles.json"
SELECTORS = ROOT / "docs/api-map/borg-damage-selectors.json"
DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"


class DamageContactRolesTest(unittest.TestCase):
    def test_normal_receiver_read_matches_exact_spec(self):
        report = json.loads(ARTIFACT.read_bytes())
        selectors = json.loads(SELECTORS.read_bytes())
        actor = bytearray(432)
        actor[156:159] = bytes((1, 1, 0))
        roles = damage_contact_roles.profile_rows(report, selectors, 0x000a, actor)
        self.assertEqual((roles["settings"], roles["rank"], roles["receiver_selector"]), (7, 0, [1, 1, 0]))
        self.assertEqual(roles["lines"][1:], [
            "damage-credit=0|1.0|0.85", "damage-credit=1|0.9|0.85",
            "damage-receive=0|0.92|1.15", "damage-receive=1|1.04|1.15",
        ])
        actor[156] = 2
        with self.assertRaisesRegex(ValueError, "selector mismatch"):
            damage_contact_roles.profile_rows(report, selectors, 0x000a, actor)

    def test_unqualified_components_only_get_attacker_credit(self):
        report = json.loads(ARTIFACT.read_bytes())
        selectors = json.loads(SELECTORS.read_bytes())
        for model, actor in ((0x061e, bytes(864)), (0x061e, bytes(432)), (0x000a, bytes(864))):
            roles = damage_contact_roles.profile_rows(report, selectors, model, actor)
            self.assertIsNone(roles["receiver_selector"])
            self.assertIsNone(roles["matchup_group"])
            self.assertTrue(all(line.startswith("damage-credit=") for line in roles["lines"][1:]))

    def test_matchup_matrix_is_sparse_and_directional(self):
        report = json.loads(ARTIFACT.read_bytes())["matchup"]
        rows = report["nonneutral_rows"]
        self.assertEqual(report["order"], ["receiver", "attacker"])
        self.assertEqual(report["dimensions"], [20, 20])
        self.assertEqual(sum(len(row) for row in rows.values()), 26)
        self.assertEqual(rows["gg4e08-class-19"]["gg4e08-class-4"], 1.1)
        self.assertEqual(rows["gg4e08-class-4"]["gg4e08-class-19"], 0.9)
        self.assertEqual(rows["gg4e08-class-15"]["gg4e08-class-12"], 0.75)
        self.assertEqual(rows["gg4e08-class-12"]["gg4e08-class-15"], 1.25)
        self.assertNotIn("gg4e08-class-4", rows["gg4e08-class-0"])

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_matchup_is_resolved_from_a_qualified_normal_record(self):
        with ReadOnlyFile(DOL) as file:
            image = DolImage(file.read(0, file.size), SOURCE_SHA256)
        report = json.loads(ARTIFACT.read_bytes())
        selectors = json.loads(SELECTORS.read_bytes())
        actor = bytearray(432)
        actor[156:159] = bytes(selectors["models"]["0105"])
        roles = damage_contact_roles.profile_rows(report, selectors, 0x0105, actor, image=image)
        self.assertEqual(roles["matchup_group"], "gg4e08-class-19")
        self.assertIn("damage-matchup-from=gg4e08-class-4|1.1", roles["lines"])
        self.assertIsNone(damage_contact_roles.profile_rows(report, selectors, 0x0105, actor * 2,
                                                            image=image)["matchup_group"])

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regenerates_without_replacing_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / ARTIFACT.name
            damage_contact_roles.export(DOL, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                damage_contact_roles.export(DOL, output)


if __name__ == "__main__":
    unittest.main()
