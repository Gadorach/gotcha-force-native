"""Offline action-program control writes and narrowly reviewed release joins."""
from functools import lru_cache
from hashlib import sha256

from tools.analyze.gred_animation_controller import decode_program


OPCODE_ROW = 0x802D0DA8
WRITER = 0x8004C424
WRITER_DIGEST = "be8ac95c75902d94a0afcbb214ac797ca8f501e0676336d015a424da509cc839"

# The exact phase-zero request site and choice feeding each reviewed phase-one
# consumer. A matching byte pair alone is not evidence of a release.
REVIEWED = {
    "804338A0":("80076B28",0), "804338A8":("80076D44",1),
    "804338B0":("80076DC8",2), "804338B8":("80076FE8",1),
    "80433A38":("800A3F54",0), "80433A40":("800A4170",1),
    "80433A48":("800A41F4",2), "80433A50":("800A4414",1),
}


def control_write_schema(dol):
    if (int.from_bytes(dol.read(OPCODE_ROW,4,"data"),"big")!=WRITER or
            sha256(dol.read(WRITER,0x64,"text")).hexdigest()!=WRITER_DIGEST):
        raise ValueError("action upper-control writer signature")
    return {"opcode":2,"source_opcode_table_entry":f"{OPCODE_ROW:08X}","writer":f"{WRITER:08X}",
            "required_lane_mask":1,"stores":{"1CEF":"8004C43C","1CF0":"8004C444"}}


class ProgramControlIndex:
    def __init__(self, linker):
        self.linker = linker

    @lru_cache(maxsize=1024)
    def read(self, root, group, choice):
        try:
            address = self.linker.table_choice(root,group,choice)
            program = decode_program(self.linker.data,address)
        except ValueError:
            return None
        writes, waits = [], []
        for op in program["commands"]:
            if op["kind"]=="wait":
                waits.append({"address":f'{op["address"]:08X}',"flags":op["flags"],"threshold":op["threshold"]})
            elif op["kind"]=="upper_controls":
                raw = bytes.fromhex(op["raw"])
                writes.append({"address":f'{op["address"]:08X}',"raw":op["raw"],
                               "actor_1cef":raw[2]-(256 if raw[2]&128 else 0),
                               "actor_1cf0":raw[3],"preceding_waits":len(waits)})
        if not any(w["actor_1cef"] or w["actor_1cf0"] for w in writes):
            return None
        return {"program_address":f"{address:08X}","decode_status":program["status"],
                "wait_commands":waits,"writes":writes}

    def collect(self, actions):
        grouped, joins = {}, []
        for profile in actions["profiles"]:
            roots = profile["program_and_descriptor_roots"]
            for group in profile["groups"]:
                for row in group["rows"]:
                    requests = row.get("request_candidates",())
                    context = {"config":profile["config"],"context_table":profile["context_table"],
                               "main_handler":profile["main_handler"],"action_group":group["group"],
                               "state":row["state"],"substate":row["substate"],"selector":row["selector"],
                               "context_rows":row["contexts"]}
                    for request in requests:
                        if request["kind"]!="program" or not request["lanes"]&1:
                            continue
                        mask,pg,pc = (request[k] for k in ("lanes","program_group","program_choice"))
                        root = roots[bool(pg&128)]
                        control = self.read(int(root,16),pg&127,pc)
                        if control is None:
                            continue
                        site,scope = request["site"],request["scope"]
                        key = (site,root,mask,pg,pc)
                        item = grouped.setdefault(key,{"request_site":site,"program_root":root,"lane_mask":mask,
                                                       "program_group":pg,"program_choice":pc,**control,"routes":[]})
                        item["routes"].append({**context,"scope":scope})
                        if "request_choice_condition" in request:
                            item["request_choice_condition"]=request["request_choice_condition"]
                        joins.extend(reviewed_joins(context,request,control,requests))
        return {"programs":[grouped[key] for key in sorted(grouped)],"reviewed_release_candidates":joins}


def reviewed_joins(context, request, control, requests):
    if (context["action_group"]!="B-shot" or request["lanes"]!=15 or request["program_group"]!=2 or
            control["decode_status"]!="terminal_wait"):
        return
    condition = request.get("request_choice_condition")
    conditional = request["scope"]=="phase-zero-conditional-candidate"
    if conditional:
        choice = request["program_choice"]
        if (request["site"]!="8015AA0C" or choice not in (0,1) or not condition or
                condition.get("set") is not bool(choice) or
                control["program_address"]!=("80344054","803440A0")[choice]):
            return
        cues = [w for w in control["writes"] if w["actor_1cef"]>0]
    elif request["scope"]=="phase-zero-candidate":
        cues = [w for w in control["writes"] if (w["actor_1cef"],w["actor_1cf0"])==(3,1)]
    else:
        return
    if len(cues)!=1 or cues[0]["preceding_waits"]!=1:
        return
    for release in requests:
        table = release.get("phase_table")
        if release["kind"]!="projectile":
            continue
        guard = release.get("release_guard_candidate",{})
        if conditional:
            if (table!="804346F8" or release.get("phase_entry")!="8015AA24" or
                    guard.get("actor_signed_byte_positive")!="1CEF"):
                continue
        else:
            if (not release.get("phase_transition_calls") or
                    REVIEWED.get(table)!=(request["site"],request["program_choice"]) or
                    guard.get("actor_byte_equals")!={"541":0,"1CEF":3,"1CF0":1}):
                continue
        yield {**context,"phase_table":table,"program_request_site":request["site"],
               "program_address":control["program_address"],"control_command":cues[0]["address"],
               "release_site":release["site"],"family":release["family"],"key":release["key"],
               "runtime_timing_verified":False,
               **({"program_choice":request["program_choice"],"request_choice_condition":condition} if conditional else {})}
