"""Draw every declared joint-local origin on its exact normal body."""
import argparse
from pathlib import Path
from collections import Counter
import json
import re
import subprocess


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable",type=Path)
    parser.add_argument("root",type=Path,help="roster install containing borgs/, made with --origin-markers --source-variants")
    args=parser.parse_args()
    exe=args.executable.resolve(strict=True)
    root=args.root.resolve(strict=True)
    borg_count=0
    passed=0
    refused=Counter()
    alternate=None
    for path in sorted((root/"borgs").glob("gg4e08-*/source-index.json")):
        with path.open("rb") as stream:
            data=stream.read((2<<20)+1)
        if len(data)>2<<20:
            raise ValueError("source index byte budget")
        index=json.loads(data)
        ident=index["id"]
        if not re.fullmatch(r"gg4e08:[0-9a-f]{4}",ident):
            raise ValueError("unexpected package identity")
        points=index.get("preview_origin_markers",[])
        borg_count+=bool(points)
        if len(points)>16 or len({p[0] for p in points})!=len(points):
            raise ValueError("origin marker count/identity")
        for point,_,*local in points:
            if not re.fullmatch(r"type-(?:(?:0|2|14|15)-[0-9]{1,3}|47-[0-5])",point) or len(local)!=3:
                raise ValueError("not a bounded source origin declaration")
            cmd=[exe,"--preview-borg",ident,"--borg-variant","normal","--preview-origin",point,"--headless","--frames","3"]
            result=subprocess.run(cmd,cwd=root,text=True,capture_output=True,timeout=15)
            log=result.stdout+result.stderr
            if result.returncode==0 and f"origin={point} marker-frames=3 point-only" in log:
                passed+=1
                if alternate is None and index["selectable_variants"][1:]:
                    alternate=cmd.copy()
                    alternate[4]=index["selectable_variants"][1]
                continue
            if result.returncode==2 and "invalid reflection normal/camera" in log:
                refused["strict-reflection"]+=1
                continue
            raise ValueError(f"unexpected origin preview result for {ident}/{point}: {log}")
    if not passed or alternate is None:
        raise ValueError("expected positive markers and at least one declared alternate")
    bad=subprocess.run(alternate,cwd=root,text=True,capture_output=True,timeout=15)
    if bad.returncode!=2 or "origin is not declared for this exact look" not in bad.stderr:
        raise ValueError("alternate inherited a normal origin")
    print(json.dumps({"mapped_borgs":borg_count,"markers_passed":passed,
                      "marker_refusals":dict(refused),"alternate_refused":True},indent=2))


if __name__=="__main__":
    main()
