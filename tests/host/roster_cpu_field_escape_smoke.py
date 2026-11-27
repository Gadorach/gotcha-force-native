"""Check a melee CPU's walk out of the field that interrupted its windup."""
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
    with TemporaryDirectory(prefix="gf-roster-field-escape-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        stage = prepare_stage(root)
        with (stage / "stage.manifest").open("a") as manifest:
            manifest.write("spawn=idle|500|0|-520\nspawn=field|0|0|-410\nspawn=hero|0|0|-550\n"
                           "spawn=clear|250|0|-630\n")
        field_borg = root / "borgs/field-stick"
        copytree(root / "borgs/flight-stick", field_borg)
        manifest = field_borg / "borg.manifest"
        manifest.write_text(manifest.read_text().replace("id=example:flight-stick", "id=example:field-stick"))
        base = "GFBORG-GAMEPLAY 1\nvariant=normal\nrig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        motion = "walk=0:1\nair=0:2\ndash=0:3\n"
        (root / "borgs/flight-stick/gameplay.txt").write_text(base + "health=1000\nmovement=stationary-v1\n")
        field_rules = field_borg / "gameplay.txt"
        field_rules.write_text(
            base + "health=1000\nmovement=grounded-v1|0.4|13|0.4\n" + motion +
            "stock=zone|1|180\nmove=pulse|b|10|grounded|20|0:4\n"
            "field=pulse|1|sphere|190|190|190|60|0|50|90|150|250|150\n"
            "field-core-damage=pulse|enemies|190|8|6\nfield-debit=pulse|zone|1\n"
        )
        (root / "borgs/jump-stick/gameplay.txt").write_text(
            base + "health=250\nmovement=grounded-v1|3.29|13|15\n" + motion +
            "move=slash|b|10|grounded|24|0:4\nrange=slash|150\nmelee=slash|9|14|88|38|52|44|12\n"
        )
        preset = root / "escape.roster"
        preset.write_text(
            "GFROSTER 1\nlocals=1\nstage=example:workshop\nround-seconds=5\n"
            "fighter=0|example:flight-stick|normal|idle\n"
            "fighter=1|example:jump-stick|normal|hero\n"
            "fighter=0|example:field-stick|normal|field\n"
        )
        # The demo gives deterministic ticks on OpenGL and restarts once.
        mode, frames, round_no = ("--demo-package-battle", "240", 1) if opengl else ("--headless", "160", 0)
        def play():
            battle = run([binary, mode, "--no-audio", "--frames", frames, "--trace-package-battle",
                          "--battle-roster", preset], cwd=root, capture_output=True, text=True, timeout=45)
            log = battle.stderr
            assert battle.returncode == 0 and f"frames={frames} actors=3 restarts={round_no}" in log, log[-4500:]
            return log

        log = play()
        tick = fr"borg-api: round={round_no} tick="
        assert re.search(tick + r"\d+ actor=1 move=slash event=interrupt ", log), log[-4500:]
        pulses = re.findall(tick + r"(\d+) actor=2 move=pulse event=hit .*recipient=1 ", log)
        hits = re.findall(tick + r"(\d+) actor=1 move=slash event=hit .*recipient=2 ", log)
        assert 0 < len(pulses) < 9 and any(int(n) > 87 for n in hits), (pulses, hits, log[-4500:])

        preset.write_text(preset.read_text().replace("|normal|idle\n", "|normal|clear\n"))
        field_rules.write_text(field_rules.read_text().replace("|190|190|190|60|", "|190|190|190|110|"))
        log = play()
        placed = re.search(tick + r"(\d+) actor=2 move=pulse event=field-placed ", log)
        interrupted = re.search(tick + r"(\d+) actor=1 move=slash event=interrupt ", log)
        alternate = re.search(tick + r"(\d+) actor=1 move=slash event=hit .*recipient=0 ", log)
        assert placed and interrupted and alternate, log[-4500:]
        assert int(interrupted[1]) < int(alternate[1]) < int(placed[1]) + 110, (placed[1], interrupted[1], alternate[1])


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster field escape: walk out, wait when blocked, or strike a clear rival "
          f"({'OpenGL' if args.opengl else 'headless'})")
