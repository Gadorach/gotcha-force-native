"""Exercise the opt-in authored retail-art pilot through the normal Battle Test loader."""
import argparse
from pathlib import Path
import re
from shutil import copytree
import subprocess
from tempfile import TemporaryDirectory

from tools.extract.portable_borg_preview import prepare


# Expected remaining stock, socket, and authored release marker.
CASES = {"0000":(9,7,6),"0002":(9,11,6),"0003":(9,7,6),"0007":(3,12,6),"0008":(9,7,6),
         "000a":(9,7,6),"000b":(9,11,6),"000c":(9,7,6),"0100":(19,7,6),"0103":(19,7,3),"0105":(4,7,6),
         "0505":(9,1,12),"0511":(9,1,12),
         "0600":(4,7,3),"0602":(9,7,6),"0606":(4,(28,27),6),"0608":(4,7,3),
         "060a":(9,7,6),"060c":(9,7,6),"060e":(9,7,6),
         "0610":(4,3,12),"0615":(4,7,12),"0616":(4,7,3),"061a":(9,(27,28),3),
         "061c":(4,7,3),"061e":(4,3,12),"0621":(4,3,12),"0623":(4,3,12),"0628":(9,27,12),"0629":(9,7,12),"062a":(4,7,12),
         "0800":(4,7,12),"0801":(4,7,12),"080c":(2,1,6),"0900":(9,7,12),"0901":(4,7,12),"0902":(9,7,12),
         "0903":(9,7,12),"0906":(4,7,12),"0907":(9,7,12),"0908":(9,7,12),"0909":(9,7,12),"090a":(9,7,12),
         "090c":(9,7,12),"090d":(9,7,12)}
CASES.update({"0604":(19,12,12),"0613":(19,12,12),"0618":(19,12,12),"0627":(19,12,12),
              "061b":(4,(28,27),12),"0620":(9,(25,27),12),
              "0d00":(None,13,8),"0d03":(9,(3,4),12),"0d04":(None,13,8),"0d05":(39,8,12),
              "0f00":(4,2,12),"0f01":(9,2,12),"0f02":(4,2,12)})
DRAGONS = ("0500","0509","050a","050c","0515","0516")
CASES.update({m:(4,12,12) for m in DRAGONS})
WORMS = ("0501","050b","050d","0517")
CASES.update({m:(4,12,12) for m in WORMS})
FOLLOWUP_COUNTS = {"0505":1,"0511":1,"0602":2,"0606":1,"0608":1,"060a":5,
                   "060c":4,"060e":3,"0604":1,"0613":1,"0618":1,"061a":1,"061b":1,"061c":1,"0620":1,"0627":1,"0628":1,"080c":1,"0d03":1}
