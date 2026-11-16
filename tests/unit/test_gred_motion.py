import hashlib
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.gred_motion import encode, export_grids, export_profile


def bundle(parts):
    raw = bytearray(16 + len(parts)*8)
    struct.pack_into(">4I", raw, 0, 0x47464E42, 1, len(parts), 0)
    for index, part in enumerate(parts):
        struct.pack_into(">2I", raw, 16 + index*8, index, len(part))
        raw.extend(part)
    struct.pack_into(">I", raw, 12, len(raw)-16)
    return bytes(raw)


class GRedMotionTests(unittest.TestCase):
    def test_profile_layout_preserves_source_numeric_extents(self):
        class Source:
            sha256 = SOURCE_SHA256
            def read(self, address, size):
                return bytes([address & 255])*size
        data = bytes(range(216))*2
        config = bytearray(192);struct.pack_into(">I", config, 8, 12)
        config[0x44:0x58] = bytes(range(20))
        setup = bytearray(1064);setup[864:876] = struct.pack(">3f", 1, 2, 3)
        with patch("tools.extract.gred_motion.qualify_setup", return_value=(data, config)):
            raw = encode(Source(), setup)
            self.assertEqual(len(raw), 376)
            self.assertEqual(struct.unpack_from(">IHHIII", raw),
                              (0x47464D4D, 9, 0x615, 0xE2B4439A, 376, 12))
            self.assertEqual(raw[32:56], data[0x18:0x30])
            self.assertEqual(raw[92:104], setup[864:876])
            self.assertEqual(raw[104:124], config[0x44:0x58])
            self.assertEqual(raw[212:260], data[0xF0:0x120])
            self.assertEqual(raw[308:320], bytes([0xF0])*4 + bytes([8])*4 + bytes([12])*4)
            self.assertEqual(raw[320:336], bytes([0x68])*4 + bytes([0x6C])*4 + bytes([0x24])*4 + bytes([0x28])*4)
            self.assertEqual(raw[336:340], data[0x42:0x44] + data[0x3E:0x3F] + b"\0")
            self.assertEqual(raw[340:348], bytes([0x34])*4 + bytes([0x58])*4)
            self.assertEqual(raw[348:352], config[0xAC:0xB0])
            self.assertEqual(raw[352:356], bytes([0xF8])*4)
            self.assertEqual(raw[356:360], bytes([0xD4])*4)
            self.assertEqual(raw[360:364], bytes([0x1C])*4)
            self.assertEqual(raw[364:368], bytes([0x2C])*4)
            self.assertEqual(raw[368:372], bytes([0x48])*4)
            self.assertEqual(raw[372:376], bytes([0x70])*4)
            config[1] = 2
            with self.assertRaises(ValueError):
                encode(Source(), setup)
        wrong = Source();wrong.sha256 = "wrong"
        with self.assertRaises(ValueError):
            encode(wrong, setup)

    def test_three_grid_export_is_read_only_and_refuses_overwrite(self):
        grids = []
        for tag in (1, 2, 3):
            grid = bytearray(44);grid[:4] = b"STIH"
            struct.pack_into(">I", grid, 4, len(grid));grid[-1] = tag
            grids.append(bytes(grid))
        encoded = bundle([b"owned model placeholder", *grids])
        with tempfile.TemporaryDirectory() as name:
            root = Path(name);source = root/"owned.gfnb";source.write_bytes(encoded)
            with patch("tools.extract.gred_motion.STAGE_SHA256", hashlib.sha256(encoded).hexdigest()):
                result = export_grids(source, root/"out")
                self.assertEqual(len(result["files"]), 3)
                for filename, raw in zip(("floor.stih", "secondary.stih", "tertiary.stih"), grids):
                    self.assertEqual((root/"out"/filename).read_bytes(), raw)
                with self.assertRaises(FileExistsError):
                    export_grids(root/"absent", root/"out")
            self.assertEqual(source.read_bytes(), encoded)

    def test_bad_grid_and_revision_refuse_before_output_creation(self):
        grid = b"STIH"+struct.pack(">I", 44)+bytes(36)
        encoded = bundle([b"owned", grid, grid, b"bad third grid"])
        with tempfile.TemporaryDirectory() as name:
            root = Path(name);source = root/"owned.gfnb";source.write_bytes(encoded)
            with self.assertRaises(ValueError):
                export_grids(source, root/"unqualified")
            with patch("tools.extract.gred_motion.STAGE_SHA256", hashlib.sha256(encoded).hexdigest()):
                with self.assertRaises(ValueError):
                    export_grids(source, root/"bad")
            self.assertFalse((root/"bad").exists())
            self.assertFalse((root/"unqualified").exists())

    def test_profile_output_refuses_existing_file_and_symlink(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name);out = root/"owned";out.write_bytes(b"user work")
            with self.assertRaises(FileExistsError):
                export_profile(root/"absent", root/"absent-setup", out)
            self.assertEqual(out.read_bytes(), b"user work")
            link = root/"dangling";link.symlink_to(root/"absent")
            with self.assertRaises(FileExistsError):
                export_profile(root/"absent", root/"absent-setup", link)

    def test_oversized_dol_refuses_before_loading_or_output_creation(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name);dol = root/"owned.dol";setup = root/"owned.gfcs"
            with dol.open("wb") as stream:
                stream.truncate((8 << 20)+1)
            setup.write_bytes(bytes(1064))
            with patch("tools.extract.gred_motion.DolImage") as image:
                with self.assertRaisesRegex(ValueError, "DOL extent"):
                    export_profile(dol, setup, root/"new"/"motion.gfmm")
                image.assert_not_called()
            self.assertFalse((root/"new").exists())
            self.assertEqual(dol.stat().st_size, (8 << 20)+1)
            self.assertEqual(setup.read_bytes(), bytes(1064))


if __name__ == "__main__":
    unittest.main()
