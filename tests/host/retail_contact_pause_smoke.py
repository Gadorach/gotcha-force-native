"""Verify the generated skeletal hold without changing battle event timing."""
import argparse
import json
from pathlib import Path
import re
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory


CASES = (
    ("0f00", "basic_shot_battle_test", "basic-shot", 4, (1,)),
    ("0001", "basic_melee_battle_test", "opening-slash", 8, (0, 1)),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path)
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, roster, stage = (p.resolve(strict=True) for p in (args.executable, args.roster, args.stage))
    with TemporaryDirectory(prefix="gf-contact-pause-") as temp:
        root = Path(temp)
        for model in ("000a", *(case[0] for case in CASES)):
            name = f"gg4e08-{model}"
            copytree(roster / name, root / "borgs" / name, symlinks=True)
        target = root / "borgs/gg4e08-000a/gameplay.txt"
        if "contact-pose-hold-receive=normal\n" not in target.read_text():
            raise ValueError("expected a receiver with a qualified normal body")
        for model, kind, move, seed, bodies in CASES:
            root_package = root / "borgs" / f"gg4e08-{model}"
            profile = root_package / "gameplay.txt"
            if profile.is_symlink():
                raise ValueError("pose pause smoke needs a scratch profile")
            held_ticks = seed - 1
            scope = "bodies" if len(bodies) == 2 else "receiver"
            marker = f"contact-pose-hold={move}|{held_ticks}|{scope}\n"
            text = profile.read_text()
            receipt = json.loads((root_package / "source-index.json").read_bytes())[kind]["contact_pose_pause"]["moves"][move]
            if text.count(marker) != 1 or {r["seed"] for r in receipt["entries"]} != {seed}:
                raise ValueError(f"{model}: unexpected pose hold binding")
            cmd = [exe, "--battle-borg", f"gg4e08:{model}", "--against-borg", "gg4e08:000a",
                   "--demo-package-battle", "--trace-package-battle", "--frames", "150",
                   "--battle-mvp-assets", stage, "--no-audio"]
            if len(bodies) == 2:
                cmd.extend(("--package-battle-distance", "90"))
            if not args.opengl:
                cmd.append("--headless")
            active = run(cmd, cwd=root, capture_output=True, text=True, timeout=60)
            profile.write_text(text.replace(marker, ""))
            control = run(cmd, cwd=root, capture_output=True, text=True, timeout=60)
            hits = re.findall(rf"borg-api: round=(\d+) tick=(\d+) actor=0 move={move} event=hit .*recipient=1 ",
                              active.stderr)
            held = re.findall(r"contact-pose-hold: round=(\d+) tick=(\d+) actor=(\d+) remaining=(\d+)", active.stderr)
            expected = [(rnd, str(int(tick) + offset), str(actor), str(held_ticks - offset))
                        for rnd, tick in hits for offset in range(held_ticks) for actor in bodies]
            events = lambda result: [line for line in result.stderr.splitlines() if "borg-api:" in line]
            if (active.returncode or control.returncode or [rnd for rnd, _ in hits] != ["0", "1"] or
                    held != expected or "contact-pose-hold:" in control.stderr or events(active) != events(control)):
                raise ValueError(f"{model}: wrong hold or changed battle trace:\n{active.stderr[-7200:]}")
    print(f"Pose pause: projectile receiver holds 3 ticks, melee holds both bodies 7 ticks; hit traces unchanged "
          f"({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
