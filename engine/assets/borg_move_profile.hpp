#pragma once
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgMoveProfileBytes=64u<<10;
inline constexpr unsigned kBorgMoveCount=64,kBorgMovePhaseCount=8,kBorgMoveOutcomeCount=8;
enum class BorgActionButton:u8 {Unresolved,None,B,X};
enum class BorgActionKind:u8 {Unresolved,Melee,Projectile,Special,NoAction};
enum class BorgMoveActivation:u8 {Unresolved,Press,Hold,Release,Charge,Automatic};
enum class BorgMoveContextRule:u8 {Unresolved,Any,Listed};
enum class BorgMoveAmmoKind:u8 {Unresolved,None,Unlimited,Finite};
enum class BorgMoveRefill:u8 {Unresolved,None,AllAtOnce,Gradual,Fragmented};
enum class BorgMoveSolidity:u8 {Unresolved,NotApplicable,Solid,NonSolid};
enum class BorgMovePenetration:u8 {Unresolved,NotApplicable,None,Borgs,Objects,BorgsAndObjects,All,Partial};
enum class BorgMoveExplosion:u8 {Unresolved,NotApplicable,None,Present,OnContact,OnExpiry};
enum class BorgMoveOutcomeKind:u8 {
    Unresolved,Damage,StatModifier,BehaviorModifier,ControlModifier,Transform,SwapPositions,PassiveContact,AreaImpact
};
enum class BorgMoveOutcomeTarget:u8 {Unresolved,Self,SelectedTarget,AllyHit,EnemyHit,ContactTarget,ImpactArea};

struct BorgMovePhaseDefinition {
    std::string id,trigger,animation_role,effect_cue,sound_cue;
    BorgMoveSolidity solidity=BorgMoveSolidity::Unresolved;
    BorgMovePenetration penetration=BorgMovePenetration::Unresolved;
    BorgMoveExplosion explosion=BorgMoveExplosion::Unresolved;
};
struct BorgMoveOutcomeDefinition {
    std::string id,event,duration,payload;
    BorgMoveOutcomeKind kind=BorgMoveOutcomeKind::Unresolved;
    BorgMoveOutcomeTarget target=BorgMoveOutcomeTarget::Unresolved;
};
struct BorgMoveDefinition {
    std::string id,form,label,fallback,guard,interrupt,replenishment;
    BorgActionButton button=BorgActionButton::Unresolved;
    BorgActionKind kind=BorgActionKind::Unresolved;
    BorgMoveActivation activation=BorgMoveActivation::Unresolved;
    std::optional<u16> priority;
    BorgMoveContextRule contexts=BorgMoveContextRule::Unresolved;
    u16 context_mask=0;
    BorgMoveAmmoKind ammo=BorgMoveAmmoKind::Unresolved;
    std::array<std::optional<u16>,2> ammo_at_level{}; // level 1 and 10, not a curve
    BorgMoveRefill refill=BorgMoveRefill::Unresolved;
    std::vector<BorgMovePhaseDefinition> phases;
    std::vector<BorgMoveOutcomeDefinition> outcomes;
};
struct BorgMoveProfile {
    std::vector<BorgMoveDefinition> moves;
    const BorgMoveDefinition* find(std::string_view id)const{
        for(const auto& move:moves)if(move.id==id)return &move;
        return nullptr;
    }
};
namespace borg_move_detail {
inline bool reserved(std::string_view text){return text=="none"||text=="unresolved";}
inline bool id(std::string_view text){return !reserved(text)&&borg_catalog_variant_id_valid(text);}
inline bool cue(std::string_view text){return reserved(text)||id(text);}
using borg_package_detail::split_fields;
inline constexpr std::array<std::string_view,9> context_names{
    "grounded","airborne","ground-dash","air-dash","flying","above-target","jumping","landing","contact"
};
inline bool contexts(std::string_view text,BorgMoveDefinition& move){
    if(text=="unresolved")return true;
    if(text=="any"){move.contexts=BorgMoveContextRule::Any;return true;}
    move.contexts=BorgMoveContextRule::Listed;
    for(;;){
        const auto pipe=text.find('|');
        const auto item=std::find(context_names.begin(),context_names.end(),text.substr(0,pipe));
        if(item==context_names.end())return false;
        const auto bit=u16(1u<<unsigned(item-context_names.begin()));
        if(move.context_mask&bit)return false;
        move.context_mask|=bit;
        if(pipe==std::string_view::npos)return true;
        text.remove_prefix(pipe+1);
    }
}
inline bool phase(std::string_view text,BorgMovePhaseDefinition* out){
    using borg_catalog_detail::label;
    std::array<std::string_view,8> fields;
    if(!split_fields(text,&fields))return false;
    const auto [name,trigger,anim,effect,sound,solid,pass,blast]=fields;
    if(!id(name)||!cue(trigger)||!cue(anim)||!cue(effect)||!cue(sound)||
        !label(solid,{"unresolved","not-applicable","solid","non-solid"},&out->solidity)||
        !label(pass,{"unresolved","not-applicable","none","borgs","objects","borgs-and-objects","all","partial"},&out->penetration)||
        !label(blast,{"unresolved","not-applicable","none","present","on-contact","on-expiry"},&out->explosion))return false;
    out->id=name;out->trigger=trigger;out->animation_role=anim;out->effect_cue=effect;out->sound_cue=sound;return true;
}
inline bool outcome(std::string_view text,BorgMoveOutcomeDefinition* out){
    using borg_catalog_detail::label;
    std::array<std::string_view,6> fields;
    if(!split_fields(text,&fields))return false;
    const auto [name,event,kind,target,duration,payload]=fields;
    if(!id(name)||!cue(event)||!cue(duration)||!cue(payload)||
       !label(kind,{"unresolved","damage","stat-modifier","behavior-modifier","control-modifier","transform",
           "swap-positions","passive-contact","area-impact"},&out->kind)||
       !label(target,{"unresolved","self","selected-target","ally-hit","enemy-hit","contact-target","impact-area"},&out->target))return false;
    out->id=name;out->event=event;out->duration=duration;out->payload=payload;return true;
}
}

