"""Bounded ordinary-B Worm evidence, separate from traps and child simulation."""

from hashlib import sha256
import struct


# (family, key, model-gated call)
MODELS = {0x0501:(49,0,0x80119888),0x050B:(94,0,0x801198A0),
          0x050D:(49,1,0x801198B8),0x0517:(94,2,0x801198D0)}
SOURCE_CUES = {49:210,94:260}
# (row table, stride, reviewed prefix count, joint offset, factory, phase zero, bind, cue, owner)
FAMILIES = {
    49:(0x803396B0,44,2,0,0x8013A050,0x8013A1FC,0x8013A380,0x8013A41C,"credited-actor"),
    94:(0x8038CD18,60,3,1,0x801D4D00,0x801D4FA8,0x801D51A0,0x801D51E0,"allocated-child"),
}
TEXT = (
    (0x80119250,0x21C,"01e435d4526d80810f1b2c4484882ef36457c42c4ddf3e08192200cfb9cb06df"),
    (0x80119848,0xA0,"889d442ed12ffff4c391c603a2dba716f2eeb49062158ba33e85a20c9058acac"),
    (0x8006DBE0,0xE8,"ee8f2cd748e39d7be053d7e4b899c0f51cf570fb0cd2707d945b1ebb8cb09134"),
    (0x8013A050,0x11C,"e4a699c547663f1ba2631a7491f288cff97602a9590c9fa45026d272430c6e02"),
    (0x8013A16C,0x2D0,"bbf17e14dd29cbfeaeafd269f753eec377b6989469cf147180f6a933c050fb2d"),
    (0x801D4D00,0x124,"91148920b1d8bde00e621dbb33e72d99f63c956ec7cae662fa30e9ce5eca4bf7"),
    (0x801D4F58,0x2A8,"6031e9fd6faa196847fe1bb4178471aa3defbf689e15749139cf07f9047b961a"),
)
DATA = (
    (0x8032B5E8,0x2C,"5fdf7e3b1778df51d1598782dd2bdf97c2df81453954ef99cd04e694d12a80c6"),
    (0x8032ABDA,0x68,"b4fe3c3ab5590b914563065f46d7c7d950b275a3da117b3935780aa61fa8dcaf"),
    (0x803396B0,88,"00d4e00d03d1af9ff523e26f52bc9baf8470f8808c6b83f841ae9fe6a31472a1"),
    (0x8038CD18,180,"0bc76b66fad2fa1fd69bb9df34b6f13c0dd8f8bba3e3b8fd0dc44469b12eb793"),
    (0x80339708,12,"de15860e95c31400c48ca17dd807e1c80b271255be234325f0b73b5bf8a9482f"),
    (0x8038CE08,12,"17172d05d2e4d3fecd3c2bcf95d17133df646d8f359bf32668aefd36b79bc549"),
)
PROFILES = {"8032B6A8","8032B7C0","8038CE28","8038CF40"}


def worm_source_index(dol, sound_resolver):
    blocks = {}
    for kind,spans in (("text",TEXT),("data",DATA)):
        for at,size,digest in spans:
            raw = dol.read(at,size,kind)
            if sha256(raw).hexdigest()!=digest:
                raise ValueError(f"Worm {kind} signature: {at:08X}")
            if kind=="data":
                blocks[at] = raw
    rows = {}
    for model,(family,key,call) in MODELS.items():
        table,stride,count,joint_offset,factory,phase_zero,bind,cue,owner = FAMILIES[family]
        if key>=count:
            raise ValueError("Worm source row domain")
        row = blocks[table][key*stride:(key+1)*stride]
        joint = row[joint_offset]
        point = struct.unpack_from(">3f",row,4)
        if joint!=12 or point not in ((0.,68.5,11.5),(0.,32.25,5.525000095367432)):
            raise ValueError("Worm source socket operands")
        sound = SOURCE_CUES[family]
        tone = sound_resolver(sound)
        if tone.get("status")!="static-tone-record":
            raise ValueError("Worm source cue bank")
        rows[model] = {"kind":"static ordinary-B evidence; not a live release receipt",
                       "model":f"{model:04X}","family":f"type-{family}","key":key,
                       "call":f"{call:08X}","factory":f"{factory:08X}",
                       "raw_row":row.hex(),"row_address":f"{table+key*stride:08X}",
                       "stock":{"site":"80119868","routine":"8006DBE0","channel":0,"cost":1,
                                "commit":True,"same_input_actor":True,"requires_debit_success":True,
                                "reviewed_refund_sites":["8013A148","8013A394"] if family==49 else []},
                       "origin":{"phase_zero":f"{phase_zero:08X}","source_joint":joint,"local_point":list(point),
                                 "basis":"credited-actor joint matrix from child +90; distinct from child +8C",
                                 "transform_site":"8013A2C8" if family==49 else "801D5030",
                                 "live_position_verified":False},
                       "sound":{"id":sound,"call":f"{cue:08X}","owner":owner,"bind_call":f"{bind:08X}",
                                "requires_collision_bind":True,**tone},
                       "live_release_verified":False}
    return rows


def attach_worm_pose(linker, actions, slots, joints, source, *, publish_preview):
    """Join the reviewed stationary-entry route to each exact normal pose bank."""
    profiles = actions["profiles"]
    if (len(profiles)!=4 or {p["config"] for p in profiles}!=PROFILES or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")) or
            source["origin"]["source_joint"]>=len(joints)):
        raise ValueError("Worm pose scope")
    local = {(g,c):at for g,c,at in slots}
    matching = []
    for p in profiles:
        roots = tuple(int(root,16) for root in p["program_and_descriptor_roots"])
        if (p["main_handler"],p["context_table"],roots)!=(
                "80119214","8032B9B0",(0x8032A8E0,0x8032AD02,0x8032AF20,0x8032B204)):
            raise ValueError("Worm installer scope")
        rows = [r for g in p["groups"] if g["group"]=="B-shot" for r in g["rows"] if (0,0) in map(tuple,r["contexts"])]
        if len(rows)!=1:
            raise ValueError("Worm stationary action context")
        row, = rows
        if ((row["state"],row["substate"],row["selector"],row.get("class_callback_candidate"))!=(0,0,44,"80119250") or
                linker.table_choice(roots[0],2,0)!=0x8032ABDA):
            raise ValueError("Worm stationary phase owner")
        pose = linker.entry_clip_binding(0x8011935C,0x801193D4,(15,2,0),roots,local)
        if not pose or pose["clip"]!=[3,0]:
            raise ValueError("Worm release pose")
        receipt = {**pose,"phase":2,"phase_table":"8032B5FC","release_site":source["call"],
                   "consume_site":"8011942C","control_byte":"1CEF","control_write":"8032ABF6",
                   "control_threshold":4,"clear_before_call":"80119428"}
        matching.append((row,receipt))
    if any(r!=matching[0][1] for _,r in matching):
        raise ValueError("Worm pose disagreement")
    if publish_preview:
        for row,receipt in matching:
            row["release_pose_candidate"] = receipt
    return {**source,"stationary_b_context":[0,0],"pose":matching[0][1],
            "installer_configs":sorted(PROFILES),"excluded_paths":["moving lead-in","B-close","X trap or poison descendants"]}
