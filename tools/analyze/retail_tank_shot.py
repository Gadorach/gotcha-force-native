"""Normal Tank-family B firing routes. These are not close melee."""
from hashlib import sha256

from tools.analyze.retail_action_links import direct_branch


NORMAL, GATLING, ICBM, LEOPARD = 0x0C00, 0x0C01, 0x0C05, 0x0C06
MODELS = {NORMAL, GATLING, ICBM, LEOPARD}
# Exact normal installer, class callback tables, paid helper and muzzle keys.
# Gatling's cycle is not replayed: the pilot explicitly picks one qualified
# barrel (key 7), not a claim about the first barrel of every retail burst.
CANNON=("802D6AD0","802D6D08","8007CCB0",(0x802C2E60,0x802BCFA8,0x802C2CC0,0x802BBD94),
        0x8007CCEC,0x804338E0,0x8007CD24,0x802D6A84,
        (0x8007CD60,0x8007CD78,0x8007CD90,0x8007CE84),
        0x8007CF6C,((0x8007CF88,3),(0x8007CF98,2)),3)
ROUTES={
    NORMAL:CANNON,
    LEOPARD:CANNON,
    GATLING:("802FA208","802FA440","800ADA28",(0x802FA4A0,0x802BCFA8,0x802FA7C0,0x802BBD94),
             0x800ADA64,0x80433A68,0x800ADA9C,0x802FA1B4,
             (0x800ADAD8,0x800ADAF0,0x800ADB08,0x800ADB78),
             0x800AE5A4,((0x800AE5E8,4),(0x800AE5F8,7),(0x800AE608,6),(0x800AE618,5)),7),
    ICBM:("802FAFF8","802FB230","800AF7F0",(0x802FB360,0x802BCFA8,0x802FB680,0x802BBD94),
          0x800AF82C,0x80433A90,0x800AF864,0x802FB30C,
          (0x800AF8A0,0x800AF8B8,0x800AF8D0,0x800AF9C4),
          0x800AFAAC,((0x800AFAC8,9),(0x800AFAD8,8)),9),
}
TEXT=(
    (0x8007ccec,0x74,"e803319801b63d590d062827a4189c8c0be83e81636ceee74beaaaa152171d8d"),
    (0x800ada64,0x74,"21543f4988daa858a9077a3022bbb65facde769f04ffa5cf630e1e1d85c60b29"),
    (0x800af82c,0x74,"e4e63af46219f17556f18908a0948719cb3217aa152870f8897adf27058f6e45"),
    (0x8007ce84,0x19c,"d97894a7f0172bb6ff887483898be7c847d7ff5bfab4cae54cabb5d777a9f977"),
    (0x800adb78,0xc0,"a7efb8641647ad7e30972f3194d23173fcf64d560239d9bd84109c3118f2f673"),
    (0x800af9c0,0x1a0,"4d313c9503fbc205a6c44764ab6aefd4d32c4bc8499a1c4c07462acc1134b274"),
    (0x800ae56c,0xc4,"21c2e06ae64dc45d0599f60a1d44b629c815acf371fe5fb65b8676f341d60ebd"),
)
FAMILY="type-0"
DEBIT=0x8006DBE0
FACTORY=0x80082824
MUZZLES={3:(7,(0.,0.,40.)),7:(11,(0.,-9.979999542236328,83.27999877929688)),9:(6,(0.,0.,41.))}


def verify_source(dol):
    for at,size,digest in TEXT:
        if sha256(dol.read(at,size,"text")).hexdigest()!=digest:
            raise ValueError(f"Tank B-shot text signature: {at:08X}")


