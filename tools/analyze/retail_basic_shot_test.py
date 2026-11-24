"""Opt-in authored Battle Test profile from reviewed normal assets and operands."""
import struct
from tools.analyze import retail_dragon_shot as dragon

from tools.analyze.retail_release_phases import STATIC_GROUND_PHASE_PAIRS, VIOLET_SHOT
from tools.analyze.retail_seeded_shot_pose import (
    ANIMATED_CRUISE_SHOTS, CLOCKED_GROUND_DIRECT_ENTRIES, CLOCKED_GROUND_SHOTS, CYBER_MACHINE_SHOTS,
    DOUBLE_NINJA, DOUBLE_NINJA_PRIMARY_SITE, EAGLE_FAMILIES, ORDERED_SHOTS,
    RIFLE_KEYS, RIFLE_PAIRS, STATIC_CRUISE_SHOTS, THROW_KEYS,
)
from tools.analyze.retail_worm_shot import MODELS as WORM_MODELS


# Only these original models passed strict native body, clip and socket
# admission for the pilot. Other models must not inherit the profile.
TYPE_2_KEYS = {0x0007:1, 0x0610:53, 0x061E:85, 0x0621:86, 0x0623:87, **RIFLE_KEYS}
TYPE_0_KEYS = {0x0100:0, 0x0103:38, 0x0105:56, 0x0800:12, 0x0801:125, 0x0900:24, 0x0901:101, 0x0902:102, 0x0903:104, 0x0906:106,
               0x0907:108, 0x0908:99, 0x0909:103, 0x090A:105, 0x090C:109, 0x090D:100}
COHORT = TYPE_2_KEYS.keys()|TYPE_0_KEYS.keys()|ORDERED_SHOTS.keys()|THROW_KEYS.keys()
RECORD_ZERO = TYPE_0_KEYS.keys()|RIFLE_KEYS.keys()|ORDERED_SHOTS.keys()|THROW_KEYS.keys()|{0x0007,0x0610,0x0621}
PREFERRED_B_SHOT = {0x0007:(4,2),0x080C:(4,0),0x0906:(4,2)}
ACTOR_RELEASE_SOUND = (0x080C,162,"8010FF18")
CONTEXT_FIELDS = ("config","context_table","main_handler","state","substate","selector")
CRUISE_CHARGE_SHOTS = {0x0D00:(20,"800F80CC"),0x0D04:(56,"800F80D8")}
# The single shot has no model check in its leaf. Require its reviewed
# B-shot phase table and entry, not merely a matching projectile key.
# (factory key/site, phase table and entry, debit site, selector, stock mode)
STATIC_CRUISE_PHASE_SHOTS = {0x0F00:(60,0x801B2AFC,"80380A80","801B2AC8","801B2AE8",45,1)}
# (family, factory key/site, phase table and entry, debit, stock mode)
STATIC_GROUND_PHASE_SHOTS = {
    0x0F01:("type-0",74,0x801B38B0,"80380EE8","801B3824","801B389C",0),
    0x0F02:("type-2",61,0x801B394C,"80380EE8","801B3824","801B392C",1),
}
STATIC_GROUND_MODELS = STATIC_GROUND_PHASE_SHOTS.keys()|STATIC_GROUND_PHASE_PAIRS.keys()


def shot_factory_keys(cruise=False):
    return ({("type-0",key) for key in TYPE_0_KEYS.values()}|
            {("type-2",key) for key in TYPE_2_KEYS.values()}|
            {(EAGLE_FAMILIES.get(model,"type-2"),key) for model,(keys,_) in ORDERED_SHOTS.items() for key in keys}|
            {("type-14",key) for key in THROW_KEYS.values()}|
            {("type-47",key) for key in dragon.DRAGON_KEYS.values()}|
            {("type-0",key) for keys,_ in CLOCKED_GROUND_SHOTS.values() for key in keys}|
            {(row[0],row[1]) for row in STATIC_GROUND_PHASE_SHOTS.values()}|
            {("type-2",key) for _,_,keys,_ in STATIC_GROUND_PHASE_PAIRS.values() for key in keys}|
            {("type-2",key) for keys,_ in STATIC_CRUISE_SHOTS.values() for key in keys if cruise}|
            {("type-2",route[0]) for route in STATIC_CRUISE_PHASE_SHOTS.values() if cruise}|
            ({("type-0",VIOLET_SHOT["keys"][0])} if cruise else set())|
            {(family,key) for _,family,keys,_ in ANIMATED_CRUISE_SHOTS.values() for key in keys if cruise}|
            {("type-2",key) for key,_ in CRUISE_CHARGE_SHOTS.values() if cruise})


def pilot_launch_tone(candidate):
    status = candidate.get("status")
    if status=="outside-qualified-normal-bank-domain" and candidate.get("source_sound_id")==-1:
        return None
    if status!="static-tone-record":
        raise ValueError("basic shot test lacks an exact launch tone or signed -1 operand")
    return candidate["bank"],candidate["tone"]


