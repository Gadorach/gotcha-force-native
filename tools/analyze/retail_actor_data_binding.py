"""Static class-installer record binding; no source execution or actor lifetime."""
from hashlib import sha256
import struct

from tools.analyze.retail_action_tables import MODEL_JUMP
from tools.analyze.retail_movement_tables import signed16


RESOURCE_SLOT_TABLE = 0x803C4D94
# Actor byte 48A is the fusion slot: 0 standalone, 1 fused, 2 partner. The
# fusion routines write 1/2, the unfuse routine clears it, so a standalone
# actor keeps 0 and its installer's fused-partner branch is unreachable.
FUSION_BYTE = 0x48A
FUSION_EVIDENCE = (
    (0x8006ABD4, 0xFC, "47c47df0bb853e042faebb6877e7af2dd7122842352d687cab68e1a16c71d7dc"),
    (0x8006A980, 0x134, "a8a59dd5b4495be9edcf84cc130e9e5139b43c12d3da8aa807d7341732b586f7"),
)
RESOURCE_PUBLICATION = (
    (0x800412C4,0xA0,"3e98fd4062de07be013cfad0ad621c3fcab2f6d0e7edbcbf6fb4661edb13797f"),
    (0x80041364,0x188,"249b0351319af19daa4da9e216a7b0740db8a479eb7d905b6941ef6b056a068f"),
    (0x80056CE0,0x48,"b07773b3d078dd35918cbd9697bc5fa43d55dd7926cbccdae108cd962e2d92b5"),
)


def binding_schema(dol):
    for address, size, digest in RESOURCE_PUBLICATION+FUSION_EVIDENCE:
        if sha256(dol.read(address,size,"text")).hexdigest()!=digest:
            raise ValueError(f"actor-data publication signature: {address:08X}")
    return {"scope":"static slot-0 publication to class-installer return only",
            "member_role":0,"resource_slot_table":f"{RESOURCE_SLOT_TABLE:08X}",
            "class_loader":"80056CE0","actor_resource_slot_byte":"3E7","actor_data_pointer":"4AC",
            "other_resource_producers_qualified":False,
            "standalone_actor_bytes":{f"{FUSION_BYTE:03X}":"fusion slot; written only by fusion, cleared by unfuse"},
            "runtime_record_binding_verified":False}


