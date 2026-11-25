"""Exercise the recovered G-Red contact factors in the real host.

Builds G-Red with an extra close move on x and Sasuke with model=10, then runs
the package battle. The shared demo script presses x at frame 41, so the close
move is selected without changing the demo profiles.
"""
import argparse
import re
import sys
import tempfile
from pathlib import Path
from subprocess import run

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.extract.native_battle_mvp import read
from tools.extract.portable_borg_preview import prepare
CLOSE = (
    b"move=close-b-test|x|10|grounded|18|0:2\n"
    b"range=close-b-test|145\n"
    b"melee=close-b-test|4|8|115|12|60|24|20\n"
    b"damage-vs=10|0.867|1.13|1.15|1.0\n"
    b"damage-chain=1.0|0.8|0.5|0.25\n"
)


def transform(rules, extra):
    return rules + extra


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", type=Path,
                        default=ROOT / "build/publication-compile-v1/gf_native")
    parser.add_argument("--art", type=Path,
                        default=ROOT / "local/content/native-battle-mvp-v1")
    parser.add_argument("--movement", type=Path,
                        default=ROOT / "local/content/source-gred-ground-v9")
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    art = args.art.resolve(strict=True)
    movement = args.movement.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="gred-sasuke-contact-") as name:
        root = Path(name)
        prepare("g-red", art, root / "borgs/g-red", movement, True,
                gameplay_transform=lambda rules: transform(rules, CLOSE))
        prepare("target", art, root / "borgs/target", None, True,
                gameplay_transform=lambda rules: transform(rules, b"damage-model=10\n"))
        stage = root / "stage-only"
        stage.mkdir()
        for asset in ("stage.hsd", "floor.stih"):
            with (stage / asset).open("xb") as dest:
                dest.write(read(art / asset, 32 << 20))
        result = run([
            str(executable),
            "--battle-borg", "gg4e08:0615",
            "--against-borg", "test:target",
            "--battle-mvp-assets", str(stage),
            "--source-ground-assets", str(movement),
            "--demo-package-battle", "--trace-package-battle",
            "--package-battle-distance", "90",
            "--no-audio", "--frames", "150", "--headless",
        ], cwd=root, capture_output=True, text=True, timeout=60)
        log = result.stdout + result.stderr
        if result.returncode:
            raise SystemExit(log or f"exit {result.returncode}")
        close = [line for line in log.splitlines() if "move=close-b-test" in line]
        hit_lines = [line for line in log.splitlines() if "event=hit" in line]
        print(f"hits={re.findall(r'hits=(\d+)', log)}")
        for line in hit_lines:
            print(line)
        if not any("event=melee-armed" in line for line in close):
            raise SystemExit("close move did not arm:\n" + log[-4000:])
        close_hit = [line for line in hit_lines if "move=close-b-test" in line]
        if not close_hit:
            raise SystemExit("close move did not hit:\n" + log[-4000:])
        value = re.search(r"value=(\d+)", close_hit[0])
        if not value or int(value.group(1)) != 18:
            raise SystemExit("close move did not apply the tier-1 amount:\n" + log[-4000:])
        if not any("move=beam-test event=hit" in line for line in hit_lines):
            raise SystemExit("expected the preceding beam hit for the tier chain")


if __name__ == "__main__":
    main()
