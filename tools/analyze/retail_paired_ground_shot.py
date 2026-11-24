"""Reviewed ordinary B pairs on the normal Titan/Victory rigs."""
from dataclasses import dataclass
from hashlib import sha256

from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_basic_shot_test import pilot_launch_tone, whole_body_pose


@dataclass(frozen=True)
class Route:
    installers: tuple
    handler: int
    roots: tuple
    callback: int
    phase_table: int
    firing_leaf: int
    debit_site: int
    family: str
    model_gate: int
    calls: tuple
    walk_choice: int
    tone: tuple


VICTORY_INSTALLERS = ((0x80345D48, 0x803461E0), (0x80345E60, 0x803461F8),
                     (0x80345F78, 0x803461E0))
VICTORY_ROOTS = (0x80345420, 0x802BD020, 0x80344F20, 0x803451C2)
ROUTES = {
    0x0619: Route(((0x803304E0, 0x80330860), (0x803305F8, 0x80330878)),
                  0x80127174, (0x8032FAA0, 0x802BD020, 0x8032FE60, 0x803300B8),
                  0x801271B0, 0x803303A4, 0x80127220, 0x80127240, "type-2", 0x80127274,
                  ((0x80127284, 79, 25, (-150., 0., -22.)), (0x80127290, 80, 23, (150., 0., -22.))),
                  1, ("snd_com02", 57)),
    0x061F: Route(VICTORY_INSTALLERS, 0x8015B7A0, VICTORY_ROOTS,
                  0x8015B7DC, 0x80345BF8, 0x8015B83C, 0x8015B85C, "type-0", 0x8015B890,
                  ((0x8015B8A0, 87, 25, (0., 50., 0.)), (0x8015B8AC, 88, 27, (0., 50., 0.))),
                  3, ("snd_com01", 6)),
    0x0625: Route(VICTORY_INSTALLERS, 0x8015B7A0, VICTORY_ROOTS,
                  0x8015B7DC, 0x80345BF8, 0x8015B83C, 0x8015B85C, "type-0", 0x8015B8D8,
                  ((0x8015B8E8, 91, 25, (0., 50., 0.)), (0x8015B8F4, 92, 27, (0., 50., 0.))),
                  3, ("snd_com01", 6)),
}
FACILITIES = {
    "type-0": (0x80082824, "credited-actor-joint"),
    "type-2": (0x800C3BE0, "parent-joint"),
}


def factory_keys():
    return {(route.family, key) for route in ROUTES.values() for _, key, _, _ in route.calls}


def verify_source(dol, linker):
    # Pin the successful stock gate, same-actor reloads, model branches and
    # nested phase dispatch. This is not a claim about their runtime timing.
    for at, size, digest in (
        (0x801271B0, 0x100, "8f05e57eeb0a3662db3569941493bcb2a889d6b52958b2483b9c866a87733c6c"),
        (0x8015B7DC, 0x138, "03c25b509cb07f1eb60ebba920dc5fda01404278af1811786d62c7920d71a0bd"),
        (0x80177634, 0x50, "2370fd5738e3b352c8ce3390880d6cd36a75bd67f6d40be5fbeb140bfde3517f"),
        (0x80177808, 0xF8, "9354e548f1787639fb5fcb6b096e9cbf350dbca4d3294b0f3b96aea43f4c6d5e"),
    ):
        if sha256(dol.read(at, size, "text")).hexdigest() != digest:
            raise ValueError(f"paired B-shot text signature: {at:08X}")
    for at, words in ((0x80330390, (0x801271F8,)*5),
                      (0x803303A4, (0, 0, 0x80127220, 0x801272B0, 0x80127300)),
                      (0x80345BF8, (0, 0, 0x8015B83C, 0x8015B914, 0x8015B970)),
                      (0x80352AE8, (0x80177684, 0x8017776C, 0x80177808, 0x80177900))):
        if tuple(linker.read_word(at+4*i) for i in range(len(words))) != words:
            raise ValueError(f"paired B-shot phase table: {at:08X}")


def reviewed_calls(route):
    _, basis = FACILITIES[route.family]
    return [{"site":f"{site:08X}", "family":route.family, "key":key, "joint":joint, "point":point, "basis":basis}
            for site,key,joint,point in route.calls]


def reviewed_debit(route):
    return {"site":f"{route.debit_site:08X}", "channel":0, "cost":1, "commit":True, "same_input_actor":True}


