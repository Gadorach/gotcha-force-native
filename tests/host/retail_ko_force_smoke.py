"""Exercise the installed normal Tau cost with unchanged native hit and round timing."""
import argparse
from pathlib import Path
import re
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.portable_borg_preview import prepare


HIT = re.compile(r"round=(\d+) tick=(\d+) actor=(\d+) move=([^ ]+) event=hit .*recipient=(\d+) value=(\d+)")
DEBIT = re.compile(r"round=(\d+) tick=(\d+) receiver=(\d+) source=(\d+) team=(\d+) cost=(\d+) debit=(\d+) "
                   r"before=(\d+) after=(\d+) exhausted=(\d+)")


def smoke(exe, roster, art):
    with TemporaryDirectory(prefix="gf-ko-force-smoke-") as scratch:
        root = Path(scratch)
        receiver = roster/"gg4e08-0807"
        if b"ko-force-cost=90" not in (receiver/"gameplay.txt").read_bytes().splitlines():
            raise ValueError("installed normal Tau lacks the verified cost")
        for lethal in (False, True):
            # Only the authored attacker is transformed. The installed Tau
            # keeps its real package, including 60 HP and its opt-in cost.
            case = root/("lethal" if lethal else "survives")
            copytree(receiver, case/"borgs/gg4e08-0807")
            if (roster/"shared").is_dir():
                copytree(roster/"shared", case/"borgs/shared")

            def rules(gameplay):
                source = b"melee=close-cut|4|8|115|12|60|30|20\n"
                if source not in gameplay:
                    raise ValueError("authored strike fixture changed")
                return (gameplay.replace(source, source.replace(b"|20\n", b"|100\n")) if lethal else gameplay
                        )+b"ko-force-cost=55\n"

            attacker = prepare("blade-scout", art, case/"borgs/blade-scout", battle_test=True, gameplay_transform=rules)
            args = [str(exe), "--battle-borg", attacker, "--against-borg", "gg4e08:0807",
                    "--package-battle-distance", "90", "--demo-package-battle", "--trace-package-battle",
                    "--frames", "60", "--battle-mvp-assets", str(art), "--no-audio", "--headless"]
            baseline_hits = None
            for force in (None, 180, 90, 40):
                p = run(args + (["--package-ko-force", f"120,{force}"] if force is not None else []),
                        cwd=case, capture_output=True, text=True, timeout=40)
                hits = HIT.findall(p.stderr)
                got = [tuple(map(int, values)) for values in DEBIT.findall(p.stderr)]
                subject = [row for row in hits if row[2:5] == ("0", "close-cut", "1")]
                if p.returncode or not subject or subject[0][-1] != ("60" if lethal else "20"):
                    raise ValueError(f"installed normal Tau contact failed:\n{p.stderr[-5000:]}")
                if force is None:
                    baseline_hits = hits
                    if "borg-ko-force" in p.stderr:
                        raise ValueError("dormant cost emitted a debit")
                    continue
                if hits != baseline_hits:
                    raise ValueError("KO Force accounting changed the native hit trace")
                spent = min(90, force) if lethal else 0
                expected = [(int(r[0]), int(r[1]), 1, 0, 1, 90, spent, force, force-spent, int(force == spent))
                            for r in subject if lethal]
                if got != expected or f"remaining=120,{force-spent}" not in p.stderr:
                    raise ValueError(f"KO Force accounting mismatch:\n{p.stderr[-5000:]}")
        missing = root/"missing-cost"
        copytree(receiver, missing/"borgs/gg4e08-0807")
        if (roster/"shared").is_dir():
            copytree(roster/"shared", missing/"borgs/shared")
        prepare("blade-scout", art, missing/"borgs/blade-scout")
        p = run(args+["--package-ko-force", "120,90"], cwd=missing, capture_output=True, text=True, timeout=40)
        if p.returncode == 0 or "KO Force accounting refused" not in p.stderr or "event=hit" in p.stderr:
            raise ValueError(f"missing participant cost was not refused:\n{p.stderr[-5000:]}")
    print("Installed normal Tau: nonlethal, full, exact and clamped KO debit; native hit trace unchanged")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("executable", "roster", "art"):
        parser.add_argument(name, type=Path)
    smoke(*(p.resolve(strict=True) for p in vars(parser.parse_args()).values()))
