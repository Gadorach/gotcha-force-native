"""Authored breath proxy for Dragons whose B drives a phase/effect window.

The source release is a paid debit followed by the shared program driver
8006D144; no origin factory, socket or area row is reached. The engine's
sustained-beam capability exists for exactly this shape, so this module
binds the exact debit, body, poses and tail and declares an authored
resident beam. The eight Dragons also get one airborne form: the two
mechanical Dragons fire straight down, and the other six sweep at a fixed
angle. Length, cadence, damage, sweep geometry and the caster joint are
native tuning, not retail breath values.
"""
from hashlib import sha256

from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_basic_shot_test import whole_body_pose
from tools.analyze.retail_dragon_shot import trailing_chain


DRIVER = 0x8006D144
DEBIT = 0x8006DBE0
# model: neutral B callback, first paid release, program driver, span, program id
COHORTS = {
    0x0502: (0x80142B30, 0x80142D40, 0x80142C48, 0x400, 0xC1),
    0x050E: (0x80142B30, 0x80142D40, 0x80142C48, 0x400, 0xC1),
    0x0503: (0x800B8F10, 0x800B93AC, 0x800B91D4, 0x500, 0x81),
    0x050F: (0x800B8F10, 0x800B93AC, 0x800B91D4, 0x500, 0x81),
    0x0504: (0x80108FBC, 0x801091D0, 0x80109114, 0x400, 0xC1),
    0x0510: (0x80108FBC, 0x801091D0, 0x80109114, 0x400, 0xC1),
    0x0506: (0x800B8AD0, 0x800B8CDC, 0x800B8FBC, 0x500, 0xC1),
    0x0512: (0x800B8AD0, 0x800B8CDC, 0x800B8FBC, 0x500, 0xC1),
}
# 0503 and 050f request their own neutral clip (local 0:1). The other four
# request none; the engine now admits a field-only pose-free move, so their
# body keeps its locomotion pose while the authored field is placed.
# The X action is a per-model mode switch (table 802FF3FC: 0506 mode 4,
# 050F mode 8, 0512 mode 12), not a shot. The two mechanical Dragons fire
# their flying-mode beam straight down from the chest.
DIVE = {0x0506, 0x0512}
# The other six switch to a fixed-angle sweeping beam while airborne. It uses
# the engine's sustained-beam sweep with `clip-on-stage`, which binds when the
# stage supplies the qualified st42 layers (the demo passes them through
# --source-ground-assets). The ground/sweep origin is the root joint: the
# airborne pose leaves the chest socket unresolved (a joint-12 launch landed
# at 1359 units above the actor), so the sweep is anchored 40 up and 60
# forward of the root instead.
SWEEP = {0x0502, 0x0503, 0x0504, 0x050E, 0x050F, 0x0510}
BOUND_POSE = {0x0503: (0, 1), 0x050F: (0, 1),
              0x0504: None, 0x0510: None, 0x0506: None, 0x0512: None,
              0x0502: None, 0x050E: None}
# Phoenix and Wing Dragon share the same paid leaf but use movement map type 5
# and a different tail topology, so their idle pose needs its own review.
MODELS = set(BOUND_POSE)


def _span_texts(dol):
    seen = {}
    for callback, _, _, size, _ in COHORTS.values():
        seen[(callback, size)] = sha256(dol.read(callback, size, "text")).hexdigest()
    return seen


def verify_source(dol, linker):
    for (callback, size), digest in _span_texts(dol).items():
        if sha256(dol.read(callback, size, "text")).hexdigest() != digest:
            raise ValueError(f"breath pilot span signature: {callback:08X}")
    for model, (callback, debit, driver, _, program) in COHORTS.items():
        if (tuple(linker.text(debit-12+4*i) for i in range(3)) != (0x38800000, 0x38A00001, 0x38C00001) or
                direct_branch(debit, linker.text(debit)) != DEBIT or linker.text(debit+4) != 0x2C030000 or
                direct_branch(driver, linker.text(driver)) != DRIVER or
                not any(linker.text(driver-4*k) == 0x38800000 | program for k in range(1, 13))):
            raise ValueError(f"breath pilot needs the exact paid program window for {model:04X}")


