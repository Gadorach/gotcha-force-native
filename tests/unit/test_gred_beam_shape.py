from pathlib import Path
import tempfile
import unittest

from tools.extract.gred_beam_shape import export


class GRedBeamShapeTests(unittest.TestCase):
    def test_bad_member_does_not_create_output(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            member, out = root / "pl0615.pzz", root / "shape.bin"
            member.write_bytes(b"wrong member")
            with self.assertRaises(ValueError):
                export(root / "unused.dol", member, out)
            self.assertFalse(out.exists())
            self.assertEqual(member.read_bytes(), b"wrong member")

    def test_occupied_destination_is_preserved_before_reads(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            out = root / "shape.bin"
            out.write_bytes(b"existing")
            with self.assertRaises(FileExistsError):
                export(root / "absent.dol", root / "absent.pzz", out)
            self.assertEqual(out.read_bytes(), b"existing")
