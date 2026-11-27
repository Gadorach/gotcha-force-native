"""Reviewed ordinary single ground shots on exact local poses.

Each route keeps the source poses that bind strictly. A route whose source
dash pose cannot be bound declines both dash inputs instead of substituting
another clip. This is not retail combat.
"""
from hashlib import sha256

from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_basic_shot_test import pilot_launch_tone, whole_body_pose


BEAM_GUNNER, GOLD_HERO, CARRIER_HELICOPTER, RED_ATTACKER, DEATH_EYE = 0x0104, 0x0802, 0x0D02, 0x0D01, 0x0F06
ULTIMATE_CANNON = 0x0C04
DELTA_II, BEAM_TANK = 0x0303, 0x0C02
MODELS = {BEAM_GUNNER, GOLD_HERO, CARRIER_HELICOPTER, RED_ATTACKER, DEATH_EYE, ULTIMATE_CANNON, DELTA_II,
          BEAM_TANK}
# Pinned installer, class handler, release leaf, paid release and muzzle.
ROUTES = {
    BEAM_GUNNER: {"installer": 0x80102824, "consumer": 0x80102BF8, "leaf": 0x80103428,
                  "debit": 0x80103554, "muzzle_site": 0x80103568, "family": "type-2", "key": 11,
                  "joint": 7, "point": (0., 0., 0.), "tone": ("snd_com02", 57),
                  "callback": 0x80102A30, "main_handler": 0x801029F4, "relay_site": 0x80102C94, "config": 0x80321218,
                  "context_table": 0x803214C8,
                  "roots": (0x803209E0, 0x802BC080, 0x80320580, 0x802BAE60),
                  "roles": ((0, 0), (0, 1), (0, 24)), "dash": "declined", "model_gate": None,
                  "body": None, "selector": 44, "state": 0, "marker": 8, "length": 24, "barrel": None,
                  "followups": (), "channel": 0, "else_gate": None},
    # Gold Hero dispatches its muzzle on the actor model ID: form 0803 would
    # request type-2 key 34, and any other model fires nothing. The ordinary
    # 0802 path is the type-0 key 45 call below.
    GOLD_HERO: {"installer": 0x8013C714, "consumer": 0x8013CB40, "leaf": 0x8013D600,
                "debit": 0x8013D74C, "muzzle_site": 0x8013D77C, "family": "type-0", "key": 45,
                "joint": 12, "point": (0., -10., 50.), "tone": ("snd_com01", 6),
                "callback": 0x8013CB7C, "main_handler": 0x8013CB40, "relay_site": 0x8013CDCC,
                "config": (0x80339B80, 0x8033D3A0), "context_table": 0x80339E00,
                "roots": (0x80339F20, 0x802BC080, 0x8033A6C0, 0x802BAE60),
                "roles": ((0, 0), (0, 1), (0, 24), (0, 2)), "dash": "bound",
                "model_gate": {"site": 0x8013D758, "branch": 0x8013D760, "form": 0x0803,
                               "form_call": 0x8013D784, "form_key": 34},
                "body": None, "selector": 44, "state": 0, "marker": 8, "length": 24, "barrel": None,
                "followups": (), "channel": 0, "else_gate": None},
    # Carrier Helicopter has no motion bank. Its paid release reads the one-bit
    # barrel byte, which the class initializer zeroes, so the first barrel is
    # key 18. Repeat alternation to key 19 is not implemented.
    CARRIER_HELICOPTER: {"installer": 0x80135EE8, "consumer": 0x80136278, "leaf": 0x801363D4,
                         "debit": 0x8013642C, "muzzle_site": 0x8013644C, "family": "type-0", "key": 18,
                         "joint": 8, "point": (34.43000030517578, 0., 57.150001525878906), "tone": ("snd_com01", 6),
                         "callback": 0x80136278, "main_handler": 0x8013623C, "relay_site": None,
                         "config": 0x80338C80, "context_table": 0x80338F18, "selector": 45, "state": 0,
                         "roots": (0, 0, 0, 0), "roles": (), "dash": "static",
                         "body": "static-grounded", "marker": 12, "length": 28,
                         "model_gate": None, "followups": (), "channel": 0, "else_gate": None,
                         "phase_chain": (0x80338C44, (0x80136334, 0x80136354, 0x801363D4)),
                         "barrel": {"byte": 0x14C, "zero_store": 0x80135FF8, "alternate": 0x80136454,
                                    "alternate_key": 19}},
    # Red Attacker is static too. One debit releases two ordered type-2 calls
    # on the two barrel joints; both stay in this authored pair.
    RED_ATTACKER: {"installer": None, "consumer": 0x80188B24, "leaf": 0x80188FAC,
                   "debit": 0x8018940C, "muzzle_site": 0x80189420, "family": "type-2", "key": 54,
                   "joint": 3, "point": (0., 0., 100.), "tone": ("snd_com02", 57),
                   "callback": 0x80188EC4, "main_handler": 0x80188B24, "relay_site": None,
                   "config": 0x803626A0, "context_table": 0x803628D8, "selector": 45, "state": 2,
                   "roots": (0, 0, 0, 0), "roles": (), "dash": "static",
                   "body": "static-grounded", "marker": 12, "length": 28, "pace": "authored",
                   "model_gate": None, "barrel": None, "else_gate": None,
                   "phase_chain": (0x80362680, (0x80188F00, 0x80188F18, 0x80188FAC)),
                   "followups": ({"site": 0x8018942C, "key": 55, "joint": 4, "point": (0., 0., 100.)},)},
    # Death Eye shares its release leaf with another model. The equality branch
    # is its own key 122; the fall-through requests key 123 for that model.
    DEATH_EYE: {"installer": 0x801E43F8, "consumer": 0x801E4598, "leaf": 0x801E46C8,
                "debit": 0x801E4734, "muzzle_site": 0x801E4754, "family": "type-0", "key": 122,
                "joint": 1, "point": (0., 0., 30.), "tone": ("snd_com01", 6),
                "callback": 0x801E45D0, "main_handler": 0x801E4598, "relay_site": None,
                "config": 0x80391CF8, "context_table": 0x80391ED0, "selector": 45, "state": 0,
                "roots": (0x802BD54C, 0x802BD65E, 0, 0), "roles": (), "dash": "static",
                "body": "static-grounded", "marker": 12, "length": 28,
                "model_gate": None, "barrel": None, "followups": (), "channel": 0, "pace": "authored",
                "phase_chain": (0x80391CD8, (0x801E460C, 0x801E463C, 0x801E46C8)),
                "else_gate": {"lha": 0x801E4740, "cmp": 0x801E4744, "branch": 0x801E4748,
                              "other_call": 0x801E475C, "other_key": 123}},
    # Ultimate Cannon pays on the source's alternate ammo channel, then fires
    # four ordered type-2 calls from its two launcher joints. No dash role
    # binds, so both dash inputs are declined.
    ULTIMATE_CANNON: {"installer": None, "consumer": 0x800B085C, "leaf": 0x800B1918,
                      "debit": 0x800B194C, "muzzle_site": 0x800B1960, "family": "type-2", "key": 4,
                      "joint": 5, "point": (-84.30000305175781, 97.5, 230.89999389648438),
                      "tone": ("snd_com02", 57),
                      "callback": 0x800B0898, "main_handler": 0x800B085C, "relay_site": None,
                      "config": 0x802FC0A0, "context_table": 0x802FC338, "selector": 44, "state": 0,
                      "roots": (0x802FB820, 0x802FBBDA, 0x802FBDE0, 0x802FBF60),
                      "roles": ((0, 25), (0, 1), (0, 24)), "dash": "declined", "body": None,
                      "marker": 8, "length": 24, "channel": 2,
                      "model_gate": None, "barrel": None, "else_gate": None, "followups": (
                          {"site": 0x800B196C, "key": 5, "joint": 5,
                           "point": (-84.30000305175781, -97.5, 230.89999389648438)},
                          {"site": 0x800B1978, "key": 6, "joint": 6,
                           "point": (84.30000305175781, 97.5, 230.89999389648438)},
                          {"site": 0x800B1984, "key": 7, "joint": 6,
                           "point": (84.30000305175781, -97.5, 230.89999389648438)})},
    # Delta II's source B has no release; its X fires type-0 key 59 on the
    # alternate ammo channel. The leaf is shared with 0301/0302/0304 and only
    # model 0303 reaches this call.
    DELTA_II: {"installer": None, "consumer": 0x8010CDEC, "leaf": 0x8010D028,
               "debit": 0x8010D098, "muzzle_site": 0x8010D0AC, "family": "type-0", "key": 59,
               "joint": 7, "point": (-50., 0., 0.), "tone": ("snd_com02", 58),
               "callback": 0x8010CDEC, "main_handler": 0x8019C4D4, "relay_site": None,
               "config": 0x803721B0, "context_table": 0x803723E8, "selector": 44, "state": 3,
               "roots": (0x80372440, 0x802BC080, 0x80372980, 0x802BAE60),
               "roles": ((0, 0), (0, 1), (0, 24)), "dash": "declined", "body": None,
               "marker": 8, "length": 24, "channel": 2, "button": "x", "move": "x-shot", "group": "X",
               "model_gate": None, "barrel": None, "else_gate": None, "followups": (),
               "pinned_words": ((0x8010D028, 0x881F1CEF), (0x8010D02C, 0x7C000775),
                                (0x8010D040, 0xA81F03E8), (0x8010D044, 0x2C000302),
                                (0x8010D050, 0x2C000301), (0x8010D05C, 0x2C000304)),
               "pinned_branches": ((0x8010D048, 0x8010D078), (0x8010D054, 0x8010D068),
                                   (0x8010D060, 0x8010D0BC), (0x8010D064, 0x8010D088))},
    # Beam Tank's B has no release either; its X pays one channel-2 debit and
    # then fires type-2 keys 8 and 9 from the two turret joints.
    BEAM_TANK: {"installer": None, "consumer": 0x80105F30, "leaf": 0x80106028,
                "debit": 0x80106050, "muzzle_site": 0x80106064, "family": "type-2", "key": 8,
                "joint": 9, "point": (0., 0., 100.), "tone": ("snd_com02", 57),
                "callback": 0x80105F30, "main_handler": 0x801052C8, "relay_site": None,
                "config": 0x80321C88, "context_table": 0x80321ED8, "selector": 44, "state": 2,
                "roots": (0x80321920, 0x802BCFA8, 0x80321760, 0x802BBD94),
                "roles": ((0, 25), (0, 26), (0, 24), (0, 2)), "dash": "bound", "body": None,
                "marker": 8, "length": 24, "channel": 2, "button": "x", "move": "x-shot", "group": "X",
                "model_gate": None, "barrel": None, "else_gate": None,
                "followups": ({"site": 0x80106070, "key": 9, "joint": 10, "point": (0., 0., 100.)},)},
}
FACTORY = {0x800C3BE0: "type-2", 0x80082824: "type-0"}
TEXT = (
    (0x80102824, 0x134, "ea1ab3ec0d2d369d5cb9469a8614bfe5c28ae1f9f54e21599af7bcdfe434623b"),
    (0x80102BF8, 0x28C, "7967766d02b9b4e0b8d603512da42890ad1a85e7279123ca7e7b02f31c91f6b1"),
    (0x80102A30, 0x80, "66d3cb317cc71626218c874b15c4c4057ab29bb34ae4648c5a5ca5326cff4154"),
    (0x80103428, 0x154, "ed112c207daa782fc1f2f17e0b945d80c7f952512de7f7728924a4843286826d"),
    (0x8013C714, 0x140, "9c1e3461afe6fe86d8114d110c21a0bd41a35a4e2ee71212508b76a72c732a4a"),
    (0x80136278, 0xBC, "e669bdebd8e97a5bbb01ccfeddb1b7b6138659a7a4d125a0a46b8045aefc97ab"),
    (0x801363D4, 0x110, "1040d82e8a33ad6d1260ab8672a7880f379759852949ee957605badeae7efb40"),
    (0x80135EE8, 0xE0, "2218f4174919722987e4ff6deecc6980043d4faacc5d729ada50654bb2baa393"),
    (0x80135FC8, 0x48, "5c7aa826105114527c9ad5eb7c5f5e3787ba003425aedf3bef19ceb8dc0eda14"),
    (0x801E43F8, 0x15C, "49ab1cb5b5d9804ecba6b2a8b6e5f07896bc508578e6de5d09386911cec18955"),
    (0x801E46C8, 0xD0, "d17e5374836e9af233184fa9a519e19c0fedfa78ed90fcd95f628c1b0891a09f"),
    (0x801E45D0, 0x28, "a2e5724709e70e4f7c8cd93677675c33ce994a146c86af9e3f1c60571f45e241"),
    (0x800B1918, 0x98, "8e443c236a8a60063ae5da61ebb0ba7b1a3f1262d936ad1073ef9296edfebf4c"),
    (0x800B0D18, 0x60, "70d265034b43502b4f5d00dbd9c42dd2720c031f1f8ff6a0b666f9bfd362f6c6"),
    (0x8010D028, 0x94, "ec5f9300209f313fa46c4fa1ec56f5c56692c4c58f6daaa296fa36133fb1d98d"),
    (0x80106028, 0x74, "005eb14dd42dc3ba25ad29f8383d586ab1caa02f6dd00b68975bf403614b2189"),
)


