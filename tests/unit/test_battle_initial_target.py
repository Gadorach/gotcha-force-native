from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from tools.extract.battle_initial_target import CLASSES, SOURCE_SHA256, encode, export
from tools.extract.source_image import ReadOnlyFile


class Image:
    sha256 = SOURCE_SHA256

    def __init__(self):
        self.blocks = {0x80436e88: struct.pack(">I", 0x4dbebc20),
                       0x802cfaf4: bytes(range(16))}
        for address, _ in CLASSES.values():
            self.blocks[address] = bytes(range(192))
            self.blocks[address + 0x108] = bytes(6)
        for selector in range(18):
            address = 0x80010000 + selector * 256
            self.blocks[0x802cb2f0 + selector * 4] = struct.pack(">I", address)
            self.blocks[address] = bytes([selector]) * 256

    def read(self, address, size):
        data = self.blocks[address]
        if len(data) != size:
            raise ValueError("unexpected extraction range")
        return data


class InitialTargetTests(unittest.TestCase):
    def test_distinct_class_identities_and_full_table_extent(self):
        for model, (address, model_hash) in CLASSES.items():
            raw = encode(Image(), model)
            self.assertEqual(len(raw), 4688)
            self.assertEqual(struct.unpack_from(">I", raw, 12)[0], model_hash)
            self.assertEqual(struct.unpack_from(">IH", raw, 20), (address, model))
            self.assertEqual(raw[64:80], bytes(range(16)))
            self.assertEqual(raw[-256:], bytes([17]) * 256)

    def test_wrong_revision_class_policy_and_operand_refuse(self):
        image = Image()
        image.sha256 = "wrong"
        with self.assertRaises(ValueError):
            encode(image, 10)
        image = Image()
        with self.assertRaises(ValueError):
            encode(image, 11)
        image.blocks[CLASSES[10][0] + 0x108] = b"\1" + bytes(5)
        with self.assertRaises(ValueError):
            encode(image, 10)
        image = Image()
        image.blocks[0x80436e88] = bytes(4)
        with self.assertRaises(ValueError):
            encode(image, 10)

    def test_unmapped_table_never_fabricates_rows(self):
        image = Image()
        image.blocks.pop(0x80010000 + 17 * 256)
        with self.assertRaises(KeyError):
            encode(image, 10)

    def test_read_only_export_and_overwrite_refusal(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, output = root / "input.dol", root / "output.gfnt"
            source.write_bytes(b"owned test fixture")
            with ReadOnlyFile(source) as file, patch(
                    "tools.extract.battle_initial_target.DolImage", return_value=Image()):
                export(file, output, 10)
                with self.assertRaises(FileExistsError):
                    export(file, output, 10)
            self.assertEqual(source.read_bytes(), b"owned test fixture")
            self.assertEqual(output.read_bytes(), encode(Image(), 10))

    def test_wrong_source_and_oversize_refuse_before_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, output = root / "input.dol", root / "output.gfnt"
            source.write_bytes(b"unqualified")
            with ReadOnlyFile(source) as file, self.assertRaises(ValueError):
                export(file, output, 10)
            self.assertFalse(output.exists())
            with source.open("wb") as stream:
                stream.truncate((64 << 20) + 1)
            with ReadOnlyFile(source) as file, self.assertRaises(ValueError):
                export(file, output, 10)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
