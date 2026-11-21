from pathlib import Path
import tempfile
import unittest

from tools.extract.gred_sasuke_receiver import export


class GRedSasukeReceiverTests(unittest.TestCase):
    def test_wrong_member_and_occupied_destination(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            member, out = root / "pl000a.pzz", root / "receiver.bin"
            member.write_bytes(b"wrong member")
            with self.assertRaises(ValueError):
                export(root / "unused.dol", member, out)
            self.assertEqual(member.read_bytes(), b"wrong member")
            self.assertFalse(out.exists())
            out.write_bytes(b"existing file")
            with self.assertRaises(FileExistsError):
                export(root / "unused.dol", member, out)
            self.assertEqual(out.read_bytes(), b"existing file")
