"""Pin the two source receiver ladders and their distinct inputs."""
import json
import os
import tempfile
import unittest
from pathlib import Path

from tools.analyze import damage_receiver_ladders

DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"
ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/damage-receiver-ladders.json"


class DamageReceiverLaddersTest(unittest.TestCase):
    def test_no_second_hp_lookup(self):
        report = json.loads(ARTIFACT.read_bytes())
        hp, force = [report["roles"][key] for key in ("hp", "force")]
        self.assertEqual((hp["actor_selector_offset"], force["actor_selector_offset"]), (157, 158))
        self.assertEqual((hp["multiply"], force["multiply"]), ("8003D1EC", "8003D260"))
        self.assertIn("raw record+0", report["hp_index"])
        self.assertIn("min(max HP,200)", report["hp_index"])
        self.assertIn("force cap", report["force_index"])
        self.assertEqual(hp["sides"][1]["rows"][1]["cells"][30], 0.98)
        self.assertEqual(force["sides"][1]["rows"][0]["cells"][31], 0.93)

    def test_separate_selectors_emit_both_complete_team_pairs(self):
        report = json.loads(ARTIFACT.read_bytes())
        rows = damage_receiver_ladders.profile_rows(report, [1, 1, 0])
        self.assertEqual([line.split("|")[0] for line in rows], [
            "damage-receive-hp=0", "damage-receive-hp=1", "damage-receive-force=0", "damage-receive-force=1"])
        self.assertTrue(all(len(line.split("|")) == 33 for line in rows))
        self.assertEqual(damage_receiver_ladders.profile_rows(report, None), [])
        with self.assertRaisesRegex(ValueError, "outside verified table"):
            damage_receiver_ladders.profile_rows(report, [1, 1, 8])

    def test_divide_magic_matches_the_entire_short_hp_domain(self):
        for probe in range(201):
            value = probe * 32
            self.assertEqual((value * 0x51EB851F) >> 38, value // 200)

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regenerates_without_replacing_output(self):
        with tempfile.TemporaryDirectory() as scratch:
            output = Path(scratch) / ARTIFACT.name
            damage_receiver_ladders.export(DOL, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                damage_receiver_ladders.export(DOL, output)


if __name__ == "__main__":
    unittest.main()
