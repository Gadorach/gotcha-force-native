"""Check opponent/all charge blast recipients in the four-actor host."""
import argparse
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe = args.executable.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-roster-charge-") as temp:
        root = Path(temp) / "fixture"
        prepare(root, proximity_bomb=True)
        profile = root / "borgs/jump-stick/gameplay.txt"
        text = profile.read_text()
        row = "blast=charge|expiry|440|12|opponent\n"
        if text.count(row) != 1:
            raise ValueError("unexpected sample charge definition")
        cmd = [exe, "--battle-borg", "example:jump-stick", "--against-borg", "example:flight-stick",
               "--battle-mvp-assets", "arena", "--demo-roster-battle", "--trace-package-battle",
               "--no-audio", "--package-battle-distance", "200", "--frames", "82"]
        if not args.opengl:
            cmd.append("--headless")
        sensors = []
        for scope, expect in (("opponent", ["1", "3"]), ("all", ["0", "1", "2", "3"])):
            profile.write_text(text.replace(row, f"blast=charge|expiry|440|12|{scope}\n"))
            result = run(cmd, cwd=root, capture_output=True, text=True, timeout=60)
            log = result.stderr
            recipients = re.findall(r"actor=0 move=drop-charge event=blast-hit .*recipient=(\d+)", log)
            sensor = re.findall(r"actor=0 move=drop-charge event=bomb-proximity .*recipient=(\d+)", log)
            if (result.returncode or recipients != expect or sensor not in (["1"], ["3"]) or
                    log.count("actor=0 move=drop-charge event=stock-returned") != 1):
                raise ValueError(f"{scope}: charge contacts differed:\n{log[-6500:]}")
            sensors.append(sensor)
        if sensors[0] != sensors[1]:
            raise ValueError("changing blast recipients altered the sensor's enemy selection")
    print(f"Roster charges: opponent hits slots 1/3, all also hits 0/2; one sensor exit and stock return "
          f"({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
