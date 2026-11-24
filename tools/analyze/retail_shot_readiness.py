"""Separate missing source joins, structural clip refusals, and native trials."""
from tools.analyze.retail_action_previews import action_preview_candidates
from tools.analyze.retail_basic_shot_test import shot_context_hits


LOCOMOTION = ("ground-stop","ground-travel","air-continuation","ground-dash-forward")


def clip_refusals(summary, clips):
    archives = {(g,c):at for g,c,at in summary["raw_animation_slots"]}
    metadata = {r["archive_offset"]:r for r in summary["local_clip_timing"]}
    joints = len(summary["logical_joints"])
    refusals = []
    for name,clip in clips:
        record = metadata.get(archives.get(tuple(clip)))
        if record is None:
            issues = ["clip-slot-unavailable"]
        elif record["aobj_timing"]!="indexed":
            issues = ["clip-metadata-unresolved"]
        else:
            issues = list(record.get("strict_metadata_refusals",()))
            if not 0<record["public_joint_count"]<=joints:
                issues.append("public-joints-outside-rig")
        if issues:
            refusals.append({"role":name,"clip":clip,"reasons":issues})
    return refusals


def shot_readiness(summary, releases):
    if "held_fire_battle_test" in summary:
        return {"status":"authored-held-fire-test-declared","preview_clip":summary["held_fire_battle_test"]["preview_clip"]}
    if "basic_shot_battle_test" in summary:
        return {"status":"authored-test-declared","preview_clip":summary["basic_shot_battle_test"]["preview_clip"]}
    actions = summary["action_candidates"]
    profiles = actions["profiles"]
    if not profiles or any(actions.get(k) for k in ("refused_bindings","open_path_boundaries","unresolved_stores")):
        return {"status":"installer-context-unresolved"}
    roles = {name:[g,c] for name,g,c in summary["role_preview_clips"]}
    gaps = [name for name in LOCOMOTION if name not in roles]
    if gaps:
        return {"status":"missing-grounded-locomotion","missing_roles":gaps}
    if not summary.get("lab_ground_pace"):
        return {"status":"walk-operand-unbound"}
    if worm := summary.get("worm_shot_source"):
        clip = worm["pose"]["clip"]
        refused = clip_refusals(summary,[(name,roles[name]) for name in LOCOMOTION]+[("B-shot",clip)])
        info = {"preview_clip":"b-shot-release-pose","clip":clip,"raw_context":worm["stationary_b_context"],
                "factory":worm["family"],"key":worm["key"],"release_site":worm["call"],"debit_site":worm["stock"]["site"]}
        if refused:
            return {"status":"strict-clip-metadata-refusal","contexts":[{**info,"clip_refusals":refused}]}
        return {"status":"source-qualified-opt-in-not-installed","contexts":[info]}
    contexts = {tuple(context) for p in profiles for g in p["groups"] if g["group"]=="B-shot"
                for row in g["rows"] if row.get("entry_clip_candidate") for context in row["contexts"]}
    candidates = {c["id"]:c for c in action_preview_candidates(actions) if c["action_group"]=="B-shot"}
    for context in sorted(contexts):
        for clip in action_preview_candidates(actions,preferred_b_shot=context):
            if clip["action_group"]=="B-shot":
                candidates[clip["id"]] = clip
    if not candidates:
        return {"status":"missing-correlated-shot-pose"}
    trials = []
    for clip in candidates.values():
        context=tuple(clip["raw_context"])
        attempt = {"preview_clip":clip["id"],"clip":clip["clip"],"raw_context":context}
        refusal=clip_refusals(summary,[(name,roles[name]) for name in LOCOMOTION]+[("B-shot",clip["clip"])])
        if refusal:
            trials.append({**attempt,"status":"strict-clip-metadata-refusal","clip_refusals":refusal})
            continue
        all_hits = [shot_context_hits(clip,expected,releases) for expected in clip["routes"]]
        if not all_hits or not all(all_hits):
            status="missing-correlated-release"
        elif any(len(hits)!=1 for hits in all_hits):
            status="multiple-release-alternatives"
        else:
            pairs = [hits[0] for hits in all_hits]
            hit = pairs[0][0]
            guards = [r.get("local_stock_guards",()) for _,r in pairs]
            if any(h!=hit for h,_ in pairs):
                status="installer-release-disagreement"
            elif (not hit["joint_present_in_normal_rig"] or not hit["parent_model_matches"] or
                    hit["basis"] not in ("credited-actor-joint","parent-joint","retained-actor-joint") or
                    hit.get("adds_parent_offset") or hit.get("adds_retained_offset")):
                status="unsupported-socket-basis"
            elif any(len(g)!=1 or g!=guards[0] for g in guards):
                status="missing-unambiguous-debit"
            else:
                debit = guards[0][0]
                attempt.update(factory=hit["family"],key=hit["key"],release_site=hit["site"],debit_site=debit["debit_site"])
                if (debit["normal_level_operand_channel"],debit["cost"])!=(0,1):
                    status="unsupported-paid-channel-or-cost"
                else:
                    initial,limit,mode,preset = summary["raw_level_rows"][0][1:5]
                    status=("needs-strict-native-trial" if 1<=initial==limit<=255 and mode in (0,1) and 1<=preset<=3600
                            else "unsupported-stock-policy")
        trials.append({**attempt,"status":status})
    status = "needs-strict-native-trial" if any(t["status"]=="needs-strict-native-trial" for t in trials) else (
        "strict-clip-metadata-refusal" if all(t["status"]=="strict-clip-metadata-refusal" for t in trials)
        else "no-qualified-shot-context")
    return {"status":status,"contexts":trials}
