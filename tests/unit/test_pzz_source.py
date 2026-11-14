import struct
import unittest

from tools.extract.pzz import PzzArchive, decompress


def words(*values):
    return struct.pack(">" + "H" * len(values), *values)


class Source:
    def __init__(self, data):
        self.data = bytes(data)
        self.size = len(data)

    def read(self, at, size):
        if at < 0 or size < 0 or at + size > self.size:
            raise ValueError("owned source extent")
        return self.data[at:at + size]


def archive(descriptors, payload=b""):
    header = struct.pack(">I", len(descriptors)) + struct.pack(">" + "I" * len(descriptors), *descriptors)
    return Source(header.ljust(0x800, b"\0") + payload)


class PzzSourceTests(unittest.TestCase):
    def test_empty_stream_and_padding(self):
        self.assertEqual(decompress(words(0x8000, 0) + b"ignored!"), (b"", 4))

    def test_literal_words_big_endian(self):
        self.assertEqual(decompress(words(0x2000, 0x1234, 0xabcd, 0)), (b"\x12\x34\xab\xcd", 8))

    def test_overlapping_back_reference(self):
        self.assertEqual(decompress(words(0x6000, 0x4142, (3 << 11) | 1, 0)), (b"ABABABAB", 8))

    def test_extended_count(self):
        self.assertEqual(decompress(words(0x6000, 0x6162, 1, 5, 0)), (b"ab" * 6, 10))

    def test_control_word_rollover(self):
        stream = words(0, *range(16), 0x8000, 0)
        self.assertEqual(decompress(stream), (words(*range(16)), len(stream)))

    def test_truncation_and_missing_terminator(self):
        for raw in [b"", b"x", words(0), words(0x8000), words(0x8000, 1), words(0, *range(16))]:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                decompress(raw)

    def test_bad_distance(self):
        with self.assertRaises(ValueError):
            decompress(words(0xc000, (1 << 11) | 1, 0))

    def test_output_budgets(self):
        for stream, limit in [(words(0x4000, 1, 0), 1), (words(0x6000, 0x6162, (3 << 11) | 1, 0), 7)]:
            with self.subTest(limit=limit), self.assertRaises(ValueError):
                decompress(stream, limit)
        for limit in [-1, True, (64 << 20) + 1]:
            with self.assertRaises(ValueError):
                decompress(words(0x8000, 0), limit)

    def test_directory_raw_compressed_and_absent(self):
        compressed = words(0x4000, 0x4142, 0).ljust(0x800, b"\0")
        packed = PzzArchive(archive([0, 1, 0x40000001], b"q" * 0x800 + compressed))
        self.assertEqual(packed.read(0), (b"", 0))
        self.assertEqual(packed.read(1), (b"q" * 0x800, 0x800))
        self.assertEqual(packed.read(2), (b"AB", 6))
        with self.assertRaises(ValueError):
            packed.read(1, 0x7ff)
        for index in [-1, 3, True]:
            with self.assertRaises(ValueError):
                packed.read(index)

    def test_invalid_directories(self):
        for source in [Source(b""), Source(b"x" * 0x801), archive([]), archive([0x80000000]), archive([2]), archive([0], b"x" * 0x800)]:
            with self.subTest(size=source.size), self.assertRaises(ValueError):
                PzzArchive(source)


if __name__ == "__main__":
    unittest.main()
