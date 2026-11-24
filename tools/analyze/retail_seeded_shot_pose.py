"""Scoped zero-seed rifle pose candidates; not a live directional binding."""
from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.lane_descriptors import descriptor_lanes


RIFLE_KEYS = {0x0600:21,0x0615:43,0x0616:62,0x0629:101,0x062A:102}
# (factory keys and sites, in reviewed call order). Both branch arms are
# in the pinned type-2 release relay. The second call has no return-value
# test on the first allocation.
RIFLE_PAIRS = {
    0x0608:((51,52),(0x800C9FB0,0x800C9FBC)),
    0x061C:((63,64),(0x800C9FEC,0x800C9FF8)),
}
CYBER_MACHINE_SHOTS = {
    model:(tuple(range(key,key+count)),tuple(site+12*i for i in range(count)))
    for model,key,site,count in ((0x0602,22,0x800CD2D8,3),(0x060A,105,0x800CD308,6),
                                 (0x060C,111,0x800CD35C,5),(0x060E,116,0x800CD3A4,4))
}
ORDERED_SHOTS = {**RIFLE_PAIRS,0x080C:((99,100),(0x8010FF24,0x8010FF30)),**CYBER_MACHINE_SHOTS,
                 0x0606:((30,31),(0x8012A384,0x8012A390)),
                 0x061A:((110,111),(0x8012A3A8,0x8012A3B4))}
STATIC_CRUISE_SHOTS = {0x0505:((103,104),(0x801E7B58,0x801E7B64)),
                      0x0511:((121,122),(0x801E7B7C,0x801E7B88))}
