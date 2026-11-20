from pathlib import Path
import tempfile
import unittest

from tools.extract.gred_sasuke_damage import export, recover


class GRedSasukeDamageExportTests(unittest.TestCase):
    def test_revision_must_be_qualified_before_any_source_read(self):
        class WrongImage:
            sha256 = "wrong"

            @staticmethod
            def read(*_args):
                raise AssertionError("must refuse before reading")

        with self.assertRaises(ValueError):
            recover(WrongImage())

    def test_invalid_input_and_occupied_outputs_are_read_only(self):
        with tempfile.TemporaryDirectory() as path:
            source, output = (Path(path) / name for name in ("source.dol", "scope.gfda"))
            source.write_bytes(b"unqualified")
            with self.assertRaises(ValueError):
                export(source, output)
            self.assertFalse(output.exists())
            self.assertEqual(source.read_bytes(), b"unqualified")
            output.write_bytes(b"owned")
            with self.assertRaises(FileExistsError):
                export(source, output)
            self.assertEqual(output.read_bytes(), b"owned")
            link = Path(path) / "dangling.gfda"
            target = Path(path) / "absent.gfda"
            link.symlink_to(target)
            with self.assertRaises(FileExistsError):
                export(source, link)
            self.assertFalse(target.exists())


if __name__ == "__main__":
    unittest.main()
