import struct
import tempfile
import unittest
from pathlib import Path
from tools.extract.native_battle_mvp import blocks, prepare, read


def bundle(parts):
    table = b"".join(struct.pack(">II", i, len(part)) for i, part in enumerate(parts))
    body = table + b"".join(parts)
    return struct.pack(">4I", 0x47464E42, 1, len(parts), len(body)) + body


class NativeBattleMvpAssetsTests(unittest.TestCase):
    def test_native_bundle_slots_and_guards(self):
        valid = bundle([b"first", b"second", b""])
        self.assertEqual(blocks(valid), [b"first", b"second", b""])
        for raw in (b"", valid[:-1], valid + b"suffix"):
            with self.assertRaises(ValueError):
                blocks(raw)
        for at, value in ((0, 0), (4, 2), (8, 512), (12, 0), (16, 1), (20, 0xFFFFFFFF)):
            edited = bytearray(valid)
            struct.pack_into(">I", edited, at, value)
            with self.assertRaises(ValueError):
                blocks(edited)

    def test_read_is_bounded_and_read_only(self):
        with tempfile.TemporaryDirectory() as name:
            source = Path(name) / "source"
            source.write_bytes(b"unchanged")
            self.assertEqual(read(source, 9), b"unchanged")
            with self.assertRaises(ValueError):
                read(source, 8)
            self.assertEqual(source.read_bytes(), b"unchanged")

    def test_invalid_art_never_creates_output(self):
        with tempfile.TemporaryDirectory() as name:
            base = Path(name)
            stage = base / "stage"
            stage.write_bytes(bundle([struct.pack(">I", 32) + bytes(28),
                                      b"STIH" + bytes(44), b"", b""]))
            character = base / "character"
            character.write_bytes(bundle([bytes(432), b"", b"", bytes(64),
                                           struct.pack(">I", 4) + struct.pack(">I", 32) + bytes(28),
                                           b"", b"", b"", b"", b""]))
            pose = base / "pose"
            pose.write_bytes(b"not a pose")
            out = base / "out"
            with self.assertRaisesRegex(ValueError, "normal g-red"):
                prepare(stage, character, character, pose, pose, out)
            self.assertFalse(out.exists())
            self.assertEqual(pose.read_bytes(), b"not a pose")


if __name__ == "__main__":
    unittest.main()