# Model-gated direct handler, factory family, keys and sites in call order.
ANIMATED_CRUISE_SHOTS = {
    0x061B:(0x8012B188,"type-2",(83,84),(0x8012B410,0x8012B41C)),
    0x0620:(0x8015C0C0,"type-0",(93,94),(0x8015C350,0x8015C35C)),
}
CLOCKED_GROUND_DIRECT_ENTRIES = {0x0604:0x80125DE8,0x0618:0x80125DE8,0x0613:0x8018C7D8,0x0627:0x8018C7D8}
CLOCKED_GROUND_SHOTS = {
    0x0604:((43,44),(0x80125F50,0x80125F5C)),
    0x0618:((75,76),(0x80125F74,0x80125F80)),
    0x0613:((50,51),(0x8018C940,0x8018C94C)),
    0x0627:((77,78),(0x8018C964,0x8018C970)),
    0x0628:((79,80),(0x8018C02C,0x8018C038)),
}
EAGLE_FAMILIES = {0x0606:"type-2",0x061A:"type-0"}
EAGLE_B_ROUTE = {
    "installer":(0x80129608,0x198,"c678e703f1e2978cb96910d4b3cdef9b4e42463d306abf9a34d06b5064ceb1b3"),
    "hook":(0x801297A0,0x3C,"2a81c1102ea675ac2228d645f189245b98094b08d33c6d4725188063c8aaa480"),
    "dispatch":(0x8012999C,0x128,"c1648349c114e8698b575a3f6fc934fba83ad2fcb6ee0ae19066edb40afbdf17"),
    "consumer":(0x80129AC4,0xC8,"8e51634d9b7b7dffcd9d96f1d60d7b819311e2830b9167fac215c7e1bf936712"),
    "row":0x803318F0,"entry":0x801299E4,"request":0x80129A38,"program_root":0x80330BA0,
    "program":0x80330F2E,"store":0x801297B4,"cue":0x80330F52,"cue_raw":"0201010000000000",
    "direction":0x80330F4A,"choices":[1,0,2],"start":0,"waits":1,
    "relay":0x80129B64,"helper":0x8012A308,"mask":15,
}
DOUBLE_NINJA = 0x0003
DOUBLE_NINJA_PRIMARY_SITE = "800705C4"
THROW_KEYS = {0x0000:0,0x0002:1,DOUBLE_NINJA:0,0x0008:0,0x000A:4,0x000B:1,0x000C:0}
THETA_ROUTE = {
    "installer":(0x801A10E8,0x114,"9eaa29614d3c00f9856da48ba78e4e25d1df2865dc887a2f08abac7c8ddcf454"),
    "hook":(0x801A1200,0x20,"0816f734e324e57db08030307a657422744dda0f2c9872115fc1ffaf702fa8d3"),
    "dispatch":(0x801A1904,0x118,"0ecd82b4a8800a73da097f69149d7f94f139b76a2e516e037dc2da0ae8fcbd53"),
    "consumer":(0x801A1A1C,0xD4,"c6b171d66c61e6c9bdab90d03dc568dc1dac3af331148c27aaa65ef3c01ce469"),
    "row":0x803786B0,"store":0x801A1218,"entry":0x801A1948,"request":0x801A19BC,
    "program":0x80378EF6,"program_root":0x80378B80,"cue":0x80378F1A,"release_helper":"801A1AF0",
}
BEAM_B_ROUTE = {
    "installer":(0x80102824,0x134,"ea1ab3ec0d2d369d5cb9469a8614bfe5c28ae1f9f54e21599af7bcdfe434623b"),
    "hook":(0x80102958,0x1C,"0b3b675ccfae2ee3178b0d5178efdeb178696dedfbed76c4b237ae0bc85d03ab"),
    "dispatch":(0x80102AB0,0x148,"9d099acef7ff51d0f500a5f2ea51b6bbda87925fdc67d4bcf53a4955e425674d"),
    "consumer":(0x80102BF8,0x28C,"7967766d02b9b4e0b8d603512da42890ad1a85e7279123ca7e7b02f31c91f6b1"),
    "row":0x803215D8,"entry":0x80102AF8,"request":0x80102B34,"program_root":0x803209E0,
    "program":0x80320D48,"store":0x80102968,"cue":0x80320D64,
}
HEAVY_B_ROUTES = {
    0x0103:{
        "installer":(0x801301F8,0x280,"82c8ad034fe354a812587b8dff4e7553e56c4a60dcb28c6ee13643d5533bf19f"),
        "hook":(0x80130478,0x40,"3ef2aea02e2b33077399aea883ac758f5a836437381ddb8539301c2b42e5fda5"),
        "dispatch":(0x80130844,0x174,"9751bf0be8cb9a5cbf620f8c446b7b857bd351a618617593470eeb5ff4c09244"),
        "consumer":(0x801309B8,0x114,"6f0d4097618846ae785fc3c1a1d62a18363bfc6ad57829a396dec40dca5947b4"),
        "row":0x80335C80,"entry":0x8013088C,"request":0x801308D4,"program_root":0x80334E20,
        "program":0x80335174,"store":0x80130490,"cue":0x803351A4,"cue_raw":"0201010000000000",
        "direction":0x8033519C,"choices":[1,0,2],"start":6,"relay":0x80130A6C,"helper":0x801313BC,
        "family":"type-0","keys":(38,),"sites":(0x80131508,),
    },
    0x0602:{
        "installer":(0x800CC454,0x1DC,"6098d27096d58cf1f3db7fb128e562d587406b26e057b3a9ccb6e9eb32f18adf"),
        "hook":(0x800CC630,0x5C,"00b811a3992c419f42fdc9e66022b38b4428cb0bb88220b5d27bd6426c656f38"),
        "dispatch":(0x800CC854,0x148,"436ae0c0b7b622f6cff6f6b4227a29e6f3d2c0bc461b9ffadeb30fd455871abf"),
        "consumer":(0x800CC99C,0x174,"6ae9a22165ab56049dd1ee577584013dda82677788562aee68dc04063dfdee57"),
        "row":0x8030CCD0,"entry":0x800CC890,"request":0x800CC8DC,"program_root":0x8030B260,
        "program":0x8030B5FC,"store":0x800CC648,"cue":0x8030B63C,"cue_raw":"02ff010001000000",
        "direction":0x8030B620,"choices":[10,9,11],"start":0,"relay":0x800CCA4C,"helper":0x800CD1A0,
        "family":"type-2","keys":(22,23,24),"sites":(0x800CD2D8,0x800CD2E4,0x800CD2F0),
        "choice_helper":(0x800CD3EC,0xE0,"099567bac27e4a04bfd50229fc09083e4605f859a51312d270c14d5042066f9d"),
    },
}

