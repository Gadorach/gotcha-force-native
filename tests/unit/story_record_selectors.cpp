#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/animated_model_scene.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"record selectors: %s\n",why);std::abort();}}
#ifdef STORY_SELECTOR_LEGACY
void print(const HsdRecordAnimationPlayer& p,unsigned id,unsigned tick){const auto* pose=p.pose();std::printf("L %u %u %u %.9g",id,tick,unsigned(pose!=nullptr),p.frame());if(pose){for(const auto& j:pose->joints)for(unsigned i=0;i<3;++i)std::printf(" %.9g %.9g %.9g",j.translation[i],j.rotation[i],j.scale[i]);for(const auto& m:pose->materials)std::printf(" %.9g %.9g %.9g %.9g",m.alpha,m.diffuse[0],m.diffuse[1],m.diffuse[2]);}for(const auto& o:p.trace()){std::printf(" O %.9g %u",o.frame,unsigned(o.stopped));for(const auto& t:o.tracks)std::printf(" T %u %u %u %u %u %zu %.17g %.17g %llu",t.channel,t.state,t.flags,t.op,t.interpolation,t.cursor,t.time,t.value,static_cast<unsigned long long>(t.callbacks));}std::puts("");}
void legacy(){std::string error;const auto bytes=hsd_record_animation_fixture::Fixture{}.archive();HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error),"legacy bank");HsdRecordAnimationBinding binding;check(bank.select(0,3,0,0,&binding,&error),"legacy binding");unsigned id=0;
    for(f32 start:{0.f,.5f,2.f,5.f})for(f32 rate:{0.f,.25f,1.f,2.f}){HsdRecordAnimationPlayer player;check(player.bind(binding,&error)&&player.request(start,rate,&error),"legacy request");print(player,id,0);for(unsigned i=1;i<12;++i){check(player.advance(rate,&error),"legacy advance");print(player,id,i);}check(player.request_again(1,0,&error),"legacy again");print(player,id,12);++id;}
}
#endif
#ifndef STORY_SELECTOR_LEGACY
void owned(){std::string error;const auto bytes=hsd_record_animation_fixture::Fixture{}.archive();HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error),"bank");HsdRecordAnimationBinding binding;check(bank.select(0,3,0,0,&binding,&error),"binding");
    for(f32 joint:{0.f,1.f,3.f,8.f})for(f32 material:{0.f,.5f,2.f,7.f}){HsdRecordAnimationPlayer player;check(player.bind(binding,&error)&&player.request_selectors(joint,material,&error),"independent requested times");const auto trace=player.trace();check(trace.size()==binding.objects()->size(),"object identity");for(unsigned i=0;i<trace.size();++i)check(trace[i].frame==((*binding.objects())[i].kind==hsd_record_detail::Object::Joint?joint:material),"correct object clock");check(player.frame()==joint&&player.request_again_selectors(material,joint,&error),"explicit re-request");}
    for(f32 bad:{-1.f,std::numeric_limits<f32>::quiet_NaN(),std::numeric_limits<f32>::infinity(),100001.f}){HsdRecordAnimationPlayer player;check(player.bind(binding,&error)&&!player.request_selectors(0,bad,&error)&&!player.pose(),"bad material selector invalidates");check(!player.request_selectors(0,0,&error),"no failed-owner revive");check(player.bind(binding,&error)&&!player.request_selectors(bad,0,&error),"bad joint selector invalidates");}
    HsdRecordAnimationPlayer player;check(player.bind(binding,&error)&&!player.request_again_selectors(0,0,&error),"no unrequested again");check(player.bind(binding,&error)&&player.request_selectors(1,3,&error),"fresh selector pair");check(!player.request_selectors(0,0,&error)&&!player.pose(),"ordinary fresh-only guard retained");
    AnimatedModelScene scene;check(scene.load(bank,0,3,0,0,&error)&&scene.request_selectors(1,3,&error)&&scene.ready(),"same scene API");check(scene.request_again_selectors(3,1,&error)&&scene.animation().frame()==3,"scene selector re-request");check(!scene.request_again_selectors(0,-1,&error)&&!scene.ready()&&!scene.pose(),"scene late failure no replay");
    check(scene.load(bank,0,3,0,0,&error)&&!scene.request_again_selectors(0,0,&error),"scene unrequested guard");
    std::printf("record selectors: %u new owned checks PASS; shared kernel, no caller/render clock\n",checks);
}
#endif
}
int main(){
#ifdef STORY_SELECTOR_LEGACY
legacy();
#else
owned();
#endif
}
