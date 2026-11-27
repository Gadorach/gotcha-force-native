"""Check recovered projectile rows in a fresh retail roster and play them in the host."""
import argparse
import re
from pathlib import Path
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.portable_borg_preview import prepare

# (native move, mode, pitch, damage against the authored target)
CASES = {
    "000a": ("basic-shot", 2, 24, 9),
    "0303": ("x-shot", 2, 24, 8),
    "0500": ("basic-shot", 1, 24, 8),  # Dragon type-47
    "0501": ("basic-shot", 4, 24, 8),  # Worm type-49
    "050b": ("basic-shot", 4, 24, 8),  # Worm type-94; alternate installer table
    "050d": ("basic-shot", 4, 24, 8),
    "0517": ("basic-shot", 4, 24, 8),
    "0615": ("basic-shot", 2, 24, 9),
    "0619": ("basic-shot", 2, 24, 8),  # Titan's two sibling releases agree
    "061e": ("basic-shot", 2, 24, 9),
    "0800": ("basic-shot", 4, 32, 9),
    "0801": ("basic-shot", 4, 32, 9),
    "0802": ("basic-shot", 4, 0, 8),
    "0c00": ("basic-shot", 0, 0, 8),   # Tank's type-0 initializer
    "0d00": ("charge-shot", 0, 16, 8),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path, help="direct output of retail_borg_roster")
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, roster, stage = (p.resolve(strict=True) for p in (args.executable, args.roster, args.stage))
    with TemporaryDirectory(prefix="gf-retail-reaction-") as scratch:
        root = Path(scratch)
        prepare("target", stage, root / "borgs/training-target")
        for model, (move, mode, pitch, damage) in CASES.items():
            name = f"gg4e08-{model}"
            package = root / "borgs" / name
            copytree(roster / name, package, symlinks=True)
            lines = (package / "gameplay.txt").read_text().splitlines()
            expected = [f"reaction-mode={move}|{mode}", f"reaction-offset={move}|0|{pitch}"]
            if mode == 4:
                expected.append(f"reaction-root={move}|linked-owner")
            if not all(line in lines for line in expected):
                raise ValueError(f"{model}: missing generated reaction rows: {expected}")
            cmd = [exe, "--battle-borg", f"gg4e08:{model}", "--against-borg", "test:target",
                   "--demo-package-battle", "--trace-package-battle", "--frames", "150",
                   "--battle-mvp-assets", stage, "--no-audio"]
            if not args.opengl:
                cmd.append("--headless")
            got = run(cmd, cwd=root, text=True, capture_output=True, timeout=60)
            hits = re.findall(rf"round=(\d+) .*move={move} event=hit .*recipient=1 value=(\d+)", got.stderr)
            reactions = re.search(r"package-battle: .* reactions=(\d+)", got.stderr)
            if (got.returncode or {int(round_) for round_, _ in hits} != {0, 1} or
                    any(int(value) != damage for _, value in hits) or not reactions or int(reactions[1]) < 2):
                raise ValueError(f"{model}: expected damage {damage} and reactions across restart:\n{got.stderr[-8000:]}")
    print(f"{len(CASES)} generated projectile reaction packages passed ({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
