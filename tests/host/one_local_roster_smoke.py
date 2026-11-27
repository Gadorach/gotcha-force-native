"""Play a roster against CPUs with no second local player."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    preset = Path(__file__).resolve().parents[2] / "examples/battle-rosters/workshop-solo.roster"
    with TemporaryDirectory(prefix="gf-one-local-roster-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        prepare_stage(root)
        cmd = [binary, "--no-audio", "--frames", "125", "--demo-package-battle",
               "--trace-package-battle", "--battle-roster", preset]
        if opengl:
            cmd.extend(("--screenshot", "solo.ppm"))
        else:
            cmd.append("--headless")
        r = run(cmd, cwd=root, capture_output=True, text=True, timeout=55)
        log = r.stderr
        assert r.returncode == 0 and "actors=6 restarts=1" in log, log[-4000:]
        assert "stage=example:workshop p1-spawn=west locals=1" in log and "local-coop=1" not in log
        slots = re.findall(r"borg-roster-slot: actor=(\d+) team=(\d+) driver=(\w+)", log)
        assert slots == [("0", "0", "local"), ("1", "0", "cpu"), ("2", "0", "cpu"),
                         ("3", "1", "cpu"), ("4", "1", "cpu"), ("5", "1", "cpu")], slots
        emitted = set(re.findall(r"borg-api: round=\d+ tick=\d+ actor=([1-5]) .*event=emit ", log))
        assert emitted == {"1", "2", "3", "4", "5"}, log[-4500:]
        for kind in ("borg-aim-cycle", "borg-ammo", "borg-mobility"):
            subjects = set(re.findall(fr"{kind}: round=\d+ tick=\d+ actor=(\d+)", log))
            assert subjects == {"0"}, (kind, subjects)
        if opengl:
            header, _, pixels = (root / "solo.ppm").read_bytes().partition(b"\n255\n")
            assert header.startswith(b"P6\n") and pixels.count(bytes((255, 205, 90))) >= 32
            assert pixels.count(bytes((115, 220, 255))) < 32, "the CPU seat must not show P2's aim label"


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Workshop one-local roster: CPU ally and rivals, P1 controls and HUD, restart PASS "
          f"({'OpenGL' if args.opengl else 'headless'})")
