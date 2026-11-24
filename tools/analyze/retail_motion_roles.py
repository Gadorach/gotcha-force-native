"""Join shared lower-lane movement roles to exact source selector and clip data."""
from hashlib import sha256
import struct
from functools import lru_cache


# Roles apply only to the exact shared lower handler. A selector number or
# installed type by itself does not establish that a movement role is present.
ROLES = {
    0x8005F2A4: "idle",
    0x8005F3A0: "ground-turn-entry",
    0x8005F578: "ground-travel",
    0x8005F6C0: "ground-stop",
    0x8005F978: "ground-takeoff",
    0x8005FAF0: "jump-ascent",
    0x80061F50: "powered-ascent",
    0x8005FECC: "air-continuation",
    0x80060228: "landing",
    0x8006235C: "air-relaunch",
    0x80060C48: "ground-dash-entry",
    0x80060EC4: "ground-dash-active",
    0x80061084: "ground-dash-recovery",
    0x80061338: "air-dash-wind-up",
    0x80061560: "air-dash-active",
    0x800617E4: "air-dash-recovery",
    0x8006266C: "glide-entry",
    0x800633C8: "cruise",
    0x80063644: "cruise-boost",
}
ALTERNATE_TRAVEL_HANDLER = 0x80063A90
ALTERNATE_TRAVEL_DIGEST = "4fa107a87a2368f6965457ba9fad753c794192c898723df31c7e34b4552242e9"
DRAGON_TAKEOFF_HANDLERS = (0x8006523C,0x80065394)
DRAGON_TAKEOFF_TEXT = (
    (0x8006523C,0x158,"d044dbd2a5baa78770378dea70ae29ac83fad6f2d3ede90e0ad69c72b7874916"),
    (0x80065394,0x238,"695f409b009cfeb38496cd8f166713739df63adac17697c5b4f8d92d9a8a6405"),
)
# These later visits advance the lane phase and request program 0:2.
# Earlier conditional requests stay separate in the review index.
DRAGON_TAKEOFF_REQUESTS = (("80065370","upper",13,(0,2,3)),("80065588","lower",2,(1,)))
HANDLERS = (0x802D36A8, 0x802D37A8)
ALTERNATE_DASH_ROLES = {
    0x80064710:"ground-dash-entry",0x8006478C:"ground-dash-active",0x800648F8:"ground-dash-recovery",
}
ALTERNATE_DASH_TEXT = (
    (0x800646B0,0x8006475C,"367f01549e9fa92753baa3f212607e0e43197de0412d6ae65bfaec1b1556c7e2"),
    (0x8006475C,0x800648BC,"f09a324f6617f4eafd90dfad8a6c254fc0a1d0c3bedc0c6b52d558a0c3a841fe"),
    (0x800648BC,0x800649D4,"3e715e2d60776e6cb7391fb076e71a2ea820c29e6bc95710ebfa0504e2257389"),
)
TURN_TO_TRAVEL_TEXT = (
    (0x8005F318, 0x8005F3A0, "7fe6e5d0850ec57f02600767503efb2cca61ae5df7382cd2d2c085a09bba9ca5"),
    (0x8005F3A0, 0x8005F554, "48380c2802c9f622c2eb320f13c6e3f519a408292889382f4ec1bf9bd550cc7d"),
    (0x8005F554, 0x8005F578, "68a2be2fe7c7f193b01d03ae977f0273e2c9d76d2fdce5bec7a0ac3f9b328c74"),
    (0x8005F578, 0x8005F620, "3ff207e9aeba4aa2b2cd20cf1305f5be75f06e7e511e3e5259fae3c31e70a9d5"),
)
# Both requests read the same signed direction byte and add 27. This is
# deliberately not generalized to other selectors or other handler bodies.
DASH_ENTRY_TEXT = (
    (0x80060BE8,0x80060C48,"114dc7925373c70d7b2bcbe2d13d921fb5af341be13c52336f53e6d323c259d2"),
    (0x80060C48,0x80060C94,"350d210bef1b851faef4c5e8506c30eac8ca27de3c5dfa9a3faaa092a9782f1d"),
    (0x80060C94,0x80060E94,"edbcb499762cdf7da20de94d85728c74ef50fd766249bcfe3ae10e665cb5ec49"),
)
DASH_DIRECTIONS = ("forward","left","backward","right")


@lru_cache(maxsize=4)
def ordinary_dash_source_matches(dol):
    try:
        return (dol.read(0x80433838,8,"data")==bytes((0,0,1,1,2,3,3,0)) and
                all(sha256(dol.read(lo,hi-lo,"text")).hexdigest()==digest for lo,hi,digest in DASH_ENTRY_TEXT))
    except ValueError:
        return False