// Structural move metadata only. Even known values cannot spawn a shot, play
// an effect, spend ammo, or admit an actor through this reader.
inline bool borg_read_move_profile(std::string_view text,BorgMoveProfile* out){
    if(!out||text.size()>kBorgMoveProfileBytes||borg_package_detail::line(text)!="GFBORG-MOVES 1")return false;
    using namespace borg_move_detail;
    using borg_catalog_detail::label;
    BorgMoveProfile next;
    constexpr std::array<std::string_view,9> fields{
        "contexts","fallback","ammo","refill","activation","guard","priority","replenishment","interrupt"
    };
    std::array<bool,fields.size()> seen{};
    auto complete=[&](){
        if(!std::all_of(seen.begin(),seen.end(),[](bool value){return value;}))return false;
        const auto& move=next.moves.back();
        if(move.kind!=BorgActionKind::NoAction)return !move.phases.empty();
        return move.phases.empty()&&move.outcomes.empty()&&move.fallback=="none"&&move.ammo==BorgMoveAmmoKind::None&&
            move.refill==BorgMoveRefill::None&&move.replenishment=="none";
    };
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="move"){
            if(next.moves.size()==kBorgMoveCount||(!next.moves.empty()&&!complete()))return false;
            std::array<std::string_view,5> parts;
            if(!split_fields(value,&parts))return false;
            const auto [move_id,form,button,kind,name]=parts;
            BorgMoveDefinition move;
            if(!id(move_id)||next.find(move_id)||!id(form)||!display_name_valid(name,96)||
                !label(button,{"unresolved","none","b","x"},&move.button)||
                !label(kind,{"unresolved","melee","projectile","special","no-action"},&move.kind))return false;
            move.id=move_id;move.form=form;move.label=name;
            next.moves.push_back(std::move(move));seen={};continue;
        }
        if(next.moves.empty())return false;
        auto& move=next.moves.back();
        if(key=="phase"){
            BorgMovePhaseDefinition decoded;
            if(move.phases.size()==kBorgMovePhaseCount||!phase(value,&decoded)||
                std::any_of(move.phases.begin(),move.phases.end(),[&](const auto& prev){return prev.id==decoded.id;}))return false;
            move.phases.push_back(std::move(decoded));continue;
        }
        if(key=="outcome"){
            BorgMoveOutcomeDefinition decoded;
            if(move.outcomes.size()==kBorgMoveOutcomeCount||!outcome(value,&decoded)||
                std::any_of(move.outcomes.begin(),move.outcomes.end(),[&](const auto& prev){return prev.id==decoded.id;}))return false;
            move.outcomes.push_back(std::move(decoded));continue;
        }
        const auto field=std::find(fields.begin(),fields.end(),key);
        if(field==fields.end())return false;
        const auto index=unsigned(field-fields.begin());
        if(seen[index])return false;
        seen[index]=true;
        switch(index){
        case 0:if(!contexts(value,move))return false;break;
        case 1:if(!cue(value))return false;move.fallback=value;break;
        case 2:
            if(value.starts_with("finite:")){
                move.ammo=BorgMoveAmmoKind::Finite;
                if(!borg_catalog_detail::number_list(value.substr(7),65535,0,&move.ammo_at_level))return false;
            }else if(!label(value,{"unresolved","none","unlimited"},&move.ammo))return false;
            break;
        case 3:if(!label(value,{"unresolved","none","all-at-once","gradual","fragmented"},&move.refill))return false;break;
        case 4:if(!label(value,{"unresolved","press","hold","release","charge","automatic"},&move.activation))return false;break;
        case 5:if(!cue(value))return false;move.guard=value;break;
        case 6:if(!borg_catalog_detail::optional_number(value,65535,0,&move.priority))return false;break;
        case 7:if(!cue(value))return false;move.replenishment=value;break;
        case 8:if(!cue(value))return false;move.interrupt=value;break;
        }
    }
    if(next.moves.empty()||!complete())return false;
    for(const auto& move:next.moves){
        auto* current=&move;
        unsigned hops=0;
        while(!reserved(current->fallback)){
            current=next.find(current->fallback);
            if(!current||current->form!=move.form||++hops>=next.moves.size()||
                (current->button!=move.button&&current->button!=BorgActionButton::Unresolved&&
                 move.button!=BorgActionButton::Unresolved))return false;
        }
    }
    *out=std::move(next);return true;
}
}
