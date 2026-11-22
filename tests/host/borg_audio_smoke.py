"""Only test the new original PCM binding, using SDL's inaudible dummy sink."""
import os
from pathlib import Path
import re
from subprocess import run
import sys


def test(executable):
    env = {**os.environ, "SDL_AUDIODRIVER": "dummy"}
    for unit in ("0615", "000a"):
        cmd = [executable, "--battle-borg", "gg4e08:" + unit, "--against-borg", "test:target",
               "--demo-package-battle", "--trace-package-battle", "--frames", "160"]
        result = run(cmd, env=env, capture_output=True, text=True, timeout=35)
        log = result.stdout + result.stderr
        assert result.returncode == 0, log
        accepted = re.search(r"borg-audio: device=open frames-accepted=([0-9]+)", log)
        assert accepted and int(accepted[1]) >= 800, log
    print("G-Red/Sasuke original movement PCM accepted by SDL dummy sink")


if __name__ == "__main__":
    test(Path(sys.argv[1]).resolve(strict=True))