def link_ground_dash(table, modes, roots, local, linker):
    # Types 0, 1 and 2 can install these exact paired lower/upper handlers.
    # The handler bytes supply the clip argument, not the installed type.
    # Flags may still change how the original chooses the direction byte.
    expected=tuple((12+i,[13+i,13+i],list(pair)) for i,pair in enumerate((
        ("80060BE8","80060C48"),("80060E94","80060EC4"),("80061048","80061084"))))
    if modes[1] not in (0,1,2) or (modes[2]&3)!=3:
        return
    def matches(rows):
        return all(any((r["selector"],r["states"],r["handler_addresses"])==want for r in table) for want in rows)
    alternate = tuple((12+i,[44+i,44+i],list(pair)) for i,pair in enumerate((
        ("800646B0","80064710"),("8006475C","8006478C"),("800648BC","800648F8"))))
    if matches(expected):
        upper,route=("80060C28","ordinary")
    elif modes[1]==0 and matches(alternate) and alternate_dash_source_matches(linker.dol):
        # The alternate lower handler calls the same entry helper before
        # its custom scene-child request. Only the shared clip is mapped.
        upper,route=("800646F0","alternate-ninja")
    else:
        return
    if not ordinary_dash_source_matches(linker.dol):
        return
    entry=next(row for row in table if row["selector"]==12)
    for value,direction in enumerate(DASH_DIRECTIONS):
        for site,lane,mask in (("80060D80","lower",2),(upper,"upper",13)):
            entry["program_candidates"].append({
                "site":site, "handler_lane":lane,
                "argument_evidence":"direction-byte-domain",
                "dash_direction":direction,
                **({"request_relay_site":"80064724"} if route=="alternate-ninja" and lane=="lower" else {}),
                **linker.program_candidate(roots,local,(mask,0,27+value)),
            })
    if route=="alternate-ninja":
        entry["source_dash_route"]=route


@lru_cache(maxsize=4)
def alternate_dash_source_matches(dol):
    try:
        return all(sha256(dol.read(lo,hi-lo,"text")).hexdigest()==digest
                   for lo,hi,digest in ALTERNATE_DASH_TEXT)
    except ValueError:
        return False


def carry_ground_travel(table, dol):
    # The turn entry requests 0:1 on both lanes, then may hand off to
    # selector 2. Travel only advances that lane's already-bound program.
    # Never label another controller's selector 2 by resemblance.
    turn = next((r for r in table if (r["selector"], r["states"]) == (1,[2,2])), None)
    travel = next((r for r in table if (r["selector"], r["states"]) == (2,[3,3])), None)
    if turn is None or travel is None or travel["program_candidates"]:
        return
    if not all(sha256(dol.read(lo,hi-lo,"text")).hexdigest()==digest
               for lo,hi,digest in TURN_TO_TRAVEL_TEXT):
        return
    routes = []
    for row in turn["program_candidates"]:
        if row["program_group"]==0 and row["program_choice"]==1 and row["site"] in ("8005F380","8005F4FC"):
            routes.append({**row,"request_scope":"carried-from-ground-turn","source_selector":1})
    if routes:
        travel["program_candidates"] = routes
        travel["entry_request_present"] = False


def motion_roles(linker, actions, slots):
    """Read only selector pairs/install tuples; do not assume reachable roles."""
    local = {(g,c): at for g,c,at in slots}
    output = {}
    for profile in actions["profiles"]:
        selector, config = (int(profile[k],16) for k in ("selector_table","config"))
        roots = tuple(int(v,16) for v in profile["program_and_descriptor_roots"])
        # Different action tables can share the entire movement binding.
        key = selector,config,roots
        if key in output:
            continue
        modes = linker.data(config, 3)
        pairs = list(struct.iter_unpack(">bb",linker.data(selector,96)))
        table, unclassified = [], []
        for index,(upper,lower) in enumerate(pairs[:32]):
            handlers = tuple(linker.read_word(base+state*4) if state>=0 else None
                             for base,state in zip(HANDLERS,(upper,lower),strict=True))
            if any(address is not None and not linker.text(address) for address in handlers):
                raise ValueError("unmapped shared movement handler")
            role = ROLES.get(handlers[1]) or ALTERNATE_DASH_ROLES.get(handlers[1])
            if handlers==(0x8005F238,ALTERNATE_TRAVEL_HANDLER) and modes[1]==6 and (upper,lower)==(0,36) and index in (2,30):
                if linker.matches_text(ALTERNATE_TRAVEL_HANDLER,0x1B8,ALTERNATE_TRAVEL_DIGEST):
                    role="alternate-ground-travel"
            if (handlers==DRAGON_TAKEOFF_HANDLERS and modes[1]==4 and modes[2]&7==7 and
                    (index,upper,lower)==(4,56,56) and all(linker.matches_text(*span) for span in DRAGON_TAKEOFF_TEXT)):
                role="dragon-takeoff"
            if role is None:
                if lower>=0:
                    unclassified.append(index)
                continue
            requests = []
            for lane,address in zip(("upper","lower"),handlers,strict=True):
                if address is None:
                    continue
                found,_,_ = linker.calls(address)
                for site,kind,args,proof in found:
                    if kind=="program":
                        requests.append({"site":f"{site:08X}","handler_lane":lane,
                                         "argument_evidence":proof,
                                         **linker.program_candidate(roots,local,args)})
            table.append({"selector":index,"states":[upper,lower],
                          "handler_addresses":[f"{a:08X}" if a is not None else None for a in handlers],
                          "role_hint":role,"program_candidates":requests})
        carry_ground_travel(table, linker.dol)
        link_ground_dash(table, modes, roots, local, linker)
        output[key] = {"config":profile["config"],"selector_table":profile["selector_table"],
                       "program_and_descriptor_roots":profile["program_and_descriptor_roots"],
                       "movement_type":modes[1],"configured_lanes":modes[2],
                       "other_present_lower_selectors":unclassified,"roles":table}
    return {"profiles":list(output.values()),"runtime_reachability_verified":False}


