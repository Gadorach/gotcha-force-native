"""Check close-B reaction rows, continuation poses and live authored contacts."""
import argparse
import json
import re
from pathlib import Path
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.portable_borg_preview import prepare

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/reaction-melee-map.json"
CONTINUATIONS = ("0106", "0107", "0300", "0301", "0302", "0304", "0305",
                 "0306", "0307", "0309", "030a", "030b", "030c", "030d")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path, help="direct output of retail_borg_roster")
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, roster, stage = (p.resolve(strict=True) for p in (args.executable, args.roster, args.stage))
    cases = json.loads(ARTIFACT.read_text())["pilots"]
    for model in CONTINUATIONS:
        cases.setdefault(model, None)
    with TemporaryDirectory(prefix="gf-retail-melee-reaction-") as scratch:
        root = Path(scratch)
        prepare("target", stage, root / "borgs/training-target")
        for model, expected in sorted(cases.items()):
            name = f"gg4e08-{model}"
            package = root / "borgs" / name
            copytree(roster / name, package, symlinks=True)
            rules = (package / "gameplay.txt").read_text().splitlines()
            info = json.loads((package / "source-index.json").read_text())["basic_melee_battle_test"]
            binding = info.get("reaction", {}).get("opening-slash")
            emitted = [rule for rule in rules if rule.startswith(
                ("# close-B reaction:", "# source reaction mode ", "reaction-mode=", "reaction-offset="))]
            if (expected is not None and binding != expected.get("binding") or
                    emitted != (binding["lines"] if binding else [])):
                raise ValueError(f"{model}: generated contact rows differ from the reviewed map")
            follow_rows = [rule for rule in rules if rule.startswith("pose-followup=")]
            follow = info.get("continuation_pose")
            if expected is not None and follow != expected.get("continuation_pose"):
                raise ValueError(f"{model}: generated pose differs from the reviewed map")
            if model in CONTINUATIONS:
                if (not binding or binding.get("continuation", {}).get("seed") != 1 or
                        binding["lines"][-2:] != ["reaction-mode=opening-slash|4", "reaction-offset=opening-slash|0|24"] or
                        follow_rows != ["pose-followup=opening-slash|6|1:1|0"] or not follow or
                        (follow["switch_frame"], follow["clip"], follow["source_start_frame"],
                         follow["source_program"]) != (6, [1, 1], 0, [3, 1])):
                    raise ValueError(f"{model}: missing exact continuation pose or reaction")
            elif follow_rows or follow:
                raise ValueError(f"{model}: a direct entry contact unexpectedly has a pose follow-up")
            cmd = [exe, "--battle-borg", f"gg4e08:{model}", "--against-borg", "test:target",
                   "--package-battle-distance", "90", "--demo-package-battle", "--trace-package-battle",
                   "--frames", "150", "--battle-mvp-assets", stage, "--no-audio"]
            if not args.opengl:
                cmd.append("--headless")
            got = run(cmd, cwd=root, text=True, capture_output=True, timeout=60)
            hits = re.findall(r"round=(\d+) .*move=opening-slash event=hit .*recipient=1 value=(\d+)",
                              got.stderr)
            if (got.returncode or hits != [("0", "12"), ("1", "12")] or
                    "shots=0 hits=2 reactions=2" not in got.stderr or "event=debit " in got.stderr):
                raise ValueError(f"{model}: expected a stockless authored hit in each round:\n{got.stderr[-8000:]}")
    print(f"{len(cases)} generated close-B packages passed ({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
