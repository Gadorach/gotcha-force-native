"""Exact neutral B openings for Time Bomber, Beta, and Tau."""
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program


TIME,REMOTE,BETA,TAU=0x0106,0x0107,0x0109,0x0807
# Remote shares Time Bomber's installer route but has its own body and pose.
MODELS={TIME,REMOTE,BETA,TAU}
BOMBER_CONFIGS={
    ("80335E30","80336310"):True,("80335F48","80336328"):False,
    ("80336060","80336310"):False,("80336060","80336328"):False,
}
# callback, relay, shared dispatch, init, clip request, seed, program, choice,
# first select index, main handler, local program/descriptor roots.
ROUTES={
    TIME:(0x80131598,0x801315D4,0x801613FC,0x8016144C,0x801615F8,0,0x8033525C,0,0,
          "80130788",0x80334E20,0x80335580),
    BETA:(0x801D88F8,0x801D8934,0x801C9004,0x801C9054,0x801C911C,2,0x8038E952,1,1,
          "801D88BC",0x8038E620,0x8038ECC0),
    TAU:(0x801A0658,0x801A0694,0x800FED6C,0x800FEDBC,0x800FEEA4,0,0x80377E40,1,1,
         "801A061C",0x80377B80,0x80377FC0),
}
TEXT=(
    (0x80131598,0x6C,"ea11d75c8ab24be47dde83d49eeeb36c426449be0ed5d902846e8d7d5f8e96ba"),
    (0x801613FC,0x200,"5c90cbfc684d78e78b6209caf88fa50889d96dc892de4e642c89cdd560587692"),
    (0x801D88F8,0x70,"710796fc3cee7c0ec6e221287d02346e5fc65ed4275a0273193679bc4453320d"),
    (0x801C9004,0x128,"70d99dfddf6dc72bb8c40e82403dec17181f7d6ce5f2f78b526c0eb8cf90d698"),
    (0x801A0658,0x70,"a7a53c3b569c7d7dbaddad87e55bacb30110aa09d0f1a41d159186097e4bd641"),
)
DATA=(
    (0x8033525C,0x1C,"d26a0ef28c30c15fba6c6ead7b14c8aec58d279ba38ddd2da9c935ed28ad4805"),
    (0x8038E952,0x4C,"afb4a8e5b383119624c6cf3032eaf1e0fd1fcf7004559e69bd21edf7c4c6add7"),
    (0x80377E40,0x38,"8c5fb911e19964385c3c53ccb4c646b064c344fa85afe2f01152d4bc3313f07a"),
    (0x80377868,4,"df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119"),
    (0x8038E0F8,2,"fcf0a6c700dd13e274b6fba8deea8dd9b26e4eedde3495717cac8408c9c5177f"),
)


def verify_source(dol):
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            if sha256(dol.read(at,size,kind)).hexdigest()!=digest:
                raise ValueError(f"Bomber/Hero close-B {kind} signature: {at:08X}")


def default_hands(model):
    return [(7,1),(11,2)] if model in (TIME,REMOTE) else []


def close_b_pose(linker,model,actions,slots,parts,*,publish_preview):
    family=TIME if model==REMOTE else model
    callback,relay,phase,entry,request,seed,script,choice,first,main,program,desc=ROUTES[family]
    configs=BOMBER_CONFIGS if family==TIME else {
        ("8038E228","8038E3D0") if family==BETA else ("80377918","80377AA8"):True}
    if (any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")) or
            len(actions["profiles"])!=len(configs) or
            {(p["config"],p["context_table"]) for p in actions["profiles"]}!=configs.keys() or
            linker.leaf(callback,0x581,0)!=relay or linker.leaf(phase,0x540,0)!=entry or
            linker.table_choice(program,3,seed)!=script):
        raise ValueError("Bomber/Hero close-B exact installer or phase scope")
    if ((family==TIME and (linker.text(relay+8)!=0x38800000 or linker.leaf(phase,0x540,1)!=0x80161508)) or
            (family==BETA and int.from_bytes(linker.data(0x8038E0F8,2),"big")!=seed) or
            (family==TAU and linker.read_word(0x80377868)!=seed)):
        raise ValueError("Bomber/Hero close-B exact opening seed")
    commands=decode_program(linker.data,script,max_commands=24,max_span=112)
    if commands["status"]!="terminal_wait":
        raise ValueError("Bomber/Hero exact close-B opening program")
    local={(g,c):at for g,c,at in slots};offset=local.get((1,choice))
    hands=[(p["joint"],p["record"]) for p in parts["default_candidates"]]
    if offset is None or hands!=default_hands(model):
        raise ValueError("Bomber/Hero exact local clip or hand records")
    roots=(program,0x802BC080,desc,0x802BAE60)
    choices=linker.program_candidate(roots,local,(15,3,seed))["clip_candidates"]
    opening=[c for c in choices if (c["command_index"],c["descriptor_choice"],c["preceding_waits"],c["start_frame"])==(first,0,0,0)]
    if len(opening)!=1 or [(l["lane"],l["bank"],l["group"],l["choice"],l["archive_offset"])
                            for l in opening[0]["lanes"] if l["lane"] in (0,1)]!=[(0,"local",1,choice,offset),(1,"local",1,choice,offset)]:
        raise ValueError("Bomber/Hero exact close-B lane")
    pose={"clip":[1,choice],"archive_offset":offset,"program":[3,seed],"descriptor":[3,0],"request_mask":15,
          "entry":f"{entry:08X}","request_site":f"{request:08X}","source_start_frame":0}
    for p in actions["profiles"]:
        if (p["main_handler"],tuple(int(r,16) for r in p["program_and_descriptor_roots"]))!=(main,roots):
            raise ValueError("Bomber/Hero close-B program owner")
        rows=[r for g in p["groups"] if g["group"]=="B-close" for r in g["rows"] if (0,0) in map(tuple,r["contexts"])]
        if not configs[p["config"],p["context_table"]]:
            if rows:
                raise ValueError("unexpected alternate Bomber close-B route")
            continue
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate"))!=(
                1,0,44,f"{callback:08X}"):
            raise ValueError("Bomber/Hero close-B exact neutral row")
        if publish_preview:
            rows[0]["entry_clip_candidate"]=pose
    return {"model":f"{model:04X}","raw_context":[0,0],"pose":pose,"default_part_candidates":hands,
             "program_addresses":[f"{script:08X}"],
             "neutral_program_seed":{"value":seed,"actor_byte":"6EA"},"live_contact_verified":False,
             **({"neutral_contact_relay":f"{relay:08X}"} if family==TIME else {}),
            **({"status":"strict normal body refuses material/texture-matrix stages"} if model==REMOTE else {})}