def dragon_takeoff_clip(profiles):
    """Bind only the reviewed later transition and its agreeing body lanes."""
    picked=[]
    for p in profiles:
        rows=[r for r in p["roles"] if r["role_hint"]=="dragon-takeoff" and r["selector"]==4 and
              r["states"]==[56,56] and r["handler_addresses"]==["8006523C","80065394"]]
        if p.get("movement_type")!=4 or p.get("configured_lanes",0)&7!=7 or len(rows)!=1:
            return None
        for site,lane,mask,slots in DRAGON_TAKEOFF_REQUESTS:
            requests=[r for r in rows[0]["program_candidates"] if r["site"]==site and r["handler_lane"]==lane and
                      (r["lanes"],r["program_group"],r["program_choice"],r["program_status"])==(mask,0,2,"terminal_wait")]
            if len(requests)!=1:
                return None
            first=[r for r in requests[0]["clip_candidates"] if r["preceding_waits"]==0]
            if len(first)!=1 or first[0]["start_frame"]!=0 or tuple(r["lane"] for r in first[0]["lanes"])!=slots:
                return None
            for binding in first[0]["lanes"]:
                key=(binding["group"],binding["choice"],binding["archive_offset"])
                if binding["bank"]!="local":
                    return None
                if binding["lane"]==3:
                    if key!=(2,0,None):
                        return None
                elif key[2] is None or not (0<=key[0]<16 and 0<=key[1]<128):
                    return None
                else:
                    picked.append(key)
    if picked and len(set(picked))==1:
        return picked[0][:2]
    return None


AIR_DASH_PHASES = (
    ("air-dash-wind-up",15,16,"80061338","80061454",10),
    ("air-dash-active",16,17,"80061560","80061644",11),
    ("air-dash-recovery",17,18,"800617E4","80061880",12),
)


def air_dash_subranges(role_mapping, timing):
    """One agreeing local start per phase; do not execute the source program."""
    profiles=role_mapping["profiles"] if role_mapping else []
    by_archive={r["archive_offset"]:r for r in timing}
    bindings=[]
    for role,selector,state,handler,site,choice in AIR_DASH_PHASES:
        exact=set()
        for p in profiles:
            rows=[r for r in p["roles"] if r["role_hint"]==role and r["selector"]==selector and
                  r["states"]==[state,state] and r["handler_addresses"][1]==handler]
            if len(rows)!=1:
                return ()
            requests=[r for r in rows[0]["program_candidates"] if r["handler_lane"]=="lower"]
            if len(requests)!=1:
                return ()
            r=requests[0]
            if ((r["site"],r["lanes"],r["program_group"],r["program_choice"])!=(site,2,0,choice) or
                    r["program_status"] not in ("terminal_wait","yielding_loop")):
                return ()
            candidates=[c for c in r["clip_candidates"] if c["preceding_waits"]==0]
            if len(candidates)!=1:
                return ()
            c=candidates[0]
            if len(c["lanes"])!=1:
                return ()
            b=c["lanes"][0]
            t=by_archive.get(b["archive_offset"],{})
            end=t.get("authored_end_frames",())
            start=c["start_frame"]
            refusals=t.get("strict_metadata_refusals",())
            # The solo lab already permits sampling the authored loop bit
            # in range. It still never evaluates the source loop lifecycle.
            inspection_loop=(refusals==["aobj-flags"] and t.get("aobj_flags")==[0x20000000] and
                             t.get("animated_joint_count",0)>0 and
                             t["animated_joint_count"]==t.get("authored_loop_joint_count"))
            if (b["lane"]!=1 or b["bank"]!="local" or not (0<=b["group"]<16 and 0<=b["choice"]<128) or
                    t.get("aobj_timing")!="indexed" or (refusals and not inspection_loop) or not end or
                    not 0<=start<=min(600,min(end))):
                return ()
            exact.add((b["group"],b["choice"],b["archive_offset"],start))
        if len(exact)!=1:
            return ()
        group,choice,_,start=exact.pop()
        bindings.append((role,group,choice,start))
    return tuple(bindings) if any(r[3] for r in bindings) else ()