def factory_keys():
    return ({(route["family"], route["key"]) for route in ROUTES.values()} |
            {(route["family"], f["key"]) for route in ROUTES.values() for f in route["followups"]})


def _leaf_checks(linker, model, route):
    leaf, debit, site = route["leaf"], route["debit"], route["muzzle_site"]
    barrel = route["barrel"]
    # The same-actor debit always loads channel 0, cost 1 and commit; a timer
    # update may sit between that load and the call.
    window = [linker.text(debit-4*k) for k in range(7, 0, -1)]
    channel = route.get("channel", 0)
    # Ignore the timer bookkeeping that may sit between the same-actor debit
    # loads; the last register loads must be channel, cost 1 and commit.
    loads = [w for w in window if w>>16 not in (0xA8FF, 0x3807, 0xB01F)]
    if (loads[-3:] != [0x38800000|channel, 0x38A00001, 0x38C00001] or
            direct_branch(debit, linker.text(debit)) != 0x8006DBE0 or linker.text(debit+4) != 0x2C030000 or
            linker.text(site-4) != (0x38800000|route["key"]) or
            FACTORY.get(direct_branch(site, linker.text(site))) != route["family"]):
        raise ValueError(f"single ground shot needs the exact paid release for {model:04X}")
    for followup in route["followups"]:
        # Each ordered release is the same actor with its own factory key.
        if (linker.text(followup["site"]-8) != 0x7FE3FB78 or
                linker.text(followup["site"]-4) != 0x38800000|followup["key"] or
                FACTORY.get(direct_branch(followup["site"], linker.text(followup["site"]))) != route["family"]):
            raise ValueError(f"single ground shot lost an ordered extra release for {model:04X}")
    for address, word in route.get("pinned_words", ()):
        if linker.text(address) != word:
            raise ValueError(f"single ground shot lost a pinned word for {model:04X}")
    for site, target in route.get("pinned_branches", ()):
        if direct_branch(site, linker.text(site)) != target:
            raise ValueError(f"single ground shot lost a pinned branch for {model:04X}")
    gate = route["model_gate"]
    other = route["else_gate"]
    if other is not None:
        # Only this model takes the equality branch; the fall-through key
        # belongs to the model that shares this release leaf.
        if (linker.text(other["lha"]) != 0xA81F03E8 or
                linker.text(other["cmp"]) != 0x2C000000|model or
                direct_branch(other["branch"], linker.text(other["branch"])) != other["other_call"] or
                linker.text(other["other_call"]) != 0x7FE3FB78 or
                linker.text(other["other_call"]+4) != 0x38800000|other["other_key"] or
                FACTORY.get(direct_branch(other["other_call"]+8, linker.text(other["other_call"]+8))) !=
                route["family"] or
                linker.text(route["muzzle_site"]-8) != 0x7FE3FB78):
            raise ValueError(f"single ground shot lost its exact model-else muzzle for {model:04X}")
    if barrel is not None:
        # One debit, then the one-bit barrel byte selects key 18 or key 19 and
        # is toggled. The class initializer zeroes that byte.
        branch, alternate = site-20, barrel["alternate"]
        if (linker.text(branch) != 0x881F0000|barrel["byte"] or linker.text(branch+4) != 0x7C000775 or
                direct_branch(branch+8, linker.text(branch+8)) != alternate or
                linker.text(alternate) != 0x7FE3FB78 or
                linker.text(alternate+4) != 0x38800000|barrel["alternate_key"] or
                FACTORY.get(direct_branch(alternate+8, linker.text(alternate+8))) != route["family"] or
                linker.text(alternate+12) != 0x881F0000|barrel["byte"] or
                linker.text(alternate+16) != 0x68000001 or
                linker.text(alternate+20) != 0x981F0000|barrel["byte"] or
                linker.text(barrel["zero_store"]-4) != 0x38000000 or
                linker.text(barrel["zero_store"]) != 0x981F0000|barrel["byte"]):
            raise ValueError(f"single ground shot lost its exact initial barrel for {model:04X}")
        table, entries = route["phase_chain"]
        if tuple(linker.read_word(table+4*i) for i in range(len(entries))) != entries:
            raise ValueError(f"single ground shot lost its exact phase chain for {model:04X}")
    if gate is None:
        return
    # The muzzle call is reached only for this model: the same actor
    # halfword selects the form's own shot key, and every other model skips.
    if (linker.text(gate["site"]) != 0xA81F03E8 or linker.text(gate["site"]+4) != 0x2C000000|gate["form"] or
            direct_branch(gate["branch"], linker.text(gate["branch"])) != gate["form_call"] or
            linker.text(gate["site"]+16) != 0x2C000000|model or
            direct_branch(gate["site"]+20, linker.text(gate["site"]+20)) != route["muzzle_site"]-8 or
            linker.text(gate["form_call"]+4) != 0x38800000|gate["form_key"] or
            FACTORY.get(direct_branch(gate["form_call"]+8, linker.text(gate["form_call"]+8))) != "type-2"):
        raise ValueError(f"single ground shot lost its exact model-gated muzzle for {model:04X}")


