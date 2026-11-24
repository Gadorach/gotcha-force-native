"""Explicit source-relative ground-pace conversion for the native lab."""
import math


def ground_pace_candidate(roles, preview_clips, operands, *, static_ground_body=False):
    """Return an opt-in native target, never infer an unbound source record."""
    profiles=roles["profiles"] if roles else []
    if not profiles or not static_ground_body and not any(r[0]=="ground-travel" for r in preview_clips):
        return None
    alternate = static_ground_body and all(p["movement_type"]==6 for p in profiles)
    if alternate:
        for profile in profiles:
            rows=[r for r in profile["roles"] if r["selector"] in (2,30)]
            if (len(rows)!=2 or {r["selector"] for r in rows}!={2,30} or any(
                    r["role_hint"]!="alternate-ground-travel" or r["states"]!=[0,36] or
                    r["handler_addresses"]!=["8005F238","80063A90"] or r["program_candidates"] for r in rows)):
                return None
    elif not all(any(r["role_hint"]=="ground-travel" and r["selector"]==2 and r["states"]==[3,3] and
                   r["handler_addresses"]==["8005F554","8005F578"] for r in p["roles"])
               for p in profiles):
        return None
    if static_ground_body and not alternate:
        for profile in profiles:
            travel=[r for r in profile["roles"] if r["selector"]==2]
            if profile["movement_type"]!=1 or len(travel)!=1:
                return None
            requests=travel[0]["program_candidates"]
            if (len(requests)!=2 or any(r.get("request_scope")!="carried-from-ground-turn" or r.get("source_selector")!=1 or
                                       r["program_status"]!="terminal_wait" or r["clip_candidates"] or
                                       (r["program_group"],r["program_choice"])!=(0,1) for r in requests) or
                    {(r["site"],r["lanes"]) for r in requests}!={("8005F380",13),("8005F4FC",2)}):
                return None
    binding=operands["class_installer_binding"]
    selected=binding["selected_record"]
    if binding["status"]!="one-static-slot-0-record" or type(selected) is not int or not 0<=selected<len(operands["records"]):
        return None
    record=operands["records"][selected]
    source=record["operands"]["02C"]
    raw=source["value"]
    # Normalize against the shared lab's existing speed 5 at source operand
    # 12. This is not a conversion from original time or world units.
    if not math.isfinite(raw) or not 1<=raw<=25:
        return None
    target=math.floor(raw*500/12+.5)
    return {"operand":"02C","source_bits":source["bits"],"source_value":raw,
            "record":selected,"record_sha256":record["record_sha256"],
            "native_units_per_tick_x100":target,"native_policy":"source-relative-ground-v1",
            "retail_speed_verified":False}
