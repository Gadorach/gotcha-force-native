"""Check live local movement resources, with generated Borgs and stage art."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    preset = Path(__file__).resolve().parents[2] / "examples/battle-rosters/workshop-coop.roster"
    with TemporaryDirectory(prefix="gf-mobility-hud-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        prepare_stage(root)
        cmd = [binary, "--battle-roster", preset, "--demo-package-battle", "--trace-package-battle",
               "--no-audio", "--frames", "125"]
        if not opengl:
            cmd.append("--headless")

        def launch(*extra):
            result = run([*cmd, *extra], cwd=root, capture_output=True, text=True, timeout=50)
            assert not result.returncode, result.stderr[-4000:]
            return result.stderr

        log = launch()
        assert "actors=4 restarts=1" in log, log[-4000:]
        pattern = r"borg-mobility: round=(\d+) tick=\d+ actor=(\d+) resource=(\S+) remaining=(\d+)/(\d+)"
        rows = re.findall(pattern, log)
        assert rows and all(row[1] in {"0", "1"} for row in rows), rows
        for round_id in ("0", "1"):
            p1 = [row[2:] for row in rows if row[:2] == (round_id, "0")]
            p2 = [row[2:] for row in rows if row[:2] == (round_id, "1")]
            assert p1 and p1[0] == ("lift", "32", "32") and any(int(row[1]) < 32 for row in p1), p1
            assert p2 == [("jumps", "3", "3")], p2
        if opengl:
            capture = launch("--frames", "70", "--screenshot", "mobility.ppm")
            assert "actor=0 resource=lift remaining=31/32" in capture, capture[-4000:]
            header, _, pixels = (root / "mobility.ppm").read_bytes().partition(b"\n255\n")
            assert b"640 480" in header, header
            lower_left = b"".join(pixels[y*640*3:(y*640+320)*3] for y in range(240, 480))
            assert lower_left.count(bytes((137, 205, 246))) >= 32, "missing visible P1 lift counter"

        path = root / "borgs/flight-stick/gameplay.txt"
        text = path.read_text()
        assert text.count("powered-flight=7|32") == 1
        path.write_text(text.replace("powered-flight=7|32", "air-action-budget=3"))
        shared = launch("--frames", "75")
        assert "actor=0 resource=air-actions remaining=3/3" in shared, shared[-4000:]
        assert "actor=0 resource=air-actions remaining=2/3" in shared, shared[-4000:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster mobility: live lift, jump points, shared actions and restart PASS "
          f"({'OpenGL' if args.opengl else 'headless'})")
