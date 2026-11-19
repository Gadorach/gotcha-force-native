#include <cstdio>
#include <cstdlib>
#include "engine/assets/borg_fusion_profile.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg fusion profile: %s\n",why);std::abort();}
}
std::string replace(std::string text,std::string_view old,std::string_view next){
    const auto at=text.find(old);check(at!=std::string::npos,"present substitution field");
    text.replace(at,old.size(),next);
    return text;
}
}
int main(){
    using namespace gf;
    const std::string source=R"(GFBORG-FUSIONS 1
fusion=example:twin-join
first=example:left|base|*
second=example:right|unresolved|normal
result=example:joined|combined|unresolved
result-use=fusion-only
trigger=hyper-request
guard=both-ready-same-team
synchronize=join-window
commit=combine-complete
interrupt=interrupt-before-commit
separation=separate-when-expired
duration=unresolved
entry=combine-left|combine-right|join-idle
exit=left-return|right-return|unresolved
transfer=combine-state|unresolved
)";
    BorgFusionProfile profile;
    check(borg_read_fusion_profile(source,&profile)&&profile.fusions.size()==1,"complete separate-result profile parses");
    const auto& f=profile.fusions.front();
    check(f.id=="example:twin-join"&&f.first.package=="example:left"&&f.first.form=="base"&&f.first.variant=="*"&&
        f.second.package=="example:right"&&f.second.form=="unresolved"&&f.second.variant=="normal"&&
        f.result.package=="example:joined"&&f.result.form=="combined"&&f.result_use==BorgFusionResultUse::FusionOnly,
        "stable relationship, three package identities, scope and intended result use remain separate");
    check(f.entry==std::array<std::string,3>{"combine-left","combine-right","join-idle"}&&
        f.exit[1]=="right-return"&&f.synchronize=="join-window"&&f.commit=="combine-complete"&&
        f.interrupt=="interrupt-before-commit"&&f.separation=="separate-when-expired"&&f.duration=="unresolved"&&
        f.transfer==std::array<std::string,2>{"combine-state","unresolved"},
        "timing, participant cues and state transfer remain explicit references");
    for(const auto& bad:{
        source+"fusion=example:twin-join\n",
        replace(source,"fusion=example:twin-join","fusion=not-namespaced"),
        replace(source,"first=example:left|base|*","first=unresolved|base|*"),
        replace(source,"first=example:left|base|*","first=example:left|../form|*"),
        replace(source,"result=example:joined|combined|unresolved","result=example:joined|*|normal"),
        replace(source,"result=example:joined|combined|unresolved","result=example:left|combined|normal"),
        replace(source,"result-use=fusion-only","result-use=grant-immediately"),
        replace(source,"guard=both-ready-same-team","guard=both-ready-same-team\nguard=none"),
        replace(source,"commit=combine-complete\n",""),
        replace(source,"entry=combine-left|combine-right|join-idle","entry=combine-left|combine-right"),
        replace(source,"transfer=combine-state|unresolved","transfer=none|none|none"),
        source+std::string(kBorgFusionProfileBytes,'\n')
    }){
        check(!borg_read_fusion_profile(bad,&profile)&&profile.fusions.size()==1&&f.id=="example:twin-join",
            "bad relationships, missing roles, duplicate declarations and invalid scope refuse atomically");
    }
    const auto placeholder=replace(source,"result=example:joined|combined|unresolved","result=unresolved|unresolved|unresolved");
    check(borg_read_fusion_profile(placeholder,&profile)&&profile.fusions[0].result.package=="unresolved",
        "unrecovered result need not be invented");
    check(borg_read_fusion_profile(replace(source,"second=example:right","second=example:left"),&profile)&&
        profile.fusions[0].first.package==profile.fusions[0].second.package,
        "two participant roles may use one type; a runtime would still need two distinct actors");
    std::puts("Borg battle fusion: identities, scoped roles, result dependency and handoff references PASS");
}
