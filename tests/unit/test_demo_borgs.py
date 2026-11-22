from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.extract.demo_borgs import prepare


class DemoBorgExport(unittest.TestCase):
    def test_standalone_generation_is_repeatable_and_non_overwriting(self):
        with TemporaryDirectory() as scratch:
            root = Path(scratch)
            first, second = root / "first", root / "second"
            prepare(first)
            prepare(second)
            expected = {p.relative_to(first): p.read_bytes() for p in first.rglob("*") if p.is_file()}
            self.assertEqual({p.relative_to(second): p.read_bytes() for p in second.rglob("*") if p.is_file()}, expected)
            self.assertIn(Path("arena/floor.stih"), expected)
            self.assertIn(b"powered-flight=7|32", expected[Path("borgs/flight-stick/gameplay.txt")])
            self.assertIn(b"air-jump-points=3", expected[Path("borgs/jump-stick/gameplay.txt")])
            self.assertFalse(any(p.suffix == ".pcm" for p in expected))
            with self.assertRaises(FileExistsError):
                prepare(first)
            self.assertEqual({p.relative_to(first): p.read_bytes() for p in first.rglob("*") if p.is_file()}, expected)


if __name__ == "__main__":
    unittest.main()
