import tempfile
import unittest
from pathlib import Path

from tools.analyze import damage_trace


def _tree(root, *, runtime=True, module=True, header=True, dol=True):
    if runtime:
        path = root / "build/moderngekko/moderngekko-run"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("")
    if module:
        path = root / "build/moderngekko-module/gGG4E08_recomp.so"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("")
    if header:
        path = root / "external/ModernGekko/include/moderngekko/mod_abi.h"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("")
    if dol:
        path = root / "original/extracted/sys/main.dol"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("")


class DamageTraceGuardTest(unittest.TestCase):
    def test_missing_runtime_refuses(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            _tree(root, runtime=False)
            with self.assertRaisesRegex(SystemExit, "missing runtime"):
                damage_trace.main(["--gotcha", str(root), "--work", str(root / "run")])

    def test_missing_module_refuses(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            _tree(root, module=False)
            with self.assertRaisesRegex(SystemExit, "missing module"):
                damage_trace.main(["--gotcha", str(root), "--work", str(root / "run")])

    def test_missing_header_refuses(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            _tree(root, header=False)
            with self.assertRaisesRegex(SystemExit, "missing mod ABI header"):
                damage_trace.main(["--gotcha", str(root), "--work", str(root / "run")])

    def test_missing_dol_refuses(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            _tree(root, dol=False)
            with self.assertRaisesRegex(SystemExit, "missing extracted main.dol"):
                damage_trace.main(["--gotcha", str(root), "--work", str(root / "run")])

    def test_existing_run_directory_refuses(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            _tree(root)
            run = root / "run"
            run.mkdir()
            with self.assertRaisesRegex(SystemExit, "refusing an existing run directory"):
                damage_trace.main(["--gotcha", str(root), "--work", str(run)])


if __name__ == "__main__":
    unittest.main()
