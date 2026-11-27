"""Exercise a native melee CPU's fallback from a runner to a clear rival."""
from argparse import ArgumentParser
from pathlib import Path
import re
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-roster-pursuit-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        stage = prepare_stage(root)
        with (stage / "stage.manifest").open("a") as manifest:
            manifest.write("spawn=parked|225|0|-900\nspawn=runner|0|0|-770\nspawn=swordsman|0|0|-950\n")
        parked = root / "borgs/parked-stick"
        copytree(root / "borgs/flight-stick", parked)
        manifest = parked / "borg.manifest"
        manifest.write_text(manifest.read_text().replace("id=example:flight-stick", "id=example:parked-stick"))
        base = "GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=1000\nrig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        motion = "walk=0:1\nair=0:2\ndash=0:3\n"
        (parked / "gameplay.txt").write_text(base + "movement=stationary-v1\n")
        (root / "borgs/flight-stick/gameplay.txt").write_text(
            base + "movement=grounded-v1|7.5|13|15\n" + motion +
            "projectile=shot|bullet|1|55|0|0|0|8|45|1|ignore-floor\n"
            "move=shot|b|10|grounded|24|0:4\nrelease=shot|4|none|0|shot\n"
        )
        (root / "borgs/jump-stick/gameplay.txt").write_text(
            base + "movement=grounded-v1|4.58|13|15\n" + motion +
            "move=slash|b|10|grounded|24|0:4\nrange=slash|150\nmelee=slash|9|14|88|38|52|44|12\n"
        )
        preset = root / "pursuit.roster"
        mode = "--demo-package-battle" if opengl else "--headless"
        for ally in (False, True):
            preset.write_text(
                "GFROSTER 1\nlocals=1\nstage=example:workshop\n"
                "fighter=0|example:parked-stick|normal|parked\n"
                f"fighter={int(ally)}|example:flight-stick|normal|runner\n"
                f"fighter={int(not ally)}|example:jump-stick|normal|swordsman\n"
            )
            battle = run([binary, mode, "--no-audio", "--frames", "78", "--trace-package-battle",
                          "--battle-roster", preset], cwd=root, capture_output=True, text=True, timeout=45)
            log = battle.stderr
            assert battle.returncode == 0 and "frames=78 actors=3 restarts=0" in log, log[-4500:]
            hits = set(re.findall(r"borg-api: round=0 tick=\d+ actor=2 move=slash event=hit .*recipient=(\d+) ", log))
            if ally:
                assert "0" not in hits, log[-4500:]
            else:
                assert hits == {"0"}, log[-4500:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster pursuit: melee reaches the clear rival instead of trailing a faster runner "
          f"({'OpenGL' if args.opengl else 'headless'})")
