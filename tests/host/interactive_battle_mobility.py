"""Verify mobility and exact package admission in the local mechanics demo."""
import argparse
import json
from pathlib import Path
import re
import shlex
import subprocess


def check(root, opengl, samurai=False, gatling=False, complete_samurai=False, ninja=False, require_body=False):
    def run(script, *args, expected=0):
        cmd=[str(root/script),*args,"--no-audio"]
        if not opengl:
            cmd.append("--headless")
        got=subprocess.run(cmd,cwd=root,text=True,capture_output=True,timeout=35)
        if got.returncode!=expected:
            raise ValueError(got.stderr)
        return got.stderr

    source=run("play.sh","--demo-battle-pick","gg4e08:000a","--frames","48","--trace-package-battle")
    rows=re.findall(r"battle-picker: row=\d+ id=(\S+)",source)
    if ("active id=gg4e08:000a" not in source or rows[-2:]!=["example:flight-stick","example:jump-stick"] or
            not {"test:rotary-scout","test:remote-scout","test:wire-trail-scout","gg4e08:020f"}<=set(rows)):
        raise ValueError(source)
    sasuke=run("show-attacks.sh","gg4e08:000a","--exercise-battle-mobility")
    if not re.search(r"strafed=1 ground-dashes=[1-9]\d* air-dashes=[1-9]\d* jump-points-used=3",sasuke):
        raise ValueError(sasuke)
    jet=run("show-attacks.sh","gg4e08:061b","--exercise-battle-mobility")
    if not re.search(r"boost-frames=[1-9]\d*",jet):
        raise ValueError(jet)
    gred=run("show-attacks.sh","gg4e08:0615")
    if "move=beam-test event=hit " not in gred or "airborne=1" not in gred:
        raise ValueError(gred)
    touchdown=run("show-attacks.sh","gg4e08:0615","--exercise-gred-touchdown","--frames","180")
    if "gred-touchdown: attempt=1 entry=1 fresh-jump=1" not in touchdown or "refused" in touchdown:
        raise ValueError(touchdown)
    for id,pace in (("000a","5.00"),("0501","0.83"),("050d","0.42")):
        profile=(root/f"borgs/gg4e08-{id}/gameplay.txt").read_text()
        if f"movement=grounded-v1|{pace}|" not in profile:
            raise ValueError(f"{id} lacks its source-relative test pace")
    if samurai:
        ids=("0700","0701","0702","0704","0707","0708","0709","070a")
        if complete_samurai:
            ids+=("0703","0706")
        unadmitted=("0705","070b","070c","070d") if complete_samurai else ("0703","0705","0706","070b","070c","070d")
        if not {f"gg4e08:{id}" for id in ids}<=set(rows) or any(f"gg4e08:{id}" in rows for id in
                unadmitted):
            raise ValueError("Samurai dropdown includes an unadmitted route or excludes an exact test")
        for id in ids:
            package=root/f"borgs/gg4e08-{id}"
            rules=(package/"gameplay.txt").read_text()
            if id=="0706":
                seed=json.loads((package/"source-index.json").read_bytes())["samurai_close_b_source"]["neutral_program_seed"]
                if (seed["guard"],seed["value"],seed["excluded_positive_branch"]["value"])!=("signed actor +6F6 <= 0",0,6):
                    raise ValueError("Beam Samurai must not borrow the charged pose branch")
            trace=run("show-attacks.sh",f"gg4e08:{id}")
            if ("move=opening-slash|b|10|grounded|24|1:1\n" not in rules or "shots=0 hits=2" not in trace or
                    trace.count("event=melee-armed ")!=2 or "event=debit " in trace):
                raise ValueError(trace)
            for rnd in (0,1):
                hit=re.search(rf"round={rnd} tick=(\d+) actor=0 .*event=hit ",trace)
                if not hit or not re.search(rf"borg-parts: round={rnd} tick={hit[1]} actor=0 draws=[1-9]",trace):
                    raise ValueError("Samurai hand records did not render at contact\n"+trace)
    if ninja:
        for id in ("0005","0009"):
            stable=f"gg4e08:{id}"
            package=root/f"borgs/gg4e08-{id}"
            rules=(package/"gameplay.txt").read_text().splitlines()
            if stable not in rows or [line for line in rules if line.startswith("move=")]!=[
                    "move=opening-slash|b|10|grounded|24|1:1"]:
                raise ValueError(f"{stable} must have only the authored close-B move")
            picked=run("play.sh","--demo-battle-pick",stable,"--frames","48")
            if f"battle-picker: active id={stable} variant=normal (new round)" not in picked:
                raise ValueError(picked)
            info=json.loads((package/"source-index.json").read_bytes())
            clips={c["id"]:c["clip"] for c in info["action_preview_clips"]}
            evidence=info["ninja_close_b_source"]
            if (clips.get("b-close-c0-s0"),clips.get("b-close-c2-s0"),evidence["neutral_program_seed"]["value"])!=(
                    [1,1],[1,6],1):
                raise ValueError(f"{stable} lost its exact neutral or moving close-B mapping")
            trace=run("show-attacks.sh",stable)
            if ("shots=0 hits=2" not in trace or trace.count("event=melee-armed ")!=2 or
                    "event=debit " in trace or trace.count("event=start ")!=2):
                raise ValueError(trace)
            for rnd in (0,1):
                hit=re.search(rf"round={rnd} tick=(\d+) actor=0 .*event=hit ",trace)
                if not hit or not re.search(rf"borg-parts: round={rnd} tick={hit[1]} actor=0 draws=2\b",trace):
                    raise ValueError(trace)
    if gatling:
        trace=run("show-attacks.sh","gg4e08:0102")
        rows0=[row for row in trace.splitlines() if "round=0" in row and "move=gatling-b" in row]
        fired=[int(re.search(r"tick=(\d+)",row)[1]) for row in rows0 if "event=emit " in row]
        intervals=[b-a for a,b in zip(fired,fired[1:]) if b-a<=8]
        if ("gg4e08:0102" not in rows or len(fired)<5 or not intervals or min(intervals)>=max(intervals) or
                sum("event=debit " in row for row in rows0)!=len(fired) or not any("event=held-fire-stop " in row for row in rows0)):
            raise ValueError(trace)
    # Verify exact model/clip admission for every declared dropdown row.
    command=shlex.split((root/"play.sh").read_text().splitlines()[-1])
    executable=command[1]
    stage=command[command.index("--battle-mvp-assets")+1]
    practice=command[command.index("--source-ground-assets")+1]
    for id in rows:
        result=subprocess.run([executable,*([] if opengl else ["--headless"]),"--frames","2","--no-audio","--battle-borg",id,
             "--against-borg","test:target","--battle-mvp-assets",stage,"--source-ground-assets",practice,
             *(["--trace-package-battle"] if require_body else [])],
             cwd=root,text=True,capture_output=True,timeout=15)
        if result.returncode:
            raise ValueError(id+"\n"+result.stderr)
        if require_body:
            counts=re.findall(r"borg-body: round=0 tick=\d+ actor=0 draws=(\d+)",result.stderr)
            if len(counts)!=2 or any(int(n)==0 for n in counts):
                raise ValueError(f"{id} entered the round without a visible body\n"+result.stderr)
    scope=" and live body checks" if require_body else ""
    print(f"{len(rows)} exact admissions{scope}; G-Red flight, Sasuke jumps/strafe/dashes, cruise boost and source-relative paces passed")


if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("root",type=Path)
    p.add_argument("--opengl",action="store_true")
    p.add_argument("--samurai",action="store_true")
    p.add_argument("--gatling",action="store_true")
    p.add_argument("--complete-samurai",action="store_true",help="verify the two additional armed Samurai routes")
    p.add_argument("--ninja",action="store_true",help="verify the scoped Teleport and Switching close-B pilots")
    p.add_argument("--require-body",action="store_true",help="also require live body submissions for every dropdown actor")
    args=p.parse_args()
    check(args.root.resolve(strict=True),args.opengl,args.samurai or args.complete_samurai,args.gatling,args.complete_samurai,args.ninja,
          args.require_body)
