"""Run native duel and roster Battle Tests on the generated Workshop."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl=False):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-workshop-smoke-") as scratch:
        root = Path(scratch) / "content"
        prepare(root, swept_beam=True)
        prepare_stage(root)
        listing = run([binary, "--list-stages"], cwd=root, capture_output=True, text=True, timeout=35)
        assert listing.returncode == 0 and "example:workshop | Workshop | discovered; battle not validated" in listing.stdout
        command = [binary, "--no-audio", "--demo-package-battle",
                   "--trace-package-battle", "--frames", "100",
                   "--battle-borg", "example:flight-stick", "--borg-variant", "normal",
                   "--against-borg", "example:jump-stick", "--against-variant", "normal", "--battle-stage"]
        if not opengl:
            command.insert(1, "--headless")

        def launch(stage, markers=("west", "east"), extra=()):
            args = [*command, stage, "--stage-spawns", *markers, *extra]
            result = run(args, cwd=root, capture_output=True, text=True, timeout=35)
            return result.returncode, result.stdout + result.stderr

        code, log = launch("example:workshop")
        assert code == 0 and "stage=example:workshop player-spawn=west enemy-spawn=east" in log, log
        assert "event=stage-impact" in log and "stage-view: viewer=0" in log and "actors=2 restarts=1" in log, log

        code, log = launch("example:missing")
        assert code == 2 and "missing stage ID" in log, log
        code, log = launch("example:workshop", ("missing", "east"))
        assert code == 2 and "missing stage spawn marker" in log, log
        code, log = launch("example:workshop", ("west", "west"))
        assert code == 2 and "markers must be distinct" in log, log
        code, log = launch("example:workshop", extra=("--battle-mvp-assets", "arena"))
        assert code == 2 and "actors=2" not in log, log
        code, log = launch("example:workshop", extra=("--gred-movement",))
        assert code == 2 and "actors=2" not in log, log
        no_variant = command.copy()
        at = no_variant.index("--against-variant")
        del no_variant[at:at + 2]
        result = run([*no_variant, "example:workshop", "--stage-spawns", "west", "east"],
                     cwd=root, capture_output=True, text=True, timeout=35)
        assert result.returncode == 2 and "actors=2" not in result.stdout + result.stderr

        # CPU fighters press B only. Give the authored B shot stage collision
        # so the roster tests the wall without relying on P1's scripted X.
        profile = root / "borgs/flight-stick/gameplay.txt"
        text = profile.read_text()
        primary = "projectile=paint|orb|1|55|0|0|0|8|110|8|ignore-floor"
        assert text.count(primary) == 1
        profile.write_text(text.replace(primary, primary.replace("ignore-floor", "stop-on-stage")))
        team0 = ("--roster-borg", "0", "example:flight-stick")
        team1 = ("--roster-borg", "1", "example:jump-stick")
        roster = (*team0, *team0, *team1, *team1)
        markers = ("west", "east", "west-rear", "west-outer", "east-outer", "east-rear")
        code, log = launch("example:workshop", markers, roster)
        assigned = re.findall(r"borg-stage-spawn: actor=(\d+) marker=(\S+)", log)
        emitted = set(re.findall(r"borg-api: round=\d+ tick=\d+ actor=([2-5]) .*event=emit ", log))
        assert code == 0 and assigned == [(str(i), marker) for i, marker in enumerate(markers)], log
        cpu_impact = re.search(r"borg-api: round=\d+ tick=\d+ actor=[23] move=paint-shot event=stage-impact ", log)
        assert emitted == {"2", "3", "4", "5"} and cpu_impact, log[-6000:]
        assert "stage-view: viewer=0" in log and "actors=6 restarts=1" in log, log
        manual_events = [line for line in log.splitlines() if "borg-api:" in line]
        code, log = launch("example:workshop", markers[:3], team0)
        assert code == 0 and "actors=3 restarts=1" in log, log
        code, log = launch("example:workshop", markers[:-1], roster)
        assert code == 2 and "requires one marker per actor (6)" in log, log
        code, log = launch("example:workshop", (*markers[:-1], "missing"), roster)
        assert code == 2 and "missing stage spawn marker: missing" in log and "actors=6" not in log, log
        code, log = launch("example:workshop", (*markers[:-1], markers[4]), roster)
        assert code == 2 and "markers must be distinct" in log and "actors=6" not in log, log
        code, log = launch("example:workshop", extra=("--demo-roster-battle",))
        assert code == 2 and "actors=4" not in log, log

        # The published asset-free preset must launch the same roster as the
        # explicit flags, using the same stage caps and native battle trace.
        sample = Path(__file__).resolve().parents[2] / "examples/battle-rosters/workshop-3v3.roster"
        preset = root / "match.roster"
        preset.write_bytes(sample.read_bytes())
        controls = command[:command.index("--battle-borg")]

        def launch_preset(file, *extra):
            result = run([*controls, "--battle-roster", file, *extra], cwd=root,
                         capture_output=True, text=True, timeout=35)
            return result.returncode, result.stdout + result.stderr

        code, log = launch_preset(sample)
        preset_spawns = re.findall(r"borg-stage-spawn: actor=(\d+) marker=(\S+)", log)
        assert code == 0 and preset_spawns == assigned and "actors=6 restarts=1" in log, log[-5000:]
        assert "event=stage-impact" in log and "stage-view: viewer=0" in log, log[-5000:]
        assert [line for line in log.splitlines() if "borg-api:" in line] == manual_events, "preset changed the scripted combat trace"
        assert preset.read_bytes() == sample.read_bytes()

        small = root / "small.roster"
        small.write_text("\n".join(preset.read_text().splitlines()[:6]) + "\n")
        code, log = launch_preset(small.name)
        assert code == 0 and "actors=3 restarts=1" in log, log[-3000:]

        code, log = launch_preset(preset, "--battle-stage", "example:workshop")
        assert code == 2 and "cannot combine --battle-roster" in log, log
        code, log = launch_preset(preset, "--roster-borg", "0", "example:flight-stick")
        assert code == 2 and "cannot combine --battle-roster" in log, log
        linked = root / "linked.roster"
        linked.symlink_to(preset)
        code, log = launch_preset(linked)
        assert code == 2 and "linked or oversized" in log, log
        too_large = root / "oversized.roster"
        too_large.write_bytes(preset.read_bytes() + b"\n" * 4096)
        code, log = launch_preset(too_large)
        assert code == 2 and "linked or oversized" in log, log
        invalid = root / "invalid.roster"
        text = preset.read_text()
        invalid.write_text(text.replace("|normal|east-rear", "|clear|east-rear"))
        code, log = launch_preset(invalid)
        assert code == 2 and "exact base variant; no variant fallback" in log and "actors=6" not in log, log
        invalid.write_text(text.replace("|normal|east-rear", "|normal|missing"))
        code, log = launch_preset(invalid)
        assert code == 2 and "missing stage spawn marker: missing" in log and "actors=6" not in log, log
        manifest = root / "stages/workshop/stage.manifest"
        with manifest.open("a") as out:
            out.write("spawn=blocked|0|0|-20\n")
        invalid.write_text(text.replace("|normal|east-rear", "|normal|blocked"))
        code, log = launch_preset(invalid)
        assert code == 2 and "stage spawn intersects a body partition: blocked" in log and "actors=6" not in log, log


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Portable stage: duel, rosters, stage impacts, partition admission, restart, and refusals PASS "
          f"({'OpenGL' if args.opengl else 'headless'})")
