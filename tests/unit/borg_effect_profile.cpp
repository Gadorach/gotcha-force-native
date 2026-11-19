#include <cstdio>
#include <cstdlib>
#include "engine/assets/borg_effect_profile.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg effect profile: %s\n",why);std::abort();}
}
std::string replace(std::string text,std::string_view old,std::string_view next){
    const auto at=text.find(old);check(at!=std::string::npos,"test field exists");
    text.replace(at,old.size(),next);
    return text;
}
}
int main(){
    using namespace gf;
    const std::string profile=R"(GFBORG-EFFECTS 1
visual=bolt-body|mesh|local:effects/bolt.hsd
visual=flash|sprite|shared:engine:flash
visual=impact-mark|decal|unresolved
sound=fire-sound|one-shot|local:audio/fire.dsp
sound=flight-hum|loop|shared:engine:hum
cue=flight|shot-spawn|projectile|projectile|none|bolt-body|flight-hum
cue=fire|move-start|socket:muzzle|event|none|flash|fire-sound
cue=impact|shot-contact|contact|timer:impact-age|none|impact-mark|none
cue=charged|charge-ready|actor|until-stop|charge-end|none|flight-hum
cue=quiet|unresolved|unresolved|unresolved|unresolved|none|none
)";
    BorgEffectProfile effects;
    check(borg_read_effect_profile(profile,&effects)&&effects.visuals.size()==3&&effects.sounds.size()==2&&
        effects.cues.size()==5,"typed visual and sound assets with explicit event cues");
    check(effects.visuals[0].asset.kind==BorgEffectAssetKind::LocalFile&&effects.visuals[0].asset.target=="effects/bolt.hsd"&&
        effects.visuals[1].asset.kind==BorgEffectAssetKind::SharedId&&effects.visuals[1].asset.target=="engine:flash"&&
        effects.visuals[2].asset.kind==BorgEffectAssetKind::Unresolved,"explicit local, shared and unresolved assets stay distinct");
    check(effects.cues[0].anchor==BorgEffectAnchor::Projectile&&effects.cues[0].lifetime==BorgEffectLifetime::Projectile&&
        effects.cues[1].anchor==BorgEffectAnchor::Socket&&effects.cues[1].socket=="muzzle"&&
        effects.cues[2].lifetime==BorgEffectLifetime::Timer&&effects.cues[2].timer=="impact-age"&&
        effects.cues[3].stop=="charge-end","projectile, attachment, timer and stop scopes are declared independently");
    for(const auto& bad:{
        replace(profile,"visual=flash","visual=bolt-body"),
        replace(profile,"sound=flight-hum","sound=fire-sound"),
        replace(profile,"cue=fire","cue=flight"),
        replace(profile,"local:effects/bolt.hsd","local:../bolt.hsd"),
        replace(profile,"shared:engine:flash","shared:flash"),
        replace(profile,"|bolt-body|flight-hum","|missing|flight-hum"),
        replace(profile,"|bolt-body|flight-hum","|bolt-body|missing"),
        replace(profile,"|projectile|projectile|none|","|projectile|event|none|"),
        replace(profile,"until-stop|charge-end","until-stop|none"),
        replace(profile,"socket:muzzle","socket:../muzzle"),
        profile+std::string(kBorgEffectProfileBytes,'\n')
    }){
        check(!borg_read_effect_profile(bad,&effects)&&effects.cues.size()==5,"invalid IDs, paths, stops and dangling references refuse atomically");
    }
    const std::string moves=R"(GFBORG-MOVES 1
move=bolt|base|b|projectile|Training Bolt
contexts=any
fallback=none
ammo=unresolved
refill=unresolved
activation=press
guard=unresolved
priority=0
replenishment=unresolved
interrupt=unresolved
phase=entry|move-start|none|fire|fire-sound|non-solid|none|none
phase=flight|shot-spawn|none|flight|none|non-solid|none|none
)";
    BorgMoveProfile actions;std::string error;
    check(borg_read_move_profile(moves,&actions)&&borg_effect_move_references(effects,actions,&error),
        "named move-phase cues bind to typed local effect and sound records");
    check(borg_read_move_profile(replace(moves,"none|fire|fire-sound","none|fire|flight-hum"),&actions)&&
        !borg_effect_move_references(effects,actions,&error)&&error.find("lifetime-bound")!=std::string::npos,
        "looping audio cannot bypass its lifetime by direct phase binding");
    check(borg_read_move_profile(replace(moves,"none|fire|fire-sound","none|fire-sound|fire-sound"),&actions)&&
        !borg_effect_move_references(effects,actions,&error)&&error.find("bolt/entry")!=std::string::npos,
        "sound IDs cannot substitute for an effect cue");
    std::puts("Borg effects: typed cues, projectile/socket ownership, asset references and move links PASS");
}
