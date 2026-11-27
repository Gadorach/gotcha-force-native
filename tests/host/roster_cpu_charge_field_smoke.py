"""Check the CPU's early start for a ground charge carrying a damaging field."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-roster-charge-field-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        manifest = prepare_stage(root) / "stage.manifest"
        layout = manifest.read_text() + "spawn=target|0|0|-260\nspawn=reserve|700|0|-300\nspawn=charge-cpu|0|0|-550\n"
        base = "GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=400\nrig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        (root / "borgs/flight-stick/gameplay.txt").write_text(base + "movement=stationary-v1\n")
        profile = base + (
            "movement=grounded-v1|5|13|15\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "move=charge-hit|x|10|grounded|28|0:4\nmove-charge=charge-hit|20\n"
            "field=charge-hit|3|sphere|168|168|168|30|0|50|90|150|250|150\n"
            "field-core-damage=charge-hit|enemies|165|9|6\n"
        )
        (root / "charge.roster").write_text(
            "GFROSTER 1\nstage=example:workshop\n"
            "fighter=0|example:flight-stick|normal|target\n"
            "fighter=0|example:flight-stick|normal|reserve\n"
            "fighter=1|example:jump-stick|normal|charge-cpu\n"
        )
        mode = "--demo-package-battle" if opengl else "--headless"
        for case in ("clear", "range-gated", "static-field", "partition"):
            rules = profile
            if case != "static-field":
                rules += "field-follow=charge-hit|source\n"
            if case == "range-gated":
                rules += "range=charge-hit|230\n"
            (root / "borgs/jump-stick/gameplay.txt").write_text(rules)
            manifest.write_text(layout + ("body-partition=charge-wall|-1800|-460|1800|-450\n" if case == "partition" else ""))
            battle = run([binary, mode, "--battle-roster", "charge.roster", "--frames", "12",
                          "--no-audio", "--trace-package-battle"], cwd=root, capture_output=True, text=True, timeout=45)
            log = battle.stderr
            assert battle.returncode == 0 and "frames=12 actors=3 restarts=0" in log, (case, log[-4500:])
            starts = re.findall(r"borg-api: round=0 tick=(\d+) actor=2 move=charge-hit event=start ", log)
            hits = re.findall(r"borg-api: round=0 tick=(\d+) actor=2 move=charge-hit event=hit .*recipient=0 ", log)
            assert (starts, hits) == ((["1"], ["10"]) if case == "clear" else ([], [])), (case, log[-4500:])


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster field charge: first-pulse reach, entry range, static field and wall checks PASS "
          f"({'OpenGL' if args.opengl else 'headless'})")
