"""Keep HP base separate from the adjacent operand and reject ambiguous routes."""
import json
import os
import struct
import tempfile
import unittest
from pathlib import Path

from tools.analyze import damage_contact_power

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/damage-contact-power.json"
SOURCE = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted"))
DOL, AFS = SOURCE / "sys/main.dol", SOURCE / "files/afs_data.afs"


class DamageContactPowerTest(unittest.TestCase):
    def test_only_hp_base_cell_controls_power(self):
        class Image:
            @staticmethod
            def read(address, size, kind):
                return struct.pack(">2H", 30 if address in (24, 48) else 40, address + 10)

        def binding(*indices):
            return {"table": "0", "contacts": [{"steps": [{"index": index} for index in indices]}]}

        same = damage_contact_power.move_power(Image(), binding(1, 2, 1))
        self.assertEqual(same["base"], 30)
        self.assertEqual([row["separate_contact_operand"] for row in same["entries"]], [34, 58])
        self.assertIsNone(damage_contact_power.move_power(Image(), binding(1, 3))["base"])
        with self.assertRaisesRegex(ValueError, "index outside"):
            damage_contact_power.move_power(Image(), binding(64))

    def test_measured_examples_and_refusals(self):
        report = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(report["source"]["base_load"], "8003CD88")
        rows = report["projectiles"]
        self.assertEqual((rows["0615"]["base"], rows["000a"]["base"]), (20, 5))
        authored = b"move=basic-shot|b|10|grounded|24|0:0\n"
        self.assertIsNone(rows["0501"]["base"])
        self.assertEqual(report["close_b"]["0001"]["base"], 40)
        self.assertEqual([row["hp_base"] for row in report["close_b"]["0202"]["entries"]], [30, 40])
        self.assertIsNone(report["close_b"]["0202"]["base"])
        self.assertEqual(damage_contact_power.append(authored, {}, None), authored)

    def test_floor_is_independent_of_a_single_uniform_hp_base(self):
        class Image:
            def read(self, address, count, kind):
                return struct.pack(">2H", (30, 40, 0, 10001)[address // 24], 400)

        def route(*indices):
            return {"table": "0", "contacts": [{"steps": [{"index": i} for i in indices]}]}

        receipt = {"reaction": {"mixed": route(0, 1), "includes-zero": route(0, 2),
                               "out-of-range": route(3)}}
        self.assertEqual(damage_contact_power.append_floor(b"", receipt, Image(), minimum=1),
                         b"contact-minimum=mixed|1\n")
        self.assertEqual(receipt["contact_floor_test"]["mixed"]["source_bases"], [30, 40])
        self.assertIsNone(receipt["contact_floor_test"]["includes-zero"]["minimum"])
        with self.assertRaises(ValueError):
            damage_contact_power.append_floor(b"", receipt, Image(), minimum=2)

    @unittest.skipUnless(DOL.is_file() and AFS.is_file(), "original DOL/AFS not available")
    def test_regenerates_without_overwriting(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / ARTIFACT.name
            damage_contact_power.export(DOL, AFS, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                damage_contact_power.export(DOL, AFS, output)


if __name__ == "__main__":
    unittest.main()
