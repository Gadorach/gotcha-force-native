"""Review one close-B opening without running the source class or damage code."""
from hashlib import sha256
import struct

from tools.analyze.gred_animation_controller import decode_program
from tools.analyze import retail_samurai_melee as samurai
from tools.analyze import retail_ninja_melee as ninja
from tools.analyze import retail_girl_melee as girl
from tools.analyze import retail_blue_close_b as blue
from tools.analyze import retail_robot_diver_melee as robots
from tools.analyze import retail_wing_melee as wings
from tools.analyze import retail_bomber_hero_melee as bombers
from tools.analyze import retail_grounded_close_b as grounded


MODELS = {0x0200:0,0x020A:1,0x020F:3} # model -> independent source child row
AXES = {0x0204:0,0x020D:1}
LANCES = {0x0208,0x020E}
IMPACT = {0x0203,0x020C}
ELEMENTAL = {0x0202,0x020B}
SHARED_NEUTRAL = {
    0x0206:("80371378","80371508","8019C2A8",(0x803716C0,0x802BC080,0x80371D80,0x802BAE60),
            0x8019C2E4,0x8019C338,0x80371A96),
    0x0207:("802FFED8","80300128","800BA418",(0x802FF460,0x802BC080,0x802FFB40,0x802BAE60),
            0x800BB004,0x800BB040,0x802FF8A6),
}
CHAINSAW=0x0201
REVIEWED = MODELS.keys()|AXES.keys()|LANCES|IMPACT|ELEMENTAL|SHARED_NEUTRAL.keys()|{CHAINSAW}
PILOTS = REVIEWED|samurai.MODELS.keys()|ninja.MODELS|girl.MODELS.keys()|blue.MODELS|robots.MODELS|wings.MODELS.keys()|bombers.MODELS|grounded.MODELS
PROFILES = {"802D4920","802D4A38"}
ROOTS = (0x802BFF80,0x802BC080,0x802BFC20,0x802BAE60)
PROGRAM = 0x802C0376
TEXT = (
    (0x800743D0,0x78,"6d33cb2a9a804301e3a2178ad87cb488d4e19a8a5e71e48a1f098da242cf64cf"),
    (0x8015809C,0x158,"b7c1bcf346d2fe88f2ef866c55791008cb66749e90c3cfdcf0ff8ef38620862e"),
    (0x80073C90,0x64,"2cebdb8af23a13e1d577da36ed3cd923acb1d7c906ce5dc26274d28766d65204"),
    (0x8004E00C,0x174,"29324f19c14331c661277ed109b1b85f82f6f4e8947b0e10b17a6b992d7f8583"),
    (0x8004E180,0x158,"290dc24f315843ff5693acc57d4459f15a6efddf3d7a6d7e92c047da4230a2f7"),
    (0x8004E2D8,0x78,"c489804e0a04ccbf9ef61d2b10424f8ccc6e6183cd593eefc5744fac9ce69397"),
    (0x8004E350,0x17C,"18f7531f28bed216ff86251a70ccd64b0acde4a04dc4c9b8e73bbfe173c011e5"),
    (0x80047C38,0x194,"0ce9475eaf83b9bf7cf95a78bdefa20b866b2029a34aade03342e23e709ec603"),
    (0x8004E518,0x68,"e7c8dfe0326e57fc7c53d59acac3c42e5c784cff41e5dce440d304aa038c9ff4"),
    (0x800D73D0,0x78,"14d711f980e9cf0f2c4465637f7bfbd43d9e38b4b3685c16a1131e77ee2ab904"),
    (0x800D6E0C,0x44,"6bd3e1ca75069d39c988e70e4d071b014f68a16e467c4572489949f64b4b2cd1"),
    (0x800BF40C,0x124,"9cbd23485118a0ecb166b438063e111edde6506573818625ba71a4033ca70ea5"),
    (0x800BF584,0x138,"a2751b3f5aa4a62240f2aaffc972bcecfb63a051a24094446cbac6f6c5b7e4dc"),
    (0x800BF6BC,0x158,"4d9650a619091a1b863aa99bf3c5f320297542e3640450ca9d97eacb3207a00c"),
    (0x800BF844,0x8C,"2308f3abb67bffbc9c06a0f506b6aaf915630ddb2257a1e882ac76127d18d539"),
    (0x800BC5D0,0x3C,"4ac0ffe8d29b60bc091fd93d4641721e9debd2f1bfc8db22ed6e933792b2a998"),
    (0x800BD008,0x24,"64f33020cd4db4ee86af6a49f63d1a002b87b7928d4b8a640ceaaa7a34ee58d9"),
    (0x800E5920,0x60,"dcff9c74ac6a3cb70cc58c16b0055b2ebffe69ff473253e7c7514f5b7dc23e63"),
    (0x80123D70,0xB0,"a2e68ea8a65b5720c4ced1110366d64938d98dc551506562acee5d1c8cb55b96"),
    (0x80123E20,0xE8,"bcfaa083748fd45d048e615519b38a0dc0cd9a0fc791e1a6fbb9ae76f3f9c936"),
    (0x8019C2E4,0x78,"13b1a8cc0eacdecae70e1b8f49ba046d7bdaa70eaefbc90bfd5aa52bf595549e"),
    (0x800BB004,0x60,"d73d89efa23960b3e0366a582b734880dbc93ba14dbdd5e24f58b54c1f79a9a7"),
    (0x800C8720,0x1B8,"96fe505fb13a46b49a5aac80c5102276d9afadeee1a083e6849af8a24df56e41"),
)
DATA = (
    (0x802D4844,0x14,"5d14274e5f91c1b2cb9bf123497108b7857fe96d356abb392c87f25029b1bd79"),
    (0x80343C00,0x14,"96612e0e81faf6f1179e0d895396e262594071778da339d0465161b12bbe01ee"),
    (PROGRAM,0x48,"7904a6a97fc77acc03dc9cf97fa9075d141beb2897a30a11f6c90bc9918cb598"),
    (0x802D0FF0,0x90,"b4ee226396850152d391f6ccbbe05f46f99626334e464ef2115c80bc05a1de95"),
    (0x802D1080,0x0C,"0e65e7d8dbe6154a35315e2925e6fb2c35d24aeef5c5b10b94d435ef8cd0d105"),
    (0x8030FAEC,0x14,"613dfb3430324012914eabcd5897c7ce3fb610b00feb18c59097020e4b4920dd"),
    (0x8030F3EE,0x48,"cb01f6c4a87c6e2fb1f3067f82aafb1ac470f3b0d634056651a86924217a1536"),
    (0x80301C58,96,"a5bf2b68d3efb5482855f962763f59762454614c72ea85f51c596189180ea30b"),
    (0x804385C0,4,"de620abd6d3615746360c1d15ce3a56291236d37424124054a49d25e947ffc4d"),
    (0x8030197C,0x18,"c0bcee62ba602a64fe34011fa87ebf5c84ec515b24d93da7d1b2a42bc0facc32"),
    (0x803008F0,0x50,"eaca36db51ba1b9dfcab508be7cc0f9c3881b3e43ead986cb681f4a178af245d"),
    (0x803188C0,0x18,"d42e51d52fcc5a0a5575ef8ed0120b77436169cdd854c1b494d1a207df0963c9"),
    (0x8032E3A4,0x2C,"ab3f5150b72b1bdc39c473e89ab7f126de19e295a4263f0392fd99574a55089a"),
    (0x80317E74,0x5C,"a826a4fd3a090f88d7f464bfb999e7d5e1dccf1552d6af659caae55e2a3f4f9a"),
    (0x8032DDE8,0x54,"700ba5de21f6f282a98cc6dcbcb3f3683291d2256c857ebe8c54be922a618881"),
    (0x80371A96,0x5C,"a826a4fd3a090f88d7f464bfb999e7d5e1dccf1552d6af659caae55e2a3f4f9a"),
    (0x802FF8A6,0x5C,"a826a4fd3a090f88d7f464bfb999e7d5e1dccf1552d6af659caae55e2a3f4f9a"),
    (0x80308B70,0x34,"cc2541fcfd34eef62a4edc5e4fcac62efa12fd3a3e7536aa82e97d89fcaacd6f"),
)