UNADMITTED = ("010a","0104","0804","0d01","0d02","0f03","0f06")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable",type=Path)
    parser.add_argument("root",type=Path)
    parser.add_argument("stage",type=Path)
    parser.add_argument("--opengl",action="store_true")
    parser.add_argument("--launch-audio",action="store_true",help="require launch PCM playback traces")
    parser.add_argument("--require-audio-device",action="store_true",help="require device submission with --opengl --launch-audio")
    parser.add_argument("--impact-audio",action="store_true",help="verify Dragon impact sound opt-in")
    args=parser.parse_args()
    if args.require_audio_device and not (args.opengl and args.launch_audio):
        parser.error("--require-audio-device needs --opengl and --launch-audio")
    if args.impact_audio and not args.launch_audio:
        parser.error("--impact-audio needs --launch-audio")
    exe,root,stage=(p.resolve(strict=True) for p in (args.executable,args.root,args.stage))
    with TemporaryDirectory(prefix="gf-basic-shot-") as scratch:
        install=Path(scratch)
        prepare("target",stage,install/"borgs"/"training-target")
        for ident in (*CASES,*UNADMITTED):
            source=root/"borgs"/f"gg4e08-{ident}"
            copytree(source,install/"borgs"/source.name,symlinks=True)

        def run(ident,*extra):
            cmd=[exe,"--battle-borg",f"gg4e08:{ident}","--against-borg","test:target",
                 "--demo-package-battle","--trace-package-battle","--frames","150","--battle-mvp-assets",stage,*extra]
            if not args.opengl:
                cmd.append("--headless")
            return subprocess.run(cmd,cwd=install,text=True,capture_output=True,timeout=20)

        for ident,(remaining,socket,marker) in CASES.items():
            if ident in WORMS:
                package=install/"borgs"/f"gg4e08-{ident}"
                rules=(package/"gameplay.txt").read_text().splitlines()
                if ("move=basic-shot|b|10|grounded|28|3:0" not in rules or
                        sum(line.startswith(("move=","release=")) for line in rules)!=2):
                    raise ValueError(f"{ident} must keep only its exact-pose authored ordinary B")
                expected_tone="snd_com02-82" if ident in ("0501","050d") else "snd_com03-4"
                if args.launch_audio and f"launch-sound=shot|audio/launch-{expected_tone}.pcm" not in rules:
                    raise ValueError(f"{ident} does not use its own source family cue")
            got=run(ident)
            followups = FOLLOWUP_COUNTS.get(ident,0)
            count = 2*(1+followups)
            sockets = (socket,)*(1+followups) if isinstance(socket,int) else socket
            if got.returncode or f"actors=2 restarts=1 moved=1 airborne=1 shots={count} hits={count}" not in got.stderr:
                raise ValueError(got.stderr)
            cue_count=got.stderr.count("event=impact-sound ")
            if args.impact_audio and ident in DRAGONS:
                if cue_count!=2 or "borg-impact-audio: starts=2" not in got.stderr:
                    raise ValueError(got.stderr)
            elif cue_count:
                raise ValueError("unexpected authored impact cue")
            if args.launch_audio:
                if ident=="080c":
                    if "borg-launch-audio:" in got.stderr or "event=launch-sound " in got.stderr:
                        raise ValueError("the signed -1 child tones must not acquire a substitute")
                    if not re.search(r"borg-release-audio: starts=2 blocks=[1-9][0-9]* nonzero=1",got.stderr):
                        raise ValueError(got.stderr)
                elif (not re.search(rf"borg-launch-audio: starts={count} blocks=[1-9][0-9]* nonzero=1",got.stderr) or
                      got.stderr.count("event=launch-sound ")!=count):
                    raise ValueError(got.stderr)
                if args.require_audio_device and not re.search(r"borg-audio: device=open frames-accepted=[1-9][0-9]*",got.stderr):
                    raise ValueError(got.stderr)
            for rnd in (0,1):
                if ident in DRAGONS and not re.search(rf"projectile-record: round={rnd} tick=[0-9]+ draws=[1-9]",got.stderr):
                    raise ValueError(f"{ident} did not draw its exact carrier in round {rnd}: {got.stderr}")
                if ident in DRAGONS:
                    drew_trail=bool(re.search(rf"projectile-record-trail: round={rnd} tick=[0-9]+ draws=[1-9]",got.stderr))
                    if drew_trail!=(ident in ("0500","050c")):
                        raise ValueError(f"{ident} violated its exact trail policy in round {rnd}")
                    hit=re.search(rf"round={rnd} tick=(\d+) .*event=hit ",got.stderr)
                    visual_ticks={int(t) for t in re.findall(
                        rf"impact-record: round={rnd} tick=(\d+) draws=[1-9]\d*",got.stderr)}
                    if not hit or visual_ticks!=set(range(int(hit[1]),int(hit[1])+18)):
                        raise ValueError(f"{ident} did not draw all 18 impact ticks from contact in round {rnd}")
                lines=[s for s in got.stderr.splitlines() if f"round={rnd} " in s]
                order=["event=start ","event=marker "]
                if remaining is None:
                    if sum("event=start " in s for s in lines)!=1 or sum("event=button-release " in s for s in lines)!=1:
                        raise ValueError(got.stderr)
                    order.insert(0,"event=button-release ")
                else:
                    order.append(f"event=debit frame={marker} object=0 recipient=255 value={remaining}")
                order.extend((f"event=socket-launch frame={marker} object=1 recipient=255 value={sockets[0]}",
                              f"event=emit frame={marker} object=1 "))
                if args.launch_audio and ident=="080c":
                    order.insert(3,f"event=release-sound frame={marker} object=0 ")
                    if sum("event=release-sound " in line for line in lines)!=1:
                        raise ValueError(got.stderr)
                for obj in range(2,2+followups):
                    order.extend((f"event=socket-launch frame={marker} object={obj} recipient=255 value={sockets[obj-1]}",
                                  f"event=extra-emit frame={marker} object={obj} "))
                def find(event):
                    return next((i for i,s in enumerate(lines) if event in s),-1)
                indices=[find(event) for event in order]
                if (-1 in indices or indices!=sorted(set(indices)) or
                        sum("event=debit " in s for s in lines)!=(remaining is not None)):
                    raise ValueError(got.stderr)
                for obj in range(1,2+followups):
                    hit=find(f"event=hit frame=0 object={obj} recipient=1 value=8")
                    retire=find(f"event=retire frame=0 object={obj} ")
                    if not indices[-1]<hit<retire:
                        raise ValueError(got.stderr)
                    if args.impact_audio and ident in DRAGONS:
                        cue=find(f"event=impact-sound frame=0 object={obj} ")
                        if not hit<cue<retire:
                            raise ValueError(got.stderr)
        for ident in ("0610","0621"):
            profile=install/"borgs"/f"gg4e08-{ident}"/"gameplay.txt"
            text=profile.read_text()
            if text.count("body-record=0\n")!=1 or profile.is_symlink():
                raise ValueError("expected an explicit body scope in the copied profile")
            profile.write_text(text.replace("body-record=0\n",""))
            got=run(ident)
            if got.returncode!=2 or "shape/unknown POBJ type" not in got.stderr:
                raise ValueError(got.stderr)
        for ident in ("0606","061a"):
            profile=install/"borgs"/f"gg4e08-{ident}"/"gameplay.txt"
            text=profile.read_text()
            if text.count("idle-loop=authored-bit\n")!=1 or profile.is_symlink():
                raise ValueError("expected an explicit authored idle-loop binding")
            profile.write_text(text.replace("idle-loop=authored-bit\n",""))
            got=run(ident)
            if got.returncode!=2 or "unsupported AObj flags" not in got.stderr:
                raise ValueError(got.stderr)
        for ident in UNADMITTED:
            got=run(ident)
            if got.returncode!=2 or "no executable gameplay profile declared" not in got.stderr:
                raise ValueError(got.stderr)
        for ident in ("0505","0f01"):
            manifest=install/"borgs"/f"gg4e08-{ident}"/"borg.manifest"
            if manifest.is_symlink():
                raise ValueError("static body test needs its copied manifest")
            (manifest.parent/"visual"/"not-a-static-body.motions").write_bytes(b"presence only")
            manifest.write_text(manifest.read_text()+"animations=visual/not-a-static-body.motions\n")
            got=run(ident)
            if got.returncode!=2 or "static body must not declare a motion bank" not in got.stderr:
                raise ValueError(got.stderr)
        profile=install/"borgs"/"gg4e08-061b"/"gameplay.txt"
        text=profile.read_text()
        if profile.is_symlink() or text.count("cruise-entry=2:0\n")!=1:
            raise ValueError("expected the copied exact flight entry binding")
        profile.write_text(text.replace("cruise-entry=2:0\n","cruise-entry=15:0\n"))
        got=run("061b")
        if got.returncode!=2 or "exact clip required; no fallback" not in got.stderr:
            raise ValueError(got.stderr)
        for ident in ("061e","0501"):
            got=run(ident,"--borg-variant","source-1")
            if got.returncode!=2 or "no variant fallback" not in got.stderr:
                raise ValueError(got.stderr)
        if args.launch_audio:
            got=run("0608","--no-audio")
            if got.returncode or "borg-launch-audio: starts=0 blocks=0 nonzero=0" not in got.stderr:
                raise ValueError(got.stderr)
            got=run("080c","--no-audio")
            if got.returncode or "borg-release-audio: starts=0 blocks=0 nonzero=0" not in got.stderr:
                raise ValueError(got.stderr)
            for ident,binding in (("0608","launch"),("080c","release")):
                profile=install/"borgs"/f"gg4e08-{ident}"/"gameplay.txt"
                cue=next(line.rsplit("|",1)[1] for line in profile.read_text().splitlines()
                         if line.startswith(f"{binding}-sound="))
                sample=profile.parent/cue
                if not sample.is_file() or sample.is_symlink() or not sample.resolve().is_relative_to(install):
                    raise ValueError("audio test needs a local copied sample")
                sample.write_bytes(b"x")
                got=run(ident)
                if got.returncode!=2 or f"missing or unqualified {binding} sound" not in got.stderr:
                    raise ValueError(got.stderr)
        if args.impact_audio:
            got=run("0500","--no-audio")
            if got.returncode or "borg-impact-audio: starts=0" not in got.stderr or got.stderr.count("event=impact-sound ")!=2:
                raise ValueError(got.stderr)
            sample=install/"borgs/gg4e08-0500/audio/impact-snd_com01-12.pcm"
            if not sample.is_file() or sample.is_symlink():
                raise ValueError("impact cue fixture is not a copied sample")
            sample.write_bytes(b"x")
            got=run("0500")
            if got.returncode!=2 or "missing or unqualified impact sound" not in got.stderr:
                raise ValueError(got.stderr)
    print(f"{len(CASES)} exact-normal authored B-shot Battle Tests passed; archive-wide shape, unadmitted Borgs and alternate refused")


if __name__=="__main__":
    main()
