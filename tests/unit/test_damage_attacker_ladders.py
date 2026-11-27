"""Pin the outgoing table reads and their direct move scope."""
import json
import os
import struct
import tempfile
import unittest
from pathlib import Path

from tools.analyze import damage_attacker_ladders

DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"
ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/damage-attacker-ladders.json"


class DamageAttackerLaddersTest(unittest.TestCase):
    def test_outgoing_tables_and_pre_hit_probe(self):
        report = json.loads(ARTIFACT.read_bytes())
        hp, force = (report["roles"][key] for key in ("hp", "force"))
        self.assertEqual((hp["attack_selector_offset"], force["attack_selector_offset"]), (6, 7))
        self.assertEqual((hp["multiply"], force["multiply"]), ("8003CF48", "8003CFC0"))
        self.assertIn("pre-hit", report["hp_index"])
        self.assertIn("no subtraction", report["hp_index"])
        self.assertEqual(hp["sides"][0]["rows"][0]["cells"][-1], 1.16)
        self.assertEqual(force["sides"][1]["rows"][0]["cells"][-1], 1.16)

    def test_shared_tables_require_matching_selectors_and_positive_source_power(self):
        class Image:
            def read(self, address, size, kind):
                index, offset = divmod(address, 24)
                if offset == 0 and size == 4:
                    return struct.pack(">2H", (20, 30, 0, 40)[index], 99)
                if offset == 6 and size == 2:
                    return bytes((index == 3, 0))
                raise AssertionError("not a qualified attack operand")

        def route(*indices):
            return {"table": "0", "releases": [{"steps": [{"index": i} for i in indices]}]}

        report = json.loads(ARTIFACT.read_bytes())
        first = damage_attacker_ladders.profile_rows(report, Image(),
            {"ordinary": route(0, 1), "zero": route(2), "disagree": route(1, 3)})
        self.assertEqual(first["selectors"], [0, 0])
        self.assertEqual(len(first["lines"]), 5)
        self.assertEqual(first["lines"][-1], "damage-credit-ladders=ordinary")
        self.assertIsNone(first["moves"]["disagree"]["selectors"])
        self.assertIsNone(first["moves"]["zero"]["selectors"])
        mixed = damage_attacker_ladders.profile_rows(report, Image(), {"a": route(0), "b": route(3)})
        self.assertEqual(mixed["lines"], [])
        self.assertEqual(mixed["status"], "different source pairs in one package")

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regenerates_without_replacing_output(self):
        with tempfile.TemporaryDirectory() as scratch:
            output = Path(scratch) / ARTIFACT.name
            damage_attacker_ladders.export(DOL, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                damage_attacker_ladders.export(DOL, output)


if __name__ == "__main__":
    unittest.main()
