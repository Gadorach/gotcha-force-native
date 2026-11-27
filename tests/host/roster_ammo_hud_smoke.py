"""Check the local roster ammo readout through spend, refill and restart."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def replace(path, before, after):
    text = path.read_text()
    assert text.count(before) == 1
    path.write_text(text.replace(before, after))


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    preset = Path(__file__).resolve().parents[2] / "examples/battle-rosters/workshop-coop.roster"
    with TemporaryDirectory(prefix="gf-ammo-hud-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        prepare_stage(root)
        replace(root / "borgs/flight-stick/gameplay.txt", "stock=paint|5|90", "stock=paint|1|24")
        replace(root / "borgs/jump-stick/gameplay.txt",
                "release=lined-shot|12|none|0|long-paint", "release=lined-shot|12|paint|1|long-paint")
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
        rows = re.findall(r"borg-ammo: round=(\d+) tick=(\d+) actor=(\d+) pool=(\S+) "
                          r"buttons=(\S+) stock=(\d+/\d+) phase=(\w+)", log)
        assert rows and all(row[2] in {"0", "1"} for row in rows), rows
        for round_id in ("0", "1"):
            first = [row for row in rows if row[0] == round_id and row[2] == "0"]
            p1 = [row[3:] for row in first]
            p2 = [row[3:] for row in rows if row[0] == round_id and row[2] == "1"]
            assert p1 == [("paint", "B", "1/1", "ready"), ("paint", "B", "0/1", "refill"),
                          ("paint", "B", "1/1", "ready")], p1
            assert p2 == [("paint", "B/X", "5/5", "ready")], p2
            for row, event in zip(first[1:], ("debit", "refill"), strict=True):
                assert re.search(fr"borg-api: round={round_id} tick={row[1]} actor=0 .*event={event} ", log), row
        if opengl:
            capture = launch("--frames", "18", "--screenshot", "ammo.ppm")
            assert "actor=0 pool=paint buttons=B stock=0/1 phase=refill" in capture, capture[-4500:]
            header, _, pixels = (root / "ammo.ppm").read_bytes().partition(b"\n255\n")
            assert b"640 480" in header, header
            lower_left = b"".join(pixels[y*640*3:(y*640+320)*3] for y in range(240, 480))
            assert lower_left.count(bytes((255, 222, 92))) >= 32, "missing visible P1 refill counter"


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster ammo: local slots, shared B/X pool, live refill and restart PASS "
          f"({'OpenGL' if args.opengl else 'headless'})")
