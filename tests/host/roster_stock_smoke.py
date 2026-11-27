"""Test CPU fallback and real refills for paid shots and fields."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-roster-stock-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        prepare_stage(root)
        (root / "borgs/jump-stick/gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=stationary-v1\nhealth=1000\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        )
        (root / "borgs/flight-stick/gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|8|13|14\nhealth=200\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "stock=mag|1|55\nprojectile=shot|bullet|1|55|0|0|0|8|100|10|ignore-floor\n"
            "move=primary|b|10|any|10|0:4\nentry-stock=primary|mag|1\nrelease=primary|2|mag|1|shot\n"
            "move=alternate|x|20|any|10|0:4\nrelease=alternate|2|none|0|shot\n"
        )
        (root / "stock.roster").write_text(
            "GFROSTER 1\nstage=example:workshop\nfighter=0|example:jump-stick|normal|west\n"
            "fighter=0|example:jump-stick|normal|west-outer\nfighter=1|example:flight-stick|normal|east\n"
        )
        cmd = [binary, "--battle-roster", "stock.roster", "--demo-package-battle", "--trace-package-battle",
               "--no-audio", "--frames", "80"]
        if not opengl:
            cmd.append("--headless")
        result = run(cmd, cwd=root, capture_output=True, text=True, timeout=50)
        log = result.stderr
        assert result.returncode == 0 and "actors=3 restarts=0" in log, log[-4500:]
        emitted = re.findall(r"borg-api: round=0 tick=\d+ actor=2 move=(\S+) event=emit ", log)
        assert emitted == ["primary", "alternate", "primary"], log[-5000:]
        assert "actor=2 move=- event=refill " in log, log[-5000:]
        assert not re.search(r"actor=2 move=\S+ event=(entry-)?stock-refused ", log), log[-5000:]

        with (root / "stages/workshop/stage.manifest").open("a") as manifest:
            manifest.write("spawn=beacon|0|0|-300\nspawn=waiter|0|0|-418\nspawn=idle|600|0|-800\n")
        (root / "stock.roster").write_text(
            "GFROSTER 1\nstage=example:workshop\nfighter=0|example:jump-stick|normal|beacon\n"
            "fighter=1|example:jump-stick|normal|idle\nfighter=1|example:flight-stick|normal|waiter\n"
        )
        field = (
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|8|13|14\nhealth=200\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "stock=cell|1|55\nmove=paid|b|10|any|14|0:4\nfield=paid|3|sphere|168|168|168|14|0|50|90|150|250|150\n"
            "field-core-damage=paid|enemies|165|9|2\nfield-debit=paid|cell|1\n"
        )
        alternate = ("projectile=shot|bullet|1|55|0|0|0|8|45|9|ignore-floor\n"
                     "move=alternate|x|1|any|10|0:4\nrelease=alternate|2|none|0|shot\n")
        for fallback in (False, True):
            (root / "borgs/flight-stick/gameplay.txt").write_text(field + (alternate if fallback else ""))
            result = run(cmd, cwd=root, capture_output=True, text=True, timeout=50)
            log = result.stderr
            assert result.returncode == 0 and "actors=3 restarts=0" in log, log[-5000:]
            starts = re.findall(r"borg-api: round=0 tick=(\d+) actor=2 move=(\S+) event=start ", log)
            assert [move for _, move in starts] == (["paid", "alternate", "paid"] if fallback else ["paid", "paid"]), log[-5000:]
            refill = re.search(r"borg-api: round=0 tick=(\d+) actor=2 move=- event=refill ", log)
            assert refill and int(starts[-1][0]) > int(refill[1]) and log.count("actor=2 move=paid event=field-placed ") == 2
            assert not re.search(r"actor=2 move=paid event=(stock-refused|field-refused) ", log), log[-5000:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster stock: paid shots and fields wait for refills or use X, without empty attempts "
          f"({'OpenGL' if args.opengl else 'headless'})")
