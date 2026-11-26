"""Load the emitted damage-vs row in the real host package battle."""
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
    with TemporaryDirectory(prefix="gf-damage-row-") as name:
        root = Path(name)
        def add_row(rules):
            return rules + (b"# authored test damage-vs: attacker settings 0, "
                            b"defender settings 16; not universal battle state\n"
                            b"damage-factors=1.0|1.0|1.0|1.0\n"
                            b"damage-vs=10|0.867|1.13|1.15|1.0\n")
        prepare("g-red", art, root / "borgs/g-red", movement, True,
                gameplay_transform=add_row)
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
        hits = [line for line in log.splitlines()
                if "move=beam-test event=hit" in line]
        if not hits:
            raise SystemExit("damage row fixture did not hit:\n" + log[-4000:])
        value = re.search(r"value=(\d+)", hits[0])
        if not value or int(value.group(1)) != 21:
            raise SystemExit("damage row not applied:\n" + log[-4000:])
        print("Damage row: recovered factors apply in the package battle PASS")


if __name__ == "__main__":
    main()
