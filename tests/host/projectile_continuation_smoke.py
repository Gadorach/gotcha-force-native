"""Compare default and opt-in recipient continuation in an authored roster."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(executable, opengl):
    executable = executable.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-projectile-continuation-") as temp:
        root = Path(temp) / "content"
        prepare(root)
        stage = prepare_stage(root)
        with (stage / "stage.manifest").open("a") as manifest:
            manifest.write("spawn=continuation-shooter|0|0|-550\n"
                           "spawn=continuation-first|0|0|-410\n"
                           "spawn=continuation-second|0|0|-270\n")
        preset = root / "continuation.roster"
        preset.write_text(
            "GFROSTER 1\nlocals=2\nstage=example:workshop\n"
            "fighter=0|example:jump-stick|normal|continuation-shooter\n"
            "fighter=1|example:flight-stick|normal|continuation-first\n"
            "fighter=1|example:flight-stick|normal|continuation-second\n",
            encoding="ascii",
        )
        basic = ("GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=180\n"
                 "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n")
        (root / "borgs/flight-stick/gameplay.txt").write_text(basic + "movement=stationary-v1\n", encoding="ascii")
        attack = basic + (
            "movement=grounded-v1|0.4|13|0.4\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "stock=primary|1|180\nprojectile=bolt|bullet|1|38|0|0|0|8|24|19|ignore-floor\n"
            "move=fire|b|10|grounded|24|0:4\nrelease=fire|2|primary|1|bolt\n"
        )
        args = ["--battle-roster", preset, "--demo-package-battle", "--trace-package-battle",
                "--frames", "40", "--no-audio"]
        if not opengl:
            args.append("--headless")
        for suffix, expected in (("", [1]), ("actor-contact=bolt|continue-once-per-actor\n", [1, 2])):
            (root / "borgs/jump-stick/gameplay.txt").write_text(attack + suffix, encoding="ascii")
            result = run([executable, *args], cwd=root, capture_output=True, text=True, timeout=45)
            hits = re.findall(r"tick=(\d+) actor=0 move=fire event=hit frame=\d+ object=(\d+) recipient=(\d+) value=(\d+)",
                              result.stderr)
            assert result.returncode == 0 and [int(row[2]) for row in hits] == expected, result.stderr[-4000:]
            assert {int(row[3]) for row in hits} == {19}, hits
            assert len({row[1] for row in hits}) == 1, hits
            assert all(int(a[0]) < int(b[0]) for a, b in zip(hits, hits[1:])), hits
    print(f"Authored projectile recipients: first-contact default and opt-in continuation ({'OpenGL' if opengl else 'headless'})")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--opengl", action="store_true")
    flags = parser.parse_args()
    smoke(flags.executable, flags.opengl)
