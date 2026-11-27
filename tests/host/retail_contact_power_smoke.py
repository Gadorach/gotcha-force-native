"""Validate recovered direct bases in a fresh opted-in generated roster."""
import argparse
import json
import re
from pathlib import Path
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.portable_borg_preview import prepare

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/damage-contact-power.json"
CASES = (
    ("0615", "projectiles", "basic-shot", "test:target", 20),
    ("000a", "projectiles", "basic-shot", "test:target", 5),
    ("0501", "projectiles", "basic-shot", "test:target", 8),
    ("0001", "close_b", "opening-slash", "test:target", 40),
    ("0202", "close_b", "opening-slash", "test:target", 12),
    ("0615", "projectiles", "basic-shot", "gg4e08:0602", 6),
    ("0615", "projectiles", "basic-shot", "gg4e08:0105", 22),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path, help="direct output with --contact-power-test and --contact-scale-test")
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, roster, stage = (p.resolve(strict=True) for p in (args.executable, args.roster, args.stage))
    artifact = json.loads(ARTIFACT.read_bytes())
    with TemporaryDirectory(prefix="gf-retail-power-") as scratch:
        root = Path(scratch)
        prepare("target", stage, root / "borgs/training-target")
        models = {case[0] for case in CASES}
        models.update(case[3].split(":")[1] for case in CASES if case[3].startswith("gg4e08:"))
        for model in sorted(models):
            name = f"gg4e08-{model}"
            copytree(roster / name, root / "borgs" / name, symlinks=True)
        for model, family, move, opponent, amount in CASES:
            package = root / "borgs" / f"gg4e08-{model}"
            record = json.loads((package / "source-index.json").read_bytes())
            key = "basic_melee_battle_test" if family == "close_b" else "basic_shot_battle_test"
            receipt = record[key]
            power = artifact[family][model]
            if receipt["contact_power_test"].get(move) != power or receipt["contact_scale_test"]["settings"] != 7:
                raise ValueError(f"{model}: wrong opted-in source snapshot")
            rules = (package / "gameplay.txt").read_text().splitlines()
            lines = [line for line in rules if line.startswith(f"contact-power={move}|")]
            if lines != ([] if power["base"] is None else [f"contact-power={move}|{power['base']}"]):
                raise ValueError(f"{model}: generated base differs from the verified table")
            if opponent == "gg4e08:0105":
                receiver_rules = (root / "borgs/gg4e08-0105/gameplay.txt").read_text().splitlines()
                if ("damage-matchup-group=gg4e08-class-4" not in rules or
                        "damage-matchup-from=gg4e08-class-4|1.1" not in receiver_rules or
                        "damage-matchup-from=gg4e08-class-19|0.9" not in rules):
                    raise ValueError("directional matchup bindings are absent or transposed")
            cmd = [exe, "--battle-borg", f"gg4e08:{model}", "--against-borg", opponent,
                   "--demo-package-battle", "--trace-package-battle", "--frames", "150",
                   "--battle-mvp-assets", stage, "--no-audio"]
            if family == "close_b":
                cmd.extend(("--package-battle-distance", "90"))
            if not args.opengl:
                cmd.append("--headless")
            result = run(cmd, cwd=root, text=True, capture_output=True, timeout=60)
            hits = re.findall(rf"round=(\d+) .*actor=0 move={move} event=hit .*recipient=1 value=(\d+)",
                              result.stderr)
            if result.returncode or hits != [("0", str(amount)), ("1", str(amount))]:
                raise ValueError(f"{model} vs {opponent}: expected two hits of {amount}:\n{result.stderr[-8000:]}")
    print(f"{len(CASES)} generated contact-power cases passed ({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
