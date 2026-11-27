"""Verify one HP at a generated sub-one hit, compared with the old truncation."""
import argparse
import json
from pathlib import Path
import re
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path)
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, roster, stage = (p.resolve(strict=True) for p in (args.executable, args.roster, args.stage))
    with TemporaryDirectory(prefix="gf-contact-floor-") as name:
        root = Path(name)
        for model in ("0008", "0602"):
            package = f"gg4e08-{model}"
            copytree(roster / package, root / "borgs" / package, symlinks=True)
        shooter = root / "borgs/gg4e08-0008"
        path = shooter / "gameplay.txt"
        if path.is_symlink():
            raise ValueError("fixture needs a scratch gameplay copy")
        profile = path.read_text()
        marker = "contact-minimum=basic-shot|1\n"
        audit = json.loads((shooter / "source-index.json").read_bytes())
        floor = audit["basic_shot_battle_test"]["contact_floor_test"]["basic-shot"]
        if (profile.count(marker) != 1 or "contact-power=basic-shot|4\n" not in profile or
                floor["minimum"] != 1 or floor["source_bases"] != [4] or
                "damage-credit-ladders=" in profile):
            raise ValueError("expected the qualified base-4 move and no HP/force ladder")
        cmd = [exe, "--battle-borg", "gg4e08:0008", "--against-borg", "gg4e08:0602",
               "--package-damage-indices", "31,0:0,7", "--demo-package-battle", "--trace-package-battle",
               "--frames", "150", "--battle-mvp-assets", stage, "--no-audio"]
        if not args.opengl:
            cmd.append("--headless")
        hit = re.compile(r"(round=(\d+) .*actor=0 move=basic-shot event=hit .*recipient=1 value=)(\d+)")
        traces = []
        for fixture, amount in ((profile, 1), (profile.replace(marker, ""), 0)):
            path.write_text(fixture)
            result = run(cmd, cwd=root, text=True, capture_output=True, timeout=60)
            observed = [(m[2], m[3]) for m in hit.finditer(result.stderr)]
            if result.returncode or observed != [("0", str(amount)), ("1", str(amount))]:
                raise ValueError(f"expected damage {amount} in both rounds:\n{result.stderr[-7000:]}")
            traces.append([hit.sub(r"\g<1>HP", row) for row in result.stderr.splitlines() if "borg-api:" in row])
        if traces[0] != traces[1]:
            raise ValueError("a damage floor changed the accepted event trace beyond the amount")
    print(f"Contact floor: base-4 sub-one hit deals 1 vs 0, with the same hit trace "
          f"({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
