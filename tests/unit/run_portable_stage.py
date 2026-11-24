"""Create disposable original-art Borg and Workshop stage fixtures."""
from pathlib import Path
from subprocess import run
import sys
from tempfile import TemporaryDirectory

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage

with TemporaryDirectory(prefix="gf-portable-stage-") as directory:
    root = Path(directory) / "fixtures"
    prepare(root, clipped_beam=True)
    prepare_stage(root)
    sys.exit(run([Path(sys.argv[1]).resolve(strict=True), root], timeout=30).returncode)
