"""Exercise CPU detours and direct melee partitions using authored content."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-partition-cpu-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        stage = prepare_stage(root)
        profile = root / "borgs/flight-stick/gameplay.txt"
        profile.write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|10|13|15\nhealth=160\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "move=slash|b|10|grounded|14|0:4\nrange=slash|150\nmelee=slash|4|7|150|40|55|42|12\n"
        )
        manifest = stage / "stage.manifest"
        text = manifest.read_text() + "spawn=pursuer|590|0|-82\nspawn=near-target|560|0|140\n"
        manifest.write_text(text)
        args = [binary, "--no-audio", "--demo-package-battle", "--frames", "145", "--trace-package-battle",
                "--battle-borg", "example:flight-stick", "--borg-variant", "normal",
                "--against-borg", "example:jump-stick", "--against-variant", "normal",
                "--roster-borg", "0", "example:flight-stick", "--battle-stage", "example:workshop",
                "--stage-spawns", "west", "near-target", "pursuer"]
        if not opengl:
            args.append("--headless")

        def launch():
            result = run(args, cwd=root, capture_output=True, text=True, timeout=50)
            log = result.stderr
            assert result.returncode == 0 and "actors=3 restarts=1" in log, log[-4000:]
            return log

        log = launch()
        assert re.search(r"borg-api: round=0 tick=\d+ actor=2 move=slash event=hit .*recipient=1 ", log), log[-5000:]
        sealed_stage = text.replace("body-partition=workshop-wall|-620|-6|620|6",
                                    "body-partition=workshop-wall|-1800|-6|1800|6")
        manifest.write_text(sealed_stage)
        sealed = launch()
        assert "actor=2 move=slash event=start" not in sealed, sealed[-5000:]

        # The nearer local is unreachable across the sealed wall. The other
        # local starts farther away, but on the CPU's side of it.
        manifest.write_text(sealed_stage + "spawn=blocked-rival|0|0|150\n"
                            "spawn=clear-rival|0|0|-430\nspawn=hunter|0|0|-100\n")
        preset = root / "opponent-choice.roster"
        preset.write_text("GFROSTER 1\nstage=example:workshop\n"
                          "fighter=0|example:flight-stick|normal|blocked-rival\n"
                          "fighter=0|example:flight-stick|normal|clear-rival\n"
                          "fighter=1|example:flight-stick|normal|hunter\n")
        choice_args = [*args[:args.index("--battle-borg")], "--battle-roster", preset]
        if not opengl:
            choice_args.append("--headless")
        choice = run(choice_args, cwd=root, capture_output=True, text=True, timeout=50)
        assert choice.returncode == 0 and "local-coop=1" in choice.stderr, choice.stderr[-4000:]
        assert re.search(r"borg-api: round=0 tick=\d+ actor=2 move=slash event=hit .*recipient=1 ",
                         choice.stderr), choice.stderr[-5000:]

        # Here two local fighters start close enough to reach through the
        # painted wall. The scripted B opens in both modes; only the flag
        # changes the actual contact, not the motion partition.
        close = text + "spawn=striker|0|0|-55\nspawn=defender|0|0|55\n"
        duel = [*args[:args.index("--roster-borg")], "--battle-stage", "example:workshop",
                "--stage-spawns", "striker", "defender", "--frames", "50"]
        if not opengl:
            duel.append("--headless")
        for flagged in (True, False):
            manifest.write_text(close if flagged else close.replace("|620|6|melee\n", "|620|6\n"))
            result = run(duel, cwd=root, capture_output=True, text=True, timeout=50)
            log = result.stderr
            assert result.returncode == 0 and "actor=0 move=slash event=start" in log, log[-4000:]
            hit = "actor=0 move=slash event=hit" in log
            assert hit == (not flagged), log[-4000:]

        # A legal corner strike is narrower than a body's walking path.
        # Give the CPU a farther rival with a clear walking path as well.
        (root / "borgs/jump-stick/gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=stationary-v1\nhealth=400\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
        )
        corner = text + ("spawn=corner-rival|619|0|88\nspawn=corner-cpu|680|0|-30\n"
                         "spawn=far-rival|1020|0|-100\nspawn=across-rival|0|0|65\nspawn=across-cpu|0|0|-65\n")
        preset = root / "corner.roster"
        mode = "--demo-package-battle" if opengl else "--headless"
        for label, flagged, expected_hit in (("corner", True, True), ("across", True, False), ("across", False, True)):
            manifest.write_text(corner if flagged else corner.replace("|620|6|melee\n", "|620|6\n"))
            preset.write_text(
                "GFROSTER 1\nstage=example:workshop\n"
                f"fighter=0|example:jump-stick|normal|{label}-rival\n"
                "fighter=0|example:jump-stick|normal|far-rival\n"
                f"fighter=1|example:flight-stick|normal|{label}-cpu\n"
            )
            result = run([binary, mode, "--no-audio", "--trace-package-battle", "--frames", "14",
                          "--battle-roster", preset], cwd=root, capture_output=True, text=True, timeout=45)
            log = result.stderr
            assert result.returncode == 0 and "frames=14 actors=3 restarts=0" in log, log[-5000:]
            starts = re.findall(r"borg-api: round=0 tick=(\d+) actor=2 move=slash event=start ", log)
            hits = set(re.findall(r"borg-api: round=0 tick=\d+ actor=2 move=slash event=hit .*recipient=(\d+) ", log))
            assert starts == (["1"] if expected_hit else []) and hits == ({"0"} if expected_hit else set()), log[-5000:]

        routes = ("spawn=cost-near|-120|0|180\nspawn=cost-far|620|0|200\nspawn=cost-cpu|0|0|-180\n",
                  "spawn=cost-near|615|0|140\nspawn=cost-far|-700|0|-200\nspawn=cost-cpu|600|0|-170\n")
        preset.write_text(
            "GFROSTER 1\nstage=example:workshop\nround-seconds=5\n"
            "fighter=0|example:jump-stick|normal|cost-near\n"
            "fighter=0|example:jump-stick|normal|cost-far\n"
            "fighter=1|example:flight-stick|normal|cost-cpu\n"
        )
        for route, recipient in zip(routes, ("1", "0"), strict=True):
            manifest.write_text(text + route)
            result = run([binary, mode, "--no-audio", "--trace-package-battle", "--frames", "210",
                          "--battle-roster", preset], cwd=root, capture_output=True, text=True, timeout=45)
            log = result.stderr
            assert result.returncode == 0 and "frames=210 actors=3 " in log, log[-5000:]
            hits = set(re.findall(r"borg-api: round=\d+ tick=\d+ actor=2 move=slash event=hit .*recipient=(\d+) ", log))
            assert hits == {recipient}, log[-5000:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    opts = parser.parse_args()
    smoke(opts.binary, opts.opengl)
    print(f"Workshop: CPU detours, reachable rivals, sealed routes and melee occlusion PASS "
          f"({'OpenGL' if opts.opengl else 'headless'})")
