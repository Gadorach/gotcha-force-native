"""Short-range cruise CPUs must route around a partition before using their field."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-roster-cruise-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        manifest = prepare_stage(root) / "stage.manifest"
        text = manifest.read_text() + "spawn=beacon|430|0|210\nspawn=ally|585|0|400\nspawn=cruiser|430|0|-380\n"
        (root / "borgs/flight-stick/gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=stationary-v1\nhealth=400\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        )
        (root / "borgs/jump-stick/gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|4|13|18\nhealth=400\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "cruise-boost=50|160|2|2|12|4\nmove=wing|x|10|airborne|14|0:4\n"
            "field=wing|2|sphere|170|170|170|14|0|40|90|150|250|150\n"
            "field-core-damage=wing|enemies|165|9|2\nfield-follow=wing|source\n"
        )
        (root / "route.roster").write_text(
            "GFROSTER 1\nstage=example:workshop\n"
            "fighter=0|example:flight-stick|normal|beacon\n"
            "fighter=0|example:flight-stick|normal|ally\n"
            "fighter=1|example:jump-stick|normal|cruiser\n"
        )
        # Demo input fixes the OpenGL tick cadence; both local fixtures are
        # stationary and have no attacks, and the run ends before its restart.
        mode = "--demo-package-battle" if opengl else "--headless"
        for sealed in (False, True):
            manifest.write_text(text.replace("workshop-wall|-620|-6|620|6", "workshop-wall|-1800|-6|1800|6") if sealed else text)
            battle = run([binary, mode, "--battle-roster", "route.roster", "--frames", "78",
                          "--no-audio", "--trace-package-battle"], cwd=root, capture_output=True, text=True, timeout=45)
            log = battle.stderr
            assert battle.returncode == 0 and "frames=78 actors=3 restarts=0" in log, log[-4000:]
            assert "actor=2 team=1 driver=cpu id=example:jump-stick" in log
            hits = re.findall(r"borg-api: round=0 tick=(\d+) actor=2 move=wing event=hit .*recipient=0 ", log)
            assert bool(hits) == (not sealed), log[-4000:]
            if sealed:
                assert "actor=2 move=wing event=start " not in log


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster CPU cruise: clear detour reaches a real rival; sealed route holds fire PASS "
          f"({'OpenGL' if args.opengl else 'headless'})")
