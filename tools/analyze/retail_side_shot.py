"""Narrow neutral B dispatch review; keep the two source-side branches separate."""
from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.retail_seeded_shot_pose import local_upper_poses


ROUTES = {
    0x0100:{
        "main":0x800722F8,"config":"802D4478","root":0x802BF040,
        "code":(
            (0x80072334,0xAC,"a70b63c89730b808971d37102fb2eb19861dffb87886b6b8a78cf650f46deaa7"),
            (0x80072438,0x140,"f036cb7caeb65827a9c12f177abed17f8feeb6d82c94810b434f5ea38716dc50"),
            (0x800729B0,0x148,"e62488f95c099497a52bf7d864c282bb28dd53a2bbb0ae6fb1939ca6a0a66e19"),
            (0x80073330,0x1A8,"5bdf7ced0be2d81106e1d37253a49399454336aa463e12e1d04e572bea79bd71")),
        "rows":((0x802D42E8,0x800729B0),(0x802D430C,0x800724C8),(0x802D4324,0x800729F8),(0x802D4328,0x80072A40)),
        "entry":0x800724C8,"request":0x80072514,"consumer":0x80072A40,"relay":0x80072AB8,
        "factory_entry":0x80073330,"site":0x800734B4,"keys":(0,1),
        "program":0x802BF536,"cue":0x802BF566,
    },
    0x0105:{
        "main":0x8019889C,"config":"8036FC50","root":0x803700C0,
        "code":(
            (0x801988D8,0xAC,"3dff92cb6bbc18b49239636e1d58fc727100fb156cba607336938557165595d0"),
            (0x801989DC,0x100,"cf8649c486a5b4b3e856b0c125642ade71b7a8cb3ca3fe24a2353dd0cf16fb74"),
            (0x80198E08,0x148,"83c4dd3c8c943d4b43f499dffd50152ed274511328baa59ed911192d4af7a979"),
            (0x8019932C,0x164,"1fc94cc66b87db7448d636ea65d91c8f0737ac1e58ab2988c8c1d812df4f66ad")),
        "rows":((0x8036FADC,0x80198E08),(0x8036FB00,0x80198A50),(0x8036FB18,0x80198E50),(0x8036FB1C,0x80198E98)),
        "entry":0x80198A50,"request":0x80198A84,"consumer":0x80198E98,"relay":0x80198F10,
        "factory_entry":0x8019932C,"site":0x8019946C,"keys":(56,57),
        "program":0x80370426,"cue":0x80370456,
    },
}


def neutral_side_shot(linker, model, profile, leaf, row, roots, local, program_request):
    route = ROUTES.get(model)
    if (route is None or profile["config"]!=route["config"] or int(profile["main_handler"],16)!=route["main"] or
            roots[0]!=route["root"] or leaf!=route["code"][0][0] or row["state"]!=0 or row["substate"]!=0 or
            row["selector"]!=44):
        return None
    entry,site = route["entry"],route["request"]
    consumer,relay,factory_entry = route["consumer"],route["relay"],route["factory_entry"]
    if (not all(linker.matches_text(*span) for span in route["code"]) or
            not all(linker.read_word(at)==expected for at,expected in route["rows"]) or
            not linker.review_call(entry,site,program_request) or not linker.review_call(consumer,relay,factory_entry) or
            not linker.review_call(factory_entry,route["site"],0x80082824)):
        return None
    if model==0x0100 and linker.data(0x80433878,2)!=bytes(route["keys"]):
        return None
    try:
        program = linker.table_choice(roots[0],2,0)
        if program!=route["program"]:
            return None
        decoded = decode_program(linker.data,program)
        commands=decoded["commands"]
        cue = next((i for i,op in enumerate(commands) if op["kind"]=="upper_controls" and
                    op["address"]==route["cue"] and op["raw"][4:8]=="0100"),None)
        if decoded["status"]!="terminal_wait" or not cue:
            return None
        select = commands[cue-1]
        if (select["kind"]!="directional_select" or select["group"]!=2 or select["choices"]!=[1,0,2] or
                select["start_frame"]!=11 or select["preceding_waits"]!=2):
            return None
        poses = local_upper_poses(linker,roots,local,2,select["choices"])
        if not poses:
            return None
    except ValueError:
        return None
    binding = {"entry":f"{entry:08X}","request_site":f"{site:08X}","request_mask":1,"program":[2,0],**poses[1],
               "source_pose_alternatives":poses,"source_start_frame":11,"authored_pose_choice":1,
               "source_side_choice":"zero","source_side_actor_byte":"1B04","source_side_seed_byte":"1B44",
               "cue_command":f'{route["cue"]:08X}',
               "release_helper":f"{factory_entry:08X}","release_relay_site":f"{relay:08X}"}
    return binding,route
