"""Verify one real generated three-projectile action advances the receiver meter."""
import argparse
import re
from pathlib import Path
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path, help="direct output with contact scale, power and tier tests")
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    parser.add_argument("--reset-gauge", action="store_true", help="compare the reset opt-in with tiers alone")
    args = parser.parse_args()
    exe, roster, stage = (p.resolve(strict=True) for p in (args.executable, args.roster, args.stage))
    with TemporaryDirectory(prefix="gf-contact-tiers-") as scratch:
        root = Path(scratch)
        for name in ("gg4e08-0602", "gg4e08-000a"):
            copytree(roster / name, root / "borgs" / name, symlinks=True)
        attacker = (root / "borgs/gg4e08-0602/gameplay.txt").read_text().splitlines()
        if ("contact-power=basic-shot|40" not in attacker or
                "contact-tier=basic-shot|100|scale" not in attacker or
                any(row.startswith(("damage-receive-hp=", "damage-credit-ladders=")) for row in attacker)):
            raise ValueError("expected the direct charge-100/base-40 test, with no HP/force ladders")
        path = root / "borgs/gg4e08-000a/gameplay.txt"
        if path.is_symlink():
            raise ValueError("the control needs a local fixture copy")
        profile = path.read_text()
        if len([row for row in profile.splitlines() if row.startswith("damage-tier-receive=60|")]) != 1:
            raise ValueError("receiver lacks the generated tier table")
        cmd = [exe, "--battle-borg", "gg4e08:0602", "--against-borg", "gg4e08:000a",
               "--demo-package-battle", "--trace-package-battle", "--frames", "150",
               "--battle-mvp-assets", stage, "--no-audio"]
        if not args.opengl:
            cmd.append("--headless")
        if args.reset_gauge:
            if "contact-tier-reset=basic-shot|25" not in attacker or "damage-tier-reset=500\n" not in profile:
                raise ValueError("expected recovered charge-100/wear-25 and Sasuke's capacity of 500")
            rows = ("damage-tier-reset=",)
            control_expected, test_expected = (40, 32, 20), (40, 32, 32)
            fixture_profile = profile.replace("damage-tier-reset=500\n", "damage-tier-reset=50\n")
        else:
            rows = ("damage-tier-receive=", "damage-tier-reset=")
            control_expected, test_expected = (40, 40, 40), (40, 32, 20)
            fixture_profile = "\n".join(row for row in profile.splitlines() if not row.startswith("damage-tier-reset=")) + "\n"
        control = "\n".join(row for row in profile.splitlines() if not row.startswith(rows)) + "\n"
        for fixture, expect in ((fixture_profile, test_expected), (control, control_expected)):
            path.write_text(fixture)
            result = run(cmd, cwd=root, text=True, capture_output=True, timeout=60)
            hits = re.findall(r"round=(\d+) .*actor=0 move=basic-shot event=hit .*recipient=1 value=(\d+)",
                              result.stderr)
            if result.returncode or hits != [(str(round_), str(amount)) for round_ in (0, 1) for amount in expect]:
                raise ValueError(f"expected {expect} per round:\n{result.stderr[-8000:]}")
    print(f"Generated contact tiers: three hits {test_expected} vs control {control_expected} "
          f"({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
