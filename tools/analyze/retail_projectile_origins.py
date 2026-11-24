"""Static GG4E08 projectile origin rows, not an attack or direction map."""
import math
import struct


# The first word immediately after each contiguous table bounds that table,
# not a guess based on which candidate floats look plausible.
TABLES = {
    "type-0": (0x802D7B10, 56, 129, 2, 0x80082B08),
    "type-2": (0x80303138, 68, 123, 1, 0x800C3D2C),
    "type-14": (0x802D39B0, 52, 5, 2, 0x0F0A0D0F),
    "type-15": (0x802D6D68, 56, 10, 2, 0x8007DD84),
    "type-47": (0x80301AE8, 48, 6, 2, 0x800BECB8),
}


def origin_tables(dol):
    result = {}
    for family, (base, stride, count, bone_offset, tail) in TABLES.items():
        if struct.unpack(">I", dol.read(base+stride*count, 4, "data")) != (tail,):
            raise ValueError(f"{family}: unexpected origin table boundary")
        block = dol.read(base, stride*count, "data")
        records = []
        for key in range(count):
            raw = block[key*stride:(key+1)*stride]
            joint = struct.unpack_from(">b", raw, bone_offset)[0]
            point = struct.unpack_from(">3f", raw, 4)
            if joint >= 32 or not all(math.isfinite(v) and abs(v) <= 1e5 for v in point):
                raise ValueError(f"{family}: unsupported source origin operands")
            # A negative selector is a separate branch; do not clamp it to
            # logical joint zero. Preserve the frame owner, not damage credit.
            if joint >= 0:
                basis = {"type-0": "credited-actor-joint", "type-2": "parent-joint",
                          "type-14": "retained-actor-joint", "type-15": "parent-joint", "type-47": "parent-joint"}[family]
            else:
                basis = {"type-2": "parent-root-matrix", "type-14": "caller-root-matrix"}.get(family, "unresolved")
            row = {"key": key, "basis": basis, "source_joint_selector": joint,
                   "local_point": point}
            if family == "type-15":
                if joint < 0:
                    raise ValueError("type-15 requires an actual joint")
                row["required_parent_model"] = struct.unpack_from(">H", raw, 46)[0]
                row["adds_parent_offset"] = bool(raw[44])
            elif family == "type-14":
                row["adds_retained_offset"] = key == 2
            records.append(row)
        result[family] = records
    return result


# Reviewed ordinary-B release links. Sparse by design: a shared row is not
# assigned by Borg family, similar geometry, or matching weapon display name.
REVIEWED = {
    0x000A: ("type-14", 4, "borg-controller-audit"),
    0x0600: ("type-2", 21, "retail-borg-package-track"),
    0x0615: ("type-2", 43, "borg-controller-audit"),
}


def origin_candidates(tables, model, logical_joints):
    row = None
    if model in REVIEWED:
        family, key, reference = REVIEWED[model]
        row = {"family": family, **tables[family][key], "reference": reference,
               "action_scope": "reviewed ordinary B request"}
    eligible = [{"family": "type-15", **r}
                for r in tables["type-15"] if r["required_parent_model"] == model]
    joined = ([row] if row else []) + eligible
    for item in joined:
        selector = item["source_joint_selector"]
        item["logical_joint_present"] = bool(logical_joints) and 0 <= selector < len(logical_joints)
    if row and not row["logical_joint_present"]:
        raise ValueError("reviewed launch joint missing from exact normal rig")
    return {"reviewed_ordinary_b": row, "parent_matching_type15_rows": eligible,
            "other_origins": "unresolved", "live_launch_verified": False}


def release_index(actions):
    """Compact review view; alternatives and exact installer contexts survive."""
    names=("config","context_table","main_handler","action_group","state","substate",
           "selector","context_rows","scope","relay_site","relay_handler","argument_evidence")
    phase_fields=("phase_table","phase_entry","phase_transition_stores","phase_transition_calls","reviewed_caller_model",
                  "release_entry")
    grouped = {}
    for profile in actions["profiles"]:
        for group in profile["groups"]:
            for row in group["rows"]:
                for request in row.get("request_candidates", ()):
                    if request["kind"]!="projectile":
                        continue
                    key=(request["site"], request["family"], request["key"])
                    if key not in grouped:
                        fields=("site","family","key","basis","source_joint_selector","local_point",
                                "joint_present_in_normal_rig","parent_model_matches",
                                "required_parent_model","adds_parent_offset","adds_retained_offset",
                                "required_model_at_call","excluded_models_at_call","release_guard_candidate",
                                "source_side_choice","source_side_actor_byte")
                        grouped[key]=({k:request[k] for k in fields if k in request},set())
                    # Installer context tables are correlated with their class
                    # handler. Neither a key nor a display slot is an action.
                    grouped[key][1].add((profile["config"],profile["context_table"],profile["main_handler"],
                                         group["group"],row["state"],row["substate"],row["selector"],
                                          tuple(map(tuple,row["contexts"])),request["scope"],
                                          request.get("release_relay_site",""),request.get("release_relay",""),
                                          request["argument_evidence"],request.get("phase_table",""),
                                          request.get("phase_entry",""),tuple(request.get("phase_transition_stores",())),
                                          tuple(request.get("phase_transition_calls",())),request.get("reviewed_caller_model",""),
                                          request.get("release_entry","")))
    def route(values):
        return {**dict(zip(names,values[:len(names)],strict=True)),
                **{k:v for k,v in zip(phase_fields,values[len(names):],strict=True) if v}}
    return [{**entry,"routes":[route(r) for r in sorted(routes)]}
            for _,(entry,routes) in sorted(grouped.items())]


def preview_origin_markers(releases):
    """Draw only exact points expressible in the current normal rig."""
    markers = {}
    for row in releases:
        if (row["basis"] not in {"credited-actor-joint","parent-joint","retained-actor-joint"} or
                not row["joint_present_in_normal_rig"] or not row["parent_model_matches"] or
                row.get("adds_parent_offset") or row.get("adds_retained_offset")):
            continue
        ident=f'{row["family"]}-{row["key"]}'
        value=(row["source_joint_selector"],*row["local_point"])
        if markers.setdefault(ident,value)!=value:
            raise ValueError("inconsistent preview point for exact factory key")
    if len(markers)>16:
        raise ValueError("preview origin marker budget")
    return [(ident,*markers[ident]) for ident in sorted(markers)]
