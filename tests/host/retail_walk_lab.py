"""Run the native no-combat walk lab for each extracted ground-travel binding."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
from shutil import copytree
import subprocess
from tempfile import TemporaryDirectory


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("root", type=Path)
    parser.add_argument("stage", type=Path, help="local native stage.hsd/floor.stih directory")
    parser.add_argument("--jump", action="store_true", help="exercise one grounded jump and a refused airborne press")
    parser.add_argument("--dash", action="store_true", help="exercise four directional ground dashes")
    parser.add_argument("--air-dash", action="store_true", help="exercise wind-up, burst, recovery and a refused second airborne dash")
    parser.add_argument("--glide", action="store_true", help="exercise held descent, release and landing")
    parser.add_argument("--cruise", action="store_true", help="exercise flight, boost depletion and airborne refill")
    parser.add_argument("--source-pace", action="store_true", help="verify the opt-in source-relative grounded target")
    parser.add_argument("--landing-audio", action="store_true", help="verify and mix exact roster landing PCM")
    args = parser.parse_args()
    if (args.landing_audio or args.air_dash or args.glide) and not args.jump:
        parser.error("--landing-audio, --air-dash and --glide require --jump")
    if args.glide and args.air_dash:
        parser.error("--glide and --air-dash are separate lab policies")
    if args.cruise and (args.jump or args.dash):
        parser.error("--cruise does not combine with jump or dash")
    if args.source_pace and (args.jump or args.dash or args.cruise):
        parser.error("--source-pace smoke measures uninterrupted ground travel")
    executable, stage = args.executable.resolve(strict=True), args.stage.resolve(strict=True)
    root = args.root.resolve(strict=True)
    profiles = {}
    cruise_ids = set()
    cruise_entries = {}
    air_dash_starts = {}
    cruise_alternate = {}
    source_paces = {}
    pace_alternate = {}
    audio = {}
    tone_counts=Counter()
    for path in sorted((root/"borgs").glob("gg4e08-*/source-index.json")):
        with path.open("rb") as stream:
            data=stream.read((2<<20)+1)
        if len(data)>2<<20:
            raise ValueError("source index byte budget")
        index=json.loads(data)
        ident=index["id"]
        if not isinstance(ident,str) or not re.fullmatch(r"gg4e08:[0-9a-f]{4}",ident) or ident in profiles:
            raise ValueError("unexpected source roster identity")
        profiles[ident]={row[0] for row in index["role_preview_clips"]}
        if (starts:=index.get("lab_air_dash_starts")):
            if len(starts)!=3 or not all(type(n) is int and 0<=n<=600 for n in starts) or not any(starts):
                raise ValueError(f"invalid air dash starts for {ident}")
            air_dash_starts[ident]=starts
        if args.source_pace and index.get("lab_ground_pace"):
            pace=index["lab_ground_pace"]
            operands=index["movement_operands"]
            selected=operands["class_installer_binding"]["selected_record"]
            if (not isinstance(selected,int) or selected!=pace["record"] or pace["retail_speed_verified"] or
                operands["records"][selected]["record_sha256"]!=pace["record_sha256"] or
                operands["records"][selected]["operands"]["02C"]["bits"]!=pace["source_bits"]):
                raise ValueError(f"source pace does not use the selected record for {ident}")
            source_paces[ident]=pace["native_units_per_tick_x100"]
            if len(index["selectable_variants"])>1:
                pace_alternate[ident]=index["selectable_variants"][1]
        if index.get("cruise_lab"):
            cruise=index["cruise_lab"]
            entry=cruise.get("entry_clip")
            if (cruise["source_type"]!=3 or cruise["retail_control_verified"] or
                    cruise["static_body"]!=(entry is None)):
                raise ValueError(f"unqualified cruise evaluation for {ident}")
            if entry is not None:
                g,c=entry
                if not any((g,c)==tuple(slot[:2]) for slot in index["raw_animation_slots"]):
                    raise ValueError(f"cruise entry is not a local slot for {ident}")
                cruise_entries[ident]=entry
            cruise_ids.add(ident)
            if len(index["selectable_variants"])>1:
                cruise_alternate[ident]=index["selectable_variants"][1]
        if args.landing_audio:
            landing=index["sfx_bindings"]["landing_candidate"]
            relative=landing.get("lab_pcm")
            if relative is None:
                continue
            if relative!="audio/lab-landing.pcm" or landing["trigger_qualified"]:
                raise ValueError(f"unexpected landing audio declaration for {ident}")
            with (path.parent/relative).open("rb") as stream:
                pcm=stream.read(48000*4*3+1)
            if not 0<len(pcm)<=48000*4*3 or len(pcm)%4 or sha256(pcm).hexdigest()!=index["asset_sha256"][relative]:
                raise ValueError(f"audio bytes differ from extracted package receipt for {ident}")
            audio[ident]=(landing["tone"],len(pcm)//4,sha256(pcm).hexdigest())
    ground_mapped=sum("ground-travel" in roles for roles in profiles.values())
    if len(profiles)!=206 or not 0<ground_mapped<len(profiles):
        raise ValueError("need complete --role-clips roster and an unmapped control")

    frames=216 if args.dash else 90 if args.jump or args.cruise else 24
    def run(ident,variant="normal",*,install=root):
        cmd=[executable,"--demo-borg-controls","--walk-borg",ident,"--borg-variant",variant,
             "--battle-mvp-assets",stage,"--headless","--frames",str(frames)]
        if args.jump:
            cmd.append("--lab-jump")
        if args.dash:
            cmd.append("--lab-dash")
        if args.air_dash:
            cmd.append("--lab-air-dash")
        if args.glide:
            cmd.append("--lab-glide")
        if args.landing_audio:
            cmd.append("--lab-landing-audio")
        if args.cruise:
            cmd.append("--lab-cruise")
        if args.source_pace:
            cmd.append("--lab-source-pace")
        return subprocess.run(cmd,cwd=install,text=True,capture_output=True,timeout=15)

    if args.source_pace:
        if len(set(source_paces.values()))<2:
            raise ValueError("need --source-pace extraction with multiple pace values")
        for ident,target in sorted(source_paces.items()):
            if not isinstance(target,int) or not 42<=target<=1042:
                raise ValueError(f"pace outside bounds for {ident}")
            result=run(ident)
            if result.returncode or f"ground-pace target={target} measured_peak={target} provisional" not in result.stderr:
                raise ValueError(f"unexpected ground pace for {ident}: {result.stderr}")
        unbound=next(ident for ident,roles in profiles.items() if "ground-travel" in roles and ident not in source_paces)
        denied=run(unbound)
        if denied.returncode!=2 or "ground pace mapping refused" not in denied.stderr:
            raise ValueError("runtime supplied a missing pace mapping")
        if pace_alternate:
            ident=min(pace_alternate)
            denied=run(ident,pace_alternate[ident])
            if denied.returncode!=2 or "movement mapping refused" not in denied.stderr:
                raise ValueError("alternate inherited a normal pace or motion binding")
        print(json.dumps({"mapped":len(source_paces),"provisional_pace_passed":len(source_paces),
                          "distinct_targets":len(set(source_paces.values())),"unbound_refused":True,
                          "alternate_refused":bool(pace_alternate)},indent=2))
        return

    if args.cruise:
        if not cruise_ids:
            raise ValueError("need --cruise-lab extraction")
        for ident in sorted(cruise_ids):
            result=run(ident)
            if result.returncode or "actors=1 moved=1 walk=0 shots=0 hits=0" not in result.stderr or not re.search(
                    r"cruise-lab boost_entries=1 depleted=1 speed=1 airborne_refill_ticks=[1-9][0-9]* full=1 poses=1",
                    result.stdout+result.stderr) or "controls resets=1 paused_frames=4" not in result.stderr:
                raise ValueError(f"unexpected cruise result for {ident}: {result.stderr}")
        denied=run(next(ident for ident in profiles if ident not in cruise_ids))
        if denied.returncode!=2 or "cruise mapping refused" not in denied.stderr:
            raise ValueError("unmapped body accepted by cruise lab")
        for animated in (False,True):
            variants=[i for i in cruise_alternate if (i in cruise_entries)==animated]
            if variants:
                ident=min(variants)
                denied=run(ident,cruise_alternate[ident])
                if denied.returncode!=2 or "cruise mapping refused" not in denied.stderr:
                    raise ValueError("source variant inherited normal cruise binding")
        print(json.dumps({"mapped":len(cruise_ids),"provisional_cruise_passed":len(cruise_ids),
                          "animated_entries":cruise_entries,
                          "unmapped_look_refused":True,"alternate_refused":bool(cruise_alternate)},indent=2))
        return

    required={"ground-travel"}
    if args.jump:
        required.update(("ground-takeoff","air-continuation","landing"))
    dash_roles={f"ground-dash-{d}" for d in ("forward","left","backward","right")}
    if args.dash:
        required.update(dash_roles)
    air_dash_roles={"air-dash-wind-up","air-dash-active","air-dash-recovery"}
    if args.air_dash:
        required.update(air_dash_roles)
    glide_roles={"glide-entry","glide-held"}
    if args.glide:
        required.update(glide_roles)
    eligible=[ident for ident,roles in profiles.items() if required<=roles]
    if args.landing_audio:
        if any(ident not in audio for ident in eligible):
            raise ValueError("fresh --landing-pcm extraction required for all mapped looks")
        by_tone={}
        for tone,frames_for_sample,digest in audio.values():
            row=frames_for_sample,digest
            if by_tone.setdefault(tone,row)!=row:
                raise ValueError("one source tone produced inconsistent PCM")
        if set(by_tone)!={15,18,22} or len(set(by_tone.values()))!=3:
            raise ValueError("need three distinct source landing samples")
    refused=[]
    for ident in eligible:
        result=run(ident)
        if result.returncode:
            known="[error] walk-lab: walk-lab assets refused: hsd bone motion: AnimJoint/constraints/flags"
            if args.dash and ident=="gg4e08:0104" and result.returncode==2 and known in result.stderr.splitlines():
                refused.append(ident)
                continue
            raise ValueError(f"unexpected walk lab failure: {ident}: {result.stderr}")
        text=result.stdout+result.stderr
        if f"id={ident} variant=normal frames={frames} actors=1 moved=1 walk=1 shots=0 hits=0" not in text:
            raise ValueError(f"missing movement transaction result for {ident}")
        if args.jump and ("jump-lab takeoffs=1 landings=1 grounded=1 poses=1 ignored_air_presses=1" not in text or
                          "controls resets=1 paused_frames=4" not in text):
            raise ValueError(f"missing complete jump result for {ident}")
        if args.dash and (f"dash-lab starts=4 poses=1 ignored_air_presses={int(args.jump and not args.air_dash)}" not in text or
                          "controls resets=1 paused_frames=4" not in text):
            raise ValueError(f"missing complete dash result for {ident}")
        if args.air_dash and "air-dash-lab bursts=1 poses=1 spent_press_refusals=1" not in text:
            raise ValueError(f"missing complete airborne dash result for {ident}")
        if args.glide and not re.search(r"glide-lab entries=1 releases=1 poses=1 steps=[1-9][0-9]*",text):
            raise ValueError(f"missing complete glide result for {ident}")
        if args.landing_audio:
            tone,frames_for_sample,_=audio[ident]
            if not re.search(rf"landing-audio cues=1 blocks=[1-9][0-9]* nonzero=1 sample_frames={frames_for_sample} device=off accepted_frames=0",text):
                raise ValueError(f"missing single accepted landing mix for {ident}")
            tone_counts[tone]+=1
    missing=run(next(ident for ident,roles in profiles.items() if "ground-travel" not in roles))
    if args.jump and not args.landing_audio:
        jump_roles={"ground-takeoff","air-continuation","landing"}
        incomplete=next((ident for ident,roles in profiles.items() if "ground-travel" in roles and not jump_roles<=roles),None)
        if incomplete:
            walk_only=run(incomplete)
        else:
            # All mapped walkers can now jump. Check a copied look with the
            # takeoff declaration removed, leaving the exact walk binding.
            ident=eligible[0]
            folder=ident.replace(":","-")
            with TemporaryDirectory(prefix="gf-incomplete-jump-") as scratch:
                install=Path(scratch)
                package=install/"borgs"/folder
                copytree(root/"borgs"/folder,package,symlinks=True)
                preview=package/"preview.txt"
                if preview.is_symlink() or not preview.is_file():
                    raise ValueError("need a copied regular preview")
                lines=preview.read_text().splitlines(keepends=True)
                keep=[line for line in lines if not line.startswith("candidate=ground-takeoff|")]
                if len(keep)!=len(lines)-1:
                    raise ValueError("expected one exact takeoff declaration")
                preview.write_text("".join(keep))
                walk_only=run(ident,install=install)
        if walk_only.returncode!=2 or "jump mapping refused" not in walk_only.stderr:
            raise ValueError("jump lab must refuse an incomplete jump map")
    if args.dash and not args.landing_audio:
        no_dash=run(next(ident for ident,roles in profiles.items()
                         if required-dash_roles<=roles and not dash_roles<=roles))
        if no_dash.returncode!=2 or "dash mapping refused" not in no_dash.stderr:
            raise ValueError("dash lab must refuse an incomplete dash map")
    if args.air_dash:
        incomplete=next((ident for ident,roles in profiles.items()
                         if required-air_dash_roles<=roles and not air_dash_roles<=roles),None)
        if incomplete:
            no_air_dash=run(incomplete)
        else:
            # A frame-zero package remains a valid ordinary jump look after
            # removing a dash candidate, so this checks lab admission.
            ident=next(i for i in eligible if i not in air_dash_starts)
            folder=ident.replace(":","-")
            with TemporaryDirectory(prefix="gf-incomplete-air-dash-") as scratch:
                install=Path(scratch)
                package=install/"borgs"/folder
                copytree(root/"borgs"/folder,package,symlinks=True)
                preview=package/"preview.txt"
                if preview.is_symlink() or not preview.is_file():
                    raise ValueError("need a copied regular preview")
                lines=preview.read_text().splitlines(keepends=True)
                keep=[line for line in lines if not line.startswith("candidate=air-dash-active|")]
                if len(keep)!=len(lines)-1:
                    raise ValueError("expected one exact air dash active declaration")
                preview.write_text("".join(keep))
                no_air_dash=run(ident,install=install)
        if no_air_dash.returncode!=2 or "air dash mapping refused" not in no_air_dash.stderr:
            raise ValueError("air dash lab must refuse an incomplete three-phase map")
        if with_starts:=set(eligible)&air_dash_starts.keys():
            ident=min(with_starts)
            folder=ident.replace(":","-")
            with TemporaryDirectory(prefix="gf-invalid-dash-start-") as scratch:
                install=Path(scratch)
                package=install/"borgs"/folder
                copytree(root/"borgs"/folder,package,symlinks=True)
                preview=package/"preview.txt"
                if preview.is_symlink() or not preview.is_file():
                    raise ValueError("need a copied regular preview")
                text=preview.read_text()
                row="lab-air-dash-starts="+"|".join(map(str,air_dash_starts[ident]))
                if text.count(row+"\n")!=1:
                    raise ValueError("expected exact air dash start declaration")
                preview.write_text(text.replace(row+"\n","lab-air-dash-starts=600|600|600\n"))
                bad_start=run(ident,install=install)
                if bad_start.returncode!=2 or "declared start outside exact clip" not in bad_start.stderr:
                    raise ValueError("air dash lab must refuse out-of-clip sampling")
    if args.glide:
        no_glide=run(next(ident for ident,roles in profiles.items() if required-glide_roles<=roles and not glide_roles<=roles))
        if no_glide.returncode!=2 or "glide mapping refused" not in no_glide.stderr:
            raise ValueError("glide lab must refuse an incomplete glide map")
    passed=len(eligible)-len(refused)
    if not passed:
        raise ValueError("no mapped packages passed the lab")
    unknown=run(eligible[0],"undeclared-look")
    expected_missing="missing exact lab landing PCM" if args.landing_audio else "movement mapping refused"
    if missing.returncode!=2 or expected_missing not in missing.stderr \
       or unknown.returncode!=2 or "undeclared preview variant" not in unknown.stderr:
        raise ValueError("missing role, exact-variant refusal, or positive walk check failed")
    print(json.dumps({"mapped":len(eligible),
                      "provisional_glide_passed" if args.glide else "provisional_air_dash_passed" if args.air_dash
                      else "provisional_dash_passed" if args.dash else "provisional_jump_passed" if args.jump
                      else "provisional_walk_passed":passed,
                      "unsupported_dash_animjoint_constraints":refused,
                       **({"nonzero_source_start_maps":len(set(eligible)&air_dash_starts.keys())} if args.air_dash else {}),
                      **({"landing_audio_mixed_by_source_tone":dict(sorted(tone_counts.items()))} if args.landing_audio else {}),
                      "unmapped_look_refused":True,"undeclared_variant_refused":True},indent=2))


if __name__=="__main__":
    main()
