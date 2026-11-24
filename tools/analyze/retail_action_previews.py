"""Publish a bounded set of raw action-entry clip candidates for inspection."""

PREVIEW_GROUPS = ("B-shot", "B-close", "X", "B-charge", "X-charge")


def action_preview_candidates(actions, *, preferred_b_shot=None):
    """Keep the first agreeing raw context per group, not a gameplay input map."""
    profiles = actions["profiles"]
    if not profiles or any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")):
        return []
    result = []
    for name in PREVIEW_GROUPS:
        contexts, modes = [], set()
        for profile in profiles:
            group, = (g for g in profile["groups"] if g["group"]==name)
            modes.add(group["mode"])
            contexts.append({tuple(context):(row, row["entry_clip_candidate"])
                             for row in group["rows"] if row.get("entry_clip_candidate")
                             for context in row["contexts"]})
        if len(modes)!=1 or not all(contexts):
            if name=="B-shot" and len(modes)==1 and preferred_b_shot is None:
                result.extend(conditional_action_previews(profiles,name))
            continue
        shared = set.intersection(*(set(rows) for rows in contexts))
        if name=="B-shot" and preferred_b_shot is not None:
            shared &= {preferred_b_shot}
        for context in sorted(shared):
            keys = {(tuple(rows[context][1]["clip"]),rows[context][1]["archive_offset"]) for rows in contexts}
            if len(keys)!=1:
                continue
            (group,choice),archive=next(iter(keys))
            result.append({"id":f"{name.lower()}-c{context[0]}-s{context[1]}",
                           "action_group":name,"context_mode":next(iter(modes)),
                           "raw_context":context,"clip":[group,choice],"archive_offset":archive,
                           "runtime_input_verified":False,
                           "routes":[{"config":profile["config"],"context_table":profile["context_table"],
                                      "main_handler":profile["main_handler"],
                                      "state":rows[context][0]["state"],"substate":rows[context][0]["substate"],
                                      "selector":rows[context][0]["selector"],**rows[context][1]}
                                     for profile,rows in zip(profiles,contexts,strict=True)]})
            break
        if name=="B-shot" and preferred_b_shot is None and not any(p["action_group"]==name for p in result):
            result.extend(conditional_action_previews(profiles,name))
    # A later pose can be more directly related to the release, but is not
    # the action entry. Keep both labels when exact installer contexts agree.
    release_rows=[]
    for p in profiles:
        release_rows.append([row for g in p["groups"] if g["group"]=="B-shot" for row in g["rows"]
                             if row.get("release_pose_candidate") and (0,0) in map(tuple,row["contexts"])])
    if all(len(rows)==1 for rows in release_rows):
        choices=[rows[0]["release_pose_candidate"] for rows in release_rows]
        if all(v==choices[0] for v in choices):
            p=choices[0]
            result.append({"id":"b-shot-release-pose","action_group":"B-shot","raw_context":[0,0],
                           "clip":p["clip"],"archive_offset":p["archive_offset"],"runtime_input_verified":False,
                           "routes":[{"config":pr["config"],"context_table":pr["context_table"],"main_handler":pr["main_handler"],
                                      **{k:rows[0][k] for k in ("state","substate","selector")},**choice}
                                     for pr,rows,choice in zip(profiles,release_rows,choices,strict=True)]})
    return result


def conditional_action_previews(profiles,name):
    """Preserve all reviewed alternatives or publish none of the pair."""
    contexts,modes = [],set()
    for profile in profiles:
        group, = (g for g in profile["groups"] if g["group"]==name)
        modes.add(group["mode"])
        contexts.append({tuple(context):(row,row["entry_clip_alternatives"])
                         for row in group["rows"] if row.get("entry_clip_alternatives")
                         for context in row["contexts"]})
    if len(modes)!=1 or not all(contexts):
        return []
    for context in sorted(set.intersection(*(set(rows) for rows in contexts))):
        _, first = contexts[0][context]
        if len(first)!=2 or [p["program"][1] for p in first]!=[0,1]:
            continue
        if any(rows[context][1]!=first for rows in contexts):
            continue
        result=[]
        for i,clip in enumerate(first):
            result.append({"id":f"{name.lower()}-c{context[0]}-s{context[1]}-p{i}",
                           "action_group":name,"context_mode":next(iter(modes)),"raw_context":context,
                           "clip":clip["clip"],"archive_offset":clip["archive_offset"],
                           "request_choice_condition":clip["request_choice_condition"],"runtime_input_verified":False,
                           "routes":[{"config":profile["config"],"context_table":profile["context_table"],
                                      "main_handler":profile["main_handler"],"state":rows[context][0]["state"],
                                      "substate":rows[context][0]["substate"],"selector":rows[context][0]["selector"],**clip}
                                     for profile,rows in zip(profiles,contexts,strict=True)]})
        return result
    return []
