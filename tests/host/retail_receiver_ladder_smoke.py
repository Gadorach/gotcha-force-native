"""Verify the generated HP response against a controlled low-health receiver."""
import argparse
import re
from pathlib import Path
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path, help="direct output with all three contact test flags")
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, roster, stage = (p.resolve(strict=True) for p in (args.executable, args.roster, args.stage))
    with TemporaryDirectory(prefix="gf-receiver-hp-") as scratch:
        root = Path(scratch)
        for name in ("gg4e08-0615", "gg4e08-000a"):
            copytree(roster / name, root / "borgs" / name, symlinks=True)
        attacker = (root / "borgs/gg4e08-0615/gameplay.txt").read_text()
        if ("contact-power=basic-shot|20\n" not in attacker or
                "damage-credit-ladders=basic-shot\n" not in attacker or
                not all(attacker.count("damage-credit-" + role + "=") == 2 for role in ("hp", "force"))):
            raise ValueError("expected recovered base and outgoing curve bindings for G-Red")
        path = root / "borgs/gg4e08-000a/gameplay.txt"
        if path.is_symlink():
            raise ValueError("the HP fixture needs a local profile copy")
        profile, edits = re.subn(r"(?m)^health=\d+$", "health=22", path.read_text())
        hp = [line for line in profile.splitlines() if line.startswith("damage-receive-hp=")]
        force = [line for line in profile.splitlines() if line.startswith("damage-receive-force=")]
        if edits != 1 or len(hp) != 2 or len(force) != 2 or hp[1].split("|")[31] != "0.98":
            raise ValueError("expected the exact receiver curves in the generated package")
        cmd = [exe, "--battle-borg", "gg4e08:0615", "--against-borg", "gg4e08:000a",
               "--demo-package-battle", "--trace-package-battle", "--frames", "150",
               "--battle-mvp-assets", stage, "--no-audio"]
        if not args.opengl:
            cmd.append("--headless")
        for rows, expected in ((profile, 19), ("\n".join(
                line for line in profile.splitlines() if not line.startswith(
                    ("damage-receive-hp=", "damage-receive-force="))) + "\n", 20)):
            path.write_text(rows)
            result = run(cmd, cwd=root, text=True, capture_output=True, timeout=60)
            hits = re.findall(r"round=(\d+) .*actor=0 move=basic-shot event=hit .*recipient=1 value=(\d+)",
                              result.stderr)
            if result.returncode or hits != [("0", str(expected)), ("1", str(expected))]:
                raise ValueError(f"expected two hits of {expected}:\n{result.stderr[-8000:]}")
    print(f"Generated receiver: HP probe 22-20=2 selects cell 30, damage 20 -> 19 ({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
