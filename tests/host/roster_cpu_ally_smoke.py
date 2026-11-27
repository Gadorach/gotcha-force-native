"""Check short-range CPU sidesteps around a solid ally in Workshop."""
from argparse import ArgumentParser
from math import hypot
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-roster-ally-pass-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        manifest = prepare_stage(root) / "stage.manifest"
        layout = manifest.read_text() + (
            "spawn=target|0|0|-260\nspawn=parked-ally|0|0|-475\nspawn=pursuer|0|0|-590\n"
        )
        lanes = ("body-partition=left-flank|-180|-650|-50|-390\n",
                 "body-partition=right-flank|50|-650|180|-390\n")
        base = "GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=500\nrig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        (root / "borgs/flight-stick/gameplay.txt").write_text(base + "movement=stationary-v1\n")
        (root / "borgs/jump-stick/gameplay.txt").write_text(
            base + "movement=grounded-v1|8|13|14\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "move=slash|b|10|grounded|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n"
        )
        (root / "ally-pass.roster").write_text(
            "GFROSTER 1\nlocals=1\nstage=example:workshop\n"
            "fighter=0|example:flight-stick|normal|target\n"
            "fighter=1|example:flight-stick|normal|parked-ally\n"
            "fighter=1|example:jump-stick|normal|pursuer\n"
        )
        mode = "--demo-package-battle" if opengl else "--headless"
        for count in range(3):
            manifest.write_text(layout + "".join(lanes[:count]))
            battle = run([binary, mode, "--battle-roster", "ally-pass.roster", "--frames", "78",
                          "--no-audio", "--trace-package-battle"], cwd=root, capture_output=True, text=True, timeout=45)
            log = battle.stderr
            assert battle.returncode == 0 and "frames=78 actors=3 restarts=0" in log, log[-4500:]
            hits = re.findall(r"borg-api: round=0 tick=\d+ actor=2 move=slash event=hit .*recipient=(\d+) ", log)
            assert set(hits) == ({"0"} if count < 2 else set()), log[-4500:]
            if count == 2:
                assert "actor=2 move=slash event=start " not in log

        manifest.write_text(layout + (
            "body-partition=left-pass|-230|-755|-126|-245\n"
            "body-partition=right-stop|65|-755|125|-575\n"
            "spawn=off-target|0|0|-295\nspawn=off-ally|35|0|-515\nspawn=off-cpu|0|0|-625\n"
        ))
        (root / "offset-pass.roster").write_text(
            "GFROSTER 1\nstage=example:workshop\n"
            "fighter=0|example:flight-stick|normal|off-target\n"
            "fighter=1|example:flight-stick|normal|off-ally\n"
            "fighter=1|example:jump-stick|normal|off-cpu\n"
        )
        battle = run([binary, mode, "--battle-roster", "offset-pass.roster", "--frames", "78",
                      "--no-audio", "--trace-package-battle"], cwd=root, capture_output=True, text=True, timeout=45)
        log = battle.stderr
        assert battle.returncode == 0 and "frames=78 actors=3 restarts=0" in log, log[-4500:]
        hits = re.findall(r"borg-api: round=0 tick=\d+ actor=2 move=slash event=hit .*recipient=(\d+) ", log)
        assert set(hits) == {"0"}, log[-4500:]

        # Put the parked ally directly on the first leg to the wall's
        # eastern corner. The pursuer must pass both objects, not just
        # find a straight shot at the target.
        dx, dz = 378, 236
        scale = 125 / hypot(dx, dz)
        manifest.write_text(layout + (
            "spawn=routed-target|500|0|170\nspawn=routed-cpu|300|0|-300\n"
            f"spawn=routed-ally|{300 + dx * scale:.6f}|0|{-300 + dz * scale:.6f}\n"
        ))
        (root / "routed-pass.roster").write_text(
            "GFROSTER 1\nstage=example:workshop\nround-seconds=4\n"
            "fighter=0|example:flight-stick|normal|routed-target\n"
            "fighter=1|example:flight-stick|normal|routed-ally\n"
            "fighter=1|example:jump-stick|normal|routed-cpu\n"
        )
        battle = run([binary, mode, "--battle-roster", "routed-pass.roster", "--frames", "225",
                      "--no-audio", "--trace-package-battle"], cwd=root, capture_output=True, text=True, timeout=45)
        log = battle.stderr
        assert battle.returncode == 0 and "frames=225 actors=3 " in log, log[-4500:]
        hits = re.findall(r"borg-api: round=\d+ tick=\d+ actor=2 move=slash event=hit .*recipient=(\d+) ", log)
        assert set(hits) == {"0"}, log[-4500:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster ally pass: direct flanks, offset fallback and routed legs work; two blocked flanks hold "
          f"({'OpenGL' if args.opengl else 'headless'})")
