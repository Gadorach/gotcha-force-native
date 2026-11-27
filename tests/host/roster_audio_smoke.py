"""Ensure later roster slots' accepted launch cues actually reach the mixer."""
import argparse
from pathlib import Path
import re
import struct
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.portable_borg_preview import prepare


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--art", type=Path, default=Path("local/content/native-battle-mvp-v1"))
    parser.add_argument("--movement", type=Path, default=Path("local/content/source-gred-ground-v9"))
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, art, movement = (p.resolve(strict=True) for p in (args.executable, args.art, args.movement))
    with TemporaryDirectory(prefix="gf-roster-audio-") as scratch:
        root = Path(scratch)
        for model, projectile, level in (("g-red", "test-beam", 2000), ("sasuke", "shuriken", 3000)):
            package = root / "borgs" / model
            prepare(model, art, package, movement if model == "g-red" else None, True)
            (package / "audio").mkdir(exist_ok=True)
            (package / "audio/roster.pcm").write_bytes(struct.pack("<hh", level, level) * 1600)
            profile = package / "gameplay.txt"
            profile.write_text(profile.read_text() + f"launch-sound={projectile}|audio/roster.pcm\n")
        cmd = [exe, "--battle-borg", "gg4e08:0615", "--against-borg", "gg4e08:000a",
               "--battle-mvp-assets", art, "--source-ground-assets", movement, "--demo-roster-battle",
               "--trace-package-battle", "--frames", "150"]
        if not args.opengl:
            cmd.append("--headless")
        traces = []
        for mute in (False, True):
            result = run([*cmd, "--no-audio"] if mute else cmd, cwd=root, text=True, capture_output=True, timeout=60)
            log = result.stderr
            requests = re.findall(r"actor=(\d+) .*event=launch-sound ", log)
            mix = re.search(r"borg-roster-audio: mixed-actors=0x([0-9a-f]+) dropped=(\d+)", log)
            launch = re.search(r"borg-launch-audio: starts=(\d+) blocks=(\d+) nonzero=(\d+)", log)
            if (result.returncode or requests != ["0", "2", "3", "0", "2"] or
                    not mix or mix.groups() != (("0", "0") if mute else ("d", "0")) or not launch or
                    int(launch[1]) != (0 if mute else len(requests)) or int(launch[3]) != int(not mute)):
                raise ValueError(f"roster audio (muted={mute}) differed:\n{log[-6500:]}")
            traces.append([line for line in log.splitlines() if "borg-api:" in line])
        if traces[0] != traces[1]:
            raise ValueError("muting altered the battle trace")
    print(f"Roster audio: five cues reach the mixer, including slots 2 and 3; mute leaves the trace unchanged "
          f"({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