def verify_source(dol, linker):
    for at, size, digest in TEXT:
        if sha256(dol.read(at, size, "text")).hexdigest() != digest:
            raise ValueError(f"single ground shot text signature: {at:08X}")
    for model, route in ROUTES.items():
        _leaf_checks(linker, model, route)


def source_shot(linker, model, actions, joints):
    route = ROUTES[model]
    profiles = actions["profiles"]
    if (not 1 <= len(profiles) <= 2 or any(actions.get(k) for k in
                                           ("refused_bindings", "open_path_boundaries", "unresolved_stores"))):
        raise ValueError("single ground shot needs its exact normal installers")
    configs = (route["config"],) if isinstance(route["config"], int) else route["config"]
    seen = []
    for profile in profiles:
        if (int(profile["context_table"], 16), int(profile["main_handler"], 16),
                tuple(int(r, 16) for r in profile["program_and_descriptor_roots"])) != (
                route["context_table"], route["main_handler"], route["roots"]):
            raise ValueError("single ground shot installer owner")
        seen.append(int(profile["config"], 16))
        rows = [r for g in profile["groups"] if g["group"] == route.get("group", "B-shot")
                for r in g["rows"] if (0, 0) in map(tuple, r["contexts"])]
        if len(rows) != 1 or (rows[0]["state"], rows[0]["substate"], rows[0]["selector"],
                              rows[0].get("class_callback_candidate")) != (route["state"], 0, route["selector"],
                                                                         f"{route['callback']:08X}"):
            raise ValueError("single ground shot exact neutral context")
    if sorted(seen) != sorted(configs):
        raise ValueError("single ground shot exact installer tables")
    basis = "credited-actor-joint" if route["family"] == "type-0" else "parent-joint"
    muzzles = [(route["key"], route["joint"], route["point"])]
    muzzles.extend((f["key"], f["joint"], f["point"]) for f in route["followups"])
    for key, joint, point in muzzles:
        muzzle = linker.origins[route["family"]][key]
        if ((muzzle["source_joint_selector"], tuple(muzzle["local_point"]), muzzle["basis"]) != (joint, point, basis) or
                not 0 <= joint < len(joints) or joints[joint]["index"] != joint):
            raise ValueError("single ground shot exact muzzle")
    if route["relay_site"] is not None:
        relay = linker.text(route["relay_site"])
        if relay>>26 != 18 or not relay&1 or direct_branch(route["relay_site"], relay) != route["leaf"]:
            raise ValueError("single ground shot exact release relay")
    elif route.get("phase_chain"):
        if route["phase_chain"][1][-1] != route["leaf"]:
            raise ValueError("single ground shot exact dispatched release")
    elif route["leaf"] not in {entry[0] for entry in TEXT}:
        raise ValueError("single ground shot needs a pinned release leaf")
    return {"model": f"{model:04X}", "raw_context": [0, 0], "action_group": route.get("group", "B-shot"),
            "release_entry": f"{route['leaf']:08X}",
            "muzzle": {"key": route["key"], "joint": route["joint"], "point": list(route["point"])},
            "debit": {"site": f"{route['debit']:08X}", "channel": route.get("channel", 0), "cost": 1, "commit": True,
                      "same_input_actor": True},
            "dash": route["dash"],
            **({"followups": [{"key": f["key"], "joint": f["joint"], "point": list(f["point"])}
                              for f in route["followups"]]} if route["followups"] else {}),
            "scope": "ordinary ranged B only; no X, charge, form, or source timing"
                     + ("; initial barrel only, repeat alternation not implemented" if route["barrel"] else ""),
            **({"barrel": {"byte": f"{route['barrel']['byte']:X}",
                           "zero_store": f"{route['barrel']['zero_store']:08X}",
                           "initial_key": route["key"], "alternate_key": route["barrel"]["alternate_key"]}}
               if route["barrel"] else {}),
            "live_equivalence_verified": False}


