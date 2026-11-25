"""The live damage-path observer must stay read-only and keep its three entries."""
import unittest
from pathlib import Path

MOD = Path(__file__).resolve().parents[2] / "tools/analyze/damage_path_mod.c"


class DamagePathModTest(unittest.TestCase):
    def setUp(self):
        self.text = MOD.read_text()

    def test_hooks_the_damage_entries(self):
        for address in ("0x8002E660u", "0x8002F108u", "0x8002F110u", "0x8003D344u"):
            self.assertIn(f"RECOMP_HOOK({address}", self.text)

    def test_reads_model_id_and_hp(self):
        self.assertIn("address + 0x3E8u", self.text)
        self.assertIn("address + 0x1C6u", self.text)

    def test_is_read_only(self):
        self.assertNotIn("RECOMP_PATCH(", self.text)
        self.assertNotIn("moderngekko_mod_write", self.text)

    def test_has_three_part_version(self):
        self.assertIn('.version = "1.0.0"', self.text)


if __name__ == "__main__":
    unittest.main()