def close_or_far_b_source(linker,model,actions,joints):
    config,context,handler,roots,callback,class_table,relay,table,phases,debit,calls,key=ROUTES[model]
    profiles=actions["profiles"]
    if (len(profiles)!=1 or any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")) or
            linker.read_word(class_table)!=relay or tuple(linker.read_word(table+4*i) for i in range(len(phases)))!=phases):
        raise ValueError(f"Tank {model:04X}: exact normal class or phase table")
    p=profiles[0]
    if ((p["config"],p["context_table"],p["main_handler"],tuple(int(s,16) for s in p["program_and_descriptor_roots"]))!=
            (config,context,handler,roots)):
        raise ValueError("Tank exact normal installer owner")
    for group in ("B-shot","B-close"):
        rows=[r for g in p["groups"] if g["group"]==group for r in g["rows"] if (0,0) in map(tuple,r["contexts"])]
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(
                0,0,44,f"{callback:08X}"):
            raise ValueError("Tank exact near and far firing contexts")
    def calls_target(site,target):
        instruction=linker.text(site)
        return instruction&0xFC000003==0x48000001 and direct_branch(site,instruction)==target
    if not calls_target(debit,DEBIT) or any(
            linker.text(site-4)!=(0x38800000|value) or not calls_target(site,FACTORY)
            for site,value in calls):
        raise ValueError("Tank exact paid type-0 release")
    muzzle=linker.origins[FAMILY][key]
    joint,point=MUZZLES[key]
    if ((muzzle["source_joint_selector"],tuple(muzzle["local_point"]),muzzle["basis"])!=(joint,point,"credited-actor-joint") or
            not joint<len(joints)):
        raise ValueError("Tank exact muzzle does not bind")
    return {"model":f"{model:04X}","raw_context":[0,0],"callback":f"{callback:08X}","firing_phase_entry":f"{phases[-1]:08X}",
            "debit":{"site":f"{debit:08X}","channel":0,"cost":1,"commit":True,"same_input_actor":True},
            "muzzle":{"key":key,"joint":joint,"point":point},
            "indexed_release_calls":[{"site":f"{site:08X}","key":which} for site,which in calls],
            "scope":"native single shot, no turret retargeting, branch selection, burst, or special weapons",
            "live_equivalence_verified":False}


def battle_test(model,joints,role_clips,pace,level,source,sfx,*,launch_audio=False):
    from tools.analyze.retail_basic_shot_test import pilot_launch_tone, whole_body_pose

    roles={name:(g,c) for name,g,c in role_clips}
    required=("ground-stop","ground-travel","air-continuation","ground-dash-forward")
    if (source["model"]!=f"{model:04X}" or tuple(roles.get(r) for r in required)!=((0,25),(0,26),(0,24),(0,2)) or
            not pace):
        raise ValueError("Tank pilot needs its exact indexed movement poses")
    key=ROUTES[model][-1]
    joint,point=MUZZLES[key]
    if source["muzzle"]!={"key":key,"joint":joint,"point":point} or source["debit"]!={
            "site":f"{ROUTES[model][-3]:08X}","channel":0,"cost":1,"commit":True,"same_input_actor":True}:
        raise ValueError("Tank pilot requires its exact muzzle and channel debit")
    hp,initial,limit,mode,_=level[:5];walk=pace["native_units_per_tick_x100"]
    if not 1<=initial==limit<=255 or mode!=0 or not 100<=walk<=1042 or not 1<=hp<=10000:
        raise ValueError("Tank pilot HP, stock or pace domain")
    lines=["GFBORG-GAMEPLAY 1","# Authored single tank shot; not retail burst or turret logic.",
           "variant=normal",f"health={hp}",f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
           "rig-prefix=visual/body.prefix","pose-profile=visual/body.pose","body-record=0"]
    lines.extend(f"{field}={roles[role][0]}:{roles[role][1]}"
                 for field,role in zip(("idle","walk","air","dash"),required,strict=True))
    local="|".join(f"{x:.9g}" for x in point)
    lines.extend((f"stock=shot-stock|{limit}|180","projectile=shot|bullet|1|43|0|0|0|24|85|8|ignore-floor",
                  f"origin=shot|{joint}|{local}|target-height","move=basic-shot|b|10|grounded|24|none",
                  "entry-stock=basic-shot|shot-stock|1","release=basic-shot|8|shot-stock|1|shot"))
    sounds=[]
    if launch_audio:
        bank,tone=pilot_launch_tone(sfx)
        if (bank,tone)!=("snd_com01",6):
            raise ValueError("Tank pilot did not join its exact launch tone")
        path=f"audio/launch-{bank}-{tone}.pcm"
        lines.append(f"launch-sound=shot|{path}")
        sounds.append({"projectile":"shot","bank":bank,"tone":tone,"path":path})
    return ("\n".join(lines)+"\n").encode("ascii"),whole_body_pose(joints),{
        "kind":"authored normal tank single shot, retains locomotion","retail_behavior_verified":False,
        "preview_clip":"none","body_binding":"strict record zero",
         "source_level":1,"source_normal_factory_family":FAMILY,"source_normal_factory_key":key,
         "reaction_releases":{"basic-shot":[(FAMILY,key)]},
        "source_debit_site":source["debit"]["site"],"native_timing":{"release_tick":8,"move_ticks":24,"refill_ticks":180},
        "authored_launch_sounds":sounds}
