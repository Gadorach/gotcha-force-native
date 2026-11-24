import hashlib
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

from tools.extract import retail_borg_placeholders as retail


def fixture(duplicate_number=False):
    rows = bytearray()
    for model in range(retail.RETAIL_ROWS):
        name = "ＢＯＲＧ　Ａ".encode("cp932")
        ordinal = 0 if duplicate_number and model == 1 else model
        rows.extend(struct.pack(">4H21h", model, ordinal, 2, len(name),
                                147, 322, 333, 444, 77, 99, *[0] * 15))
        rows.extend(name)
    rows.extend(b"\0\0AAB")
    header = struct.pack(">4sI8H2I", b"GFBC", 1, retail.RETAIL_ROWS, 1, 0, 0,
                         1, 1, 1, 0, 0, len(rows))
    return header + rows


class RetailBorgPlaceholders(unittest.TestCase):
    def setUp(self):
        temp = TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.path = Path(temp.name)
        self.source = self.path / "source.gfbc"
        self.source.write_bytes(fixture())
        self.root = self.path / "borgs"
        self.root.mkdir()
        pin = patch.object(retail, "CATALOG_SHA256", hashlib.sha256(fixture()).hexdigest())
        pin.start()
        self.addCleanup(pin.stop)

    def existing(self, folder, identity):
        path = self.root / folder
        path.mkdir()
        data = f"GFBORG 2\nid={identity}\nname=Existing\ncatalog=catalog.txt\ngameplay=gameplay.txt\n"
        (path / "borg.manifest").write_text(data, encoding="utf-8")
        return path, data

    def test_only_missing_identities_and_only_inert_files(self):
        kept, original = self.existing("renamed-gameplay-package", "gg4e08:0001")
        self.existing("authored", "test:fixture")
        self.assertEqual(retail.prepare(self.source, self.root, dry_run=True), (205, 1))
        self.assertEqual(len(list(self.root.iterdir())), 2)
        self.assertEqual(retail.prepare(self.source, self.root), (205, 1))
        self.assertEqual(retail.prepare(self.source, self.root), (0, 206))
        self.assertEqual((kept / "borg.manifest").read_text(), original)
        self.assertFalse((self.root / "gg4e08-0001").exists())
        placeholder = self.root / "gg4e08-0000"
        self.assertEqual({x.name for x in placeholder.iterdir()}, {"borg.manifest", "catalog.txt"})
        fields = {line.split("=")[0] for line in (placeholder / "borg.manifest").read_text().splitlines()
                  if "=" in line}
        self.assertEqual(fields, {"id", "name", "catalog"})
        catalog = (placeholder / "catalog.txt").read_text()
        self.assertIn("collection-number=1\n", catalog)
        self.assertIn("variant=normal|Normal|147|unresolved\n", catalog)
        self.assertIn("hp=unresolved|unresolved\n", catalog)
        self.assertIn("GG4E08:0000/0\n", catalog)
        self.assertEqual(self.source.read_bytes(), fixture())

    def test_conflicts_refuse_before_writing(self):
        reserved = self.root / "gg4e08-000f"
        reserved.symlink_to(self.path / "missing")
        with self.assertRaises(FileExistsError):
            retail.prepare(self.source, self.root)
        self.assertEqual(list(self.root.iterdir()), [reserved])
        self.existing("first", "gg4e08:0000")
        self.existing("second", "gg4e08:0000")
        with self.assertRaisesRegex(ValueError, "duplicate installed package ID"):
            retail.prepare(self.source, self.root)
        self.assertEqual(len(list(self.root.iterdir())), 3)

    def test_bad_catalog_and_linked_manifest_refuse(self):
        with self.assertRaisesRegex(ValueError, "duplicate GFBC row"):
            retail.decode_catalog(fixture(duplicate_number=True))
        with self.assertRaisesRegex(ValueError, "GFBC header"):
            retail.decode_catalog(fixture()[:-1])
        extra = self.path / "modified.gfbc"
        extra.write_bytes(fixture() + b"\0")
        with self.assertRaisesRegex(ValueError, "not the qualified"):
            retail.prepare(extra, self.root)
        self.assertEqual(list(self.root.iterdir()), [])
        folder = self.root / "linked"
        folder.mkdir()
        (folder / "borg.manifest").symlink_to(self.source)
        with self.assertRaises(OSError):
            retail.prepare(self.source, self.root)
        self.assertEqual(list(self.root.iterdir()), [folder])

    def test_combined_discovery_entry_budget(self):
        under = 2048 - retail.RETAIL_ROWS
        installed = patch.object(retail, "read_installed", return_value=({}, set(range(under + 1))))
        with installed, self.assertRaisesRegex(ValueError, "combined Borg scan entry budget"):
            retail.prepare(self.source, self.root)
        self.assertEqual(list(self.root.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
