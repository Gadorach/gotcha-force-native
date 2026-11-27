"""Check that a slow drop CPU closes range without changing its native fuse."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-roster-drop-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        stage = prepare_stage(root)
        with (stage / "stage.manifest").open("a") as manifest:
            manifest.write("spawn=drop-target|0|0|-250\nspawn=reserve|700|0|-250\nspawn=dropper|0|0|-800\n")
        base = "GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=1000\nrig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        (root / "borgs/flight-stick/gameplay.txt").write_text(base + "movement=stationary-v1\n")
        preset = root / "drop.roster"
        preset.write_text(
            "GFROSTER 1\nstage=example:workshop\nround-seconds=5\n"
            "fighter=0|example:flight-stick|normal|drop-target\n"
            "fighter=0|example:flight-stick|normal|reserve\n"
            "fighter=1|example:jump-stick|normal|dropper\n"
        )
        mode = "--demo-package-battle" if opengl else "--headless"
        for speed in (1, 10):
            (root / "borgs/jump-stick/gameplay.txt").write_text(
                base + "movement=grounded-v1|3.33|13|15\nwalk=0:1\nair=0:2\ndash=0:3\n"
                f"stock=cell|1|on-return\nprojectile=mine|bullet|1|{speed}|0|1|0|16|70|0|land-on-floor\n"
                "actor-contact=mine|ignore\nblast=mine|expiry|310|12|opponent\nreturn-stock=mine|expiry\n"
                "move=drop|b|10|grounded|28|0:4\nentry-stock=drop|cell|1\nrelease=drop|12|cell|1|mine\n"
            )
            battle = run([binary, mode, "--no-audio", "--frames", "240", "--trace-package-battle",
                          "--battle-roster", preset], cwd=root, capture_output=True, text=True, timeout=45)
            log = battle.stderr
            assert battle.returncode == 0 and "frames=240 actors=3 " in log, log[-4500:]
            first = re.search(r"borg-api: round=0 tick=(\d+) actor=2 move=drop event=start ", log)
            assert first and (int(first[1]) > 1 if speed == 1 else first[1] == "1"), log[-4500:]
            if speed == 1:
                assert re.search(r"actor=2 move=drop event=blast-hit .*recipient=0 ", log), log[-4500:]
                assert "actor=2 move=drop event=stock-returned " in log, log[-4500:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster drops: slow drop closes to blast range; thrown variant keeps its opening "
          f"({'OpenGL' if args.opengl else 'headless'})")