def verify_source(dol):
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"Knight close-B {kind} signature: {at:08X}")


def gamma_sword_placement(row):
    if len(row)!=36:
        raise ValueError("Gamma II exact sword row extent")
    record,joint,group = struct.unpack_from(">HBB",row)
    default,active = struct.unpack_from(">3f",row,4),struct.unpack_from(">3f",row,16)
    if ((record,joint,group)!=(1,10,8) or
            default!=(45.18000030517578,0.,0.) or default!=active or struct.unpack_from(">2h",row,28)!=(0,0)):
        raise ValueError("Gamma II exact sword row")
    return {"record":record,"joint":joint,"local_point":list(default),"transform_flags":8,
            "point_site":"8004E47C","basis":"world axes; does not copy the joint matrix",
            "roll":{"source":"credited actor +74","add_binary_angle":-16384,"site":"8004E498"},
            "model_matrix_site":"8004E55C","live_verified":False}


def axe_child_parts(linker,model):
    key = AXES[model]
    rows=[]
    for sub in range(2):
        addr=0x80301C58+key*48+sub*24
        row=linker.data(addr,24)
        record,joint,x,y,z,rx,ry,rz,pad=struct.unpack(">hh3f4h",row)
        expected=((8,7,-10.,-5.,0.,0,0,0,0),(9,13,0.,0.,-22.,16384,0,16384,0))[sub]
        if (record,joint,x,y,z,rx,ry,rz,pad)!=expected:
            raise ValueError("Axe exact child mount row")
        rows.append({"record":record,"joint":joint,"point":[x,y,z],"binary_angles":[rx,ry,rz],"raw_row":row.hex(),
                     "at":f"{addr:08X}"})
    return {"factory":"800BF40C","key":key,"parts":rows,"transform_flags":95,
            "draw_gate":{"callback":"800BF844","requires_positive_scale":True,"initial_scale":1.0},
            "live_visibility_verified":False,"native_policy":"authored fixed visible neutral mount; no source growth or spin"}

