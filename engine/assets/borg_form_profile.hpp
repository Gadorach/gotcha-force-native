#pragma once
#include "engine/assets/borg_move_profile.hpp"
#include "engine/assets/borg_spawn_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgFormProfileBytes=64u<<10;
inline constexpr unsigned kBorgForms=16,kBorgFormTransitions=64;
enum class BorgFormAssetRole:u8 {Model,Rig,Animations,AnimationMap,Movement,Moves,Collision,Effects,Count};
enum class BorgFormAssetKind:u8 {Unresolved,None,Inherit,LocalFile};
enum class BorgFormChangeKind:u8 {Unresolved,Selected,TemporaryEntry,Return};
enum class BorgFormTrigger:u8 {Unresolved,Selection,MoveEvent,Expiry,Stagger,Damage,Contact};

struct BorgFormAsset {
    BorgFormAssetKind kind=BorgFormAssetKind::Unresolved;
    std::string file;
};
struct BorgFormDefinition {
    std::string id,label;
    std::array<BorgFormAsset,unsigned(BorgFormAssetRole::Count)> assets;
};
struct BorgFormTransition {
    std::string id,from,to,event,guard,interrupt,timer,animation,effect,transfer;
    BorgFormChangeKind kind=BorgFormChangeKind::Unresolved;
    BorgFormTrigger trigger=BorgFormTrigger::Unresolved;
    std::optional<u16> priority;
};
struct BorgFormProfile {
    std::string default_form;
    std::vector<BorgFormDefinition> forms;
    std::vector<BorgFormTransition> transitions;
    const BorgFormDefinition* find(std::string_view id)const{
        for(const auto& form:forms)if(form.id==id)return &form;
        return nullptr;
    }
};

namespace borg_form_detail {
using borg_catalog_detail::label;
using borg_move_detail::cue;
using borg_move_detail::id;
using borg_move_detail::reserved;
using borg_package_detail::split_fields;
inline constexpr std::array<std::string_view,unsigned(BorgFormAssetRole::Count)> roles{
    "model","rig","animations","animation-map","movement","moves","collision","effects"
};
inline bool asset(std::string_view value,BorgFormAsset* out){
    if(value=="unresolved")return true;
    if(value=="none"){out->kind=BorgFormAssetKind::None;return true;}
    if(value=="inherit"){out->kind=BorgFormAssetKind::Inherit;return true;}
    if(!borg_package_asset_path_valid(value))return false;
    out->kind=BorgFormAssetKind::LocalFile;out->file=value;return true;
}
}

// Form and transition declarations, not a state-graph interpreter. No model
// swap, state transfer, timer, outcome, or gameplay actor is produced here.
inline bool borg_read_form_profile(std::string_view text,BorgFormProfile* out){
    if(!out||text.size()>kBorgFormProfileBytes||borg_package_detail::line(text)!="GFBORG-FORMS 1")return false;
    using namespace borg_form_detail;
    constexpr std::array<std::string_view,9> fields{
        "trigger","event","guard","interrupt","timer","animation","effect","transfer","priority"
    };
    BorgFormProfile next;std::array<bool,fields.size()> seen{};
    enum class Row {None,Form,Transition};
    Row block=Row::None;
    auto complete=[&](){
        return block==Row::None||std::all_of(seen.begin(),
            seen.begin()+(block==Row::Form?roles.size():fields.size()),[](bool present){return present;});
    };
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="default-form"){
            if(block!=Row::None||!next.default_form.empty()||!id(value))return false;
            next.default_form=value;continue;
        }
        if(key=="form"){
            if(next.forms.size()==kBorgForms||!complete())return false;
            std::array<std::string_view,2> parts;
            if(!split_fields(value,&parts)||!id(parts[0])||next.find(parts[0])||!display_name_valid(parts[1],96))return false;
            BorgFormDefinition form;form.id=parts[0];form.label=parts[1];
            next.forms.push_back(std::move(form));block=Row::Form;seen={};continue;
        }
        if(key=="transition"){
            if(next.transitions.size()==kBorgFormTransitions||!complete())return false;
            std::array<std::string_view,4> parts;
            if(!split_fields(value,&parts))return false;
            const auto [name,from,to,kind]=parts;
            BorgFormTransition change;
            if(!id(name)||!id(from)||!(id(to)||to=="unresolved")||from==to||
               !label(kind,{"unresolved","selected","temporary-entry","return"},&change.kind)||
               std::any_of(next.transitions.begin(),next.transitions.end(),[&](const auto& row){return row.id==name;}))return false;
            change.id=name;change.from=from;change.to=to;
            next.transitions.push_back(std::move(change));block=Row::Transition;seen={};continue;
        }
        if(block==Row::Form){
            const auto role=std::find(roles.begin(),roles.end(),key);
            if(role==roles.end())return false;
            const auto i=unsigned(role-roles.begin());
            if(seen[i]||!asset(value,&next.forms.back().assets[i]))return false;
            seen[i]=true;
        }else if(block==Row::Transition){
            const auto field=std::find(fields.begin(),fields.end(),key);
            if(field==fields.end())return false;
            const auto i=unsigned(field-fields.begin());
            if(seen[i])return false;
            seen[i]=true;
            auto& change=next.transitions.back();
            if(i==0){
                if(!label(value,{"unresolved","selection","move-event","expiry","stagger","damage","contact"},&change.trigger))return false;
            }else if(i==8){
                if(!borg_catalog_detail::optional_number(value,65535,0,&change.priority))return false;
            }else{
                if(!cue(value))return false;
                std::string* refs[]{&change.event,&change.guard,&change.interrupt,&change.timer,
                    &change.animation,&change.effect,&change.transfer};
                *refs[i-1]=value;
            }
        }else return false;
    }
    if(!complete()||!next.find(next.default_form))return false;
    for(const auto& change:next.transitions){
        if(!next.find(change.from)||(change.to!="unresolved"&&!next.find(change.to)))return false;
        if(change.trigger==BorgFormTrigger::Expiry&&(change.kind!=BorgFormChangeKind::Return||change.timer=="none"))return false;
        if(change.kind==BorgFormChangeKind::Selected&&!reserved(change.timer))return false;
        if(change.kind!=BorgFormChangeKind::TemporaryEntry||change.to=="unresolved")continue;
        // A known temporary destination must have a declared return path. A
        // named duration also needs an expiry path; interruption is separate.
        bool has_return=false,has_expiry=false;
        for(const auto& candidate:next.transitions)if(candidate.from==change.to&&candidate.kind==BorgFormChangeKind::Return){
            has_return=true;
            has_expiry|=candidate.trigger==BorgFormTrigger::Expiry&&
                (candidate.timer==change.timer||candidate.timer=="unresolved");
        }
        if(!has_return||(!reserved(change.timer)&&!has_expiry))return false;
    }
    *out=std::move(next);return true;
}
inline bool borg_form_move_references(const BorgFormProfile& forms,const BorgMoveProfile& moves,std::string* error){
    for(const auto& move:moves.moves)if(!forms.find(move.form)){
        if(error)*error="move "+move.id+" references undeclared form "+move.form;
        return false;
    }
    if(error)error->clear();
    return true;
}
inline bool borg_form_spawn_references(const BorgFormProfile& forms,const BorgSpawnProfile& spawn,std::string* error){
    for(const auto& rule:spawn.rules)if(rule.form!="*"&&!forms.find(rule.form)){
        if(error)*error="spawn "+rule.id+" references undeclared form "+rule.form;
        return false;
    }
    if(error)error->clear();
    return true;
}
}