# Installer, copied hook row, zero store, phase dispatch and entry request.
# Each reset is conditional on actor +5E0 bit 0 being clear. Other values
# of +6EE, held repeat, and the current facing branch are not resolved.
ROUTES = {
    0x800C9590:{
        "installer":(0x800C91BC,0xFC,"188f58d58811df81c6950389b4dbfa6efab96b19e1cd8dfe9ebd091dedae981c"),
        "hook":(0x800C93A0,0x5C,"376746364dc5a644acc50ee4bd761a6c55b1d21195688cd8e7d8d967468bac77"),
        "dispatch":(0x800C9590,0x130,"c6c2b783b0f8366affd151bcc2692f29e84143c06617656c606b5bb917b6268d"),
        "row":0x8030A520,"store":0x800C93B8,"entry":0x800C95D8,"request":0x800C9634,
        "program_root":0x80308CA0,"program":0x8030903E,"release_helper":"800C9F40",
    },
    0x8018D308:{
        "installer":(0x8018CCFC,0x120,"dda3561372b459527f131057a11caf6f4645b8bb0c80ac17372cb2e42ff6a58f"),
        "hook":(0x8018CEF8,0x5C,"7d036da1af1acbcd5ddfcd6d055df07b1ccec57b0bf74d3fff0b8e696fc1adcf"),
        "dispatch":(0x8018D308,0x130,"1f5c3102638c0237e45b30db38a3462cbf7298a26bfaeda487215c0a0394aa98"),
        "row":0x80365540,"store":0x8018CF10,"entry":0x8018D350,"request":0x8018D3AC,
        "program_root":0x80366220,"program":0x80366638,"release_helper":"8018DCB0",
    },
}

# The throwers use an equivalent conditional zero reset but their selected
# pose can begin at a nonzero frame and their cue uses a different program.
THROW_ROUTES = {
    0x8006FB44:{
        "installer":(0x8006F4F8,0x114,"9bedc00559917b9e5fa76d02977c59be604bed473605fc394e3df7af750ee800"),
        "hook":(0x8006F7C0,0xCC,"e4b751102a00fb7b424274976e8bb1f140fe0807706cd1517078b40dbe3de8dd"),
        "dispatch":(0x8006FB44,0xE0,"d5aa675498aedd77ddbbf19c99017666da7b41b0d3375a2906bcb69474e2bc1b"),
        "row":0x802D3ADC,"store":0x8006F808,"entry":0x8006FB8C,"request":0x8006FBC8,
        "program":0x802BE23A,"program_root":0x802BDE80,"cue":0x802BE256,"mask":1,"release_helper":"80070558",
    },
    0x800D0454:{
        "installer":(0x800CFE9C,0xFC,"447f424caf1485804ea6e4e40c21b88fc671aee1610d2b735d9aae02acf93255"),
        "hook":(0x800D0064,0x40,"5041115b2f8f0ac26717262bab2031c839f521ddab0a17939e2b5638098ec6ac"),
        "dispatch":(0x800D0454,0xC0,"94a1d6cad7cde1a7bfdd16f6437224fb8925c92f028f39572a490fce59abac22"),
        "row":0x8030DD88,"store":0x800D009C,"entry":0x800D049C,"request":0x800D04D8,
        "program":0x8030DA28,"program_root":0x8030D4C0,"cue":0x8030DA44,"mask":15,"release_helper":"800D0C50",
    },
    0x8019EE34:{
        "installer":(0x8019E9A4,0xFC,"f33fe36385a4b2b2cae99fcc61865e65818d019983f3af031568dab5afcab3bb"),
        "hook":(0x8019EB8C,0x3C,"9bbc1ab4d416b20c5a944b473ec7b5ddb73e96a49cb61a0f78606bd5a5643927"),
        "dispatch":(0x8019EE34,0xE0,"3f72a940aa5608bb281e79468f9ff3ac3f49cee04d888ebdd16c5a6f6760f0f6"),
        "row":0x803763F0,"store":0x8019EBA8,"entry":0x8019EE7C,"request":0x8019EEB8,
        "program":0x80376E1C,"program_root":0x80376A60,"cue":0x80376E44,"mask":1,"release_helper":"8019F850",
    },
}
# Double Ninja installs a different row, although the zero-seed hook,
# dispatcher and entry program are shared with the reviewed Sasuke route.
DOUBLE_NINJA_ROUTE = {**THROW_ROUTES[0x8006FB44],"row":0x802D3B1C,
    "installer":(0x8006F60C,0xFC,"201ead69a9e7e0cb04851db5f7498352ebdf5cbecc88be4486e68e2122e3e962")}


def seed_route_matches(linker, route, installer, phases, roots, program_request):
    return (installer==route["installer"][0] and roots[0]==route["program_root"] and
            (route["entry"],"phase-zero-candidate") in phases and
            all(linker.matches_text(*route[field]) for field in ("installer","hook","dispatch")) and
            linker.read_word(route["row"])==route["hook"][0] and
            linker.review_call(route["entry"],route["request"],program_request))