def interpret_installer(words, installer, model, model_branch):
    """Bounded symbolic transfer of only resource pointers and their offsets."""
    if (len(words)!=144 or words[0]!=0x38000000 or words[1]>>16!=0x3C80 or
            words[2]!=0x2C000010 or words[3]>>16!=0x3884 or words[4]!=0x40800084 or
            any(words[5+2*i:7+2*i]!=(0x80040000+4*i,0x900304B4+4*i) for i in range(16))):
        return (),1
    # The same pinned copy prefix as the class-table scan. r3 remains the
    # actor, r13 can only produce an unqualified pointer here.
    # A model-ID guard is a reviewed comparison of actor halfword 3E8, the
    # same field the class handlers compare with their own model number. It
    # selects a form-specific record, so the ordinary model keeps its own
    # path instead of both branch arms being merged.
    queue=[(37,(None,)*32,None,0,None)]
    seen, returned, refused=set(),set(),set()
    while queue:
        index,regs,binding,store,guard=queue.pop()
        key=(index,regs,binding,store,guard)
        if key in seen:
            continue
        if not 37<=index<len(words) or len(seen)>=384:
            refused.add(index);continue
        seen.add(key)
        if words[index:index+10]==MODEL_JUMP:
            member=model-0x0506
            if 0<=member<15:
                displacement=model_branch(member)-installer
                if displacement%4 or not index+10<=displacement//4<len(words):
                    refused.add(index)
                else:
                    queue.append((displacement//4,regs,binding,store,guard))
            else:
                returned.add((binding,store))
            continue
        w=words[index]
        op,rt,ra,rb,xo=w>>26,w>>21&31,w>>16&31,w>>11&31,w>>1&1023
        imm=signed16(w)
        bo,bi=w>>21&31,w>>16&31
        if guard is None:
            resolved=None
        elif bo in (12,4) and bi==2:
            resolved=(model==guard[1]) if guard[0]=="model" else guard[1]
        else:
            resolved=None
        if op in (16,18):
            bits=16 if op==16 else 26
            raw=w&((1<<bits)-4)
            displacement=raw-(1<<bits) if raw&(1<<(bits-1)) else raw
            target=index+displacement//4
            if w&3 or target<=index:
                refused.add(index);continue
            taken=None if resolved is None else (resolved if bo==12 else not resolved)
            if taken is not False:
                queue.append((target,regs,binding,store,None))
            if taken is not True:
                queue.append((index+1,regs,binding,store,None))
            continue
        if op==19:
            if xo!=16 or w&1:
                refused.add(index);continue
            if resolved is None or (resolved if bo==12 else not resolved):
                returned.add((binding,store))
            if resolved is None and bo!=20:
                queue.append((index+1,regs,binding,store,None))
            elif resolved is not None and not (resolved if bo==12 else not resolved):
                queue.append((index+1,regs,binding,store,None))
            continue

        dest,value,next_guard=None,None,None
        if op==34:
            dest=rt
            if ra==3 and imm==0x3E7:
                value=("slot-byte",0)
            elif ra==3 and imm==FUSION_BYTE:
                value=("known",0)
        elif op in (32,42): # any other memory load is not our resource pointer
            dest=rt
            if op==42 and ra==3 and imm==0x3E8:
                value=("model-id",0)
        elif op==15:
            dest=rt
            if not ra:
                value=("literal",(imm<<16)&0xFFFFFFFF)
        elif op==14:
            dest=rt
            if not ra:
                value=("literal",imm&0xFFFFFFFF)
            elif regs[ra] and regs[ra][0] in ("literal","resource"):
                kind,base=regs[ra]
                value=(kind,(base+imm)&0xFFFFFFFF)
        elif op==31 and xo==954:
            dest=ra
            if regs[rt]==("slot-byte",0):
                value=("signed-slot",0)
        elif op==21:
            dest=ra
            if (w&0x0000FFFF)==0x103A and regs[rt]==("signed-slot",0):
                value=("slot-index",0)
        elif op==31 and xo==23:
            dest=rt
            # r0 is a normal index register in RB, but zero base in RA.
            base,index_reg=("literal",RESOURCE_SLOT_TABLE),("slot-index",0)
            if ra and (regs[ra],regs[rb]) in ((base,index_reg),(index_reg,base)):
                value=("resource",0)
        elif op==36 and ra==3:
            if imm==0x4AC:
                binding,store=regs[rt],installer+index*4
        elif op==11:
            # cmpwi/ cmplwi set cr0 only; a model-ID or reviewed constant
            # operand becomes a resolved guard.
            if (w>>23&7)==0 and regs[w>>16&31] and rt==0:
                kind=regs[w>>16&31][0]
                if kind=="model-id":
                    next_guard=("model",imm)
                elif kind=="known":
                    next_guard=("bool",regs[w>>16&31][1]==imm)
        else:
            refused.add(index);continue
        if dest==3:
            refused.add(index);continue
        if dest is not None:
            current=list(regs);current[dest]=value;regs=tuple(current)
        queue.append((index+1,regs,binding,store,next_guard))
    return sorted(returned,key=lambda row:(row[1],row[0] or ("",0))),len(refused)


def actor_record_binding(dol, installer, model, records, record_bytes):
    words=struct.unpack(">144I",dol.read(installer,576,"text"))
    returned,unresolved=interpret_installer(
        words,installer,model,lambda n:struct.unpack(">I",dol.read(0x802FF278+4*n,4,"data"))[0])
    rows=[]
    for value,site in returned:
        if value and value[0]=="resource" and value[1]%record_bytes==0 and value[1]//record_bytes<len(records):
            offset=value[1]
            rows.append({"kind":"slot-0-record","record":offset//record_bytes,
                         "record_sha256":records[offset//record_bytes]["record_sha256"],"byte_offset":offset,
                         "store_site":f"{site:08X}"})
        elif value and value[0]=="resource":
            rows.append({"kind":"unqualified-resource-offset","byte_offset":value[1],"store_site":f"{site:08X}"})
        else:
            rows.append({"kind":"unqualified-pointer" if site else "missing-store",
                         "store_site":f"{site:08X}" if site else None})
    selected={row["record"] for row in rows if row["kind"]=="slot-0-record"}
    fusion = any(words[i]==0x88000000|0x30000|FUSION_BYTE and words[i+1]==0x2C000001 for i in range(len(words)-1))
    common={"installer":f"{installer:08X}","return_candidates":rows,"unresolved_boundaries":unresolved,
            **({"guard":f"fusion slot {FUSION_BYTE:03X} == 1 selects the fused partner record; "
                         "a standalone actor keeps 0"} if fusion else {})}
    if not unresolved and len(selected)==1 and all(row["kind"]=="slot-0-record" for row in rows):
        return {"status":"one-static-slot-0-record","selected_record":next(iter(selected)),
                **common}
    return {"status":"conditional-or-unresolved","selected_record":None,
            **common}
