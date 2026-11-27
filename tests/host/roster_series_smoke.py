"""Run a two-win local roster series through the real hosted battle loop."""
from argparse import ArgumentParser
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary, opengl):
    binary = binary.resolve(strict=True)
    preset = Path(__file__).resolve().parents[2] / "examples/battle-rosters/workshop-first-to-two.roster"
    with TemporaryDirectory(prefix="gf-roster-series-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        prepare_stage(root)
        enemy = root / "borgs/jump-stick/gameplay.txt"
        text = enemy.read_text()
        assert text.count("health=160") == 1
        enemy.write_text(text.replace("health=160", "health=1"))
        cmd = [binary, "--no-audio", "--frames", "125", "--demo-package-battle", "--trace-package-battle"]
        if not opengl:
            cmd.append("--headless")

        def launch(file, *extra):
            result = run([*cmd, "--battle-roster", file, *extra], cwd=root,
                         capture_output=True, text=True, timeout=50)
            assert result.returncode == 0 and "actors=3 restarts=1" in result.stderr, result.stderr[-5000:]
            return result.stderr

        log = launch(preset)
        point = r"borg-series: round=(\d+) tick=\d+ outcome=(\w+) score=(\d+:\d+) goal=(\d+) winner=(\d+)"
        assert re.findall(point, log) == [("0", "team0", "1:0", "2", "255"), ("1", "team0", "2:0", "2", "0")], log[-6000:]
        assert re.findall(r"borg-roster-ko: round=(\d+) tick=\d+ actor=(\d+)", log) == [("0", "2"), ("1", "2")]
        assert re.search(r"borg-roster-round: round=1 tick=\d+ result=team0 alive=0x3\b", log)
        state = re.findall(r"borg-roster-state: round=1 tick=\d+ actor=(\d+) hp=(\d+)"
                           r" position=\S+ travel=\S+ nearest=\d+ aim=\d+", log)
        assert [actor for actor, _ in state] == ["0", "1", "2"] and state[-1][1] == "0", log[-5000:]
        ordinary = root / "ordinary.roster"
        ordinary.write_text(preset.read_text().replace("rounds-to-win=2\n", ""))
        assert "borg-series:" not in launch(ordinary)
        enemy.write_text(text)
        ally = root / "borgs/flight-stick/gameplay.txt"
        ally_text = ally.read_text()
        assert ally_text.count("health=160") == 1
        ally.write_text(ally_text.replace("health=160", "health=1"))
        loss = launch(preset, "--frames", "145")
        assert re.findall(point, loss) == [("0", "team1", "0:1", "2", "255"), ("1", "team1", "0:2", "2", "1")], loss[-6000:]


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    options = parser.parse_args()
    smoke(options.binary, options.opengl)
    print(f"Roster series: one point per KO round, score kept through P2 restart, ordinary mode unchanged "
          f"({'OpenGL' if options.opengl else 'headless'})")
