import tempfile
import unittest
from pathlib import Path

from tools.extract import borg_movement_sfx, portable_borg_preview


class MovementSfxExport(unittest.TestCase):
    def test_wrong_archive_and_existing_output_are_not_written(self):
        with tempfile.TemporaryDirectory() as scratch:
            base = Path(scratch)
            source = base / "wrong.afs"
            source.write_bytes(b"AFS\0\x01\0\0\0")
            dest = base / "audio"
            with self.assertRaises(ValueError):
                borg_movement_sfx.prepare(source, source, dest)
            self.assertFalse(dest.exists())
            self.assertEqual(source.read_bytes(), b"AFS\0\x01\0\0\0")
            dest.mkdir()
            with self.assertRaises(FileExistsError):
                borg_movement_sfx.prepare(source, source, dest)
            self.assertEqual(list(dest.iterdir()), [])

    def test_export_uses_only_pinned_samples_for_reviewed_borgs(self):
        audio = Path("local/content/source-borg-movement-pcm-v1")
        art = Path("local/content/native-battle-mvp-v1")
        if not (audio / "jump.pcm").is_file() or not (art / "sasuke.hsd").is_file():
            self.skipTest("optional original audio/art pack is absent")
        with tempfile.TemporaryDirectory() as scratch:
            base = Path(scratch)
            sasuke = base / "sasuke"
            portable_borg_preview.prepare("sasuke", art, sasuke, battle_test=True, movement_sfx=audio)
            text = (sasuke / "gameplay.txt").read_text()
            self.assertIn("movement-sound=takeoff|audio/jump.pcm\n", text)
            self.assertIn("movement-sound=air-jump|audio/jump.pcm\n", text)
            self.assertIn("movement-sound=landing|audio/landing.pcm\n", text)
            self.assertEqual({p.name for p in (sasuke / "audio").iterdir()}, {"landing.pcm", "jump.pcm"})
            with self.assertRaises(ValueError):
                portable_borg_preview.prepare("seeker-scout", art, base / "custom", movement_sfx=audio)
            bad = base / "unqualified"
            bad.mkdir()
            (bad / "jump.pcm").write_bytes(b"not a pinned sample")
            with self.assertRaises(ValueError):
                portable_borg_preview.prepare("sasuke", art, base / "refused", battle_test=True, movement_sfx=bad)
            self.assertFalse((base / "refused").exists())


if __name__ == "__main__":
    unittest.main()
