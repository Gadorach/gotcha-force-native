"""Verify independent damage banks through the desktop roster CLI."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def rows(key, width, selected):
    return "".join(f"{key}={bank}|" + "|".join(selected.get(i, ("1", "1"))[bank] for i in range(width)) + "\n"
                   for bank in range(2))


def smoke(executable, opengl):
    executable = executable.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-roster-damage-banks-") as temp:
        root = Path(temp) / "content"
        prepare(root)
        stage = prepare_stage(root)
        with (stage / "stage.manifest").open("a") as manifest:
            manifest.write("spawn=damage-shooter|0|0|-550\nspawn=damage-target|0|0|-410\n"
                           "spawn=damage-idle|600|0|-800\nspawn=damage-extra-a|-700|0|-800\n"
                           "spawn=damage-extra-b|-450|0|-800\nspawn=damage-extra-c|-200|0|-800\n")
        base = ("GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=180\n"
                "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n")
        (root / "borgs/jump-stick/gameplay.txt").write_text(
            base + "movement=grounded-v1|0.4|13|0.4\nwalk=0:1\nair=0:2\ndash=0:3\n"
                   "move=slash|b|10|grounded|24|0:4\nrange=slash|150\nmelee=slash|9|14|95|38|52|44|12\n"
                   "contact-power=slash|40\ndamage-credit=0|1|0.85\ndamage-credit=1|1|0.85\n" +
            rows("damage-credit-setting", 32, {0: ("1.02", "0.92"), 7: ("1", "0.9")}) +
            rows("damage-credit-rank", 8, {0: ("0.85", "0.85"), 3: ("1", "1")})
        )
        (root / "borgs/flight-stick/gameplay.txt").write_text(
            base + "movement=stationary-v1\ndamage-receive=0|0.92|1.15\ndamage-receive=1|1.04|1.15\n" +
            rows("damage-receive-setting", 32, {7: ("0.92", "1.04"), 31: ("0.98", "2")}) +
            rows("damage-receive-rank", 8, {0: ("1.15", "1.15")})
        )
        preset = root / "damage.roster"
        base_preset = (
            "GFROSTER 1\nlocals=2\nstage=example:workshop\n"
            "fighter=0|example:jump-stick|normal|damage-shooter\n"
            "fighter=1|example:flight-stick|normal|damage-target\n"
            "fighter=1|example:flight-stick|normal|damage-idle\n"
        )
        preset.write_text(base_preset)

        def host(*args):
            return run([executable, *args], cwd=root, capture_output=True, text=True, timeout=45)

        def play(selection=None, short=False):
            args = ["--battle-roster", preset, "--no-audio"]
            if selection is not None:
                args.extend(("--package-damage-indices", selection))
            if short:
                args += ["--headless", "--frames", "1"]
            else:
                args += ["--demo-package-battle", "--trace-package-battle", "--frames", "78"]
                if not opengl:
                    args.append("--headless")
            return host(*args)

        def hit(result, expected):
            hits = re.findall(r"actor=0 move=slash event=hit .*recipient=(\d+) value=(\d+)", result.stderr)
            assert result.returncode == 0 and hits == [("1", str(expected))], result.stderr[-4500:]

        # Reproduce the four source-setup sums with authored geometry.
        # The third fighter is idle and is not part of the damage comparison.
        for selection, expected in (("7,7:0,0,0", 40), ("7,7:0,0,0:0,0,1", 35),
                                    ("7,7:0,0,0:0,1,1", 40), ("7,7:3,0,0:0,1,1", 47),
                                    ("0,31:0,0,0:0,1,1", 79)):
            hit(play(selection), expected)
        for malformed in ("7,7:0,0,0:", "7,7:0,0,0:0,1", "7,7:0,0,0:0,2,1",
                          "7,7:0,0,0:0,1,1,", "7,7:0,0,0:0,1,1:0", "7,7:0,0:0,1"):
            assert play(malformed, short=True).returncode != 0, malformed
        selection = "7,7:0,0,0:0,0,1"
        preset.write_text(base_preset + f"damage-indices={selection}\n")
        hit(play(), 35)
        admitted = host("--check-battle-roster", preset)
        assert admitted.returncode == 0 and "Roster admission passed:" in admitted.stdout, admitted.stderr
        conflict = play(selection, short=True)
        assert conflict.returncode != 0 and "damage indices supplied by both" in conflict.stderr

        # The browser has no match override flags. Use a CPU attacker so
        # the headless match proves that the stored bank reaches real hits.
        library = root / "presets"
        library.mkdir()
        browser_file = library / "damage-cpu.roster"
        browser_file.write_text(
            "GFROSTER 1\nlocals=1\nstage=example:workshop\ndamage-indices=7,7:0,0,0:1,0,0\n"
            "fighter=0|example:flight-stick|normal|damage-target\n"
            "fighter=0|example:flight-stick|normal|damage-idle\n"
            "fighter=1|example:jump-stick|normal|damage-shooter\n"
        )
        browser = ["--browse-battle-rosters", library, "--frames", "65", "--no-audio",
                   "--demo-roster-select", browser_file.name, "--trace-package-battle"]
        if not opengl:
            browser.append("--headless")
        browsed = host(*browser)
        assert browsed.returncode == 0 and f"selected {browser_file.name}" in browsed.stderr, browsed.stderr[-4500:]
        if not opengl:
            hits = re.findall(r"actor=2 move=slash event=hit .*recipient=(\d+) value=(\d+)", browsed.stderr)
            assert hits and all(hit == ("0", "40") for hit in hits), browsed.stderr[-4500:]

        plain = library / "damage-default.roster"
        plain.write_text(browser_file.read_text().replace("damage-indices=7,7:0,0,0:1,0,0\n", ""))
        switched = host(*browser, "--demo-roster-next", plain.name)
        assert switched.returncode == 0 and f"selected {plain.name}" in switched.stderr, switched.stderr[-4500:]
        if not opengl:
            later = switched.stderr.split(f"returned {browser_file.name}", 1)[-1]
            hits = re.findall(r"actor=2 move=slash event=hit .*recipient=(\d+) value=(\d+)", later)
            assert hits and all(hit == ("0", "35") for hit in hits), switched.stderr[-4500:]

        preset.write_text(base_preset)
        with preset.open("a") as extra:
            for marker, team in (("a", 0), ("b", 0), ("c", 1)):
                extra.write(f"fighter={team}|example:flight-stick|normal|damage-extra-{marker}\n")
        six_actors = "31,31:7,7,7,7,7,7:1,1,1,1,1,1"
        full = play(six_actors, short=True)
        assert full.returncode == 0, full.stderr[-4500:]
        receiver = root / "borgs/flight-stick/gameplay.txt"
        receiver.write_text("\n".join(line for line in receiver.read_text().splitlines()
                                      if not line.startswith(("damage-receive-setting=", "damage-receive-rank="))) + "\n")
        stale = play(six_actors, short=True)
        assert stale.returncode != 0 and "indexed damage selection requires matching complete package tables" in stale.stderr
        missing_tables = host("--check-battle-roster", browser_file)
        assert missing_tables.returncode != 0 and "indexed damage selection requires matching complete package tables" in missing_tables.stderr
        refused = host(*browser)
        assert refused.returncode == 2 and f"refused {browser_file.name}:" in refused.stderr
        assert "indexed damage selection requires matching complete package tables" in refused.stderr


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.executable, args.opengl)
    print(f"Roster damage banks: CLI, stored indices, preflight and browser ({'OpenGL' if args.opengl else 'headless'})")
