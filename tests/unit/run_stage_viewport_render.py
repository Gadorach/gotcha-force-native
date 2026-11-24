"""Run the viewport test from a disposable original-art fixture."""
from pathlib import Path
from subprocess import run
import sys
from tempfile import TemporaryDirectory

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.extract.demo_borgs import prepare

with TemporaryDirectory(prefix="gf-stage-viewport-") as directory:
    root = Path(directory) / "assets"
    prepare(root, clipped_beam=True, ink_bomb=True)
    sys.exit(run([Path(sys.argv[1]).resolve(strict=True), root], timeout=30).returncode)
