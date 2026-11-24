from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.extract.portable_ui_font_example import prepare


class PortableFontExampleTests(unittest.TestCase):
    def test_owned_atlas_coverage_and_no_overwrites(self):
        with TemporaryDirectory() as scratch:
            root = Path(scratch)
            prepare(root / "one")
            prepare(root / "two")
            files = {name: (root / "one" / name).read_bytes() for name in ("font.manifest", "ui.a8")}
            for name, data in files.items():
                self.assertEqual(data, (root / "two" / name).read_bytes())
            rows = files["font.manifest"].decode("utf-8").splitlines()
            self.assertEqual(rows[0], "GFFONT 1")
            width, height = map(int, next(row for row in rows if row.startswith("size="))[5:].split("|"))
            alpha = files["ui.a8"]
            self.assertEqual(len(alpha), width * height)
            for code in (ord("A"), ord("a"), ord("é"), ord("界"), 0xFFFD):
                row = next(row for row in rows if row.startswith(f"glyph=U+{code:04X}|"))
                x, y, w, h = map(int, row.split("|")[1:5])
                self.assertTrue(any(alpha[dy * width + dx] for dy in range(y, y + h) for dx in range(x, x + w)))
            with self.assertRaises(FileExistsError):
                prepare(root / "one")
            for name, data in files.items():
                self.assertEqual(data, (root / "one" / name).read_bytes())