def knight_default_hands(model):
    if model==0x020F or model in SHARED_NEUTRAL:
        return []
    return [(7,2)] if model==CHAINSAW else [(7,2),(11,5)]


def close_b_pose(linker, model, actions, slots, parts, *, publish_preview):
    configs,context,callback,relay,program_at,select_raw,start,desc = (
        {"8030FBC8":("800D6FD8",(0x8030EFC0,ROOTS[1],0x8030F740,ROOTS[3]))},
        "8030FE18","800D73D0","800D7424",0x8030F3EE,"0103000208000000",0,0) if model in AXES else (
        {"803013D0":("800BB66C",(0x80300420,ROOTS[1],0x80300CE0,ROOTS[3])),
         "803014E8":("800BB6A8",(0x80300420,ROOTS[1],0x80300CEC,ROOTS[3]))},
         "80301738","800BC5D0","800BD008",0x803008F0,"0103020204000000",0,2) if model in LANCES else (
        dict.fromkeys(PROFILES,("80073F3C",ROOTS)),"802D4C88","800743D0","80074424",PROGRAM,"0103000104000008",8,0)
    context_by_config={}
    entry,request,group,choice,opening="80158114","801581D8",1,1,"0a0100ff"
    chain=[callback,relay,"8015809C"]
    if model in IMPACT:
        roots=(0x80317AC0,ROOTS[1],0x803177C0,ROOTS[3])
        context_by_config={"80318288":"80318650","803183A0":"80318668"}
        configs=dict.fromkeys(context_by_config,("800E55B0",roots))
        callback,program_at="800E5920",0x80317E74
        chain=[callback,"800E595C","8015809C"]
        if linker.leaf(0x800E5920,0x581,0)!=0x800E595C:
            raise ValueError("Spike/Hammer close-B relay")
    elif model in ELEMENTAL:
        roots=(0x8032DA80,ROOTS[1],0x8032E060,ROOTS[3])
        configs={"8032E4B0":("801238BC",roots)}
        context,callback,program_at="8032E700","80123D70",0x8032DDE8
        entry,request,opening,select_raw,start,choice="80123E20","80123EE8","0a0300ff","0103000105000000",0,4
        chain=[callback,"80123DB8",entry]
        if (linker.leaf(0x80123D70,0x581,0),linker.leaf(0x80123DB8,0x540,0))!=(0x80123DB8,0x80123E20):
            raise ValueError("Elemental/Ghost close-B phase chain")
    elif model in SHARED_NEUTRAL:
        config,context,handler,roots,callback_at,relay_at,program_at=SHARED_NEUTRAL[model]
        configs={config:(handler,roots)}
        callback=f"{callback_at:08X}"
        chain=[callback,f"{relay_at:08X}","8015809C"]
        if linker.leaf(callback_at,0x581,0)!=relay_at:
            raise ValueError("Gamma/Alien close-B relay")
    elif model==CHAINSAW:
        configs={"80309CA8":("800C86E4",(0x80308820,ROOTS[1],0x803085C0,ROOTS[3]))}
        context,callback,program_at="80309E98","800C8720",0x80308B70
        entry,request,select_raw,start,choice="800C87E4","800C88AC","0103000104000000",0,4
        chain=[callback,"800C875C",entry]
        if (linker.leaf(0x800C8720,0x581,0),linker.leaf(0x800C875C,0x540,0))!=(0x800C875C,0x800C87E4):
            raise ValueError("Chainsaw close-B phase chain")
    profiles = actions["profiles"]
    if (len(profiles)!=len(configs) or {p["config"] for p in profiles}!=configs.keys() or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores"))):
        raise ValueError("Knight close-B installer scope")
    local = {(g,c):at for g,c,at in slots}
    program = decode_program(linker.data,program_at,max_commands=24,max_span=128)
    prefix = program["commands"][:2]
    if program["status"]!="terminal_wait" or [op["raw"] for op in prefix]!=[opening,select_raw]:
        raise ValueError("Knight close-B opening command")
    bindings = []
    for p in profiles:
        main,roots=configs[p["config"]]
        if ((p["context_table"],p["main_handler"],tuple(int(v,16) for v in p["program_and_descriptor_roots"]))!=
                (context_by_config.get(p["config"],context),main,roots) or linker.table_choice(roots[0],3,0)!=program_at):
            raise ValueError("Knight close-B program owner")
        selects = linker.program_candidate(roots,local,(15,3,0))["clip_candidates"]
        first = [c for c in selects if c["command_index"]==1 and not c["preceding_waits"] and
                 (c["start_frame"],c["descriptor_choice"])==(start,desc)]
        if len(first)!=1:
            raise ValueError("Knight close-B initial select scope")
        lanes = [(l["bank"],l["group"],l["choice"],l["archive_offset"]) for l in first[0]["lanes"] if l["lane"] in (0,1)]
        if len(lanes)!=2 or any(l!=("local",group,choice,local.get((group,choice))) or l[3] is None for l in lanes):
            raise ValueError("Knight close-B opening pose")
        rows = [r for g in p["groups"] if g["group"]=="B-close" for r in g["rows"] if (0,0) in map(tuple,r["contexts"])]
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(
                1,0,44,callback):
            raise ValueError("Knight close-B scope")
        bindings.append(rows[0])
    pose = {"clip":[group,choice],"archive_offset":local[group,choice],"program":[3,0],"descriptor":[3,desc],"request_mask":15,
            "entry":entry,"request_site":request,"source_start_frame":start,"class_callback_chain":chain}
    defaults = [(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if defaults!=knight_default_hands(model):
        raise ValueError("Knight default part slots")
    placement = {}
    if model in MODELS:
        key = MODELS[model]
        row = linker.data(0x802D0FF0+36*key,36)
        record,joint,group = struct.unpack_from(">HBB",row)
        placement["independent_child"]={"key":key,"factory":"8004E00C","record":record,"joint":joint,
                                        "collision_group":group,"raw_row":row.hex(),"bound_in_native_pilot":False}
        if model==0x020F:
            placement["sword_mount"]=gamma_sword_placement(row)
    elif model in AXES:
        placement["axe_parts"]=axe_child_parts(linker,model)
    elif model==CHAINSAW:
        placement["stockless_test_part"]={"record":2,"joint":7,"policy":"static hierarchy; no saw spin or damage"}
    if publish_preview:
        for entry in bindings:
            entry["entry_clip_candidate"] = pose
    return {"model":f"{model:04X}","pose":pose,"raw_context":[0,0],"default_part_candidates":defaults,
            "program_class_commands":[{"site":f"{op['address']:08X}","raw":op["raw"]} for op in program["commands"]
                                      if op["kind"]=="class_event"],
            "live_contact_verified":False,
            **placement}


def melee_battle_test(model,joints,role_clips,pace,level,source,*,clip_metadata=(),mounts=(),body_material=None):
    # Keep the cohort on the existing rig encoder.
    from tools.analyze.retail_basic_shot_test import whole_body_pose
    roles = {name:(g,c) for name,g,c in role_clips}
    required = ("ground-stop","ground-travel","air-continuation","ground-dash-forward")
    g,c = source["pose"]["clip"]
    start = source["pose"]["source_start_frame"]
    gamma = model==0x020F
    axe = model in AXES
    samurai_family = model in samurai.MODELS
    girl_family = model in girl.MODELS
    if model in wings.MODELS:
        hands=wings.default_hands(model)
    elif model in robots.MODELS:
        hands=robots.default_hands(model)
    elif model in bombers.MODELS:
        hands=bombers.default_hands(model)
    elif model in grounded.MODELS:
        hands=grounded.default_hands(model)
    elif girl_family:
        hands=girl.default_hands(model)
    elif samurai_family:
        hands=[(7,1),(11,2)]
    elif model in ninja.MODELS or model in blue.MODELS:
        hands=[(7,1),(11,4)]
    else:
        hands=knight_default_hands(model)
    mount = source.get("sword_mount",{})
    expected=(1,0,0) if girl_family else (1,4,0) if model in ELEMENTAL or model==CHAINSAW else (
        1,1,8 if model in MODELS or model in IMPACT or model in SHARED_NEUTRAL else 0)
    if model in wings.MODELS:
        expected=(1,1,wings.MODELS[model][-2])
    elif model in bombers.MODELS:
        expected=(1,0 if model in (bombers.TIME,bombers.REMOTE) else 1,0)
    elif model in grounded.MODELS:
        expected=grounded.clip(model)
    if (model not in PILOTS or source["model"]!=f"{model:04X}" or (g,c,start)!=expected or
            source["default_part_candidates"]!=hands or len(joints)<=11 or
            (gamma and (mount.get("record"),mount.get("joint"),mount.get("transform_flags"),
                       mount.get("local_point"),mount.get("roll",{}).get("add_binary_angle"))!=
                       (1,10,8,[45.18000030517578,0.,0.],-16384)) or
            not pace or any(role not in roles for role in required)):
        raise ValueError("Knight melee pilot lacks exact pose, locomotion or part slots")
    assembled=model in ELEMENTAL
    if assembled:
        exact_hands=tuple((record,bone,0,0,0) for bone,record in hands)
        if len(mounts)!=15 or mounts[0]!=(17,7,-15,-5,0) or tuple(mounts[-2:])!=exact_hands:
            raise ValueError("assembled Knight needs its exact body and default hand mount rows")
    elif mounts:
        raise ValueError("unexpected assembly for a complete Knight body")
    walk = pace["native_units_per_tick_x100"]
    if not 100<=walk<=1042 or not 1<=level[0]<=10000:
        raise ValueError("Knight melee pilot HP or pace domain")
    lines = ["GFBORG-GAMEPLAY 1","# Authored close-B opening; not retail contact or combo logic.",
             "variant=normal",f"health={level[0]}",f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
             "rig-prefix=visual/body.prefix","pose-profile=visual/body.pose","body-record=0"]
    if body_material:
        lines.append(f"body-material={body_material}")
    loop_fields=("idle","walk") if axe or samurai_family or model==robots.DRILL else ("walk",) if model in robots.MODELS-robots.DIVING else ()
    for field in loop_fields:
        t=clip_metadata[0 if field=="idle" else 1] if len(clip_metadata)==2 else None
        if (not t or t["aobj_flags"]!=[0x20000000] or t.get("strict_metadata_refusals")!=["aobj-flags"] or
                not t["animated_joint_count"] or t["animated_joint_count"]!=t["authored_loop_joint_count"]):
            raise ValueError(f"Melee {field} needs uniformly authored loop flags")
        lines.append(f"{field}-loop=authored-bit")
    for field,role in zip(("idle","walk","air","dash"),required,strict=True):
        a,b = roles[role]
        lines.append(f"{field}={a}:{b}")
    if gamma:
        point = "|".join(f"{v:.9g}" for v in mount["local_point"])
        lines.extend(("part-record=sword|10|1","part-basis=sword|world-zyx",f"part-pose=sword|{point}|0|0|-90"))
    elif assembled:
        for mount in mounts:
            lines.append("body-mount="+"|".join(map(str,mount)))
    elif hands:
        if model==ninja.FLAME:
            if source.get("empty_default_hand_records")!=[1,4]:
                raise ValueError("Flame Ninja may omit only the exact empty defaults")
        else:
            lines.extend(f"part-record={'right-hand' if joint==7 else 'left-hand'}|{joint}|{record}"
                         for joint,record in hands)
        if model==CHAINSAW:
            part=source.get("stockless_test_part",{})
            if (part.get("record"),part.get("joint"))!=(2,7):
                raise ValueError("Chainsaw needs its own static hierarchy")
            lines.append("part-hierarchy=2")
        elif model in robots.TOOLS:
            if source.get("static_left_hand_record")!=4:
                raise ValueError("Robot needs its exact left-hand hierarchy")
            lines.append("part-hierarchy=4")
        elif model in wings.HIERARCHIES:
            if source.get("static_hand_hierarchies")!=[1,5]:
                raise ValueError("Wing/Valkyrie needs its exact static hand hierarchies")
            lines.extend(("part-hierarchy=1","part-hierarchy=5"))
    lines.append(f"move=opening-slash|b|10|grounded|24|{g}:{c}")
    if model==girl.AUTHORED_ACTION_LOOP:
        timing=source.get("strict_clip_metadata",{})
        if (timing.get("aobj_flags")!=[0x20000000] or timing.get("strict_metadata_refusals")!=["aobj-flags"] or
                timing.get("authored_end_frames")!=[38.] or not timing.get("animated_joint_count") or
                timing.get("authored_loop_joint_count")!=timing["animated_joint_count"]):
            raise ValueError("Wire Girl requires the exact bounded authored attack loop window")
        lines.append("pose-loop-window=opening-slash|authored-bit")
    if start:
        lines.append(f"pose-start=opening-slash|{start}")
    lines.extend(("range=opening-slash|150","melee=opening-slash|9|14|88|38|52|44|12"))
    if axe:
        parts=source["axe_parts"]["parts"]
        if [(p["joint"],p["record"]) for p in parts]!=[(7,8),(13,9)]:
            raise ValueError("Axe pilot lacks its own mount records")
        for index,p in enumerate(parts):
            slot=f"axe-{index}"
            local="|".join(f"{v:.9g}" for v in (*p["point"],*(a*360/65536 for a in p["binary_angles"])))
            lines.extend((f"part-record={slot}|{p['joint']}|{p['record']}",f"part-basis={slot}|bone-zyx",
                          f"part-pose={slot}|{local}"))
    return ("\n".join(lines)+"\n").encode("ascii"),whole_body_pose(joints),{
        "kind":"opt-in authored close B; not retail combat","source_level":1,"preview_clip":"b-close-c0-s0",
        "retail_behavior_verified":False,"source_pose":[g,c],"source_start_frame":start,
        "native_timing":{"move_ticks":24,"active_frames":[9,14]},"native_damage":12,
        "native_contact":"stockless body-proxy melee; no weapon collision",
        "visual_policy":("strict body record zero and exact record 1, joint 10 origin, world axes with authored zero actor roll"
                          if gamma else "strict body record zero; exact default hands and visible record 8/9 axe mounts"
                          if axe else "strict same-bank authored neutral body assembly and default hands; no child actor lifecycle"
                          if assembled else "strict body record zero; exact empty hand defaults, no substitute"
                          if model==ninja.FLAME else "strict body record zero and exact authored default hand mounts"),
    }
