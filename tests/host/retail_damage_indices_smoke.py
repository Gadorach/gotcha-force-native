"""Check both independent settings and rank axes on generated package hits."""
import argparse
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
    with TemporaryDirectory(prefix="gf-indexed-damage-") as temp:
        root = Path(temp)
        for model in ("0615", "000a"):
            name = f"gg4e08-{model}"
            copytree(roster / name, root / "borgs" / name, symlinks=True)
        shooter = root / "borgs/gg4e08-0615/gameplay.txt"
        if shooter.is_symlink():
            raise ValueError("indexed damage smoke needs a scratch gameplay copy")
        profile = shooter.read_text().splitlines()
        if ("contact-power=basic-shot|20" not in profile or
                any(line.startswith("damage-credit-ladders=") for line in profile)):
            raise ValueError("expected base-20 G-Red with no HP/force ladders")
        common = [exe, "--battle-borg", "gg4e08:0615", "--against-borg", "gg4e08:000a",
                  "--demo-package-battle", "--trace-package-battle", "--frames", "150",
                  "--no-audio", "--battle-mvp-assets", stage]
        if not args.opengl:
            common.append("--headless")

        def duel(selection):
            cmd = common if selection is None else [*common, "--package-damage-indices", selection]
            return run(cmd, cwd=root, text=True, capture_output=True, timeout=60)

        def verify(selection, expected):
            result = duel(selection)
            observed = re.findall(r"round=(\d+) .*actor=0 move=basic-shot event=hit .*recipient=1 value=(\d+)",
                                  result.stderr)
            if result.returncode or observed != [("0", str(expected)), ("1", str(expected))]:
                raise ValueError(f"{selection}: expected {expected} damage per round:\n{result.stderr[-6500:]}")

        verify(None, 20)
        verify("7,7:0,0", 20)
        verify("0,31:3,0", 46)
        verify("7,7:7,0", 28)
        verify("7,7:0,7", 14)
        # In the scripted four-actor round G-Red's shot lands on actor 3,
        # not actor 1. Both receive on team 1; only actor 3's rank changes.
        for selection, expected in (("7,7:7,0,0,0", 28), ("7,7:7,0,0,7", 19)):
            result = run([*common, "--demo-roster-battle", "--package-damage-indices", selection],
                         cwd=root, text=True, capture_output=True, timeout=60)
            hits = re.findall(r"round=(\d+) .*actor=0 move=basic-shot event=hit .*recipient=(\d+) value=(\d+)",
                              result.stderr)
            if result.returncode or hits != [("0", "3", str(expected)), ("1", "3", str(expected))]:
                raise ValueError(f"{selection}: expected actor 3 to take {expected}:\n{result.stderr[-6500:]}")
            histories = re.findall(r"borg-hit-history: round=1 actor=(\d+) cursor=(\d+) ally-hits=(\d+) "
                                   r"ally-damage=(\d+) last=(\d+):(\d+)", result.stderr)
            if [tuple(map(int, row)) for row in histories] != [
                    (0, 1, 0, 0, 3, 3), (1, 1, 1, expected, 4, 3), (2, 1, 0, 0, 3, 3), (3, 0, 0, 0, 0, 0)]:
                raise ValueError(f"{selection}: expected final-round per-slot hit history:\n{result.stderr[-6500:]}")
        if not duel("32,7:0,0").returncode or not duel("7,7:0,0,0,0").returncode:
            raise ValueError("out-of-range index or wrong actor count was admitted")
        shooter.write_text("\n".join(line for line in profile
                                     if not line.startswith(("damage-credit-setting=", "damage-credit-rank="))) + "\n")
        stale = duel("7,7:0,0")
        if not stale.returncode or "indexed damage selection requires matching complete package tables" not in stale.stderr:
            raise ValueError("the snapshot must not impersonate indexed tables")
        verify(None, 20)
    print(f"Indexed damage: snapshot 20, selected setting 46, attack rank 28, receiver rank 14; "
          f"four-actor recipient rank 28 -> 19 "
          f"({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