def battle_test(model, joints, role_clips, pace, level, source, sounds, *, launch_audio=False):
    route = ROUTES[model]
    expected = {"key": route["key"], "joint": route["joint"], "point": list(route["point"])}
    followups = ([{"key": f["key"], "joint": f["joint"], "point": list(f["point"])} for f in route["followups"]]
                 if route["followups"] else None)
    if (source["model"], source["action_group"], source["muzzle"], source["debit"], source["dash"],
            source.get("followups")) != (
            f"{model:04X}", route.get("group", "B-shot"), expected,
            {"site": f"{route['debit']:08X}", "channel": route.get("channel", 0), "cost": 1, "commit": True,
             "same_input_actor": True},
            route["dash"], followups):
        raise ValueError("single ground shot requires its exact muzzle, debit and dash policy")
    roles = {name: (g, c) for name, g, c in role_clips}
    required = (("ground-stop", "ground-travel", "air-continuation") +
                (("ground-dash-forward",) if route["dash"] == "bound" else ())
                if route["body"] is None else ())
    if tuple(roles.get(r) for r in required) != route["roles"]:
        raise ValueError("single ground shot needs its exact indexed local poses")
    authored_pace = route.get("pace") == "authored"
    walk = pace["native_units_per_tick_x100"] if pace else (600 if authored_pace else 0)
    hp, initial, limit, mode, _ = level[:5]
    if not 1 <= hp <= 10000 or not 1 <= initial == limit <= 255 or mode not in (0, 1) or not 100 <= walk <= 1042:
        raise ValueError("single ground shot HP, stock or pace domain")
    description = ("static turret, initial barrel only" if route["body"] else
                   "both dash inputs declined" if route["dash"] == "declined" else "retains locomotion")
    comment = ("# Authored static turret shot; not retail combat." if route["body"] else
               "# Authored ground shot with both dash inputs declined; not retail combat."
               if route["dash"] == "declined" else "# Authored ground shot; retains locomotion; not retail combat.")
    lines = ["GFBORG-GAMEPLAY 1", comment,
             "variant=normal", f"health={hp}", f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
             "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0"]
    if route["body"]:
        lines.append(f"body-motion={route['body']}")
    else:
        lines.extend(f"{field}={roles[name][0]}:{roles[name][1]}"
                     for field, name in zip(("idle", "walk", "air", "dash"), required))
    if route["dash"] == "declined":
        lines.append("dash-policy=disabled")
    lines.append(f"stock=shot-stock|{limit}|180")
    names = ("shot", "shot-second", "shot-third", "shot-fourth", "shot-fifth", "shot-sixth")
    shots = list(zip(names, (route["key"], *(f["key"] for f in route["followups"])),
                     (route["joint"], *(f["joint"] for f in route["followups"])),
                     (route["point"], *(f["point"] for f in route["followups"]))))
    for name, key, joint, point in shots:
        local = "|".join(f"{v:.9g}" for v in point)
        lines.extend((f"projectile={name}|{'beam' if model == BEAM_GUNNER else 'bullet'}|1|43|0|0|0|24|85|8|ignore-floor",
                      f"origin={name}|{joint}|{local}|target-height"))
    move = route.get("move", "basic-shot")
    button = route.get("button", "b")
    lines.extend((f"move={move}|{button}|10|grounded|{route['length']}|none",
                  f"entry-stock={move}|shot-stock|1",
                  f"release={move}|{route['marker']}|shot-stock|1|shot"))
    for name, *_ in shots[1:]:
        lines.append(f"release-followup={move}|{route['marker']}|{name}")
    if mode == 1:
        lines.append("stock-clock=shot-stock|add-on-debit")
    audio = []
    if launch_audio:
        bank, tone = route["tone"]
        path = f"audio/launch-{bank}-{tone}.pcm"
        if pilot_launch_tone(sounds[route["family"]][route["key"]]) != (bank, tone):
            raise ValueError("single ground shot did not join its exact launch tone")
        for follow in route["followups"]:
            if pilot_launch_tone(sounds[route["family"]][follow["key"]]) != (bank, tone):
                raise ValueError("single ground shot followup did not join its exact launch tone")
        for name, *_ in shots:
            lines.append(f"launch-sound={name}|{path}")
            audio.append({"projectile": name, "bank": bank, "tone": tone, "path": path})
    return ("\n".join(lines)+"\n").encode("ascii"), whole_body_pose(joints), {
        "kind": f"authored single ground shot, exact poses, {description}", "retail_behavior_verified": False,
        "preview_clip": "none", "body_binding": "strict record zero", "source_level": 1,
        "source_release_entry": source["release_entry"], "source_debit_site": source["debit"]["site"],
         "source_normal_factory_family": route["family"], "source_normal_factory_key": route["key"],
         "reaction_releases": {move: [(route["family"], key) for _, key, _, _ in shots]},
        "native_timing": {"release_tick": route["marker"], "move_ticks": route["length"], "refill_ticks": 180},
        **({"native_pace": "authored 6.00; this static body never walks, so no source-relative pace applies"}
           if authored_pace else {}),
        "declined": {"dash": "both ground and airborne dash inputs refuse; source dash pose unbound"}
        if route["dash"] == "declined" else {},
        **({"source_model_gated_muzzle": f"only model {model:04X}; form {route['model_gate']['form']:04X} "
                                          f"would request {route['model_gate']['form_key']}"}
           if route["model_gate"] else {}),
        **({"source_selector_scope": "initial barrel only; repeat alternation not implemented",
            "source_barrel": {"byte": f"{route['barrel']['byte']:X}",
                              "zero_store": f"{route['barrel']['zero_store']:08X}",
                              "alternate_key": route["barrel"]["alternate_key"]}}
           if route["barrel"] else {}),
        "authored_launch_sounds": audio,
    }
