"""Browse presets, refuse a bad match, and return from Battle Test for a fresh match."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    presets = Path(__file__).resolve().parents[2] / "examples/battle-rosters"
    solo = (presets / "workshop-solo.roster").read_text()
    with TemporaryDirectory(prefix="gf-roster-browser-") as scratch:
        root = Path(scratch)
        library = root / "presets"
        library.mkdir()
        bad = solo.replace("|normal|east-rear", "|normal|missing")
        (library / "a-bad-spawn.roster").write_text(bad)
        (library / "m0.roster").write_bytes((presets / "workshop-timed.roster").read_bytes())
        for i in range(1, 3):
            (library / f"m{i}.roster").write_text(bad)
        (library / "z-solo.roster").write_text(solo)
        (library / "ignored.roster").symlink_to("z-solo.roster")

        content = root / "content"
        prepare(content)
        prepare_stage(content)

        def host(*args, cwd=content):
            return run([binary, *args], cwd=cwd, capture_output=True, text=True, timeout=40)

        browser = ["--browse-battle-rosters", library, "--no-audio", "--size", "480", "160"]
        if not opengl:
            browser.append("--headless")

        refused = host(*browser, "--frames", "8", "--demo-roster-select", "a-bad-spawn.roster")
        assert refused.returncode == 2, refused.stderr
        assert "skipped ignored.roster:" in refused.stderr and "presets=5 (metadata only)" in refused.stderr
        assert "refused a-bad-spawn.roster: missing stage spawn marker: missing" in refused.stderr, refused.stderr
        assert "closed menu-frames=8 without launch" in refused.stderr, refused.stderr
        assert "borg-roster-slot:" not in refused.stderr and "package-battle: frames=" not in refused.stderr

        # On this short viewport, the fifth row needs two page changes.
        played = host(*browser, "--frames", "16", "--demo-roster-select", "z-solo.roster", "--trace-package-battle")
        assert played.returncode == 0, played.stderr
        assert "selected z-solo.roster menu-frames=3" in played.stderr, played.stderr
        assert "stage=example:workshop p1-spawn=west locals=1" in played.stderr
        assert "actor=1 team=0 driver=cpu id=example:jump-stick" in played.stderr
        assert "package-battle: frames=13 actors=6 restarts=0" in played.stderr, played.stderr
        assert "Roster admission passed:" not in played.stdout

        switched = host(*browser, "--frames", "24", "--demo-roster-select", "z-solo.roster",
                        "--demo-roster-next", "m0.roster", "--trace-package-battle")
        log = switched.stderr
        assert switched.returncode == 0, log
        assert "returned z-solo.roster battle-frames=4 status=closed" in log, log
        # Return swallows a held Esc for one menu frame. The active fifth
        # row starts on page 3, and the next script clicks Prev twice.
        assert "selected m0.roster menu-frames=7" in log, log
        assert re.findall(r"package-battle: frames=(\d+) actors=(\d+) restarts=(\d+)", log) == [
            ("4", "6", "0"), ("13", "3", "0"),
        ], log
        if not opengl:
            assert log.count("borg-body: round=0 tick=1 actor=0 ") == 2, log
        before, after = log.split("returned z-solo.roster battle-frames=4 status=closed")
        assert "actor=1 team=0 driver=cpu id=example:jump-stick" in before, before
        assert "actor=1 team=0 driver=local id=example:flight-stick" in after, after
        assert "actor=2 team=1 driver=cpu id=example:jump-stick" in after, after

        gamepad = host(*browser, "--frames", "26", "--demo-roster-pad", "--demo-roster-select", "z-solo.roster",
                       "--demo-roster-next", "m0.roster", "--trace-package-battle")
        pad_log = gamepad.stderr
        assert gamepad.returncode == 0, pad_log
        assert "selected z-solo.roster menu-frames=6" in pad_log, pad_log
        assert "returned z-solo.roster battle-frames=4 status=closed" in pad_log, pad_log
        assert "selected m0.roster menu-frames=13" in pad_log, pad_log
        assert re.findall(r"package-battle: frames=(\d+) actors=(\d+) restarts=(\d+)", pad_log) == [
            ("4", "6", "0"), ("9", "3", "0"),
        ], pad_log
        # The same controls would fire and pause in battle. The script
        # holds both for three frames after launch and then releases them.
        assert not re.search(r"borg-api: .*actor=0 .*event=start ", pad_log), pad_log
        if not opengl:
            assert re.findall(r"borg-body: round=0 tick=(\d+) actor=0 ", pad_log) == [
                str(tick) for ticks in (4, 9) for tick in range(1, ticks + 1)
            ], pad_log

        assert host(*browser, "--frames", "8", "--demo-roster-select", "ignored.roster").returncode == 2
        assert host(*browser, "--frames", "8", "--demo-roster-next", "m0.roster").returncode == 2
        assert host(*browser, "--frames", "8", "--demo-roster-pad").returncode == 2
        assert host(*browser, "--battle-roster", library / "z-solo.roster").returncode == 2
        empty = root / "empty"
        empty.mkdir()
        if opengl:
            closed = host(*browser, "--frames", "4", cwd=empty)
            assert closed.returncode == 0 and "presets=5 (metadata only)" in closed.stderr, closed.stderr
            assert "closed menu-frames=4 without launch" in closed.stderr, closed.stderr
        missing_assets = host(*browser, "--frames", "8", "--demo-roster-select", "z-solo.roster", cwd=empty)
        assert missing_assets.returncode == 2, missing_assets.stderr
        assert "presets=5 (metadata only)" in missing_assets.stderr
        assert "refused z-solo.roster:" in missing_assets.stderr
        assert "closed menu-frames=8 without launch" in missing_assets.stderr
        assert not list(empty.iterdir())
        assert not any((content / name).exists() for name in ("save", "saves"))


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
    print(f"Roster browser: mouse and pad navigation, failed admission, and fresh matches PASS "
          f"({'OpenGL' if args.opengl else 'headless'})")
