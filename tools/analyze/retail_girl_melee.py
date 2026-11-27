"""Reviewed normal close-B openings; not the source combo or weapon system."""
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program


ROUTES={
    0x800C08A8:(0x800C08F0,0x800C086C,{"80302A40":("80302C90",0x80301D20,0x803024C0,0x8030216C)}),
    0x801345BC:(0x80134604,0x80134580,{"80336E40":("80337090",0x803371E0,0x80337A40,0x80337636),
                                     "80389D48":("80389EF0",0x80389120,0x80389820,0x8038941A)}),
    0x8012F5F8:(0x8012F640,0x8012F5BC,{"80333BD0":("80333F50",0x803340A0,0x803348E0,0x803344F4),
                                     "80333CE8":("80333F68",0x803340A0,0x803348E0,0x803344F4)}),
    0x801BA378:(0x801BA3C0,0x801BA33C,{"80382798":("80382988",0x803829A0,0x80383100,0x80382D64)}),
    0x801D44B8:(0x801D4500,0x801D447C,{"8038BB60":("8038BCF0",0x8038B2E0,0x8038B860,0x8038B67A)}),
    0x801072B4:(0x801072FC,0x80107278,{"80322F40":("803231F0",0x80322060,0x803228A0,0x80322548)}),
}
MODELS={
    0x0300:(0x800C08A8,6),0x030B:(0x800C08A8,6),
    0x0304:(0x801345BC,6),
    0x0305:(0x8012F5F8,5),0x0309:(0x8012F5F8,5),0x030A:(0x8012F5F8,0),
    0x0307:(0x801BA378,5),0x030D:(0x801BA378,0),
    0x030C:(0x801D44B8,0),
    0x0301:(0x801072B4,6),
}
MISSING=0x0308  # same Killer/Shadow descriptor, but the exact local 1:0 slot is absent
AUTHORED_ACTION_LOOP=0x0301
TEXT=(
    (0x800C08A8,0x6C,"e6da6921f6ed13fa3924567889785dfce6b403ae4151c213fe6f149d1083bf93"),
    (0x801345BC,0x6C,"94359d3183a6cb1adb72f3deba47f16a0a845a201ca77d9f0b214f992595c76d"),
    (0x8012F5F8,0x6C,"01321f1a8c1ae03e037cb0b06f716dae9868a77921465782dcbc24da880e17b9"),
    (0x801BA378,0x6C,"fd8c5fb13431616f1a56a13326723d708545cd42f3bd2c56136cccd281555837"),
    (0x8010B2F4,0x210,"c2c13f777af5b7cdacd1c5e4fc4c7d1cb2ef225d15562d42bafde94d6d760618"),
    (0x801D44B8,0x6C,"28c212ba8e9dcd012adc38046400b7831f26ed185cb1438a54b17b3a1b0fda68"),
    (0x801072B4,0x6C,"567147e6a82792596e32b6dbe563799c6f61426b8a4fd7ebb90ac16b87e3f27d"),
)
DATA=(
    (0x80325A68,8,"bd33fadb0ba244fc578606ebecd5d81617e49db2cf4dc836d2b1b7188a6dbf73"),
    *((a,0x20,"cbcc3fc2cc9d76861d4603281774a12bfdc11da37e44e44ed6c8bb01d2351363") for a in
      (0x8030216C,0x803344F4,0x80382D64,0x80322548)),
    *((a,0x24,"df6b41154d2789fe5ae4e4ce24748370059292f528d4d752be34f418102cbdb1") for a in
      (0x80337636,0x8038941A)),
    (0x8038B67A,0x20,"beeb07bb1130719e0876b8ec23f1559db2b19d76fbcce35c36a02b0ca3196ecb"),
)


def verify_source(dol):
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"Girl-family close-B {kind} signature: {at:08X}")


def default_hands(model):
    _,left=MODELS[model] if model in MODELS else (0,6)
    return [(7,1),(11,left)] if left else []


def close_b_pose(linker,model,actions,slots,parts,*,publish_preview):
    callback=MODELS[model][0] if model in MODELS else 0x801345BC
    relay,handler,configs=ROUTES[callback]
    profiles=actions["profiles"]
    if (len(profiles)!=len(configs) or {p["config"] for p in profiles}!=configs.keys() or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")) or
            linker.leaf(callback,0x581,0)!=relay or
            tuple(linker.leaf(0x8010B2F4,0x540,i) for i in range(2))!=(0x8010B344,0x8010B400)):
        raise ValueError("Girl-family close-B installer or phase scope")
    hands=[(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if hands!=default_hands(model):
        raise ValueError("Girl-family close-B exact hand records")
    local={(g,c):at for g,c,at in slots}
    offset=local.get((1,0))
    if (offset is None)!=(model==MISSING):
        raise ValueError("Girl-family close-B exact clip presence changed")
    bindings=[]
    for p in profiles:
        context,program_root,descriptor_root,script=configs[p["config"]]
        roots=(program_root,0x802BC080,descriptor_root,0x802BAE60)
        if (p["context_table"],int(p["main_handler"],16),tuple(int(v,16) for v in p["program_and_descriptor_roots"]))!=(context,handler,roots):
            raise ValueError("Girl-family close-B route owner")
        if linker.table_choice(program_root,3,0)!=script:
            raise ValueError("Girl-family close-B program owner")
        program=decode_program(linker.data,script,max_commands=12,max_span=64)
        if program["status"]!="terminal_wait" or program["commands"][0]["raw"]!="0103000000000000":
            raise ValueError("Girl-family close-B opening select")
        [initial]=linker.program_candidate(roots,local,(15,3,0))["clip_candidates"][:1]
        lanes=[(l["lane"],l["bank"],l["group"],l["choice"],l["archive_offset"]) for l in initial["lanes"] if l["lane"] in (0,1)]
        if ((initial["command_index"],initial["descriptor_choice"],initial["preceding_waits"],initial["start_frame"])!=(0,0,0,0) or
                lanes!=[(0,"local",1,0,offset),(1,"local",1,0,offset)]):
            raise ValueError("Girl-family close-B local lane binding")
        rows=[r for g in p["groups"] if g["group"]=="B-close" for r in g["rows"] if (0,0) in map(tuple,r["contexts"])]
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(
                1,0,44,f"{callback:08X}"):
            raise ValueError("Girl-family close-B exact neutral context")
        bindings.append((rows[0],script))
    evidence={"model":f"{model:04X}","raw_context":[0,0],"default_part_candidates":hands,"live_contact_verified":False,
              "class_callback_chain":[f"{callback:08X}",f"{relay:08X}","8010B2F4","8010B344","8010B400"],
              "neutral_contact_relay":f"{relay:08X}",
              "neutral_program_seed":{"actor_byte":"6EA","value":0,"request_site":"8010B478"},
              "program_addresses":sorted({f"{script:08X}" for _,script in bindings})}
    if offset is None:
        return {**evidence,"status":"exact local 1:0 slot absent; no substitution"}
    pose={"clip":[1,0],"archive_offset":offset,"program":[3,0],"descriptor":[3,0],"request_mask":15,
          "entry":"8010B400","request_site":"8010B478","source_start_frame":0}
    if publish_preview:
        for row,_ in bindings:
            row["entry_clip_candidate"]=pose
    return {**evidence,"pose":pose,"scope":"local lanes 0/1 opening only; authored single native lane"}
