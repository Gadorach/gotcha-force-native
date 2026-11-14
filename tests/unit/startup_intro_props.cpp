#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "tests/unit/startup_intro_props_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const std::string& why){++checks;if(!ok){std::fprintf(stderr,"intro props: %s\n",why.c_str());std::abort();}}
struct Test {
    std::vector<gf::u8> raw,style;gf::HsdRecordAnimationBank bank;gf::StartupIntroPropProfile profile;gf::StartupIntroPropScene scene;std::string error;
    Test(){raw=startup_intro_props_fixture::archive();style=attachment_alternate_fixture::profile();check(bank.load({raw.data(),raw.size()},&error),error);auto p=startup_intro_props_fixture::profile(bank);check(gf::startup_intro_prop_load({p.data(),p.size()},&profile,&error),error);reload();}
    void reload(){check(scene.load({raw.data(),raw.size()},bank,profile,{style.data(),style.size()},gf::StartupIntroPropMode::DeclaredSceneObservations,&error),error);}
    bool step(unsigned tick,unsigned phase,unsigned mode=1,unsigned gate=0){gf::StartupDirectorState d;d.updates=tick+1;d.phase=gf::u8(phase);return scene.before_actors(d,gf::u8(mode),gf::u8(gate),&error);}
};
void timeline(){
    Test t;for(unsigned tick=0;tick<240;++tick){const unsigned phase=tick<120?1:tick<210?2:3;check(t.step(tick,phase),t.error);const auto& s=*t.scene.state();
        if(tick==0)check(s.path_phase==1&&s.path_visible&&s.fade_visible&&s.alpha==1&&s.path_updates==0,"constructor FIRST_PLAY and immediate source update");
        if(tick==120)check(s.path_substate==1&&s.fade_substate==1&&s.path_updates==0&&s.alpha==1,"phase2 activation does not advance either clock");
        if(tick==121)check(s.path_updates==1&&s.alpha==.9f&&s.alpha_updates==1,"source path step and float alpha subtraction");
        if(tick==129)check(s.fade_visible&&s.alpha>0&&s.alpha<.1f,"source repeated f32 subtraction retained");
        if(tick==130)check(s.fade_phase==2&&!s.fade_visible&&s.alpha==0&&s.alpha_updates==10,"zero clamp then same-callback hide");
        if(tick==132)check(s.fade_phase==4&&!t.scene.fade()->instance_binding(),"later fade release");
        if(tick==209)check(s.path_visible&&s.path_updates==89,"last phase2 visible path state");
        if(tick==210)check(s.path_phase==2&&!s.path_visible&&s.path_updates==90,"path advances before noticing phase change and hiding");
        if(tick==212)check(s.path_phase==4&&!t.scene.path()->asset(),"later path release");
    }
    t.reload();check(t.step(0,1,2)&&t.scene.state()->path_phase==2&&t.scene.state()->fade_phase==2,"mode2 constructor requests once then hides, no forced animation finish");
    t.reload();check(t.step(0,2)&&t.step(1,2,2)&&t.scene.state()->path_updates==1&&t.scene.state()->alpha_updates==1&&t.scene.state()->alpha==.9f,"active mode change updates before hide");
    t.reload();check(t.step(0,12)&&t.scene.state()->path_visible&&t.step(1,12)&&!t.scene.state()->path_visible,"late mode1 construction retains source one-callback behavior");
}
void guards(){
    Test t;auto p=startup_intro_props_fixture::profile(t.bank);gf::StartupIntroPropProfile bad;
    for(unsigned n=0;n<64;++n)check(!gf::startup_intro_prop_load({p.data(),n},&bad,&t.error)&&!bad.loaded,"profile truncation");
    for(unsigned at:{0u,4u,6u,8u,22u,32u,36u,40u,44u,48u,52u,53u,54u,55u,56u,57u,58u,60u}){auto bytes=p;bytes[at]^=1;check(!gf::startup_intro_prop_load({bytes.data(),bytes.size()},&bad,&t.error),"source operand guards");}
    bad=t.profile;bad.bank_hash^=1;check(!t.scene.load({t.raw.data(),t.raw.size()},t.bank,bad,{t.style.data(),t.style.size()},gf::StartupIntroPropMode::DeclaredSceneObservations,&t.error),"resource mismatch");t.reload();
    check(!t.step(1,1)&&!t.scene.ready()&&t.scene.frames()==0,"skipped frame invalidates all publication");t.reload();check(t.step(0,1)&&!t.step(0,1),"duplicate frame refuses");
    gf::headless::Render backend;gf::RenderEngine render(backend);check(render.initialize({},nullptr,&t.error),"headless");check(!t.scene.draw(render,gf::Camera{}),"no failed-frame replay");t.reload();
    check(t.step(0,2,1,1)&&t.step(1,2,1,1)&&t.scene.state()->path_updates==1&&t.scene.state()->alpha==.9f&&!t.scene.draw(render,gf::Camera{}),"display suppression keeps both callbacks active");
    auto moved=std::move(t.scene);check(!t.scene.ready()&&moved.ready(),"move ownership");moved.release();check(!moved.ready()&&!moved.state(),"release both models");t.reload();
    for(unsigned tick=0;tick<8192;++tick)if(!t.step(tick,1))check(false,"bounded waiting prefix");
    check(!t.step(8192,1),"work budget refuses without forced finish");
}
}
int main(){timeline();guards();std::printf("{\"intro_prop_checks\":%u,\"owned_only\":true,\"all_passed\":true}\n",checks);}
