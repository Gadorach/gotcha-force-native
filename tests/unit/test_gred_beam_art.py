from pathlib import Path
import tempfile
import unittest

from tools.extract.gred_beam_art import export


class GRedBeamArtTests(unittest.TestCase):
    def test_reject_bad_member_without_output(self):
        with tempfile.TemporaryDirectory() as name:
            member, out = (Path(name) / leaf for leaf in ("bad.pzz", "beam.hsd"))
            member.write_bytes(b"not the common effects bank")
            with self.assertRaises(ValueError):
                export(member, out)
            self.assertEqual(member.read_bytes(), b"not the common effects bank")
            self.assertFalse(out.exists())

    def test_existing_or_dangling_destination_refused_before_input(self):
        with tempfile.TemporaryDirectory() as name:
            member, out = (Path(name) / leaf for leaf in ("missing.pzz", "beam.hsd"))
            out.write_bytes(b"user file")
            with self.assertRaises(FileExistsError):
                export(member, out)
            self.assertEqual(out.read_bytes(), b"user file")
            link = Path(name) / "link.hsd"
            link.symlink_to("not-created.hsd")
            with self.assertRaises(FileExistsError):
                export(member, link)
            self.assertTrue(link.is_symlink())
            self.assertFalse((Path(name) / "not-created.hsd").exists())
