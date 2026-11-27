"""Exercise timed roster rounds with and without series scoring."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    sample = Path(__file__).resolve().parents[2] / "examples/battle-rosters/workshop-timed.roster"
    with TemporaryDirectory(prefix="gf-roster-clock-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        prepare_stage(root)
        (root / "borgs/jump-stick/gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=stationary-v1\nhealth=1000\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        )
        text = sample.read_text().replace("round-seconds=45", "round-seconds=1")
        assert "round-seconds=1\n" in text
        preset = root / "fast.roster"
        cmd = [binary, "--no-audio", "--frames", "170", "--demo-package-battle",
               "--trace-package-battle", "--battle-roster", preset]
        if not opengl:
            cmd.append("--headless")

        for scored in (False, True):
            preset.write_text(text + ("rounds-to-win=2\n" if scored else ""))
            r = run(cmd, cwd=root, capture_output=True, text=True, timeout=55)
            log = r.stderr
            assert r.returncode == 0 and "frames=170 actors=3 restarts=1" in log, log[-5000:]
            timeouts = re.findall(r"borg-round-clock: round=(\d+) tick=(\d+) timeout=(\w+)", log)
            assert timeouts == [("0", "60", "team0"), ("1", "60", "team0")], log[-5000:]
            rendered = re.findall(r"borg-body: round=1 tick=(\d+) actor=0", log)
            assert rendered and max(map(int, rendered)) == 60 and rendered[-3:] == ["60"] * 3, log[-5000:]
            points = re.findall(r"borg-series: round=\d+ tick=60 outcome=team0 score=(\d+:\d+) goal=2 winner=(\d+)", log)
            assert points == ([("1:0", "255"), ("2:0", "0")] if scored else []), log[-5000:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster clock: time-only and scored rounds end at tick 60, restart and freeze correctly "
          f"({'OpenGL' if args.opengl else 'headless'})")
