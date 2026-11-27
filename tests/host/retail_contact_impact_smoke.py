"""Compare bound contact art to the same generated round without it."""
import argparse
import json
from pathlib import Path
import re
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory


CASES = (
    ("0615", "basic_shot_battle_test", "basic-shot", (2,), (1,) * 16),
    ("0200", "basic_melee_battle_test", "opening-slash", (144, 145, 146), (5,) * 7 + (3,) * 3 + (1,) * 3),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path)
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, roster, stage = (item.resolve(strict=True) for item in (args.executable, args.roster, args.stage))
    with TemporaryDirectory(prefix="gf-contact-impact-") as temp:
        root = Path(temp)
        for model in ("000a", *(item[0] for item in CASES)):
            name = f"gg4e08-{model}"
            copytree(roster / name, root / "borgs" / name, symlinks=True)
        receiver = root / "borgs/gg4e08-000a/gameplay.txt"
        if receiver.is_symlink():
            raise ValueError("smoke requires a scratch gameplay copy")
        profile = receiver.read_text()
        marker = "contact-impact-receive=normal\n"
        if profile.count(marker) != 1:
            raise ValueError("fixture receiver has no qualified impact binding")
        for model, receipt, move, records, depths in CASES:
            package = root / "borgs" / f"gg4e08-{model}"
            claims = json.loads((package / "source-index.json").read_bytes())
            bound = claims[receipt]["contact_visuals"]["moves"][move]["teams"]
            if tuple(item["record"] for item in bound[0]) != records:
                raise ValueError(f"{model}: incorrect local art receipt")
            cmd = [exe, "--battle-borg", f"gg4e08:{model}", "--against-borg", "gg4e08:000a",
                   "--demo-package-battle", "--trace-package-battle", "--frames", "150",
                   "--battle-mvp-assets", stage, "--no-audio"]
            if move == "opening-slash":
                cmd.extend(("--package-battle-distance", "90"))
            if not args.opengl:
                cmd.append("--headless")
            receiver.write_text(profile)
            with_art = run(cmd, cwd=root, capture_output=True, text=True, timeout=60)
            receiver.write_text(profile.replace(marker, ""))
            without = run(cmd, cwd=root, capture_output=True, text=True, timeout=60)
            events = lambda result: [line for line in result.stderr.splitlines() if "borg-api:" in line]
            hits = re.findall(rf"borg-api: round=(\d+) tick=(\d+) actor=0 move={move} event=hit .*recipient=1 ",
                              with_art.stderr)
            draws = re.findall(r"impact-record: round=(\d+) tick=(\d+) draws=(\d+)", with_art.stderr)
            expected = [(rnd, str(int(tick) + offset), str(n)) for rnd, tick in hits for offset, n in enumerate(depths)]
            if (with_art.returncode or without.returncode or [rnd for rnd, _ in hits] != ["0", "1"] or
                    draws != expected or "impact-record:" in without.stderr or events(with_art) != events(without)):
                raise ValueError(f"{model}: art did not match the expected render-only layers:\n{with_art.stderr[-7000:]}")
    print(f"Contact art: projectile and layered melee records submit with unchanged hit traces "
          f"({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
