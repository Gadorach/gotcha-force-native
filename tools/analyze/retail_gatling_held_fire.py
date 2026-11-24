"""Scoped Gatling Gunner ordinary-B bindings; the native cadence is authored."""
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program


MODEL=0x0102
FAMILY="type-0"
KEY=29
TEXT=(
    (0x80135034,0x64,"1ce42843160ccd2ac66429e692836df82c0a523a092110eee58ae5d1934e1a86"),
    (0x801350F0,0xDC,"ce7a9dd8247aae017721471b4249e9ecef9a320fa985182e5f73b0b13b1ff0f6"),
    (0x801354C4,0x20C,"fd678a00ce28aedb45cbf11479cc0072d82372c110b8373eb3add94eef228a8c"),
    (0x80135B10,0x4C,"61b7a81bacb66a91d5a3b37b25922e7ec2fc9bd896905939f2912dfa3fa200aa"),
    (0x801359EC,0x124,"03f8ed3b512925f1462aea0c35c65a60ef747b9a121f7d8736cf456deaa50532"),
)
DATA=(
    (0x80337F38,0x6C,"7fd3d9ec1b72cd8e041791609f753666af6d1c8848ddf71da60363a50fee501d"),
    (0x80338716,0x54,"47193a58458f60b088d23c75edfedac0ad66b4ddde38c5a0a8ac68e69b5bd7a0"),
)
ROOTS=(0x80338400,0x802BC080,0x80338860,0x802BAE60)


def verify_source(dol):
    for kind, spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"Gatling ordinary-B {kind} signature: {at:08X}")


def held_fire_source(linker, actions, slots, parts, origins, sounds, *, publish_preview):
    profiles=actions["profiles"]
    if len(profiles)!=1 or any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")):
        raise ValueError("Gatling installer scope")
    p,=profiles
    roots=tuple(int(x,16) for x in p["program_and_descriptor_roots"])
    if ((p["config"],p["context_table"],p["main_handler"],roots)!=("803380C0","803382F8","80134FF8",ROOTS) or
            linker.read_word(0x80337F38)!=0x801354C4 or
            tuple(linker.leaf(0x801354C4,0x540,i) for i in range(3))!=(0x8013550C,0x80135554,0x8013560C) or
            linker.leaf(0x801350F0,0x581,0)!=0x80135150 or linker.table_choice(roots[0],2,0)!=0x80338716):
        raise ValueError("Gatling neutral release dispatch")
    matches=[r for g in p["groups"] if g["group"]=="B-shot" for r in g["rows"] if (0,0) in map(tuple,r["contexts"])]
    if len(matches)!=1 or (matches[0]["state"],matches[0]["substate"],matches[0]["selector"],
                             matches[0].get("class_callback_candidate"))!=(0,0,44,"80135034"):
        raise ValueError("Gatling neutral action context")
    local={(g,c):at for g,c,at in slots}
    program=decode_program(linker.data,0x80338716,max_commands=24,max_span=96)
    if (program["status"]!="terminal_wait" or [c["raw"] for c in program["commands"][4:6]]!=
            ["0d02040305000000","0201010000000000"]):
        raise ValueError("Gatling shot pose/cue command")
    found={}
    for c in linker.program_candidate(roots,local,(15,2,0))["clip_candidates"]:
        if (c["command_index"],c["start_frame"],c["preceding_waits"])!=(4,0,1):
            continue
        lanes=[(l["bank"],l["group"],l["choice"],l["archive_offset"]) for l in c["lanes"] if l["lane"] in (0,1)]
        if len(lanes)==2 and lanes[0]==lanes[1] and lanes[0][0]=="local" and lanes[0][3] is not None:
            found[c["descriptor_choice"]]=list(lanes[0][1:3])
    if found!={4:[3,12],3:[3,11],5:[3,13]}:
        raise ValueError("Gatling exact directional release clips")
    hands=[(r["joint"],r["record"]) for r in parts["default_candidates"]]
    if hands!=[(7,1),(11,5)]:
        raise ValueError("Gatling exact hand records")
    point=origins[FAMILY][KEY]
    sound=sounds[FAMILY][KEY]
    if ((point["basis"],point["source_joint_selector"],tuple(point["local_point"]))!=
            ("credited-actor-joint",7,(-100.,24.200000762939453,0.)) or
            (sound["status"],sound["bank"],sound["tone"])!=("static-tone-record","snd_com01",6)):
        raise ValueError("Gatling exact muzzle or cue")
    chosen={"clip":[3,11],"archive_offset":local[3,11],"program":[2,0],"descriptor":[2,3],"request_mask":1,
            "entry":"80135150","request_site":"80135184","source_start_frame":0,
            "phase_table":"80337F6C","release_site":"80135B44"}
    if publish_preview:
        matches[0]["release_pose_candidate"]=chosen
    return {"pose":chosen,"directional_clips":[found[i] for i in (4,3,5)],"direction_selection":"authored middle candidate",
            "stock":{"debit":"80135B30","check_only":"80135674","channel":0,"cost":1,"same_input_actor":True},
            "repeat_gate":{"held_b_read":"80135658","clock_read":"80135628","repeat_call":"80135684"},
            "factory":{"family":FAMILY,"key":KEY,"site":"80135B44"},"origin":point,"sound":sound,
            "hand_records":hands,"retail_cadence_verified":False}


def battle_test(joints,role_clips,pace,level,source,*,launch_audio=False):
    from tools.analyze.retail_basic_shot_test import whole_body_pose
    roles={name:(g,c) for name,g,c in role_clips}
    required=("ground-stop","ground-travel","air-continuation","ground-dash-forward")
    if not pace or any(role not in roles for role in required) or len(joints)<=11 or source["pose"]["clip"]!=[3,11]:
        raise ValueError("Gatling pilot exact movement, rig or shot pose")
    hp,initial,capacity,mode,preset=level[:5]
    walk=pace["native_units_per_tick_x100"]
    if not 100<=walk<=1042 or (initial,capacity,mode,preset)!=(100,100,0,480):
        raise ValueError("Gatling pilot stock or pace domain")
    lines=["GFBORG-GAMEPLAY 1","# Authored held-fire cadence; not retail combat.",
           "variant=normal",f"health={hp}",f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
           "rig-prefix=visual/body.prefix","pose-profile=visual/body.pose","body-record=0"]
    for field,role in zip(("idle","walk","air","dash"),required,strict=True):
        g,c=roles[role];lines.append(f"{field}={g}:{c}")
    lines.extend(("part-record=right-hand|7|1","part-record=left-hand|11|5","stock=gatling-stock|100|180",
                  "projectile=gatling|bullet|1|42|0|0|0|12|70|8|ignore-floor",
                  "origin=gatling|7|-100|24.2000008|0|target-height","move=gatling-b|b|10|grounded|5|3:11",
                  "held-fire=gatling-b|gatling-stock|1|gatling|12|8|3|10"))
    sounds=[]
    if launch_audio:
        path="audio/launch-snd_com01-6.pcm"
        lines.append(f"launch-sound=gatling|{path}")
        sounds.append({"projectile":"gatling","bank":"snd_com01","tone":6,"path":path})
    return ("\n".join(lines)+"\n").encode("ascii"),whole_body_pose(joints),{
        "kind":"opt-in authored held-B Gatling test; not retail combat","source_level":1,
        "preview_clip":"b-shot-release-pose","retail_behavior_verified":False,
        "native_cadence":{"spin_up":12,"slow_interval":8,"fast_interval":3,"cooldown":10},
        "native_refill_ticks":180,"authored_launch_sounds":sounds}
