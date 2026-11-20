from pathlib import Path
import struct
import tempfile
import unittest

from tools.extract.gred_locomotion import SOURCE_SHA256, export, recover_neutral_b


class GRedNeutralBTests(unittest.TestCase):
    def test_exact_slice_layout_with_synthetic_source(self):
        spans = {
            (0x8036631C, 84): b"i" * 84, (0x80366638, 80): b"s" * 80,
            (0x80365B86, 2): bytes(2),
        }
        descriptor_keys = ((0, 0), (0, 1), (0, 2), (2, 0), (2, 1), (2, 2), (2, 6), (2, 7), (2, 8))
        addresses = (0x80367542, 0x80367556, 0x8036756A, 0x803676AA, 0x803676BE,
                     0x803676D2, 0x80367722, 0x80367736, 0x8036774A)
        for marker, at in enumerate(addresses, 1):
            spans[at, 20] = bytes([marker]) * 20

        class Source:
            sha256 = SOURCE_SHA256

            @staticmethod
            def read(at, size):
                return spans.pop((at, size))

        raw = recover_neutral_b(Source())
        self.assertFalse(spans)
        self.assertEqual(struct.unpack_from(">4sHBBIHBBIIII", raw),
                         (b"GFMP", 3, 3, 2, 0xE2B4439A, 9, 0, 0, 436, 32, 56, 272))
        self.assertEqual(struct.unpack_from(">BBHII", raw, 32), (0, 0, 0, 272, 84))
        self.assertEqual(struct.unpack_from(">BBHII", raw, 44), (2, 0, 0, 356, 80))
        for marker, (group, choice) in enumerate(descriptor_keys, 1):
            at = 56 + (marker - 1) * 24
            self.assertEqual(struct.unpack_from(">BBH", raw, at), (group, choice, 0))
            self.assertEqual(raw[at + 4:at + 24], bytes([marker]) * 20)
        self.assertEqual(raw[272:], b"i" * 84 + b"s" * 80)

    def test_wrong_dol_and_occupied_destination_refuse(self):
        with tempfile.TemporaryDirectory() as name:
            source, output = (Path(name) / f for f in ("invalid.dol", "neutral-b.gfmp"))
            source.write_bytes(b"wrong revision")
            with self.assertRaises(ValueError):
                export(source, output, "neutral-b")
            self.assertFalse(output.exists())
            self.assertEqual(source.read_bytes(), b"wrong revision")
            output.write_bytes(b"user-owned")
            with self.assertRaises(FileExistsError):
                export(source, output, "neutral-b")
            self.assertEqual(output.read_bytes(), b"user-owned")


if __name__ == "__main__":
    unittest.main()
