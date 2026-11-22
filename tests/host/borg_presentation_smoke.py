"""Check only the normal G-Red/Sasuke launch and movement presentation changes."""
import argparse
import re
from pathlib import Path
from subprocess import run


def smoke(executable, opengl):
    for model, move, frame in (("0615", "beam-test", 16), ("000a", "shuriken-test", 3)):
        args = [executable, "--battle-borg", "gg4e08:" + model, "--against-borg", "test:target",
                "--demo-package-battle", "--trace-package-battle", "--no-audio", "--frames", "170"]
        if not opengl:
            args.append("--headless")
        result = run(args, capture_output=True, text=True, timeout=45)
        log = result.stdout + result.stderr
        assert result.returncode == 0, log
        assert ("backend=opengl" if opengl else "backend=headless") in log, log
        assert re.search(rf"move={move} event=socket-launch frame={frame} object=\d+ recipient=255 value=7", log), log
        assert "event=socket-refused" not in log and f"move={move} event=hit" in log, log
        cues = {int(n) for n in re.findall(r"borg-fx: .*actor=0 cues=(\d+)", log)}
        assert any(cue & 8 for cue in cues), log
        if model == "000a":
            assert {4, 9, 10} <= cues, log
    print("G-Red/Sasuke socket launches and movement cues PASS"
          f" ({'OpenGL' if opengl else 'headless'})")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.executable.resolve(strict=True), args.opengl)
