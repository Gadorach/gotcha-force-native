#include <cstdio>
#include <cstdlib>
#include "engine/assets/borg_acquisition_profile.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg acquisition profile: %s\n",why);std::abort();}
}
std::string replace(std::string text,std::string_view old,std::string_view next){
    const auto at=text.find(old);check(at!=std::string::npos,"test replacement field");
    text.replace(at,old.size(),next);return text;
}
}
int main(){
    using namespace gf;
    const std::string profile=R"(GFBORG-ACQUISITION 1
route=start|story|normal|start-grant
source=story:new-run
condition=campaign-start
grant=owned
component=none
repeat=once-per-save
trade=blocked
discard=blocked
retirement=story:planned-removal
route=drop-piece|challenge|normal|encounter-drop
source=challenge:training-cup
condition=unresolved
grant=crystal-part
component=body
repeat=repeatable
trade=unresolved
discard=unresolved
retirement=none
recipe=normal-body|normal|body|core
route=assemble|*|normal|crystal-assembly
source=normal-body
condition=parts-ready
grant=owned
component=none
repeat=repeatable
trade=allowed
discard=allowed
retirement=none
route=joined|story|unresolved|battle-fusion
source=example:combine
condition=unresolved
grant=battle-only
component=none
repeat=unresolved
trade=blocked
discard=blocked
retirement=separate
)";
    BorgAcquisitionProfile parsed;
    check(borg_read_acquisition_profile(profile,&parsed)&&parsed.routes.size()==4&&parsed.recipes.size()==1,
        "routes and recipes coexist without changing an inventory");
    check(parsed.routes[0].repeat==BorgAcquisitionRepeat::OncePerSave&&parsed.routes[0].trade==BorgOwnershipPermission::Blocked&&
        parsed.routes[0].retirement=="story:planned-removal"&&parsed.routes[1].grant==BorgAcquisitionGrant::CrystalPart&&
        parsed.routes[1].component=="body","starting restrictions and crystal drop metadata remain separate");
    check(parsed.routes[2].method==BorgAcquisitionMethod::CrystalAssembly&&parsed.recipe("normal-body")&&
        parsed.recipe("normal-body")->parts==std::vector<std::string>{"body","core"}&&
        parsed.routes[3].method==BorgAcquisitionMethod::BattleFusion&&parsed.routes[3].grant==BorgAcquisitionGrant::BattleOnly,
        "same-variant crystal assembly is not a battle fusion grant");
    for(const auto& bad:{
        replace(profile,"route=drop-piece","route=start"),
        replace(profile,"source=challenge:training-cup","source=../../outside"),
        replace(profile,"grant=crystal-part\ncomponent=body","grant=crystal-part\ncomponent=none"),
        replace(profile,"grant=crystal-part\ncomponent=body","grant=owned\ncomponent=body"),
        replace(profile,"recipe=normal-body|normal|body|core","recipe=normal-body|normal|body|body"),
        replace(profile,"recipe=normal-body|normal|body|core","recipe=normal-body|normal|body"),
        replace(profile,"route=assemble|*|normal","route=assemble|*|silver"),
        replace(profile,"route=assemble|*|normal","route=assemble|*|*"),
        replace(profile,"source=normal-body","source=missing-recipe"),
        replace(profile,"grant=battle-only","grant=owned"),
        replace(profile,"repeat=unresolved\ntrade=blocked","repeat=unresolved\ntrade=allowed"),
        replace(profile,"retirement=separate\n",""),
        profile+"recipe=normal-body|normal|a|b\n",
        profile+std::string(kBorgAcquisitionBytes,'\n')
    }){
        check(!borg_read_acquisition_profile(bad,&parsed)&&parsed.routes.size()==4&&parsed.recipe("normal-body"),
            "malformed or unsafe references refuse without replacing prior profile data");
    }
    check(borg_read_acquisition_profile(replace(profile,"source=normal-body","source=unresolved"),&parsed)&&
        parsed.routes[2].source=="unresolved","unknown assembly recipe is explicit, not guessed");
    std::puts("Borg acquisition: owned rewards, scoped routes, crystal recipes and battle-only results PASS");
}
