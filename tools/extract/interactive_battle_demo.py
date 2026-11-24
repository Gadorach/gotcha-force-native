"""Assemble a private, fresh Battle Test install from an already extracted roster."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import shlex

from tools.extract.demo_borgs import prepare as generate_sticks
from tools.extract.portable_borg_preview import CUSTOM_NAMES, prepare as generate_example


def source_movement_rules(source, id, data, *, generated=False):
    """Bind exact indexed clips and a published native pace, never borrowed art."""
    index=source/"source-index.json"
    if index.is_symlink() or not index.is_file():
        raise ValueError(f"missing exact pace index for {id}")
    with index.open("rb") as inp:
        raw=inp.read((2<<20)+1)
    if len(raw)>2<<20:
        raise ValueError("pace index size")
    evidence=json.loads(raw)
    pace=evidence.get("lab_ground_pace")
    if b"\ncruise-boost=" in data or b"\nbody-motion=" in data:
        return data
    if evidence["id"]!=id or not generated and evidence["asset_sha256"].get("gameplay.txt")!=sha256(data).hexdigest():
        raise ValueError(f"pace index does not match {id}")
    if pace:
        speed=pace["native_units_per_tick_x100"]
        if (pace["native_policy"]!="source-relative-ground-v1" or pace["operand"]!="02C" or
                type(speed) is not int or not 40<=speed<=1600):
            raise ValueError(f"unqualified ground pace for {id}")
        data,n=re.subn(rb"(?m)^movement=grounded-v1\|[^|\n]+\|",
                      f"movement=grounded-v1|{speed//100}.{speed%100:02d}|".encode("ascii"),data)
        if n!=1:
            raise ValueError(f"ambiguous ground pace for {id}")
    slots={(g,c):at for g,c,at in evidence["raw_animation_slots"]}
    timing={t["archive_offset"]:t for t in evidence["local_clip_timing"]}
    roles={name:(g,c) for name,g,c in evidence["role_preview_clips"]}
    def qualified(role,start=0):
        pair=roles.get(role)
        meta=timing.get(slots.get(pair))
        return pair if (meta and meta["aobj_timing"]=="indexed" and not meta.get("strict_metadata_refusals") and
                        meta.get("animated_joint_count") and min(meta.get("authored_end_frames") or [0])>=start) else None
    additions=[]
    for key,role in (("takeoff-clip","ground-takeoff"),("landing-clip","landing")):
        if pair:=qualified(role):
            additions.append(f"{key}={pair[0]}:{pair[1]}")
    if b"\ndash=" in data and b"\ndash-policy=disabled\n" not in data:
        for key,role in (("dash-back","ground-dash-backward"),("dash-left","ground-dash-left"),("dash-right","ground-dash-right")):
            if pair:=qualified(role):
                additions.append(f"{key}={pair[0]}:{pair[1]}")
        starts=evidence.get("lab_air_dash_starts")
        start=starts[1] if starts else 0
        pair=qualified("air-dash-active",start)
        additions.append(f"air-dash={pair[0]}:{pair[1]}|{start}" if pair else "air-dash=none")
    return data+("".join(row+"\n" for row in additions)).encode("ascii")


def prepare(roster, executable, stage, output, *, practice=None, movement_sfx=None):
    roster, executable, stage = (Path(p).resolve(strict=True) for p in (roster,executable,stage))
    output = Path(output)
    repo = Path(__file__).resolve().parents[2]
    if output.resolve().is_relative_to(repo) and not output.resolve().is_relative_to(repo/"local"):
        raise ValueError("in-repository demo output must stay under local/")
    if output.exists() or output.is_symlink():
        raise FileExistsError("refusing to overwrite a demo install")
    if not executable.is_file() or not stage.is_dir():
        raise ValueError("provide the native executable and prepared Battle Test stage")
    bank = roster/"borgs"
    receipt=bank/"roster-extraction.json"
    if bank.is_symlink() or receipt.is_symlink() or not receipt.is_file():
        raise ValueError("expected an owned roster directory and ledger")
    with receipt.open("rb") as inp:
        raw = inp.read((1<<20)+1)
    if len(raw)>1<<20:
        raise ValueError("roster ledger size")
    ledger = json.loads(raw)
    if not isinstance(ledger,dict):
        raise ValueError("expected a roster ledger object")
    shots = ledger.get("authored_basic_shot_test_ids",[])
    melee = ledger.get("authored_basic_melee_test_ids",[])
    held = ledger.get("authored_held_fire_test_ids",[])
    if ledger.get("revision")!="GG4E08" or not all(isinstance(group,list) for group in (shots,melee,held)):
        raise ValueError("expected a qualified local roster ledger")
    ids = shots+melee+held
    if (not 1<=len(ids)<=206 or any(not isinstance(id,str) or not re.fullmatch(r"gg4e08:[0-9a-f]{4}",id) for id in ids) or
            len(set(ids))!=len(ids)):
        raise ValueError("playable roster ID extent or identity")
    files = []
    total = 0
    if practice is not None:
        practice=Path(practice).resolve(strict=True)
        if not {"gg4e08:0615","gg4e08:000a"}<=set(ids):
            raise ValueError("mechanics demo needs the G-Red and Sasuke slots")
    overrides={"gg4e08:0615","gg4e08:000a"} if practice is not None else set()
    for id in sorted(ids):
        if id in overrides:
            continue
        source = bank/id.replace(":","-")
        if not source.is_dir() or source.is_symlink():
            raise ValueError(f"not an owned package directory: {id}")
        for path in sorted(source.rglob("*")):
            if path.is_symlink() or not (path.is_dir() or path.is_file()):
                raise ValueError(f"{id}: linked or unsupported path")
            if path.is_file():
                size = path.stat().st_size
                total += size
                if size>32<<20 or total>1<<30 or len(files)>=20000:
                    raise ValueError("demo copy byte or file budget")
                files.append((path,Path("borgs")/path.relative_to(bank),size,id))
        if not (source/"gameplay.txt").is_file() or not (source/"borg.manifest").is_file():
            raise ValueError(f"{id}: missing playable package files")
    generate_sticks(output)
    generate_example("target",stage,output/"borgs/training-target")
    if practice is not None:
        generate_example("g-red",stage,output/"borgs/gg4e08-0615",practice=practice,battle_test=True,movement_sfx=movement_sfx)
        source_sasuke=bank/"gg4e08-000a"
        if sha256((source_sasuke/"visual/body.motions").read_bytes()).digest()!=sha256((stage/"sasuke.motions").read_bytes()).digest():
            raise ValueError("Sasuke movement bank differs from the indexed exact source")
        generate_example("sasuke",stage,output/"borgs/gg4e08-000a",battle_test=True,movement_sfx=movement_sfx,
                         gameplay_transform=lambda data:source_movement_rules(source_sasuke,"gg4e08:000a",data,generated=True))
        for name in sorted(CUSTOM_NAMES.keys()-{"target"}):
            generate_example(name,stage,output/"borgs"/name)
    for source,relative,size,id in files:
        dest = output/relative
        dest.parent.mkdir(parents=True,exist_ok=True)
        with source.open("rb") as inp:
            data = inp.read(size+1)
        if len(data)!=size:
            raise ValueError(f"package changed while copying: {relative}")
        if practice is not None and relative.name=="gameplay.txt":
            data=source_movement_rules(source.parent,id,data)
        with dest.open("xb") as out:
            out.write(data)
    default = "gg4e08:0615" if "gg4e08:0615" in ids else sorted(ids)[0]
    starter = '#!/bin/sh\nset -eu\ncd -- "$(dirname -- "$0")"\n'
    command = (f'exec {shlex.quote(str(executable))} --battle-borg "$borg" --against-borg test:target '
               f'--battle-mvp-assets {shlex.quote(str(stage))}')
    if practice is not None:
        command+=f' --source-ground-assets {shlex.quote(str(practice))}'
    with (output/"play.sh").open("x") as f:
        f.write(starter+f'borg=${{GF_DEMO_BORG:-{default}}}\n'+command+' --battle-picker "$@"\n')
    with (output/"show-attacks.sh").open("x") as f:
        f.write(starter+f'borg=${{1:-{default}}}\nif [ "$#" -gt 0 ]; then shift; fi\n'
                'case "$borg" in\n'
                +("  "+"|".join(melee)+') set -- --package-battle-distance 90 "$@" ;;\n' if melee else "")
                +'esac\n'+command+' --demo-package-battle --trace-package-battle "$@"\n')
    for file in ("play.sh","show-attacks.sh"):
        (output/file).chmod(0o755)
    return len(ids)+2+(len(CUSTOM_NAMES)-1 if practice is not None else 0)


if __name__=="__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("roster","executable","stage","output"):
        parser.add_argument(name,type=Path)
    parser.add_argument("--practice",type=Path,help="add checked G-Red/Sasuke mobility profiles and authored mechanics examples")
    parser.add_argument("--movement-sfx",type=Path,help="optional qualified flight/jump/landing PCM for G-Red and Sasuke")
    args = parser.parse_args()
    try:
        if args.movement_sfx is not None and args.practice is None:
            raise ValueError("--movement-sfx requires --practice")
        count = prepare(args.roster,args.executable,args.stage,args.output,practice=args.practice,movement_sfx=args.movement_sfx)
    except (OSError,ValueError) as error:
        parser.exit(2,f"interactive Battle Test: {error}\n")
    print(f"{count} selectable combatants. Start with {args.output/'play.sh'}")
