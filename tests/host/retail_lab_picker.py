"""Exercise exact live selections and refusal with a private copied roster."""
import argparse
from pathlib import Path
from shutil import copytree
import subprocess
from tempfile import TemporaryDirectory


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable",type=Path)
    parser.add_argument("root",type=Path)
    parser.add_argument("stage",type=Path)
    modes=parser.add_mutually_exclusive_group()
    modes.add_argument("--source-pace",action="store_true",help="check pace admission and retention instead of jump")
    modes.add_argument("--cruise",action="store_true",help="switch between static and animated cruise, then refuse a missing entry")
    args=parser.parse_args()
    exe,root,stage=(p.resolve(strict=True) for p in (args.executable,args.root,args.stage))
    mode="--lab-cruise" if args.cruise else "--lab-source-pace" if args.source_pace else "--lab-jump"
    initial,target=("0505","061b") if args.cruise else ("0000","0001")
    walk=int(not args.cruise)
    cmd=[exe,"--demo-borg-controls","--demo-lab-picker","--walk-borg",f"gg4e08:{initial}","--borg-variant","normal",
         mode,"--headless","--frames","90","--battle-mvp-assets",stage]
    pace=("ground-pace target=500 measured_peak=500 provisional",) if args.source_pace else ()
    with TemporaryDirectory(prefix="gf-lab-picker-") as tmp:
        install=Path(tmp)
        # 0802 can walk, but its conditional actor data must exclude it from pace mode.
        for ident in (initial,target,"0802") if args.source_pace else (initial,target):
            source=root/"borgs"/f"gg4e08-{ident}"
            copytree(source,install/"borgs"/source.name,symlinks=True)
        def run():
            return subprocess.run(cmd,cwd=install,text=True,capture_output=True,timeout=15)
        valid=run()
        if valid.returncode or any(s not in valid.stderr for s in (
            "picker looks=2",f"picker active id=gg4e08:{target} variant=normal actors=1 tick=0",
            f"picker active id=gg4e08:{initial} variant=normal actors=1 tick=0",
            "picker switches=2 refusals=0",f"actors=1 moved=1 walk={walk} shots=0 hits=0",*pace)):
            raise ValueError(valid.stderr)
        # Its declaration still appears; the switch must fail in actual binding.
        broken=install/"borgs"/f"gg4e08-{target}"/("preview.txt" if args.cruise else "visual/body.hsd")
        if broken.is_symlink() or not broken.is_file() or not broken.resolve().is_relative_to(install):
            raise ValueError("copied fixture must be a regular file")
        if args.cruise:
            text=broken.read_text()
            if text.count("candidate=cruise-entry|2:0\n")!=1:
                raise ValueError("expected the exact Eagle Jet entry")
            broken.write_text(text.replace("candidate=cruise-entry|2:0\n","candidate=cruise-entry|15:0\n"))
        else:
            broken.write_bytes(bytes(32))
        failed=run()
        if failed.returncode!=2 or any(s not in failed.stderr for s in (
            f"picker refused id=gg4e08:{target} variant=normal",
            f"id=gg4e08:{initial} variant=normal frames=90 actors=1 moved=1 walk={walk} shots=0 hits=0",
            "picker switches=0 refusals=1",*pace)):
            raise ValueError(failed.stderr)
        if args.cruise and "movement lab: exact mapped clips required" not in failed.stderr:
            raise ValueError(failed.stderr)
    print("exact lab picker switch-back and rejected-switch retention PASS")


if __name__=="__main__":
    main()