def source_pair(linker, model, actions, joints):
    route = ROUTES[model]
    profiles = actions["profiles"]
    pairs = [(int(p["config"],16), int(p["context_table"],16)) for p in profiles]
    if (sorted(pairs) != list(route.installers) or
            any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores"))):
        raise ValueError("paired B-shot needs its exact normal installers")
    for profile in profiles:
        if (int(profile["main_handler"],16), tuple(int(r,16) for r in profile["program_and_descriptor_roots"])) != (
                route.handler, route.roots):
            raise ValueError("paired B-shot installer owner")
        rows = [r for g in profile["groups"] if g["group"]=="B-shot" for r in g["rows"] if (0,0) in map(tuple,r["contexts"])]
        if len(rows)!=1 or (rows[0]["state"],rows[0]["substate"],rows[0]["selector"],rows[0].get("class_callback_candidate")) != (
                0,0,44,f"{route.callback:08X}"):
            raise ValueError("paired B-shot exact neutral context")

    def calls_target(site, target):
        instruction = linker.text(site)
        return instruction&0xFC000003 == 0x48000001 and direct_branch(site,instruction) == target

    if (linker.text(route.model_gate) != (0x2C000000|model) or not calls_target(route.debit_site,0x8006DBE0) or
            linker.read_word(route.phase_table+8) != route.firing_leaf):
        raise ValueError("paired B-shot model dispatch or stock gate")
    calls = reviewed_calls(route)
    factory, basis = FACILITIES[route.family]
    for site,key,bone,point in route.calls:
        muzzle = linker.origins[route.family][key]
        if (linker.text(site-4) != (0x38800000|key) or not calls_target(site,factory) or
                (muzzle["source_joint_selector"],tuple(muzzle["local_point"]),muzzle["basis"]) != (bone,point,basis) or
                not 0<=bone<len(joints) or joints[bone]["index"]!=bone):
            raise ValueError("paired B-shot needs both exact muzzle bindings")
    return {"model":f"{model:04X}", "raw_context":[0,0], "action_group":"B-shot",
            "phase_table":f"{route.phase_table:08X}", "firing_phase_entry":f"{route.firing_leaf:08X}",
            "model_gate":f"{route.model_gate:08X}", "debit":reviewed_debit(route), "ordered_calls":calls,
            "scope":"ordinary ranged pair only; no close-B, source phase timing or transform behavior",
            "live_equivalence_verified":False}


def battle_test(model, joints, role_clips, pace, level, source, sounds, *, launch_audio=False):
    route = ROUTES[model]
    if (source["model"],source["action_group"],source["ordered_calls"],source["debit"]) != (
            f"{model:04X}","B-shot",reviewed_calls(route),reviewed_debit(route)):
        raise ValueError("paired B-shot requires its ordered sockets and one same-actor debit")
    roles = {name:(g,c) for name,g,c in role_clips}
    required = ("ground-stop","ground-travel","air-continuation","ground-dash-forward")
    if tuple(roles.get(r) for r in required) != ((2,2),(2,route.walk_choice),(2,14),(2,4)):
        raise ValueError("paired B-shot needs its indexed local locomotion")
    walk = pace["native_units_per_tick_x100"] if pace else 0
    hp, initial, limit, mode, _ = level[:5]
    if not 1<=hp<=10000 or not 1<=initial==limit<=255 or mode!=0 or not 100<=walk<=1042:
        raise ValueError("paired B-shot HP, stock or pace domain")
    lines = ["GFBORG-GAMEPLAY 1","# Authored paired ground shot; not retail combat.",
             "variant=normal",f"health={hp}",f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
             "rig-prefix=visual/body.prefix","pose-profile=visual/body.pose","body-record=0"]
    lines.extend(f"{field}={roles[role][0]}:{roles[role][1]}"
                 for field,role in zip(("idle","walk","air","dash"),required,strict=True))
    lines.append(f"stock=shot-stock|{limit}|180")
    audio = []
    for name,call in zip(("shot","shot-second"),source["ordered_calls"],strict=True):
        point = "|".join(f"{v:.9g}" for v in call["point"])
        lines.extend((f"projectile={name}|bullet|1|32|0|0|0|12|70|8|ignore-floor",
                      f"origin={name}|{call['joint']}|{point}|target-height"))
        if launch_audio:
            tone = pilot_launch_tone(sounds[call["family"]][call["key"]])
            if tone!=route.tone:
                raise ValueError("paired B-shot needs its exact launch tone")
            bank,index = tone
            path = f"audio/launch-{bank}-{index}.pcm"
            lines.append(f"launch-sound={name}|{path}")
            audio.append({"projectile":name,"bank":bank,"tone":index,"path":path})
    lines.extend(("move=basic-shot|b|10|grounded|28|none","entry-stock=basic-shot|shot-stock|1",
                  "release=basic-shot|12|shot-stock|1|shot","release-followup=basic-shot|12|shot-second"))
    return ("\n".join(lines)+"\n").encode("ascii"), whole_body_pose(joints), {
        "kind":"authored normal ground shot pair, retains locomotion","retail_behavior_verified":False,
        "preview_clip":"none","body_binding":"strict record zero","source_level":1,
        "source_factory_calls":source["ordered_calls"],"source_debit_site":source["debit"]["site"],
        "native_timing":{"release_tick":12,"move_ticks":28,"refill_ticks":180},"authored_launch_sounds":audio,
    }
