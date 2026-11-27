"""Exact normal close-B pose evidence for Teleport and Switching Ninja."""
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.retail_dragon_art import record_receipts


SHURIKEN,FLAME=0x0001,0x0006
MODELS={SHURIKEN,0x0005,FLAME,0x0009}
PROFILES={"8033E6B0":"8033EAD8","8033E7C8":"8033EAF0"}
ROOTS=(0x8033D5C0,0x802BC080,0x8033DE20,0x802BAE60)
PROGRAM=0x8033DB7E
TEXT=(
    (0x80146560,0x84,"44c12d8c3fcc4db65d06360d1225440f952e1ae09c530d7001133ac882f46441"),
    (0x800FED6C,0x158,"40e126a9bd8621246da0ef6e81ce07f1b448b06d7c3f018d2f3a56b74cccb103"),
    (0x800B411C,0x7C,"21994b21b559f1bcde3dd1d53e8889155e7d4e7358a3688c9ac5848c811adeb0"),
    (0x800EB288,0xC8,"b16fc450e2f488ca1fc401adee8c2e378e65cbd9f8f78ea6da6cbedf225dcca1"),
)
DATA=(
    (0x8033E53C,0x2C,"705a905fba643b053074b2a9c6a460661636a3a38608e685cb07ecf69f332fbe"),
    (0x8031EFF0,0x14,"6931b41e06e4efaba8cbfa0ac3c248972026eccdeaab813f145d79e9e4da5957"),
    (PROGRAM,0x4C,"a4246486d966f17e03edfdbc37a52a8226fc3a41481b3e093a52438afbecf694"),
    (0x802FD30C,4,"221f8af2372a95064f2ef7d7712216a9ab46e7ef98482fd237e106f83eaa7569"),
    (0x802FCC88,0x48,"d1e480fb3a7818b63237eec7ffa41cf0833beb1af920311eea2ae8fd8353eda0"),
    (0x8031AAD4,0x40,"7bb593f8e8fafe582900429835c9c728390cc877b567eda1b489be6e0b9653d1"),
)


def verify_source(dol):
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"Ninja close-B {kind} signature: {at:08X}")


def close_b_pose(linker,model,actions,slots,parts,*,publish_preview,body=None):
    flame=model==FLAME
    configs,roots,main,callback,relay,seed_at,seed,script,first_event,select=(
        {"802FD450":"802FD6A0"},(0x802FC7E0,0x802BC080,0x802FCEC0,0x802BAE60),
        "800B31A8",0x800B411C,0x800B4158,0x802FD30C,5,0x802FCC88,"0a0500ff","0103000204000000"
    ) if flame else (
        PROFILES,ROOTS,"80145924",0x80146560,0x801465B0,0x8033E550,1,PROGRAM,"0a0200ff","0103000104000000"
    )
    if model==SHURIKEN:
        configs,roots,main,callback,relay,seed_at,seed,script,first_event,select=(
            {"8031AD70":"8031AF78"},(0x8031A620,0x802BC080,0x8031A120,0x802BAE60),
            "800EA73C",0x800EB288,0x800EB328,0x8031B130,0,0x8031AAD4,"0a0400ff","0103000104000000")
    profiles=actions["profiles"]
    if (model not in MODELS or len(profiles)!=len(configs) or {p["config"] for p in profiles}!=configs.keys() or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores"))):
        raise ValueError("Ninja close-B installer scope")
    neutral_relay=linker.read_word(0x8031B11C) if model==SHURIKEN else linker.leaf(callback,0x581,0)
    if (neutral_relay!=relay or linker.leaf(0x800FED6C,0x540,0)!=0x800FEDBC or
            linker.read_word(seed_at)!=seed or linker.table_choice(roots[0],3,seed)!=script):
        raise ValueError("Ninja close-B seed or phase table")
    program=decode_program(linker.data,script,max_commands=24,max_span=96)
    commands=program["commands"]
    if (program["status"]!="terminal_wait" or [c["raw"] for c in commands[:2]]!=[first_event,select]):
        raise ValueError("Ninja close-B opening program")
    local={(g,c):offset for g,c,offset in slots}
    offset=local.get((1,1))
    choices=linker.program_candidate(roots,local,(15,3,seed))["clip_candidates"]
    if offset is None or len(choices)!=1:
        raise ValueError("Ninja close-B local clip")
    selected,=choices
    lanes=[(l["lane"],l["bank"],l["group"],l["choice"],l["archive_offset"]) for l in selected["lanes"] if l["lane"] in (0,1)]
    if ((selected["command_index"],selected["descriptor_choice"],selected["preceding_waits"],selected["start_frame"])!=(1,0,0,0) or
            lanes!=[(0,"local",1,1,offset),(1,"local",1,1,offset)]):
        raise ValueError("Ninja exact close-B clip binding")
    bindings=[]
    for p in profiles:
        if (p["context_table"],p["main_handler"],tuple(int(v,16) for v in p["program_and_descriptor_roots"]))!=(
                configs[p["config"]],main,roots):
            raise ValueError("Ninja close-B route owner")
        rows=[r for g in p["groups"] if g["group"]=="B-close" for r in g["rows"] if (0,0) in map(tuple,r["contexts"])]
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(
                1,0,44,f"{callback:08X}"):
            raise ValueError("Ninja close-B raw context")
        bindings.append(rows[0])
    hands=[(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if hands!=[(7,1),(11,4)]:
        raise ValueError("Ninja exact default hand records")
    empty_hands=[]
    if flame:
        if body is None:
            raise ValueError("Flame Ninja needs its own hand-record archive")
        receipt=record_receipts(body,{1,4})
        if not all(entry["empty_static"] for entry in receipt["records"].values()):
            raise ValueError("Flame Ninja normal hand records are not empty")
        empty_hands=[1,4]
    pose={"clip":[1,1],"archive_offset":offset,"program":[3,seed],"descriptor":[3,0],"request_mask":15,
          "entry":"800FEDBC","request_site":"800FEEA4","source_start_frame":0}
    if publish_preview:
        for row in bindings:
            row["entry_clip_candidate"]=pose
    return {"model":f"{model:04X}","raw_context":[0,0],"pose":pose,"default_part_candidates":hands,
            "program_addresses":[f"{script:08X}"],
            **({"empty_default_hand_records":empty_hands,"hand_record_receipt":receipt} if flame else {}),
            "neutral_program_seed":{"address":f"{seed_at:08X}","value":seed,"actor_byte":"6EA"},
            "upper_controls":[{"site":f"{op['address']:08X}","raw":op["raw"]} for op in commands if op["kind"]=="upper_controls"],
            "live_contact_verified":False,"excluded_moves":["ranged B","X","charged X"]}
