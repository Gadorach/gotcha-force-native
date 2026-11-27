"""Exercise the generated size-selected contact cue in the native mixer."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory


def check_normal_roster(roster):
    root = Path(__file__).resolve().parents[2]
    proof = json.loads((root / "docs/api-map/gg4e08-normal-receiver-sound.json").read_bytes())
    statuses = Counter()
    for row in proof["records"]:
        package = roster / row["borg"].replace(":", "-")
        lines = (package / "gameplay.txt").read_text().splitlines()
        summary = json.loads((package / "source-index.json").read_bytes())
        receipts = [summary[key]["contact_audio"]["receiver"]
                    for key in ("basic_shot_battle_test", "basic_melee_battle_test", "held_fire_battle_test")
                    if key in summary]
        receiver, = receipts
        statuses[receiver["status"]] += 1
        bands = [line for line in lines if line.startswith("contact-sound-band=")]
        if receiver["band"] is None:
            matches = not bands
        else:
            matches = (bands == [f"contact-sound-band={row['band']}"] and
                       receiver["band"] == row["band"] and receiver["spec_record"] == row["spec_record"] and
                       receiver["observed_config"] == row["config"])
        if not matches:
            raise ValueError(f"{row['borg']}: receiver band did not match its normal record")
    if statuses != {"verified normal size band": 202, "installer bands disagree": 4}:
        raise ValueError(f"unexpected normal receiver coverage: {dict(statuses)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("roster", type=Path)
    parser.add_argument("stage", type=Path)
    parser.add_argument("--opengl", action="store_true")
    parser.add_argument("--normal-roster", action="store_true", help="validate the full shot and melee conversion")
    args = parser.parse_args()
    exe, roster, stage = (p.resolve(strict=True) for p in (args.executable, args.roster, args.stage))
    if args.normal_roster:
        check_normal_roster(roster)
    with TemporaryDirectory(prefix="gf-retail-contact-sound-") as temp:
        root = Path(temp)
        models = ("0615", "000a", "0602", "0001")
        cases = (("0615", "basic-shot", "000a", 0, False), ("0615", "basic-shot", "0602", 3, False),
                 ("0615", "basic-shot", "000a", 0, True), ("0001", "opening-slash", "000a", 0, False))
        if args.normal_roster:
            models += ("061b",)
            cases += (("0615", "basic-shot", "061b", 1, False),)
        for model in models:
            name = f"gg4e08-{model}"
            copytree(roster / name, root / "borgs" / name, symlinks=True)
        shooter = root / "borgs/gg4e08-0615"
        attack = json.loads((shooter / "source-index.json").read_bytes())["basic_shot_battle_test"]["contact_audio"]
        choice = attack["moves"]["basic-shot"]
        if choice["code"] != 0x88 or [choice["bands"][i]["sound_ids"] for i in (0, 3)] != [[64, 65], [70, 71]]:
            raise ValueError("unexpected G-Red contact audio binding")
        common = [exe, "--demo-package-battle", "--trace-package-battle",
                  "--frames", "150", "--battle-mvp-assets", stage]
        if not args.opengl:
            common.append("--headless")
        for attacker, move, receiver, band, mute in cases:
            profile = (root / "borgs" / f"gg4e08-{receiver}" / "gameplay.txt").read_text().splitlines()
            if f"contact-sound-band={band}" not in profile:
                raise ValueError(f"{receiver}: expected size band {band}")
            cmd = [*common, "--battle-borg", f"gg4e08:{attacker}", "--against-borg", f"gg4e08:{receiver}"]
            if move == "opening-slash":
                cmd.extend(("--package-battle-distance", "90"))
            if mute:
                cmd.append("--no-audio")
            result = run(cmd, cwd=root, capture_output=True, text=True, timeout=60)
            cues = re.findall(rf"round=(\d+) .*actor=0 move={move} event=contact-sound .*recipient=1 value=(\d+)",
                              result.stderr)
            summary = re.search(r"borg-contact-audio.*starts=(\d+) nonzero=(\d+)", result.stderr)
            if (result.returncode or cues != [("0", str(band * 2)), ("1", str(band * 2))] or not summary or
                    summary.groups() != (("0", "0") if mute else ("2", "1"))):
                raise ValueError(f"{receiver} muted={mute}: contact audio differed:\n{result.stderr[-6500:]}")
        corrupt = shooter / "audio/contact-64.pcm"
        if corrupt.is_symlink():
            raise ValueError("smoke requires a scratch PCM copy")
        corrupt.write_bytes(b"bad")
        result = run([*common, "--battle-borg", "gg4e08:0615", "--against-borg", "gg4e08:000a", "--no-audio"],
                     cwd=root, capture_output=True, text=True, timeout=60)
        if not result.returncode or "unqualified contact sound" not in result.stderr:
            raise ValueError("malformed contact PCM was not refused")
    print(f"Contact audio: projectile and melee cues, {3 if args.normal_roster else 2} size bands, "
          f"restarts, mute and malformed PCM passed "
          f"({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