def theta_shot_pose(linker, installer, handler, phases, model, roots, local, requests, program_request):
    route = THETA_ROUTE
    if (model!=0x0906 or handler!=route["dispatch"][0] or
            not seed_route_matches(linker,route,installer,phases,roots,program_request)):
        return None
    if ((route["consumer"][0],"phase-1-entry-increment-candidate") not in phases or
            not linker.matches_text(*route["consumer"]) or
            not linker.review_call(route["consumer"][0],0x801A1ABC,0x801A1AF0) or
            not any(r["kind"]=="projectile" and (r["site"],r["family"],r["key"])==("801A1B48","type-0",106) and
                    r.get("release_relay")==route["release_helper"] and r.get("required_model_at_call")=="0906"
                    for r in requests)):
        return None
    try:
        address = linker.table_choice(roots[0],2,6)
        if address!=route["program"]:
            return None
        program = decode_program(linker.data,address)
        pairs = [(a,b) for a,b in zip(program["commands"],program["commands"][1:])
                 if a["kind"]=="directional_select" and a["group"]==2 and a["choices"]==[4,3,5] and
                 a["preceding_waits"]==1 and a["start_frame"]==3 and b["kind"]=="upper_controls" and
                 b["address"]==route["cue"] and b["raw"][4:8]=="0101"]
        if len(pairs)!=1 or program["status"]!="terminal_wait":
            return None
        poses = local_upper_poses(linker,roots,local,2,pairs[0][0]["choices"])
        if poses is None:
            return None
    except ValueError:
        return None
    return {"entry":f'{route["entry"]:08X}',"request_site":f'{route["request"]:08X}',"request_mask":1,
            "program":[2,6],**poses[1],"source_start_frame":3,"source_pose_alternatives":poses,
            "authored_pose_choice":1,"cue_command":f'{route["cue"]:08X}',"cue_bytes":[1,1],
            "release_helper":route["release_helper"],"release_relay_site":"801A1ABC",
            "seed_candidate":{"actor_byte":"6EE","value":0,"choice_addend":6,"reset_store":f'{route["store"]:08X}',
                              "requires_actor_word_5e0_bit_0_clear":True}}


def beam_b_shot_pose(linker, installer, handler, phases, model, roots, local, requests, program_request):
    """Index the exact ordinary B pose, not the unsupported X beam or a native fallback."""
    route = BEAM_B_ROUTE
    if (model!=0x0104 or handler!=route["dispatch"][0] or
            not seed_route_matches(linker,route,installer,phases,roots,program_request) or
            (route["consumer"][0],"phase-1-entry-increment-candidate") not in phases or
            not linker.matches_text(*route["consumer"]) or
            not linker.review_call(route["consumer"][0],0x80102C94,0x80103428) or
            not any(r["kind"]=="projectile" and (r["site"],r["family"],r["key"])==("80103568","type-2",11) and
                    r.get("release_relay")=="80103428" for r in requests)):
        return None
    try:
        addr = linker.table_choice(roots[0],2,0)
        if addr!=route["program"]:
            return None
        program = decode_program(linker.data,addr)
        commands = program["commands"]
        if (program["status"]!="terminal_wait" or not commands or commands[0]["raw"]!="0d02010002060000" or
                not any(op["address"]==route["cue"] and op["raw"]=="0201010001000000" for op in commands)):
            return None
        poses = local_upper_poses(linker,roots,local,2,[1,0,2])
        if not poses:
            return None
    except ValueError:
        return None
    return {"entry":f'{route["entry"]:08X}',"request_site":f'{route["request"]:08X}',"request_mask":1,
            "program":[2,0],**poses[1],"source_start_frame":0,"source_pose_alternatives":poses,
            "authored_pose_choice":1,"cue_command":f'{route["cue"]:08X}',"release_helper":"80103428",
            "release_relay_site":"80102C94",
            "seed_candidate":{"actor_byte":"6EE","value":0,"reset_store":f'{route["store"]:08X}',
                              "requires_actor_word_5e0_bit_0_clear":True}}


