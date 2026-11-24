#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/portable_fusion.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"portable fusion: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using Result=PortableFusionStep;
    struct Binding {unsigned asset=0;};
    using Transaction=PortableFusion<Binding>;
    using Slot=Transaction::Slot;
    const auto slot=[](u64 instance,u64 revision,u8 team,std::string_view id,std::string_view form,unsigned asset){
        return Slot{instance,revision,team,{{std::string(id),"normal"},std::string(form)},std::make_shared<const Binding>(Binding{asset})};
    };
    const std::array initial{slot(101,3,1,"test:alpha","base",1),slot(102,5,1,"test:beta","base",2),
                             slot(103,1,2,"test:opponent","base",3)};
    const std::array loaded{slot(101,4,1,"test:alpha","joined-body",4),slot(102,6,1,"test:beta","joined-wing",5)};
    const std::array keys{initial[0].key,initial[1].key};
    auto live=initial;
    auto candidates=loaded;
    Transaction fusion;
    candidates[1].loaded.reset();
    check(fusion.prepare(live,{0,1},keys,candidates,3)==Result::Refused&&live==initial,"one missing form refuses both slots");
    check(fusion.prepare(live,{0,2},keys,loaded,3)==Result::Refused&&live==initial,"opposing actor cannot be the companion");
    check(fusion.prepare(live,{0,1},keys,loaded,3)==Result::Waiting&&
          fusion.commit(live,{true,false})==Result::Waiting&&live==initial,"both forms and cues precede mutation");
    ++live[1].revision;
    const auto changed=live;
    check(fusion.commit(live,{true,true})==Result::Stale&&live==changed,"replace neither slot if either changed");
    live=initial;
    check(fusion.prepare(live,{0,1},keys,loaded,3)==Result::Waiting&&
          fusion.commit(live,{true,true})==Result::Committed&&live[0]==loaded[0]&&live[1]==loaded[1]&&live[2]==initial[2],
          "paired publication preserves instance identity and the unrelated actor");
    check(fusion.tick(false)==Result::Waiting&&fusion.remaining()==3,"ineligible update does not spend shared clock");
    check(fusion.tick(true)==Result::Waiting&&fusion.tick(true)==Result::Waiting&&
          fusion.tick(true)==Result::Expired&&fusion.remaining()==0,"expiry does not publish either restore");
    check(fusion.restore(live,0,false)==Result::Waiting&&live[0]==loaded[0],"owner can delay restoration");
    check(fusion.restore(live,1,true)==Result::Restored&&live[1].key==initial[1].key&&
          live[1].loaded==initial[1].loaded&&live[1].revision==7&&live[0]==loaded[0],
          "other owner restores independently");
    ++live[0].revision;
    check(fusion.restore(live,0,true)==Result::Stale&&live[0].key==loaded[0].key&&
          fusion.restore(live,1,true)==Result::Refused,"stale owner cannot overwrite a replacement");
    live=initial;
    check(fusion.prepare(live,{0,1},keys,loaded,1)==Result::Waiting,"closed transaction releases both preparations");
    fusion.cancel();
    check(fusion.commit(live,{true,true})==Result::Refused&&live==initial,"cancellation preserves both live bindings");
}