def source_breath(linker, model, actions):
    callback, debit, driver, _, program = COHORTS[model]
    if (not actions["profiles"] or any(actions.get(k) for k in
                                       ("refused_bindings", "open_path_boundaries", "unresolved_stores"))):
        raise ValueError("breath pilot needs its exact normal installers")
    rows = []
    for profile in actions["profiles"]:
        rows.extend(r for g in profile["groups"] if g["group"] == "B-shot" for r in g["rows"]
                    if (0, 0) in map(tuple, r["contexts"]))
    if not rows or any(r.get("class_callback_candidate") != f"{callback:08X}" for r in rows):
        raise ValueError("breath pilot exact neutral B context")
    pose = BOUND_POSE[model]
    requested = [tuple(r.get("entry_clip_candidate", {}).get("clip", ())) for r in rows]
    if pose is None:
        if any(requested):
            raise ValueError("breath pilot expected no neutral B pose")
    elif any(r != pose for r in requested):
        raise ValueError("breath pilot exact neutral B pose")
    return {"model": f"{model:04X}", "raw_context": [0, 0], "action_group": "B-shot",
            "callback": f"{callback:08X}",
            "pose": list(BOUND_POSE[model]) if BOUND_POSE[model] else None,
            "debit": {"site": f"{debit:08X}", "channel": 0, "cost": 1,
                                                     "commit": True, "same_input_actor": True},
            "program_driver": {"site": f"{driver:08X}", "helper": f"{DRIVER:08X}", "id": program},
            "origin_evidence": "none; the leaf reaches no origin factory, socket or area row",
            "attack": "authored forward field; not retail breath geometry, damage or timing",
            "live_equivalence_verified": False}


def _idle(mapping):
    idles = set()
    for profile in mapping["profiles"]:
        if profile["movement_type"] not in (4, 5):
            raise ValueError("breath pilot needs a scoped Dragon movement map")
        candidates = [b for r in profile["roles"] if r["selector"] == 0 and r["states"] == [1, 1] and
                      r["handler_addresses"] == ["8005F23C", "8005F2A4"] for q in r["program_candidates"]
                      if (q["site"], q["lanes"], q["program_group"], q["program_choice"]) == ("8005F2F8", 2, 0, 0)
                      for c in q["clip_candidates"] if c["preceding_waits"] == 0 and c["start_frame"] == 0
                      for b in c["lanes"] if b["lane"] == 1 and b["bank"] == "local" and
                      b["archive_offset"] is not None]
        if len(candidates) != 1:
            raise ValueError("breath pilot lacks the exact frame-zero idle")
        b = candidates[0]
        idles.add((b["group"], b["choice"]))
    if len(idles) != 1:
        raise ValueError("breath pilot idle differs across profiles")
    return idles.pop()