def local_upper_poses(linker, roots, local, group, choices):
    poses = []
    for choice in choices:
        desc = linker.table_choice(roots[2],group,choice)
        lane, = descriptor_lanes(linker.data,desc,1)["lanes"]
        g,c = lane["group"],lane["index"]
        if g>=16 or c>=128 or (g,c) not in local:
            return None
        poses.append({"descriptor":[group,choice],"clip":[g,c],"archive_offset":local[g,c]})
    return poses


def heavy_b_shot_pose(linker, installer, handler, phases, model, roots, local, requests, program_request):
    if model in EAGLE_FAMILIES:
        keys,sites = ORDERED_SHOTS[model]
        route = {**EAGLE_B_ROUTE,"keys":keys,"sites":sites,"family":EAGLE_FAMILIES[model]}
    elif model in CYBER_MACHINE_SHOTS:
        keys,sites = CYBER_MACHINE_SHOTS[model]
        route = {**HEAVY_B_ROUTES[0x0602],"keys":keys,"sites":sites}
    else:
        route = HEAVY_B_ROUTES.get(model)
    if (not route or handler!=route["dispatch"][0] or
            not seed_route_matches(linker,route,installer,phases,roots,program_request) or
            (route["consumer"][0],"phase-1-entry-increment-candidate") not in phases or
            not linker.matches_text(*route["consumer"]) or
            not linker.review_call(route["consumer"][0],route["relay"],route["helper"])):
        return None
    choice_helper = route.get("choice_helper")
    if choice_helper and (not linker.matches_text(*choice_helper) or
                          not linker.review_call(route["entry"],0x800CC8B0,choice_helper[0])):
        return None
    expected = set(zip((f"{site:08X}" for site in route["sites"]),route["keys"],strict=True))
    linked = {(r["site"],r["key"]) for r in requests if r["kind"]=="projectile" and r["family"]==route["family"] and
              r.get("required_model_at_call")==f"{model:04X}" and
              r.get("release_relay_site")==f'{route["relay"]:08X}' and r.get("release_relay")==f'{route["helper"]:08X}'}
    if linked!=expected:
        return None
    try:
        address=linker.table_choice(roots[0],2,0)
        if address!=route["program"]:
            return None
        program=decode_program(linker.data,address)
        select=[op for op in program["commands"] if op["address"]==route["direction"] and
                op["kind"]=="directional_select" and op["group"]==2 and op["choices"]==route["choices"] and
                op["start_frame"]==route["start"] and op["preceding_waits"]==route.get("waits",2)]
        cue=[op for op in program["commands"] if op["address"]==route["cue"] and op["raw"]==route["cue_raw"]]
        if program["status"]!="terminal_wait" or len(select)!=1 or len(cue)!=1:
            return None
        poses=local_upper_poses(linker,roots,local,2,route["choices"])
        if not poses:
            return None
    except ValueError:
        return None
    seed={"actor_byte":"6EE","value":0,"reset_store":f'{route["store"]:08X}',
          "requires_actor_word_5e0_bit_0_clear":True}
    if choice_helper:
        seed.update(source_choice_addends=[0,2,6],authored_choice_addend=0)
    return {"entry":f'{route["entry"]:08X}',"request_site":f'{route["request"]:08X}',"request_mask":route.get("mask",1),
            "program":[2,0],**poses[1],"source_start_frame":route["start"],"source_pose_alternatives":poses,
            "authored_pose_choice":1,"seed_candidate":seed,"cue_command":f'{route["cue"]:08X}',
            "release_helper":f'{route["helper"]:08X}',"release_relay_site":f'{route["relay"]:08X}'}


