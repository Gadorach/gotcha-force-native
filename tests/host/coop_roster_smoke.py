"""Play the published Workshop co-op preset with generated stage and Borg art."""
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
    with TemporaryDirectory(prefix="gf-coop-roster-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        prepare_stage(root)
        cmd = [binary, "--no-audio", "--frames", "125", "--demo-package-battle",
               "--trace-package-battle", "--battle-roster", preset]
        if not opengl:
            cmd.append("--headless")
        result = run(cmd, cwd=root, capture_output=True, text=True, timeout=50)
        log = result.stderr
        assert not result.returncode and "actors=4 restarts=1" in log, log[-4600:]
        assert "stage=example:workshop p1-spawn=west p2-spawn=west-outer local-coop=1" in log, log[-4600:]
        slots = re.findall(r"borg-roster-slot: actor=(\d+) team=(\d+) driver=(\w+)", log)
        assert slots == [("0", "0", "local"), ("1", "0", "local"), ("2", "1", "cpu"), ("3", "1", "cpu")], slots
        cycles = re.findall(r"borg-aim-cycle: round=0 tick=(\d+) actor=(\d+) target=(\d+)", log)
        assert cycles == [("3", "0", "3"), ("3", "1", "3"), ("7", "0", "2"), ("7", "1", "2")], cycles
        hits = re.findall(r"borg-api: .*? actor=(\d+) move=\S+ event=hit .*? recipient=(\d+) ", log)
        assert hits and all((int(a) < 2) != (int(b) < 2) for a, b in hits), hits
        if opengl:
            # At frame 8 both locals have just chosen the same CPU slot.
            capture = run([*cmd, "--frames", "8", "--screenshot", "coop.ppm"], cwd=root,
                          capture_output=True, text=True, timeout=30)
            assert not capture.returncode and "actor=0 target=2" in capture.stderr and "actor=1 target=2" in capture.stderr, (
                capture.stderr[-4600:]
            )
            header, _, pixels = (root / "coop.ppm").read_bytes().partition(b"\n255\n")
            assert header.startswith(b"P6\n") and all(pixels.count(bytes(rgb)) >= 64
                                                       for rgb in ((255, 205, 90), (115, 220, 255))), (
                "the two aim labels should both be visible over the shared target"
            )
        # A proximity gate around P2's spawn only. Keep the same wall mesh
        # and collision data; the shared viewport should still fade it.
        (root / "stages/workshop/art/occlusion.txt").write_text(
            "GFSTAGE-FADE 3\nobstacle=near|2|3|40|0.25|0.5|0.025|-605|0|295|-595|5|305\n"
        )
        near = run([*cmd, "--frames", "8"], cwd=root, capture_output=True, text=True, timeout=30)
        assert not near.returncode and "stage-view: viewer=0 tick=1 alpha=0.500" in near.stderr, near.stderr[-4600:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Workshop co-op: allied locals, shared aim, P2 obstacle fade and restart PASS "
          f"({'OpenGL' if args.opengl else 'headless'})")
