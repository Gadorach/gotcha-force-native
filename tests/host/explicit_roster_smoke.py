"""Exercise six individually admitted package actors in the desktop host."""
import argparse
from pathlib import Path
import re
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.portable_borg_preview import prepare


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--art", type=Path, default=Path("local/content/native-battle-mvp-v1"))
    parser.add_argument("--movement", type=Path, default=Path("local/content/source-gred-ground-v9"))
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    exe, art, movement = (p.resolve(strict=True) for p in (args.executable, args.art, args.movement))
    models = ("arc-scout", "blade-scout", "grapple-scout", "seeker-scout", "guard-scout", "wingmate-scout")
    with TemporaryDirectory(prefix="gf-explicit-roster-") as temp:
        root = Path(temp)
        for model in models:
            prepare(model, art, root / "borgs" / model)
        prepare("g-red", art, root / "borgs/g-red", movement, True)
        base = [exe, "--battle-borg", f"test:{models[0]}", "--against-borg", f"test:{models[1]}",
                "--battle-mvp-assets", art, "--demo-package-battle", "--trace-package-battle", "--frames", "125",
                "--no-audio"]
        if not args.opengl:
            base.append("--headless")
        extras = [arg for i, model in enumerate(models[2:]) for arg in
                  ("--roster-borg", str(i % 2), f"test:{model}")]

        def launch(*extra):
            return run([*base, *extra], cwd=root, capture_output=True, text=True, timeout=60)

        screenshot = root / "aim-labels.ppm"
        result = launch(*extras, *(("--screenshot", screenshot) if args.opengl else ()))
        log = result.stderr
        slots = re.findall(r"borg-roster-slot: actor=(\d+) team=(\d+) driver=(\w+) id=(\S+)", log)
        expected = [(str(i), str(i % 2), "local" if i < 2 else "cpu", f"test:{model}") for i, model in enumerate(models)]
        emitted = set(re.findall(r"borg-api: round=\d+ tick=\d+ actor=([2-5]) .*event=emit ", log))
        cycled = re.findall(r"borg-aim-cycle: round=(\d+) tick=(\d+) actor=(\d+) target=(\d+)", log)
        if (result.returncode or slots != expected or emitted != {"2", "3", "4", "5"} or
                "actors=6 restarts=1" not in log or "round=1" not in log):
            raise ValueError(f"explicit roster did not play or restart:\n{log[-8500:]}")
        if cycled != [("0", "3", "0", "3"), ("0", "3", "1", "2"),
                      ("0", "7", "0", "5"), ("0", "7", "1", "4")]:
            raise ValueError(f"unexpected local target-cycle edges: {cycled}")
        if args.opengl:
            header, _, pixels = screenshot.read_bytes().partition(b"\n255\n")
            if not header.startswith(b"P6\n") or any(pixels.count(bytes(rgb)) < 12 for rgb in
                                                       ((255, 205, 90), (115, 220, 255),
                                                        (255, 175, 83), (108, 178, 255))):
                raise ValueError("screenshot did not show both aim labels and both team health colors")
            compact = root / "compact-hud.ppm"
            resized = launch(*extras, "--size", "180", "150", "--frames", "14", "--screenshot", compact)
            if resized.returncode:
                raise ValueError(f"compact roster viewport failed:\n{resized.stderr[-2500:]}")
            header, _, pixels = compact.read_bytes().partition(b"\n255\n")
            top = pixels[:180 * 18 * 3]
            if b"180 150" not in header or any(top.count(bytes(rgb)) < 6 for rgb in
                                               ((255, 175, 83), (108, 178, 255))):
                raise ValueError("small viewport did not show both team counters")

        borrowed = launch("--roster-borg", "0", "gg4e08:0615")
        if not borrowed.returncode or "explicit rosters require native package movement" not in borrowed.stderr:
            raise ValueError("an unbound borrowed source-motion actor was admitted")
        for refused in (["--demo-roster-battle", *extras],
                        ["--roster-borg", "0", "test:arc-scout", "--roster-borg", "0", "test:blade-scout",
                         "--roster-borg", "0", "test:guard-scout"],
                        [*extras, "--package-damage-indices", "7,7:0,0,0,0"]):
            if not launch(*refused).returncode:
                raise ValueError(f"unsupported roster combination was admitted: {refused}")

        # Scratch variants isolate the same two limits found in converted Borgs:
        # a fighter with only close B and one with only a long X charge.
        prepare("charge-scout", art, root / "borgs/charge-scout")
        for model in ("blade-scout", "charge-scout"):
            profile = root / "borgs" / model / "gameplay.txt"
            text = profile.read_text()
            assert text.count("move=primary|") == text.count("release=primary|") == 1
            rows = [row for row in text.splitlines() if not row.startswith(("move=primary|", "release=primary|"))]
            profile.write_text(("\n".join(rows) + "\n").replace("|release-window|8|600", "|release-window|24|600"))
        fought = launch("--roster-borg", "0", "test:blade-scout", "--roster-borg", "0", "test:charge-scout",
                        "--package-battle-distance", "460", "--frames", "175")
        melee_hit = re.search(r"borg-api: round=\d+ tick=\d+ actor=2 move=close-cut event=hit .*recipient=1 ", fought.stderr)
        charged = re.search(r"borg-api: round=\d+ tick=\d+ actor=3 move=pulse-charge event=emit ", fought.stderr)
        if fought.returncode or not melee_hit or not charged:
            combat = "\n".join(line for line in fought.stderr.splitlines() if "borg-api:" in line or "package-battle:" in line)
            raise ValueError(f"melee approach or long X charge did not resolve:\n{combat[-7000:]}")
    print(f"Explicit roster: six packages, melee and charged CPUs, local aim cycling, P2 restart and refusals passed "
          f"({'OpenGL' if args.opengl else 'headless'})")


if __name__ == "__main__":
    main()
