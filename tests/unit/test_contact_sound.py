"""Check the source sound selectors without substituting an unproved hit route."""
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest

from tools.analyze import contact_sound

DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"
ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/contact-sound.json"


class ContactSoundTest(unittest.TestCase):
    def setUp(self):
        self.report = json.loads(ARTIFACT.read_bytes())

    def test_pairs_and_fixed_code(self):
        pairs = lambda code: contact_sound.normal_pairs(self.report, code)
        self.assertEqual(pairs(0x80), [(0, 48), (1, 50), (2, 52), (3, 54)])
        self.assertEqual(pairs(0x84), [(4, 56), (5, 58), (6, 60), (7, 62)])
        self.assertEqual(pairs(0x88), [(8, 64), (9, 66), (10, 68), (11, 70)])
        self.assertEqual(pairs(0), [(0, 48)] * 4)
        self.assertEqual(pairs(0xC0), [])

    def test_banded_size_requires_closed_unanimous_configurations(self):
        class Image:
            def read(self, address, length, kind):
                self.args = (address, length, kind)
                return struct.pack(">2f", 100, {76: 420, 176: 421, 276: 110}[address])

        binding = {"status": "one-static-slot-0-record", "selected_record": 0}
        def configs(*pointers):
            return {"profiles": [{"config": f"{p:x}"} for p in pointers]}
        get = lambda report, size=432: contact_sound.receiver_band(self.report, Image(), report, binding, size)
        self.assertEqual(get(configs(0, 100))["band"], 2)
        self.assertIsNone(get(configs(0, 200))["band"])
        self.assertIsNone(get({**configs(0), "open_path_boundaries": 1})["band"])
        self.assertIsNone(get(configs(0), 864)["band"])

    def test_routes_require_agreement_and_refuse_special_status(self):
        class Image:
            def read(self, address, count, kind):
                index, at = divmod(address, 24)
                row = bytearray(24)
                row[1] = 0 if index == 5 else 20
                row[9] = 1
                row[10] = (0x80, 0x80, 0x84, 0xC0, 0x80, 0x80)[index]
                if index == 4:
                    struct.pack_into(">H", row, 18, 0x0020)
                return row[at:at + count]

        def route(*indices):
            return {"table": "0", "contacts": [{"steps": [{"index": i} for i in indices]}]}
        use = lambda *indices: contact_sound.move_sound(Image(), route(*indices))
        self.assertEqual(use(0, 1)["code"], 0x80)
        for indices in ((0, 2), (3,), (4,), (5,)):
            self.assertIsNone(use(*indices)["code"])

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regeneration_and_refusal_to_replace(self):
        with tempfile.TemporaryDirectory() as name:
            out = Path(name) / ARTIFACT.name
            contact_sound.export(DOL, out)
            self.assertEqual(out.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                contact_sound.export(DOL, out)


if __name__ == "__main__":
    unittest.main()
