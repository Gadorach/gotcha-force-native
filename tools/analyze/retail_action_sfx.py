"""Reviewed literal actor sound requests, separate from projectile-child cues."""

from tools.analyze.retail_projectile_sfx import SOUND_REQUEST, tone_candidate


# Entry -> (byte extent, signature, call site, expected ID). These are
# actor-owned calls outside the projectile consumers, not sound triggers.
DIRECT = {
    0x800A8F44:(0xFC,"f0170c958ed6b7fedb34ed36cb332abac58d4331086c1851dafc983522e8d05f",0x800A9028,267),
    0x800A9040:(0x148,"8367ed6c51bcbb9350bf473deb89d55adbcffbf53781c47054fe5e886ec268c9",0x800A9148,268),
    0x800AFF70:(0x54,"b8f0efac5ced912149e4586c6a9a326b999c876fa8969e418bb5f0c5431514f3",0x800AFFB0,190),
    0x800B0188:(0xCC,"6c094abd51165f6981993307a4b70a83f93c226bb0644e1102701a42adb7ae73",0x800B01D0,190),
    0x800E5664:(0xDC,"41f30a2b13f016cdab2e850399dc5083ea87155831b3992003f3c39506e439ae",0x800E5720,217),
    0x801037F8:(0x198,"30827fbdfca8f53110cce27941ed151d058c78328f650c756ae36f50932adbe9",0x801038D4,195),
    0x8012745C:(0x50,"389ea24bdd1bd54edbf5a764a69ea34c1f40fff4647a45c34efa366a1a28170c",0x8012747C,178),
    0x8012B458:(0x4C,"51f6e07bf6538d27a2dc2dbe414621ab6fcdbd53f8caae3ca4b8dd6ba4f5f271",0x8012B4A0,32),
    0x8015BAB0:(0x40,"4ac320b3b011f06baaad2f12df5c9e93cb5f541af7e5791f64095dc291faf942",0x8015BAEC,178),
    0x8015C430:(0x40,"ba074773fb70489ec886b534f6859381bc120233457f710bfeafb32297c8859d",0x8015C46C,178),
    0x801E7F74:(0x6C,"a11932aded525593c34e0112ede0ed815fde0ac14f9fc7f08eeafbe47cc50885",0x801E7FC8,235),
}
RELAY = 0x8010FE78
RELAY_SPAN = (0xD0,"2fab2550f62506165cac0797bc7c4f2aefa6d7b4d05b0eb95038450261e870af")
RELAY_PARENTS = {
    0x8010FC7C:(0xC8,"e86e20f44124a98b136c6224ece8218ad3a83d32fa9883a1d681189dc78ab8ae",0x8010FCB0),
    0x8010FD44:(0x134,"5f942b7a7461cc2188984d1aeec520284ef60f73c3a4c86d4c543b839b1ee7a7",0x8010FDF4),
}
RELAY_SOUND_SITES = {0x0804:0x8010FEB8,0x080C:0x8010FF18}


def review_requests(linker, entry, model, group):
    """Use the linker's bounded graph; a reviewed site still needs its exact call/operand."""
    result = []
    direct = DIRECT.get(entry)
    if direct:
        size,digest,site,sound_id = direct
        if linker.matches_text(entry,size,digest) and linker.review_call(entry,site,SOUND_REQUEST,sound_id):
            result.append({"site":f"{site:08X}","source_sound_id":sound_id})
    parent = RELAY_PARENTS.get(entry)
    if group=="B-shot" and parent and model in RELAY_SOUND_SITES:
        size,digest,relay_site = parent
        site = RELAY_SOUND_SITES[model]
        if (linker.matches_text(entry,size,digest) and linker.matches_text(RELAY,*RELAY_SPAN) and
                linker.review_call(entry,relay_site,RELAY) and linker.review_call(RELAY,site,SOUND_REQUEST,162)):
            result.append({"site":f"{site:08X}","source_sound_id":162,"relay_site":f"{relay_site:08X}",
                           "relay_handler":f"{RELAY:08X}","required_model_at_call":f"{model:04X}"})
    return result


def action_sfx_index(actions, banks):
    """Keep exact installer/input contexts, with no allocation or playback rule."""
    entries = {}
    names = ("config","context_table","main_handler","action_group","state","substate",
             "selector","context_rows","entry","scope","relay_site","relay_handler")
    for profile in actions["profiles"]:
        for group in profile["groups"]:
            for row in group["rows"]:
                for cue in row.get("action_sfx_candidates",()):
                    key = cue["site"],cue["source_sound_id"],cue.get("required_model_at_call","")
                    routes = entries.setdefault(key,set())
                    routes.add((profile["config"],profile["context_table"],profile["main_handler"],group["group"],
                                row["state"],row["substate"],row["selector"],tuple(map(tuple,row["contexts"])),
                                cue["entry"],cue["scope"],cue.get("relay_site",""),cue.get("relay_handler","")))
    result = []
    for (site,sound_id,model),routes in sorted(entries.items()):
        cue = {"site":site,"source_sound_id":sound_id,"owner":"action-actor",
               **tone_candidate(sound_id,banks),"routes":[dict(zip(names,r,strict=True)) for r in sorted(routes)]}
        if model:
            cue["required_model_at_call"]=model
        result.append(cue)
    return result
