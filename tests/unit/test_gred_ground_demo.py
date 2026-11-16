import hashlib
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from tools.extract.gred_ground_demo import prepare


class GRedGroundPackTests(unittest.TestCase):
    def test_existing_output_and_symlinks_refuse_before_input_access(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name);out = root/"owned";out.mkdir();(out/"keep").write_bytes(b"owned")
            with self.assertRaises(FileExistsError):
                prepare(root/"absent", root/"absent", root/"absent", out)
            self.assertEqual((out/"keep").read_bytes(), b"owned")
            link = root/"link";link.symlink_to(root/"missing")
            with self.assertRaises(FileExistsError):
                prepare(root/"absent", root/"absent", root/"absent", link)

    def test_composition_keeps_components_and_inputs_owned(self):
        parts = [b"owned scene"]+[b"STIH"+bytes([i])*40 for i in (1, 2, 3)]
        encoded = bytearray(48);struct.pack_into(">4I", encoded, 0, 0x47464E42, 1, 4, 0)
        for i, part in enumerate(parts):
            struct.pack_into(">2I", encoded, 16+i*8, i, len(part));encoded.extend(part)
        struct.pack_into(">I", encoded, 12, len(encoded)-16);encoded = bytes(encoded)
        with tempfile.TemporaryDirectory() as name:
            root = Path(name);dol = root/"owned.dol";setup = root/"owned.gfcs";stage = root/"owned.gfnb"
            dol.write_bytes(b"owned synthetic DOL");setup.write_bytes(bytes(1064));stage.write_bytes(encoded)
            with patch("tools.extract.gred_ground_demo.DolImage", return_value=object()), \
                 patch("tools.extract.gred_ground_demo.STAGE_SHA256", hashlib.sha256(encoded).hexdigest()), \
                 patch("tools.extract.gred_ground_demo.programs", return_value=b"owned-program"), \
                 patch("tools.extract.gred_ground_demo.recover_height", return_value=b"owned-height"), \
                 patch("tools.extract.gred_ground_demo.inputs", return_value=b"owned-input"), \
                 patch("tools.extract.gred_ground_demo.motion", return_value=b"owned-motion"):
                manifest = prepare(dol, setup, stage, root/"out")
            self.assertEqual(len(manifest["files"]), 7)
            self.assertEqual((root/"out"/"locomotion.gfmp").read_bytes(), b"owned-program")
            self.assertEqual((root/"out"/"tertiary.stih").read_bytes(), parts[3])
            self.assertEqual(dol.read_bytes(), b"owned synthetic DOL")
            self.assertEqual(setup.read_bytes(), bytes(1064))
            self.assertEqual(stage.read_bytes(), encoded)

    def test_bad_source_creates_no_output(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name);dol = root/"bad.dol";setup = root/"owned.gfcs"
            dol.write_bytes(b"not qualified");setup.write_bytes(bytes(1064))
            with self.assertRaises(ValueError):
                prepare(dol, setup, root/"absent-stage", root/"out")
            self.assertFalse((root/"out").exists())

    def test_oversized_dol_refuses_before_stage_or_output_access(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name);dol = root/"owned.dol";setup = root/"owned.gfcs"
            with dol.open("wb") as stream:
                stream.truncate((8 << 20)+1)
            setup.write_bytes(bytes(1064))
            with patch("tools.extract.gred_ground_demo.DolImage") as image:
                with self.assertRaisesRegex(ValueError, "DOL extent"):
                    prepare(dol, setup, root/"absent-stage", root/"out")
                image.assert_not_called()
            self.assertFalse((root/"out").exists())
            self.assertEqual(dol.stat().st_size, (8 << 20)+1)


if __name__ == "__main__":
    unittest.main()
