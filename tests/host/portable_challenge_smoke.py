"""Run an authored Challenge with a disposable saved Force and Workshop stage."""
from argparse import ArgumentParser
from pathlib import Path
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, fixture, opengl=False):
    binary = binary.resolve(strict=True) if binary is not None else None
    fixture = fixture.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-challenge-smoke-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        prepare_stage(root)
        # A deliberately fragile player makes a deterministic AI KO testable
        # with real simulation ticks and no fake campaign/demo input.
        profile = root / "borgs/flight-stick/gameplay.txt"
        profile.write_text(profile.read_text().replace("health=160", "health=1"))
        saves = root / "fixture-save"
        saves.mkdir()
        run([fixture, saves], check=True, timeout=20)

        def challenge(folder, identifier, ticks, stage="example:workshop", enemy="normal",
                      enemy_package="example:jump-stick", extra=""):
            package = root / "challenges" / folder
            package.mkdir(parents=True)
            (package / "challenge.manifest").write_text(
                f"GFCHALLENGE 1\nid={identifier}\nname=Fixture\n"
                f"encounter=round-one|{stage}|west|east|duel-ai-v1|defeat-opponent|{ticks}\n"
                f"enemy=round-one|{enemy_package}|{enemy}\n{extra}")

        challenge("some-folder", "example:trials", 1000)
        challenge("one-tick", "example:expires", 1)
        challenge("wrong-stage", "example:wrong-stage", 1, stage="example:missing")
        challenge("wrong-variant", "example:wrong-variant", 1, enemy="clear")
        challenge("roster-too-big", "example:roster-too-big", 1, extra="enemy=round-one|example:jump-stick|normal\n")
        challenge("two-encounters", "example:two", 1, extra=
                  "encounter=round-two|example:workshop|west|east|duel-ai-v1|defeat-opponent|1\n"
                  "enemy=round-two|example:jump-stick|normal\n")
        challenge("duplicate-a", "example:duplicate", 1)
        challenge("duplicate-b", "example:duplicate", 1)
        # A fixture opponent can knock itself out via the real all-target
        # blast rule. This exercises a terminal win without scripted pad input.
        jumper = root / "borgs/jump-stick"
        fragile = root / "borgs/fragile-stick"
        copytree(jumper, fragile)
        manifest = fragile / "borg.manifest"
        manifest.write_text(manifest.read_text().replace("example:jump-stick", "example:fragile-stick"))
        (fragile / "gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|11|14\nhealth=1\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "projectile=burst|orb|1|1|0|0|0|8|1|0|ignore-floor\nblast=burst|expiry|120|1|all\n"
            "move=burst|b|10|any|14|0:4\nrelease=burst|3|none|0|burst\n")
        challenge("self-blast", "example:win", 1000, enemy_package="example:fragile-stick")

        def snapshot():
            return {p.relative_to(root): p.read_bytes() if p.is_file() else None for p in root.rglob("*")}

        before = snapshot()
        run([fixture, "--run-adapter", saves], cwd=root, check=True, timeout=35)
        assert before == snapshot(), "host adapter altered content or selected save files"
        if binary is None:
            return

        listing = run([binary, "--list-challenges"], cwd=root, capture_output=True, text=True, timeout=20)
        assert listing.returncode == 0 and "example:trials | Fixture | 1 encounter(s); battle not validated" in listing.stdout
        assert "round-one" in listing.stdout and "round-two" in listing.stdout and "actors=2" not in listing.stdout
        assert "example:duplicate" not in listing.stdout and "duplicate Challenge ID" in listing.stderr

        def launch(identifier, save=1, frames=300, extra=(), encounter="round-one", force="1"):
            result = run([binary, "--headless", "--no-audio", "--frames", str(frames),
                          "--run-challenge", identifier, encounter, saves, "1", str(save), force, *extra],
                         cwd=root, capture_output=True, text=True, timeout=35)
            return result.returncode, result.stdout + result.stderr

        code, log = launch("example:trials", extra=("--trace-package-battle",))
        assert code == 0 and "actors=2" in log and "event=hit" in log, log
        assert "stage=example:workshop player-spawn=west enemy-spawn=east" in log, log
        assert "slot=0 id=example:flight-stick variant=normal" in log, log
        assert "slot=1 id=example:jump-stick variant=normal" in log, log
        assert "challenge=example:trials encounter=round-one result=loss" in log and "completed=0" in log, log

        code, log = launch("example:win", extra=("--trace-package-battle",))
        assert code == 0 and "event=blast-hit" in log and "result=win" in log and "completed=1" in log, log

        code, log = launch("example:expires")
        assert code == 0 and "result=timeout" in log and "ticks=1" in log and "completed=0" in log, log
        code, log = launch("example:trials", frames=1)
        assert code == 0 and "result=cancelled" in log and "completed=0" in log, log

        for identifier, save, detail in (
            ("example:wrong-stage", 1, "stage ID is missing"),
            ("example:wrong-variant", 1, "exact Borg variant is missing"),
            ("example:roster-too-big", 1, "exactly one actor per side"),
            ("example:two", 1, "exactly one encounter"),
            ("example:duplicate", 1, "duplicate"),
            ("example:trials", 2, "full selected Force"),
            ("example:trials", 3, "no validated exact catalogue variant"),
            ("example:trials", 4, "disabled copy"),
        ):
            code, log = launch(identifier, save)
            assert code == 2 and detail in log and "actors=2" not in log, log
        code, log = launch("example:trials", extra=("--battle-stage", "example:workshop"))
        assert code == 2 and "actors=2" not in log, log
        code, log = launch("example:trials", extra=("--demo-package-battle",))
        assert code == 2 and "actors=2" not in log, log
        code, log = launch("example:trials", encounter="not-the-one")
        assert code == 2 and "encounter ID is missing" in log and "actors=2" not in log, log
        code, log = launch("example:trials", force="2")
        assert code == 2 and "selected Force is empty" in log and "actors=2" not in log, log
        code, log = launch("example:trials", force="0")
        assert code == 2 and "actors=2" not in log, log
        if opengl:
            result = run([binary, "--no-audio", "--seconds", "3", "--size", "640", "480",
                          "--run-challenge", "example:expires", "round-one", saves, "1", "1", "1"],
                         cwd=root, capture_output=True, text=True, timeout=20)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "backend=opengl" in log, log
            assert "actors=2" in log and "result=timeout ticks=1 completed=0" in log, log
        assert before == snapshot(), "Challenge altered content or selected save files"


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("fixture", type=Path)
    parser.add_argument("--binary", type=Path, help="also test the full native CLI")
    parser.add_argument("--opengl", action="store_true", help="also run the CLI with the SDL/OpenGL backend")
    options = parser.parse_args()
    if options.opengl and options.binary is None:
        parser.error("--opengl requires --binary")
    smoke(options.binary, options.fixture, options.opengl)
    print("portable Challenge host smoke: " +
          ("adapter and CLI PASS" if options.binary is not None else "adapter PASS (CLI not exercised)"))