def whole_body_pose(joints):
    """GFCP single authored lane; no source lane ownership or mirroring."""
    n = len(joints)
    if not 2<=n<=32 or [j["index"] for j in joints]!=list(range(n)):
        raise ValueError("basic shot test rig extent or logical order")
    offsets = [j["descriptor_offset"] for j in joints]
    if len(set(offsets))!=n or any(at<0 or at>0xFFFFFFFF or at%4 for at in offsets):
        raise ValueError("basic shot test joint identity")
    identities = (32+2*n+3)&~3
    return (struct.pack(">IHHBBBBIIIII",0x47464350,1,n,1,n,n,n,32,32+n,identities,
                        identities+4*n,offsets[0])+bytes(range(n))*2+bytes(identities-32-2*n)+
                        struct.pack(f">{n}I",*offsets))


def dragon_battle_test(model,joints,roles,mapping,actions,pace,level,releases,carrier,*,launch_audio=False):
    """Authored grounded projectile test, not the elemental carrier lifecycle."""
    key=dragon.DRAGON_KEYS[model]
    clips={name:(g,c) for name,g,c in roles}
    if not pace or any(k not in clips for k in ("ground-travel","air-continuation")) or any(
            actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")):
        raise ValueError("elemental pilot lacks exact movement evidence")
    idles=set()
    for p in mapping["profiles"]:
        if p["movement_type"]!=4:
            raise ValueError("elemental pilot needs the scoped type-4 map")
        candidates=[b for r in p["roles"] if r["selector"]==0 and r["states"]==[1,1] and
                    r["handler_addresses"]==["8005F23C","8005F2A4"] for q in r["program_candidates"]
                    if (q["site"],q["lanes"],q["program_group"],q["program_choice"])==("8005F2F8",2,0,0)
                    for c in q["clip_candidates"] if c["preceding_waits"]==0 and c["start_frame"]==0
                    for b in c["lanes"] if b["lane"]==1 and b["bank"]=="local" and b["archive_offset"] is not None]
        if len(candidates)!=1:
            raise ValueError("elemental pilot lacks the exact frame-zero idle")
        b=candidates[0]
        idles.add((b["group"],b["choice"],b["archive_offset"]))
    if len(idles)!=1:
        raise ValueError("elemental pilot idle differs across profiles")
    g,c,_=idles.pop()
    contexts=[]
    poses=set()
    for p in actions["profiles"]:
        rows=[r for group in p["groups"] if group["group"]=="B-shot" for r in group["rows"]
              if (0,0) in map(tuple,r["contexts"]) and r.get("release_pose_candidate")]
        if len(rows)!=1:
            raise ValueError("elemental pilot lacks the later release pose")
        row=rows[0]
        pose=row["release_pose_candidate"]
        if pose["phase_table"]!="802D56EC" or pose["release_site"]!="80075CA4":
            raise ValueError("elemental pilot phase scope")
        poses.add((tuple(pose["clip"]),pose["archive_offset"]))
        contexts.append({k:(p if k in ("config","context_table","main_handler") else row)[k] for k in CONTEXT_FIELDS}
                        |{"phase_table":"802D56EC","release_entry":"80075BF8","release_site":"80075CA4"})
    if len(poses)!=1:
        raise ValueError("elemental pilot release poses disagree")
    ((action_group,action_choice),_)=poses.pop()
    (hit,),debit=match_shot_context({"raw_context":[0,0],"routes":contexts},releases)
    if ((hit["family"],hit["key"],hit["basis"],hit["source_joint_selector"],tuple(hit["local_point"]))!=
            ("type-47",key,"parent-joint",12,(0,0,0)) or not hit["joint_present_in_normal_rig"] or
            hit.get("required_model_at_call")!=f"{model:04X}" or
            (debit["debit_site"],debit["channel"],debit["cost"],debit["commit"],debit["same_input_actor"])!=
            ("80075C38",0,1,True,True)):
        raise ValueError("elemental pilot release, socket or stock proof mismatch")
    art=carrier["visuals"]
    if art["owner"]!="caster-selected-wrapper" or art["separate_record"]["key"]!=8 or 8 not in art["records"]:
        raise ValueError("elemental pilot lacks exact carrier record")
    trail=art["repeated_record"]
    if trail["key"]!=9 or trail["count"]!=5 or 9 not in art["records"]:
        raise ValueError("elemental pilot lacks exact trail record")
    trail_drawable=not art["records"][9]["empty_static"]
    hp,initial,limit,mode,_=level[:5]
    if not 1<=initial==limit<=255 or mode!=1:
        raise ValueError("elemental pilot stock policy")
    walk=2*pace["native_units_per_tick_x100"] # authored relative tuning keeps both source paces distinct above the API minimum
    # The smaller cohort now uses its corrected socket and the native
    # target-height aim without the old bias tuned for the floating rig.
    rise=-55 if key<3 else 0
    lines=["GFBORG-GAMEPLAY 1","# Authored elemental orb test; not retail combat.",
           "variant=normal",f"health={hp}",f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|12",
           "rig-prefix=visual/body.prefix","pose-profile=visual/body.pose","body-record=0","dash-policy=disabled",
           f"idle={g}:{c}"]
    for field,role in (("walk","ground-travel"),("air","air-continuation")):
        x,y=clips[role]
        lines.append(f"{field}={x}:{y}")
    lines.extend(("stock=shot-stock|"+str(limit)+"|180","stock-clock=shot-stock|add-on-debit",
                  f"projectile=orb|orb|1|45|{rise}|0|0|36|100|8|ignore-floor","projectile-record=orb|8|10",
                  "origin=orb|12|0|0|0|target-height",f"move=basic-shot|b|10|grounded|28|{action_group}:{action_choice}",
                  "entry-stock=basic-shot|shot-stock|1","release=basic-shot|12|shot-stock|1|orb"))
    if trail_drawable:
        lines.append("projectile-record-trail=orb|9|5|1|6")
    lines.extend(("impact-record-bank=visual/impact.hsd","impact-record=orb|actor|22|10|18",
                  "impact-record-growth=orb|25"))
    sounds=[]
    if launch_audio:
        bank,tone=pilot_launch_tone(hit.get("sfx_candidate",{}))
        path=f"audio/launch-{bank}-{tone}.pcm"
        lines.append(f"launch-sound=orb|{path}")
        sounds.append({"projectile":"orb","bank":bank,"tone":tone,"path":path})
    return ("\n".join(lines)+"\n").encode("ascii"),whole_body_pose(joints),{
        "kind":"opt-in authored grounded Dragon orb; not retail combat","preview_clip":"b-shot-release-pose",
        "source_level":1,"retail_behavior_verified":False,"source_normal_factory_family":"type-47","source_normal_factory_key":key,
        "source_release_site":hit["site"],"source_debit_site":debit["debit_site"],"body_binding":"strict record zero",
        "native_timing":{"release_tick":12,"move_ticks":28,"refill_ticks":180},
        "native_ground_pace":walk,"native_rise_bias":rise,"projectile_record":8,
        "projectile_trail":"authored record-9 history" if trail_drawable else "authored empty record 9; no substitute",
        "impact_record":{"record":22,"source_archive_slot":1,"duration_ticks":18,"visual_only":True,
                         "authored_scale_percent":[10,25],"authored_alpha_fade":True},
        "authored_launch_sounds":sounds}


def worm_battle_test(model,joints,role_clips,pace,level,source,*,launch_audio=False):
    """Authored single orb using each Worm's reviewed stationary-B pose and socket."""
    family,key,call = WORM_MODELS[model]
    pose = source["pose"]
    origin = source["origin"]
    stock = source["stock"]
    roles = {name:(g,c) for name,g,c in role_clips}
    locomotion = ("ground-stop","ground-travel","air-continuation","ground-dash-forward")
    if (not pace or any(r not in roles for r in locomotion) or
            (source["model"],source["family"],source["key"],source["call"],pose["release_site"])!=
            (f"{model:04X}",f"type-{family}",key,f"{call:08X}",f"{call:08X}") or
            pose["clip"]!=[3,0] or origin["source_joint"]!=12 or not 12<len(joints) or
            (stock["channel"],stock["cost"],stock["commit"],stock["same_input_actor"])!=(0,1,True,True)):
        raise ValueError("Worm pilot lacks exact movement, release or socket evidence")
    health,initial,capacity,mode,_ = level[:5]
    walk = 3*pace["native_units_per_tick_x100"] # authored relative pacing; retain both source cohorts above API minimum
    if not 1<=initial==capacity<=255 or mode not in (0,2) or not 100<=walk<=1042:
        raise ValueError("Worm pilot stock or walk domain")
    lines = ["GFBORG-GAMEPLAY 1","# Authored Worm orb test; not retail combat.",
             "variant=normal",f"health={health}",f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
             "rig-prefix=visual/body.prefix","pose-profile=visual/body.pose","body-record=0"]
    for field,role in zip(("idle","walk","air","dash"),locomotion,strict=True):
        g,c = roles[role]
        lines.append(f"{field}={g}:{c}")
    point = "|".join(f"{axis:.9g}" for axis in origin["local_point"])
    lines.extend((f"stock=shot-stock|{capacity}|180","projectile=shot|orb|1|36|0|0|0|16|90|8|ignore-floor",
                  f"origin=shot|12|{point}|target-height","move=basic-shot|b|10|grounded|28|3:0",
                  "entry-stock=basic-shot|shot-stock|1","release=basic-shot|12|shot-stock|1|shot"))
    sounds = []
    if launch_audio:
        bank,tone = pilot_launch_tone(source["sound"])
        path = f"audio/launch-{bank}-{tone}.pcm"
        lines.append(f"launch-sound=shot|{path}")
        sounds.append({"projectile":"shot","bank":bank,"tone":tone,"path":path,
                       "trigger":"accepted native allocation; not the retail collision bind"})
    return ("\n".join(lines)+"\n").encode("ascii"),whole_body_pose(joints),{
        "kind":"opt-in authored Worm orb; not retail combat","preview_clip":"b-shot-release-pose",
        "source_level":1,"source_stock_mode":mode,"retail_behavior_verified":False,
        "source_normal_factory_family":source["family"],"source_normal_factory_key":key,"source_release_site":source["call"],
        "source_debit_site":stock["site"],"body_binding":"strict record zero","native_ground_pace":walk,
        "native_timing":{"release_tick":12,"move_ticks":28,"refill_ticks":180},
        "native_projectile":"authored orb; no retail string, poison or trap","authored_launch_sounds":sounds,
    }


def basic_shot_test(model, joints, role_clips, action_clips, pace, level, releases, *,
                    paired_sites=None, launch_audio=False, actor_sfx=(), idle_timing=None):
    if model not in COHORT:
        return None
    conditional = model in TYPE_2_KEYS and model not in PREFERRED_B_SHOT and model not in RIFLE_KEYS
    pair = ORDERED_SHOTS.get(model)
    if pair and paired_sites!=pair[1]:
        raise ValueError("basic shot test lacks reviewed ordered sibling calls")
    if model in EAGLE_FAMILIES:
        family,key = EAGLE_FAMILIES[model],pair[0][0]
        basis = "parent-joint" if family=="type-2" else "credited-actor-joint"
    elif pair or model in TYPE_2_KEYS:
        family,key,basis = "type-2",pair[0][0] if pair else TYPE_2_KEYS[model],"parent-joint"
    elif model in THROW_KEYS:
        family,key,basis = "type-14",THROW_KEYS[model],"retained-actor-joint"
    else:
        family,key,basis = "type-0",TYPE_0_KEYS[model],"credited-actor-joint"
    roles = {name:(g,c) for name,g,c in role_clips}
    locomotion = ("ground-stop","ground-travel","air-continuation","ground-dash-forward")
    context = PREFERRED_B_SHOT.get(model,(0,0))
    clip_id = f"b-shot-c{context[0]}-s{context[1]}"+("-p0" if conditional else "")
    chosen = [r for r in action_clips if r["id"]==clip_id]
    if (not pace or not all(role in roles for role in locomotion) or len(chosen)!=1 or
            not chosen[0]["routes"] or (conditional and chosen[0]["request_choice_condition"]["set"] is not False)):
        raise ValueError("basic shot test lacks exact movement, entry or release evidence")
    if (model in RIFLE_KEYS or model in RIFLE_PAIRS) and any(
            r.get("authored_direction_choice")!=1 or not r.get("seed_candidate") for r in chosen[0]["routes"]):
        raise ValueError("basic shot test lacks the seeded rifle pose")
    if model==0x080C and any(r.get("release_relay_site")!="8010FDF4" or not r.get("cue_command")
                             for r in chosen[0]["routes"]):
        raise ValueError("basic shot test lacks the guarded cue pose")
    if model in THROW_KEYS and any(not r.get("seed_candidate") or not r.get("source_pose_alternatives")
                                   for r in chosen[0]["routes"]):
        raise ValueError("basic shot test lacks the seeded throw pose")
    if model==DOUBLE_NINJA and any(r.get("release_site")!=DOUBLE_NINJA_PRIMARY_SITE or
                                  r.get("source_clone_byte",{}).get("value")!=0 for r in chosen[0]["routes"]):
        raise ValueError("basic shot test lacks the scoped no-clone throw")
    if model==0x0906 and any(r.get("cue_bytes")!=[1,1] or r.get("program")!=[2,6] for r in chosen[0]["routes"]):
        raise ValueError("basic shot test lacks the Theta cue pose")
    if model in (0x0100,0x0105) and any(r.get("source_side_choice")!="zero" or r.get("source_start_frame")!=11
                                        for r in chosen[0]["routes"]):
        raise ValueError("basic shot test lacks the reviewed zero-side pose")
    if (model==0x0103 or model in CYBER_MACHINE_SHOTS or model in EAGLE_FAMILIES) and any(
            not r.get("seed_candidate") or not r.get("cue_command") for r in chosen[0]["routes"]):
        raise ValueError("basic shot test lacks the exact seeded release pose")
    if model in EAGLE_FAMILIES:
        if (not idle_timing or idle_timing.get("strict_metadata_refusals")!=["aobj-flags"] or
                idle_timing.get("aobj_flags")!=[0x20000000] or
                not idle_timing.get("animated_joint_count") or
                idle_timing["animated_joint_count"]!=idle_timing["authored_loop_joint_count"]):
            raise ValueError("basic shot test lacks a uniform authored idle loop")
    hits,debit = match_shot_context(chosen[0],releases,ordered_sites=paired_sites)
    keys = pair[0] if pair else (key,)
    if (tuple(hit["key"] for hit in hits)!=keys or
            any((hit["family"],hit["basis"])!=(family,basis) or not hit["joint_present_in_normal_rig"] or
                not hit["parent_model_matches"] or hit.get("adds_parent_offset") or hit.get("adds_retained_offset") or
                hit.get("required_model_at_call",f"{model:04X}")!=f"{model:04X}" for hit in hits) or
            (debit.get("channel"),debit.get("cost"),debit.get("commit"),debit.get("same_input_actor"))!=(0,1,True,True)):
        raise ValueError("basic shot test lacks exact socket or stock guard")
    if pair and len({(h["source_joint_selector"],tuple(h["local_point"])) for h in hits})!=len(hits):
        raise ValueError("basic shot test ordered sockets must stay distinct")
    health,initial,limit,mode,preset = level[:5]
    walk = pace["native_units_per_tick_x100"]
    if not 1<=initial==limit<=255 or mode not in (0,1) or not 1<=preset<=3600 or not 100<=walk<=1042:
        raise ValueError("basic shot test stock or walk domain")
    # The native clock value is AUTHORED even when it equals the raw
    # source preset. It is not read from the source or a duration estimate.
    marker,length = (3,6) if model in RIFLE_PAIRS else (6,10) if model in CYBER_MACHINE_SHOTS else {
        0x0007:(6,10),0x0100:(6,10),0x0103:(3,6),0x0105:(6,10),0x0600:(3,6),0x0602:(6,10),
        0x0606:(6,10),0x0616:(3,6),0x061A:(3,7),0x080C:(6,10),
    }.get(model,(12,28))
    if model in THROW_KEYS:
        marker,length = 6,10
    timing = {"release_tick":marker,"move_ticks":length,"refill_ticks":180}
    lines = ["GFBORG-GAMEPLAY 1","# Authored one-shot test; not retail combat.",
             "variant=normal",f"health={health}",f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
             "rig-prefix=visual/body.prefix","pose-profile=visual/body.pose"]
    if model in RECORD_ZERO:
        lines.append("body-record=0")
    if model in EAGLE_FAMILIES:
        lines.append("idle-loop=authored-bit")
    for field,role in zip(("idle","walk","air","dash"),locomotion,strict=True):
        g,c = roles[role]
        lines.append(f"{field}={g}:{c}")
    g,c = chosen[0]["clip"]
    sound_rows = []
    lines.append(f"stock=shot-stock|{limit}|{timing['refill_ticks']}")
    projectiles=("shot","shot-second","shot-third","shot-fourth","shot-fifth","shot-sixth")[:len(hits)]
    for projectile,hit in zip(projectiles,hits,strict=True):
        point = "|".join(f"{axis:.9g}" for axis in hit["local_point"])
        visual = "shuriken" if model in THROW_KEYS else "bullet"
        lines.extend((f"projectile={projectile}|{visual}|1|32|0|0|0|12|70|8|ignore-floor",
                      f"origin={projectile}|{hit['source_joint_selector']}|{point}|target-height"))
        if launch_audio:
            candidate = hit.get("sfx_candidate",{})
            tone_key = pilot_launch_tone(candidate)
            if tone_key is None:
                sound_rows.append({"projectile":projectile,"source_sound_id":-1,"status":"unbound-signed-operand"})
                continue
            bank,tone = tone_key
            path = f"audio/launch-{bank}-{tone}.pcm"
            lines.append(f"launch-sound={projectile}|{path}")
            sound_rows.append({"projectile":projectile,"bank":bank,"tone":tone,"path":path})
    lines.extend((f"move=basic-shot|b|10|grounded|{timing['move_ticks']}|{g}:{c}",
                  "entry-stock=basic-shot|shot-stock|1",
                  f"release=basic-shot|{timing['release_tick']}|shot-stock|1|shot"))
    lines.extend(f"release-followup=basic-shot|{marker}|{projectile}" for projectile in projectiles[1:])
    if mode==1:
        lines.append("stock-clock=shot-stock|add-on-debit")
    release_sound = None
    if launch_audio and model==ACTOR_RELEASE_SOUND[0]:
        release_sound = bind_actor_release_sound(chosen[0],actor_sfx,debit,hits[0],marker)
        lines.append(f"release-sound=basic-shot|{marker}|{release_sound['path']}")
    hit=hits[0]
    summary = {"kind":"opt-in authored Battle Test rules; not retail combat","source_level":1,
               "source_normal_factory_family":family,"source_normal_factory_key":key,
               "source_release_site":hit["site"],"source_debit_site":debit["debit_site"],"source_stock_mode":mode,
               "preview_clip":chosen[0]["id"],"pose_policy":"single authored lane",
               "body_binding":"strict record zero" if model in RECORD_ZERO else "archive-wide validation",
               "native_timing":timing,"retail_behavior_verified":False}
    if pair:
        summary["ordered_factory_calls"]=[{"key":h["key"],"site":h["site"]} for h in hits]
    if model==DOUBLE_NINJA:
        summary["source_clone_scope"]="byte 7C8 zero; clone and X not implemented"
    if model in EAGLE_FAMILIES:
        summary["idle_loop_policy"]="uniform authored bit; native idle clock"
    if sound_rows:
        summary["authored_launch_sounds"]=sound_rows
    if release_sound:
        summary["authored_release_sound"]=release_sound
    return ("\n".join(lines)+"\n").encode("ascii"),whole_body_pose(joints),summary


def shot_context_hits(clip, expected, releases):
    context = tuple(clip["raw_context"])
    return [(hit,route) for hit in releases for route in hit["routes"]
            if route["action_group"]=="B-shot" and context in map(tuple,route["context_rows"]) and
               all(route[key]==expected[key] for key in CONTEXT_FIELDS) and
               (not expected.get("phase_table") or route.get("phase_table")==expected["phase_table"]) and
               (not expected.get("release_site") or hit["site"]==expected["release_site"]) and
               ("seeded_factory_key" not in expected or hit["key"]==expected["seeded_factory_key"]) and
               (not expected.get("reviewed_caller_model") or route.get("reviewed_caller_model")==expected["reviewed_caller_model"]) and
               (not expected.get("release_entry") or route.get("release_entry")==expected["release_entry"]) and
               (not expected.get("release_helper") or route.get("relay_handler")==expected["release_helper"]) and
               (not expected.get("release_relay_site") or route.get("relay_site")==expected["release_relay_site"]) and
               (not expected.get("source_side_choice") or hit.get("source_side_choice")==expected["source_side_choice"])]


def match_shot_context(clip, releases, *, ordered_sites=None):
    evidence = []
    for expected in clip["routes"]:
        matches = shot_context_hits(clip,expected,releases)
        if ordered_sites:
            matches.sort(key=lambda pair:int(pair[0]["site"],16))
            if tuple(int(hit["site"],16) for hit,_ in matches)!=ordered_sites:
                raise ValueError("basic shot test paired factory order or clip context disagrees")
        elif len(matches)!=1:
            raise ValueError("basic shot test needs one guarded factory in each exact clip context")
        guards = [route.get("local_stock_guards",()) for _,route in matches]
        if any(len(g)!=1 or g!=guards[0] for g in guards):
            raise ValueError("basic shot test needs one shared debit across its factory calls")
        evidence.append(([hit for hit,_ in matches],guards[0][0]))
    if any(pair!=evidence[0] for pair in evidence):
        raise ValueError("basic shot test clip contexts disagree on factory or debit identity")
    return evidence[0]


def bind_actor_release_sound(clip, actor_sfx, debit, first, marker):
    model,sound_id,site = ACTOR_RELEASE_SOUND
    context = tuple(clip["raw_context"])
    cues = [s for s in actor_sfx if (s["site"],s["source_sound_id"],s.get("required_model_at_call"))==
            (site,sound_id,f"{model:04X}") and s.get("status")=="static-tone-record"]
    if len(cues)!=1 or not int(debit["debit_site"],16)<int(site,16)<int(first["site"],16):
        raise ValueError("basic shot test lacks the paid actor sound")
    cue=cues[0]
    for expected in clip["routes"]:
        if not any(r["action_group"]=="B-shot" and context in map(tuple,r["context_rows"]) and
                   all(r[k]==expected[k] for k in CONTEXT_FIELDS) and
                   r["relay_site"]==expected["release_relay_site"] and r["relay_handler"]==debit["entry"]
                   for r in cue["routes"]):
            raise ValueError("basic shot test actor sound context disagrees with the paid release")
    bank,tone=cue["bank"],cue["tone"]
    return {"bank":bank,"tone":tone,"source_sound_id":sound_id,"source_call_site":site,
            "native_release_tick":marker,"path":f"audio/release-{bank}-{tone}.pcm"}


def pose_free_shot_test(model,joints,actions,level,releases,cruise,ordered_sites,*,
                        launch_audio=False,role_clips=(),pace=None,cruise_entry=None):
    phase_binding = STATIC_CRUISE_PHASE_SHOTS.get(model)
    violet = model==VIOLET_SHOT["model"]
    ground_phase = STATIC_GROUND_PHASE_SHOTS.get(model)
    ground_pair = STATIC_GROUND_PHASE_PAIRS.get(model)
    static_ground = model in STATIC_GROUND_MODELS
    animated = ANIMATED_CRUISE_SHOTS.get(model)
    flying = model in STATIC_CRUISE_SHOTS or phase_binding is not None or animated is not None or violet
    direct_entry = CLOCKED_GROUND_DIRECT_ENTRIES.get(model)
    family = "type-2" if flying else "type-0"
    if violet:
        r=VIOLET_SHOT
        phase,entry,debit_site=f'{r["table"]:08X}',f'{r["leaves"][-1]:08X}',f'{r["debit"]:08X}'
        family,selector,stock_mode="type-0",45,0
        keys,sites=(r["keys"][0],),(r["site"],)
    elif ground_pair:
        table,helper,keys,sites=ground_pair
        phase,entry,debit_site=f"{table:08X}",f"{helper:08X}","8018940C"
        family,selector,stock_mode="type-2",44,0
    elif ground_phase:
        family,key,site,phase,entry,debit_site,stock_mode=ground_phase
        selector,keys,sites=45,(key,),(site,)
    elif phase_binding is not None:
        key,site,phase,entry,debit_site,selector,stock_mode = phase_binding
        keys,sites = (key,),(site,)
    else:
        selector,stock_mode = 44,0
        if animated:
            direct_entry,family,keys,sites = animated
        else:
            keys,sites = (STATIC_CRUISE_SHOTS if flying else CLOCKED_GROUND_SHOTS)[model]
    phase_bound = static_ground or phase_binding is not None or violet
    if (flying and not cruise or bool(animated)!=(cruise_entry is not None) or
            ordered_sites!=(None if len(keys)==1 else sites) or not actions["profiles"] or any(
            actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores"))):
        raise ValueError("pose-free shot lacks exact motion or release evidence")
    contexts=[]
    for profile in actions["profiles"]:
        (group,) = (g for g in profile["groups"] if g["group"]=="B-shot")
        candidates=[row for row in group["rows"] if (0,0) in map(tuple,row["contexts"]) and
                     (row["state"],row["substate"],row["selector"])==(0,0,selector)]
        if len(candidates)!=1:
            raise ValueError("pose-free shot lacks a common B-shot context")
        contexts.append({k:(profile if k in ("config","context_table","main_handler") else candidates[0])[k]
                         for k in CONTEXT_FIELDS})
        if phase_bound:
            contexts[-1].update(phase_table=phase,release_entry=entry)
            if violet:
                contexts[-1].update(seeded_factory_key=keys[0],reviewed_caller_model=f"{model:04X}")
        elif direct_entry is not None:
            contexts[-1]["release_entry"]=f"{direct_entry:08X}"
        elif not flying:
            contexts[-1].update(release_helper="8018BF2C",release_relay_site="8018BF18")
    hits,debit=match_shot_context({"raw_context":[0,0],"routes":contexts},releases,ordered_sites=ordered_sites)
    basis=(family,"parent-joint" if family=="type-2" else "credited-actor-joint")
    if (tuple(r["key"] for r in hits)!=keys or tuple(int(r["site"],16) for r in hits)!=sites or
            any((h["family"],h["basis"])!=basis or not h["joint_present_in_normal_rig"] or
                not h["parent_model_matches"] or h.get("adds_parent_offset") or
                h.get("required_model_at_call")!=(None if phase_binding or ground_pair or violet else f"{model:04X}") for h in hits) or
            len({(h["source_joint_selector"],tuple(h["local_point"])) for h in hits})!=len(hits) or
            (phase_bound and (debit.get("entry"),debit["debit_site"])!=(entry,debit_site)) or
            (debit["channel"],debit["cost"],debit["commit"],debit["same_input_actor"])!=(0,1,True,True)):
        raise ValueError("pose-free shot lacks the reviewed sockets or debit")
    if violet and hits[0].get("release_guard_candidate",{}).get("first_value")!=0:
        raise ValueError("pose-free shot lacks a proven initial barrel")
    hp,initial,limit,mode,_=level[:5]
    if not 1<=initial==limit<=255 or mode!=stock_mode:
        raise ValueError("unsupported pose-free stock")
    marker,length=12,28
    rig=("rig-prefix=visual/body.prefix","pose-profile=visual/body.pose","body-record=0")
    if flying:
        body=f"cruise-entry={cruise_entry[0]}:{cruise_entry[1]}" if animated else "body-motion=static-cruise"
        description="Authored flight and B shot" if animated else "Authored static flight and B shot"
        lines=["GFBORG-GAMEPLAY 1",f"# {description}; not retail combat.",
               "variant=normal",f"health={hp}","movement=grounded-v1|6|13|12",*rig,
               body,"cruise-boost="+"|".join(map(str,cruise))]
    else:
        roles={name:(g,c) for name,g,c in role_clips}
        walk = pace["native_units_per_tick_x100"] if pace else 0
        required=("ground-stop","ground-travel","air-continuation","ground-dash-forward")
        if not 100<=walk<=1042 or (not static_ground and not all(name in roles for name in required)):
            raise ValueError("clocked ground shot lacks exact locomotion")
        lines=["GFBORG-GAMEPLAY 1","# Authored pose-free B shot; not retail combat.",
               "variant=normal",f"health={hp}",f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",*rig]
        if static_ground:
            lines.append("body-motion=static-grounded")
        else:
            for field,role in zip(("idle","walk","air","dash"),required,strict=True):
                g,c=roles[role]
                lines.append(f"{field}={g}:{c}")
    lines.append(f"stock=shot-stock|{limit}|180")
    sounds=[]
    for name,hit in zip(("shot","shot-second")[:len(keys)],hits,strict=True):
        point="|".join(f"{n:.9g}" for n in hit["local_point"])
        lines.extend((f"projectile={name}|bullet|1|32|0|0|0|12|70|8|ignore-floor",
                      f"origin={name}|{hit['source_joint_selector']}|{point}|target-height"))
        if launch_audio:
            tone_key=pilot_launch_tone(hit.get("sfx_candidate",{}))
            if tone_key is None:
                raise ValueError("unbound pose-free launch sample")
            bank,tone=tone_key
            path=f"audio/launch-{bank}-{tone}.pcm"
            lines.append(f"launch-sound={name}|{path}")
            sounds.append({"projectile":name,"bank":bank,"tone":tone,"path":path})
    scope = "airborne" if flying else "grounded"
    lines.extend((f"move=basic-shot|b|10|{scope}|{length}|none","entry-stock=basic-shot|shot-stock|1",
                  f"release=basic-shot|{marker}|shot-stock|1|shot"))
    if len(keys)==2:
        lines.append(f"release-followup=basic-shot|{marker}|shot-second")
    if mode==1:
        lines.append("stock-clock=shot-stock|add-on-debit")
    return ("\n".join(lines)+"\n").encode("ascii"),whole_body_pose(joints),{
        "kind":("opt-in authored cruise-entry Battle Test; not retail flight or combat" if animated else
                "opt-in authored static-cruise Battle Test; not retail flight or combat" if flying else
                "opt-in authored pose-free Battle Test; not retail combat"),
        "source_level":1,"preview_clip":"cruise-entry" if animated else "none",
        "body_binding":"strict static record zero" if static_ground or (flying and not animated) else "strict record zero",
        "retail_behavior_verified":False,
        **({"source_selector_scope":"initial barrel only; repeat alternation not implemented"} if violet else {}),
        **({"source_cruise_entry":cruise_entry,"pose_policy":"play once, then hold through boost and shot"} if animated else {}),
        **({"native_cruise":cruise} if flying else {"pose_policy":"static body during authored ground movement","native_ground_pace":walk}
           if static_ground else {
            "pose_policy":"keep locomotion during the authored shot",
            "source_clock_candidate":({"timer_read":f"{direct_entry+0xF4:08X}","count_read":f"{direct_entry+0x11C:08X}"}
                                      if direct_entry is not None else
                                      {"timer_read":"8018BF78","count_read":"8018BFA0","check_only_stock":"8018BFC4"})}),
        "source_debit_site":debit["debit_site"],
        **({"source_phase_table":phase,"source_release_entry":entry,"source_stock_mode":mode}
           if phase_bound else {}),
        "ordered_factory_calls":[{"key":h["key"],"site":h["site"]} for h in hits],
        "authored_launch_sounds":sounds}


def cruise_charge_shot_test(model,joints,actions,level,releases,cruise,*,launch_audio=False):
    """A stockless authored charge, scoped to the reviewed static branch."""
    key,site = CRUISE_CHARGE_SHOTS[model]
    if not cruise or not actions["profiles"] or any(
            actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")):
        raise ValueError("cruise charge test lacks exact flight and action contexts")
    hits=[]
    for profile in actions["profiles"]:
        (group,) = (g for g in profile["groups"] if g["group"]=="B-charge")
        rows = [row for row in group["rows"] if (0,0) in map(tuple,row["contexts"])]
        if len(rows)!=1:
            raise ValueError("cruise charge test lacks an exact B-charge context")
        (row,) = rows
        candidates = [h for h in releases for r in h["routes"] if r["action_group"]=="B-charge" and
                       (0,0) in map(tuple,r["context_rows"]) and r.get("phase_table")=="8031D158" and
                       r.get("release_entry")=="800F809C" and not r.get("local_stock_guards") and
                       all(r[k]==(profile if k in ("config","context_table","main_handler") else row)[k]
                           for k in CONTEXT_FIELDS)]
        if len(candidates)!=1:
            raise ValueError("cruise charge test lacks one reviewed release")
        hits.append(candidates[0])
    hit=hits[0]
    if (any(h!=hit for h in hits) or (hit["family"],hit["key"],hit["site"],hit["basis"])!=("type-2",key,site,"parent-joint") or
            not hit["joint_present_in_normal_rig"] or not hit["parent_model_matches"]):
        raise ValueError("cruise charge test release or socket disagrees across profiles")
    if (model==0x0D00 and hit.get("required_model_at_call")!="0D00") or (
            model==0x0D04 and hit.get("excluded_models_at_call")!=["0D00"]):
        raise ValueError("cruise charge test lacks the expected model branch")
    marker,length,minimum,maximum=8,20,6,45
    point="|".join(f"{n:.9g}" for n in hit["local_point"])
    lines=["GFBORG-GAMEPLAY 1","# Authored static flight and held B; no retail charge timing.",
           "variant=normal",f"health={level[0]}","movement=grounded-v1|6|13|12",
           "rig-prefix=visual/body.prefix","pose-profile=visual/body.pose","body-record=0","body-motion=static-cruise",
           "cruise-boost="+"|".join(map(str,cruise)),"projectile=shot|bullet|1|32|0|0|0|12|70|8|ignore-floor",
           f"origin=shot|{hit['source_joint_selector']}|{point}|target-height",
           f"move=charge-shot|b|10|airborne|{length}|none",
           f"activation=charge-shot|release-window|{minimum}|{maximum}",
           f"release=charge-shot|{marker}|none|0|shot"]
    sounds=[]
    if launch_audio:
        tone_key=pilot_launch_tone(hit.get("sfx_candidate",{}))
        if tone_key is None:
            raise ValueError("missing cruise charge launch sample")
        bank,tone=tone_key
        path=f"audio/launch-{bank}-{tone}.pcm"
        lines.append(f"launch-sound=shot|{path}")
        sounds.append({"projectile":"shot","bank":bank,"tone":tone,"path":path})
    return ("\n".join(lines)+"\n").encode("ascii"),whole_body_pose(joints),{
        "kind":"opt-in authored static-cruise B-charge; not retail combat",
        "source_level":1,"preview_clip":"none","body_binding":"strict static record zero",
        "native_cruise":cruise,"native_charge":{"minimum_hold":minimum,"maximum_hold":maximum,"launch_tick":marker},
        "source_charge_factory":{"key":key,"site":site},"source_debit_in_reviewed_leaf":False,
        "retail_behavior_verified":False,"authored_launch_sounds":sounds}
