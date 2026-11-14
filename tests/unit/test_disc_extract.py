import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest

from tools.extract.disc import extract, safe_name
from tools.extract.pzz_files import extract as extract_pzz


def disc_bytes(names=("asset.bin",), payload=b"owned-data"):
    data = bytearray(4096)
    data[:6] = b"OWND00"
    struct.pack_into(">I", data, 0x1c, 0xc2339f3d)
    count = len(names) + 1
    strings = b""
    entries = []
    for i, name in enumerate(names):
        entries.append((len(strings), 0x900 + i * 64, len(payload)))
        strings += name.encode() + b"\0"
        data[0x900 + i * 64:0x900 + i * 64 + len(payload)] = payload
    fst = struct.pack(">III", 0x01000000, 0, count)
    fst += b"".join(struct.pack(">III", *row) for row in entries) + strings
    data[0x500:0x500 + len(fst)] = fst
    struct.pack_into(">III", data, 0x420, 0x600, 0x500, len(fst))
    struct.pack_into(">I", data, 0x600, 0x100)
    struct.pack_into(">I", data, 0x648, 0x80003100)
    struct.pack_into(">I", data, 0x690, 4)
    struct.pack_into(">I", data, 0x6e0, 0x80003100)
    data[0x700:0x704] = b"TEST"
    return bytes(data)


class DiscExtractionTests(unittest.TestCase):
    def test_raw_files_and_dol_new_output_input_unchanged(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "owned.iso"
            source = disc_bytes()
            image.write_bytes(source)
            report = extract(image, root / "output")
            self.assertEqual((root / "output/files/asset.bin").read_bytes(), b"owned-data")
            self.assertEqual((root / "output/sys/main.dol").read_bytes()[-4:], b"TEST")
            self.assertEqual(len(report["files"]), 2)
            self.assertEqual(image.read_bytes(), source)
            self.assertEqual(json.loads((root / "output/extraction.json").read_text()), report)
            self.assertEqual(report["files"][0]["sha256"], hashlib.sha256(b"owned-data").hexdigest())
            with self.assertRaises(FileExistsError):
                extract(image, root / "output")

    def test_case_collision_refuses_before_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "owned.iso"
            image.write_bytes(disc_bytes(("A.bin", "a.bin")))
            with self.assertRaisesRegex(ValueError, "collision"):
                extract(image, root / "output")
            self.assertFalse((root / "output").exists())

    def test_output_symlink_refuses(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "owned.iso"
            image.write_bytes(disc_bytes())
            (root / "target").mkdir()
            (root / "output").symlink_to(root / "target", target_is_directory=True)
            with self.assertRaises(FileExistsError):
                extract(image, root / "output")
            self.assertEqual(list((root / "target").iterdir()), [])

    def test_unsafe_components_and_missing_selection(self):
        for name in ("../x", "/x", "A/../../x", ".git/config", "CON", "a\\b", "x:stream", "a."):
            with self.subTest(name=name), self.assertRaises(ValueError):
                safe_name(name)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "owned.iso"
            image.write_bytes(disc_bytes())
            with self.assertRaises(ValueError):
                extract(image, root / "output", ["missing"])
            self.assertFalse((root / "output").exists())

    def test_selected_files_without_executable(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "owned.iso"
            image.write_bytes(disc_bytes(("a.bin", "b.bin")))
            result = extract(image, root / "output", ["b.bin"], False)
            self.assertEqual([x["path"] for x in result["files"]], ["files/b.bin"])
            self.assertFalse((root / "output/sys").exists())

    def test_missing_output_parent_created_after_input_validation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "owned.iso"
            image.write_bytes(disc_bytes())
            extract(image, root / "original/extracted", include_dol=False)
            self.assertEqual((root / "original/extracted/files/asset.bin").read_bytes(), b"owned-data")

    def test_pzz_empty_raw_and_compressed_blocks(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            data = bytearray(0x1800)
            struct.pack_into(">4I", data, 0, 3, 0, 1, 0x40000001)
            data[0x800:0x804] = b"RAW!"
            # First literal word, then terminating distance-zero compressed command.
            data[0x1000:0x1006] = bytes.fromhex("400041420000")
            image = root / "owned.pzz"
            image.write_bytes(data)
            report = extract_pzz(image, root / "output")
            self.assertEqual((root / "output/000.bin").read_bytes(), b"")
            self.assertEqual((root / "output/001.bin").stat().st_size, 0x800)
            self.assertEqual((root / "output/002.bin").read_bytes(), b"AB")
            self.assertEqual(report["blocks"][2]["consumed_bytes"], 6)
            self.assertEqual(image.read_bytes(), data)
            with self.assertRaises(FileExistsError):
                extract_pzz(image, root / "output")


if __name__ == "__main__":
    unittest.main()
