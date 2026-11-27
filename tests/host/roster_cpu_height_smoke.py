"""Exercise CPU height checks for melee and paid fields."""
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
    with TemporaryDirectory(prefix="gf-roster-height-") as scratch:
        root = Path(scratch) / "content"
        prepare(root, cruise_boost=True)
        stage = prepare_stage(root)
        with (stage / "stage.manifest").open("a") as manifest:
            manifest.write("spawn=cruiser|0|0|-160\nspawn=pursuer|0|0|-280\n")
        melee = root / "borgs/jump-stick/gameplay.txt"
        melee.write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|8|13|14\nhealth=500\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "move=slash|b|10|any|14|0:4\nrange=slash|150\nmelee=slash|4|7|95|24|60|35|19\n"
        )
        cruiser = root / "borgs/flight-stick/gameplay.txt"
        recipe = cruiser.read_text()
        assert "cruise-boost=80|" in recipe and "movement=grounded-v1|7|11|14" in recipe
        recipe = recipe.replace("movement=grounded-v1|7|11|14", "movement=grounded-v1|0.4|11|0.4")
        cmd = [binary, "--no-audio", "--frames", "72", "--demo-package-battle", "--trace-package-battle",
               "--battle-borg", "example:jump-stick", "--borg-variant", "normal",
               "--against-borg", "example:flight-stick", "--against-variant", "normal",
               "--roster-borg", "0", "example:jump-stick", "--battle-stage", "example:workshop",
               "--stage-spawns", "west", "cruiser", "pursuer"]
        if not opengl:
            cmd.append("--headless")

        for height, expected_hit in ((140, True), (500, False)):
            cruiser.write_text(recipe.replace("cruise-boost=80|", f"cruise-boost={height}|"))
            r = run(cmd, cwd=root, capture_output=True, text=True, timeout=50)
            log = r.stderr
            assert not r.returncode and "actors=3 restarts=0" in log, log[-4500:]
            hit = re.search(r"actor=2 move=slash event=hit .* recipient=1 ", log)
            assert bool(hit) == expected_hit, log[-5000:]
            if not expected_hit:
                assert "actor=2 move=slash event=start " not in log, log[-5000:]

        target = root / "borgs/target-fixture"
        copytree(root / "borgs/jump-stick", target)
        target_manifest = target / "borg.manifest"
        target_manifest.write_text(target_manifest.read_text().replace("id=example:jump-stick", "id=example:target-fixture"))
        (target / "gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=stationary-v1\nhealth=500\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        )
        # Keep the flyer in range horizontally. Its placeholder move is
        # grounded-only, and never starts while it cruises.
        cruiser.write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|0.4|11|0.4\nhealth=500\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "cruise-boost=500|18|4|2|7|4\n"
            "projectile=shot|bullet|1|55|0|0|0|8|45|19|ignore-floor\n"
            "move=parked|b|10|grounded|8|0:4\nrelease=parked|2|none|0|shot\n"
        )
        melee.write_text(melee.read_text().replace("|10|any|", "|10|grounded|"))
        with (stage / "stage.manifest").open("a") as manifest:
            manifest.write("spawn=lower-rival|0|0|-610\nspawn=beside-rival|135|0|-280\n")
        preset = root / "cruise-rival.roster"
        mode = "--demo-package-battle" if opengl else "--headless"
        for lower_team in (0, 1):
            preset.write_text(
                "GFROSTER 1\nlocals=1\nstage=example:workshop\n"
                "fighter=0|example:flight-stick|normal|cruiser\n"
                f"fighter={lower_team}|example:target-fixture|normal|lower-rival\n"
                "fighter=1|example:jump-stick|normal|pursuer\n"
            )
            result = run([binary, mode, "--no-audio", "--frames", "78", "--trace-package-battle",
                          "--battle-roster", preset], cwd=root, capture_output=True, text=True, timeout=50)
            log = result.stderr
            assert result.returncode == 0 and "frames=78 actors=3 restarts=0" in log, log[-4000:]
            recipients = set(re.findall(r"borg-api: round=0 tick=\d+ actor=2 move=slash event=hit .* recipient=(\d+) ", log))
            assert recipients == ({"1"} if lower_team == 0 else set()), log[-5000:]

        melee.write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|8|13|14\nhealth=500\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "stock=cell|1|80\nmove=paid|b|10|grounded|14|0:4\n"
            "field=paid|3|sphere|168|168|168|14|0|50|90|150|250|150\n"
            "field-core-damage=paid|all|165|9|2\nfield-debit=paid|cell|1\n"
        )
        manifest = stage / "stage.manifest"
        layout = manifest.read_text()
        for other_team, marker, sealed in ((0, "lower-rival", False), (0, "beside-rival", False),
                                           (1, "beside-rival", False), (0, "lower-rival", True)):
            manifest.write_text(layout + ("body-partition=sealed-lower|-1800|-480|1800|-470\n" if sealed else ""))
            preset.write_text(
                "GFROSTER 1\nlocals=1\nstage=example:workshop\n"
                "fighter=0|example:flight-stick|normal|cruiser\n"
                f"fighter={other_team}|example:target-fixture|normal|{marker}\n"
                "fighter=1|example:jump-stick|normal|pursuer\n"
            )
            result = run([binary, mode, "--no-audio", "--frames", "74", "--trace-package-battle",
                          "--battle-roster", preset], cwd=root, capture_output=True, text=True, timeout=50)
            log = result.stderr
            assert result.returncode == 0 and "frames=74 actors=3 restarts=0" in log, log[-4000:]
            reaches_enemy = other_team == 0 and not sealed
            recipients = set(re.findall(r"borg-api: round=0 tick=\d+ actor=2 move=paid event=hit .* recipient=(\d+) ", log))
            assert recipients == ({"1"} if reaches_enemy else set()), log[-5000:]
            assert ("actor=2 move=paid event=debit " in log) == reaches_enemy, log[-5000:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster CPU height: melee and fields can pursue a lower rival without attacking allies or through a sealed wall "
          f"({'OpenGL' if args.opengl else 'headless'})")
