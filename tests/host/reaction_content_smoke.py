"""Load emitted reaction keys in the real host package battle."""
import argparse
import re
import sys
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.extract.native_battle_mvp import read
from tools.extract.portable_borg_preview import prepare

DEFAULT_ART = ROOT / "local/content/native-battle-mvp-v1"
DEFAULT_MOVEMENT = ROOT / "local/content/source-gred-ground-v9"
DEFAULT_EXE = ROOT / "build/publication-compile-v1/gf_native"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, default=DEFAULT_EXE)
    parser.add_argument("--art", type=Path, default=DEFAULT_ART)
    parser.add_argument("--movement", type=Path, default=DEFAULT_MOVEMENT)
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    art = args.art.resolve(strict=True)
    movement = args.movement.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-reaction-content-") as name:
        root = Path(name)
        def add_reaction(rules):
            return rules + (b"reaction-mode=beam-test|2\n"
                            b"reaction-offset=beam-test|48|16\n")
        prepare("g-red", art, root / "borgs/g-red", movement, True,
                gameplay_transform=add_reaction)
        prepare("target", art, root / "borgs/target", None, True)
        stage = root / "stage-only"
        stage.mkdir()
        for asset in ("stage.hsd", "floor.stih"):
            (stage / asset).write_bytes(read(art / asset, 32 << 20))
        result = run([
            str(executable), "--battle-borg", "gg4e08:0615",
            "--against-borg", "test:target", "--battle-mvp-assets", str(stage),
            "--source-ground-assets", str(movement), "--demo-package-battle",
            "--trace-package-battle", "--no-audio", "--frames", "60",
            "--headless",
        ], cwd=root, capture_output=True, text=True, timeout=60)
        log = result.stdout + result.stderr
        if result.returncode:
            raise SystemExit(log or f"exit {result.returncode}")
        if "move=beam-test event=hit" not in log:
            raise SystemExit("reaction content fixture did not hit:\n" + log[-4000:])
        match = re.search(r"reactions=(\d+)", log)
        if not match or int(match.group(1)) < 1:
            raise SystemExit("reaction window did not open:\n" + log[-4000:])
        print("Reaction content: mode 2 and offset load in the package battle PASS")


if __name__ == "__main__":
    main()