def throw_shot_pose(linker, installer, handler, phases, model, roots, local, requests, program_request):
    route,key = THROW_ROUTES.get(handler),THROW_KEYS.get(model)
    if model==DOUBLE_NINJA:
        if handler!=0x8006FB44:
            return None
        route=DOUBLE_NINJA_ROUTE
    if route is None or key is None or not seed_route_matches(linker,route,installer,phases,roots,program_request):
        return None
    if not any(r["kind"]=="projectile" and (r["family"],r["key"])==("type-14",key) and
               r.get("release_relay")==route["release_helper"] for r in requests):
        return None
    no_clone={}
    if model==DOUBLE_NINJA:
        # Pin the consumer and the zero branch that bypasses only the clone
        # call. Keep the alternative in the index; this proof is for c0:s0.
        consumer,relay_site,helper=0x8006FC24,0x8006FCA0,0x80070558
        if ((consumer,"phase-1-entry-increment-candidate") not in phases or
                not linker.matches_text(consumer,0x94,"83bc308825a1176532702197a9935affc5bc734bad30aa7958d99bbb433087db") or
                not linker.review_call(consumer,relay_site,helper) or
                not linker.matches_text(helper,0xAC,"69d0ddf36e6edf532b931695a3cf68c1d9275292a7fb7449872dd787e8b88c84") or
                not linker.review_call(helper,int(DOUBLE_NINJA_PRIMARY_SITE,16),0x8006EE14,0) or
                not linker.review_call(helper,0x800705DC,0x8006EE14,2) or
                not all(any(r.get("site")==site and r.get("key")==key and r.get("required_model_at_call")=="0003" and
                            r.get("release_relay_site")=="8006FCA0" and r.get("release_relay")=="80070558" for r in requests)
                        for site,key in ((DOUBLE_NINJA_PRIMARY_SITE,0),("800705DC",2)))):
            return None
        no_clone={"source_clone_byte":{"offset":"7C8","value":0,"branch":"800705D0"},
                  "release_site":DOUBLE_NINJA_PRIMARY_SITE,"release_relay_site":"8006FCA0"}
    try:
        address = linker.table_choice(roots[0],2,0)
        if address!=route["program"]:
            return None
        program = decode_program(linker.data,address)
        commands = program["commands"]
        select = commands[0] if commands else {}
        if (program["status"]!="terminal_wait" or select.get("group")!=2 or select.get("preceding_waits")!=0 or
                not any(op["kind"]=="upper_controls" and op["address"]==route["cue"] and op["raw"][4:8]=="0100"
                        for op in commands)):
            return None
        if select["kind"]=="directional_select" and select["choices"]==[2,0,1]:
            choices,authored = select["choices"],1
        elif select["kind"]=="select_descriptor" and select["choice"]==0:
            choices,authored = [0],0
        else:
            return None
        bindings = local_upper_poses(linker,roots,local,2,choices)
        if bindings is None:
            return None
    except ValueError:
        return None
    return {"entry":f'{route["entry"]:08X}',"request_site":f'{route["request"]:08X}',
            "request_mask":route["mask"],"program":[2,0],**bindings[authored],
            "source_start_frame":select["start_frame"],"source_pose_alternatives":bindings,
            "authored_pose_choice":authored,"cue_command":f'{route["cue"]:08X}',"release_helper":route["release_helper"],
             "seed_candidate":{"actor_byte":"6EE","value":0,"reset_store":f'{route["store"]:08X}',
                               "requires_actor_word_5e0_bit_0_clear":True},**no_clone}


def rifle_shot_pose(linker, installer, handler, phases, model, roots, local, requests, program_request):
    route = ROUTES.get(handler)
    keys = RIFLE_PAIRS[model][0] if model in RIFLE_PAIRS else (RIFLE_KEYS.get(model),)
    if route is None or keys==(None,) or not seed_route_matches(linker,route,installer,phases,roots,program_request):
        return None
    entry,site = route["entry"],route["request"]
    if not set(keys)<={r["key"] for r in requests if r["kind"]=="projectile" and r["family"]=="type-2" and
                       r.get("required_model_at_call")==f"{model:04X}" and r.get("release_relay")==route["release_helper"]}:
        return None
    try:
        address = linker.table_choice(roots[0],2,0)
        if address!=route["program"]:
            return None
        program = decode_program(linker.data,address)
        candidates = [(a,b) for a,b in zip(program["commands"],program["commands"][1:])
                      if a["kind"]=="directional_select" and a["preceding_waits"]==2 and a["start_frame"]==0 and
                      a["group"]==2 and a["choices"]==[7,6,8] and b["kind"]=="upper_controls" and
                      b["raw"][4:8]=="0100"]
        if len(candidates)!=1 or program["status"]!="terminal_wait":
            return None
        select,cue = candidates[0]
        bindings = local_upper_poses(linker,roots,local,select["group"],select["choices"])
        if bindings is None:
            return None
    except ValueError:
        return None
    return {"entry":f"{entry:08X}","request_site":f"{site:08X}","request_mask":15,"program":[2,0],
            **bindings[1],"authored_direction_choice":1,"source_direction_alternatives":bindings,
            "seed_candidate":{"actor_byte":"6EE","value":0,"reset_store":f'{route["store"]:08X}',
                              "requires_actor_word_5e0_bit_0_clear":True},
            "release_helper":route["release_helper"],"cue_command":f'{cue["address"]:08X}'}
