from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
from tools.extract.native_bundle import encode, export, fnv32
from tools.extract.source_image import ReadOnlyFile


class NativeBundleTests(unittest.TestCase):
    def test_header_roles_and_empty_blocks_preserve_payload(self):
        b = encode([b"", b"abc", b"\0" * 5])
        self.assertEqual(struct.unpack_from(">4I", b), (0x47464e42, 1, 3, len(b) - 16))
        self.assertEqual(struct.unpack_from(">6I", b, 16), (0, 0, 1, 3, 2, 5))
        self.assertEqual(b[40:], b"abc" + b"\0" * 5)
        self.assertEqual(fnv32(b"a"), 0xe40c292c)

    def test_empty_count_and_byte_budget_refuse(self):
        for blocks in ([], [b""] * 512, [b"x" * (32 << 20)]):
            with self.assertRaises(ValueError):
                encode(blocks)

    def test_pzz_export_source_unchanged_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = bytearray(0x1000)
            struct.pack_into(">2I", source, 0, 1, 1)
            source[0x800:0x804] = b"TEST"
            image = root / "owned.pzz"
            image.write_bytes(source)
            output = root / "native/owned.gfnb"
            with ReadOnlyFile(image) as file:
                result = export(file, output)
            b = output.read_bytes()
            self.assertEqual(len(b), 24 + 0x800)
            self.assertEqual(result["bytes"], len(b))
            self.assertEqual(result["fnv32"], fnv32(b))
            self.assertEqual(b[24:28], b"TEST")
            self.assertEqual(image.read_bytes(), source)
            with ReadOnlyFile(image) as file, self.assertRaises(FileExistsError):
                export(file, output)

    def test_total_budget_limits_decode_before_output_creation(self):
        class Archive:
            entries = [None] * 3
            budgets = []

            def read(self, index, max_output):
                self.budgets.append(max_output)
                if max_output < 16 << 20:
                    raise ValueError("PZZ output budget")
                return b"x" * (16 << 20), 2

        archive = Archive()
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "unwritten.gfnb"
            with patch("tools.extract.native_bundle.PzzArchive", return_value=archive):
                with self.assertRaisesRegex(ValueError, "output budget"):
                    export(object(), output)
            self.assertEqual(archive.budgets, [16 << 20, (16 << 20) - 40])
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
