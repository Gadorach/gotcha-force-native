#include <cstdio>
#include <cstdlib>
#include "engine/assets/borg_form_profile.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg form profile: %s\n",why);std::abort();}
}
std::string replace(std::string text,std::string_view old,std::string_view next){
    const auto at=text.find(old);check(at!=std::string::npos,"test replacement field");
    text.replace(at,old.size(),next);
    return text;
}
}
int main(){
    using namespace gf;
    const std::string source=R"(GFBORG-FORMS 1
default-form=base
form=base|Standard
model=inherit
rig=inherit
animations=inherit
animation-map=unresolved
movement=inherit
moves=inherit
collision=inherit
effects=none
form=wings|Wing Form
model=visual/wings.hsd
rig=visual/wings.rig
animations=unresolved
animation-map=unresolved
movement=rules/wings.txt
moves=inherit
collision=unresolved
effects=inherit
form=boost|Boost Form
model=inherit
rig=inherit
animations=inherit
animation-map=inherit
movement=unresolved
moves=inherit
collision=inherit
effects=inherit
transition=select-wing|base|wings|selected
trigger=selection
event=change-form
guard=airborne
interrupt=unresolved
timer=none
animation=wing-entry
effect=unresolved
transfer=unresolved
priority=10
transition=select-base|wings|base|selected
trigger=selection
event=change-form
guard=grounded
interrupt=unresolved
timer=none
animation=none
effect=none
transfer=unresolved
priority=unresolved
transition=enter-boost|base|boost|temporary-entry
trigger=move-event
event=charge-complete
guard=ready
interrupt=charge-cancel
timer=boost-life
animation=boost-entry
effect=boost-flash
transfer=boost-enter-data
priority=20
transition=expire-boost|boost|base|return
trigger=expiry
event=boost-expired
guard=none
interrupt=none
timer=boost-life
animation=unresolved
effect=none
transfer=boost-return-data
priority=10
transition=hit-return|boost|base|return
trigger=stagger
event=staggered
guard=none
interrupt=none
timer=none
animation=unresolved
effect=none
transfer=boost-return-data
priority=1
)";
    BorgFormProfile profile;
    check(borg_read_form_profile(source,&profile)&&profile.default_form=="base"&&profile.forms.size()==3&&
        profile.transitions.size()==5,"selected cycles and timed forms can coexist");
    const auto& assets=profile.find("wings")->assets;
    check(assets[unsigned(BorgFormAssetRole::Model)].kind==BorgFormAssetKind::LocalFile&&
        assets[unsigned(BorgFormAssetRole::Model)].file=="visual/wings.hsd"&&
        assets[unsigned(BorgFormAssetRole::AnimationMap)].kind==BorgFormAssetKind::Unresolved&&
        assets[unsigned(BorgFormAssetRole::Moves)].kind==BorgFormAssetKind::Inherit,
        "form-specific asset overrides and unknowns stay distinct from inheritance");
    check(profile.transitions[0].guard=="airborne"&&profile.transitions[1].guard=="grounded"&&
        profile.transitions[2].kind==BorgFormChangeKind::TemporaryEntry&&profile.transitions[2].timer=="boost-life"&&
        profile.transitions[3].trigger==BorgFormTrigger::Expiry&&profile.transitions[4].trigger==BorgFormTrigger::Stagger&&
        profile.transitions[4].transfer=="boost-return-data","timed entry, expiry and interruption need separate transitions");
    for(const auto& bad:{
        replace(source,"default-form=base","default-form=costume"),
        replace(source,"form=wings|","form=base|"),
        replace(source,"model=visual/wings.hsd","model=../wings.hsd"),
        replace(source,"collision=unresolved\n",""),
        replace(source,"transition=select-base","transition=select-wing"),
        replace(source,"select-wing|base|wings","select-wing|base|missing"),
        replace(source,"select-wing|base|wings","select-wing|base|base"),
        replace(source,"timer=none\nanimation=wing-entry","timer=must-not-expire\nanimation=wing-entry"),
        replace(source,"trigger=expiry","trigger=damage"),
        replace(source,"event=boost-expired\nguard=none\ninterrupt=none\ntimer=boost-life",
            "event=boost-expired\nguard=none\ninterrupt=none\ntimer=wrong-clock"),
        replace(source,"event=boost-expired\nguard=none\ninterrupt=none\ntimer=boost-life",
            "event=boost-expired\nguard=none\ninterrupt=none\ntimer=none"),
        source+std::string(kBorgFormProfileBytes,'\n')
    }){
        check(!borg_read_form_profile(bad,&profile)&&profile.forms.size()==3&&profile.transitions.size()==5,
            "dangling forms, duplicate fields, missing expiry and invalid scope cannot replace prior data");
    }
    check(borg_read_form_profile(replace(source,"expire-boost|boost|base","expire-boost|boost|unresolved"),&profile)&&
        profile.transitions[3].to=="unresolved","unknown return destination remains a declared placeholder");
    BorgMoveProfile moves;moves.moves.emplace_back();moves.moves[0].id="shot";moves.moves[0].form="wings";
    BorgSpawnProfile spawn;spawn.rules.emplace_back();spawn.rules[0].id="entry";spawn.rules[0].form="*";
    std::string error;
    check(borg_form_move_references(profile,moves,&error)&&borg_form_spawn_references(profile,spawn,&error),"same-package form links");
    moves.moves[0].form="bronze";spawn.rules[0].form="bronze";
    check(!borg_form_move_references(profile,moves,&error)&&error.find("shot")!=std::string::npos&&
        !borg_form_spawn_references(profile,spawn,&error)&&error.find("entry")!=std::string::npos,"a costume name cannot substitute for a form");
    std::puts("Borg forms: selected and timed forms, return paths, overrides and cross-links PASS");
}