def battle_test(model, joints, roles, mapping, pace, level, source, *, chain=None):
    expected = {"debit": {"site": f"{COHORTS[model][1]:08X}", "channel": 0, "cost": 1, "commit": True,
                          "same_input_actor": True},
                "pose": list(BOUND_POSE[model]) if BOUND_POSE[model] else None}
    if any(source[k] != v for k, v in expected.items()):
        raise ValueError("breath pilot requires its exact paid debit")
    clips = {name: (g, c) for name, g, c in roles}
    if not pace or "ground-travel" not in clips or "air-continuation" not in clips:
        raise ValueError("breath pilot lacks exact movement evidence")
    idle = _idle(mapping)
    hp, initial, limit, mode, _ = level[:5]
    walk = 2*pace["native_units_per_tick_x100"]  # same authored relative tuning as the elemental cohort
    if not 1 <= hp <= 10000 or not 1 <= initial == limit <= 255 or mode not in (0, 1) or not 100 <= walk <= 1042:
        raise ValueError("breath pilot HP, stock or pace domain")
    lines = ["GFBORG-GAMEPLAY 1", "# Authored sustained breath beam; not retail breath behaviour.",
             "variant=normal", f"health={hp}", f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
             "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0",
             "dash-policy=disabled",
             f"idle={idle[0]}:{idle[1]}",
             f"walk={clips['ground-travel'][0]}:{clips['ground-travel'][1]}",
             f"air={clips['air-continuation'][0]}:{clips['air-continuation'][1]}"]
    if chain is not None:
        lines.append("trailing-chain="+"|".join(map(str, chain["bones"])))
    lines.extend((f"stock=breath-stock|{limit}|180",
                  "projectile=breath|beam|1|1|0|0|0|60|30|6|ignore-floor",
                  "sustained-beam=breath|1500|8|0",
                  "look=breath|160|255|150|60",
                  "origin=breath|0|0|40|60|target-height",
                  ("move=breath-test|b|10|grounded|48|none" if source["pose"] is None else
                   f"move=breath-test|b|10|grounded|48|{source['pose'][0]}:{source['pose'][1]}"),
                  "activation=breath-test|release-window|24|90",
                  "release=breath-test|12|breath-stock|1|breath"))
    if model in DIVE:
        lines.extend(("projectile=dive|beam|1|30|-100|0|0|60|25|40|stop-on-floor",
                      "origin=dive|12|0|0|0|fixed",
                      "move=breath-dive|b|11|airborne|40|none",
                      "release=breath-dive|8|breath-stock|1|dive"))
    elif model in SWEEP:
        lines.extend(("projectile=sweep|beam|1|1|0|0|0|60|40|8|clip-on-stage",
                      "sustained-beam=sweep|2400|6|0",
                      "beam-sweep=sweep|1200|300|180",
                      "look=sweep|160|255|150|60",
                      "origin=sweep|0|0|40|60|fixed",
                      "move=breath-sweep|b|11|airborne|60|none",
                      "release=breath-sweep|8|breath-stock|1|sweep"))
    if mode == 1:
        lines.append("stock-clock=breath-stock|add-on-debit")
    return ("\n".join(lines)+"\n").encode("ascii"), whole_body_pose(joints), {
        "kind": ("authored sustained breath beam on a pose-free action" if source["pose"] is None else
                 "authored sustained breath beam, exact body and paid release"),
        "retail_behavior_verified": False,
        "preview_clip": "none", "body_binding": "strict record zero", "source_level": 1,
        "source_pose": source["pose"],
        "source_debit_site": source["debit"]["site"], "source_program_driver": source["program_driver"],
        "source_origin_evidence": source["origin_evidence"],
        "native_beam": {"length": 1500, "interval": 8, "turn": 0, "radius": 60, "lifetime": 30, "damage": 6,
                        "look": {"size": 160, "rgb": [255, 150, 60]},
                        "charge": {"activation": "release-window", "hold": [24, 90]},
                        "ground_form": "horizontal only, no committed pitch",
                        "floor": "ignore-floor",
                        "origin": {"joint": 0, "local": [0, 40, 60],
                                   "evidence": "authored caster joint, not an indexed socket"}},
        **({"native_sweep": {"projectile": "sweep", "origin_joint": 0, "origin_local": [0, 40, 60],
                             "length": 2400, "interval": 6, "travel": 1200, "drop": 300,
                             "impact_radius": 180, "damage": 8, "floor": "clip-on-stage",
                             "authored": "fixed-angle airborne sweep; geometry and damage are native tuning",
                             "verified": "stage-impact plus actor contact and damage on the qualified "
                                         "retail-floor stage with its source grids"}}
           if model in SWEEP else {}),
        **({"native_dive": {"projectile": "dive", "origin_joint": 12, "speed": 30, "rise": -100,
                            "radius": 60, "lifetime": 25, "damage": 40, "floor": "stop-on-floor",
                            "authored": "downward flying beam; geometry and damage are native tuning"}}
           if model in DIVE else {}),
        "native_pace": walk, "trailing_chain": chain,
        "authored": "beam length, cadence, damage, radius, charge window, sweep angle and caster joint are "
                    "native tuning, not retail values",
    }


def chain_for(joints):
    """Bind the reviewed 27-31 tail only where its exact topology holds."""
    try:
        return trailing_chain(joints)
    except ValueError:
        return None
