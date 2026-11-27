"""Exercise split contact-scale roles with real generated receiver packages."""
import argparse
import json
import re
from pathlib import Path
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.portable_borg_preview import prepare

# Class 1 and class 7 take different amounts from the same generated shot.
# The combined form and authored training target have no qualified rows.
CASES = (("gg4e08:000a", [1, 1, 0], 8), ("gg4e08:0602", [7, 2, 2], 2),
         ("gg4e08:061e", None, 8), ("test:target", None, 8))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path, help="direct output of retail_borg_roster --contact-scale-test")
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, roster, stage = (p.resolve(strict=True) for p in (args.executable, args.roster, args.stage))
    with TemporaryDirectory(prefix="gf-retail-scale-") as scratch:
        root = Path(scratch)

        def package(ident):
            name = ident.replace(":", "-")
            destination = root / "borgs" / name
            copytree(roster / name, destination, symlinks=True)
            record = json.loads((destination / "source-index.json").read_bytes())
            policies = [record[key]["contact_scale_test"] for key in
                        ("basic_shot_battle_test", "basic_melee_battle_test") if key in record]
            if len(policies) != 1:
                raise ValueError(f"{ident}: not one opted-in gameplay policy")
            lines = (destination / "gameplay.txt").read_text().splitlines()
            if not all(line in lines for line in policies[0]["lines"]) or any(
                    line.startswith(("damage-vs=", "damage-side=", "damage-factors=")) for line in lines):
                raise ValueError(f"{ident}: split and legacy modifiers not cleanly separated")
            return policies[0]

        attacker = package("gg4e08:0615")
        if (attacker["settings"], attacker["rank"], attacker["lines"][1:3]) != (
                7, 0, ["damage-credit=0|1.0|0.85", "damage-credit=1|0.9|0.85"]):
            raise ValueError("unexpected authored attacker settings snapshot")
        for ident, selector, expected in CASES:
            if ident == "test:target":
                prepare("target", stage, root / "borgs/training-target")
            else:
                policy = package(ident)
                if policy["receiver_selector"] != selector:
                    raise ValueError(f"{ident}: receiver settings do not match the exact normal selector")
            cmd = [exe, "--battle-borg", "gg4e08:0615", "--against-borg", ident,
                   "--demo-package-battle", "--trace-package-battle", "--frames", "150",
                   "--battle-mvp-assets", stage, "--no-audio"]
            if not args.opengl:
                cmd.append("--headless")
            got = run(cmd, cwd=root, text=True, capture_output=True, timeout=60)
            hits = re.findall(r"round=(\d+) .*actor=0 move=basic-shot event=hit .*recipient=1 value=(\d+)",
                              got.stderr)
            if got.returncode or hits != [("0", str(expected)), ("1", str(expected))]:
                raise ValueError(f"{ident}: expected two hits of {expected}:\n{got.stderr[-8000:]}")
    print(f"{len(CASES)} generated receiver fixtures passed ({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
