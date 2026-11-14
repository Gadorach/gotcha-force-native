#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "tests/unit/startup_reveal_fixture.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"startup reveal: %s\n",why);std::abort();}}
struct Test {
    gf::HsdRecordAnimationBank bank;gf::StartupRevealProfile profile;std::array<gf::CharacterModelScene,2> actors;gf::StartupRevealScene scene;std::vector<gf::StartupRevealSound> sounds;std::string error;
    Test(){auto bytes=startup_reveal_fixture::archive();check(bank.load({bytes.data(),bytes.size()},&error),error.c_str());auto model=character_model_fixture::archive(),prefix=character_model_fixture::prefix(),rig=character_model_fixture::profile();
        for(auto& actor:actors)check(actor.load({model.data(),model.size()},{prefix.data(),prefix.size()},{rig.data(),rig.size()},&error),error.c_str());
        auto p=startup_reveal_fixture::profile(bank,startup_reveal_fixture::hash(model));check(gf::startup_reveal_load({p.data(),p.size()},&profile,&error),error.c_str());reload();}
    void reload(){check(scene.load(bank,profile,{&actors[0],&actors[1]},gf::StartupRevealMode::DeclaredNativeActorSnapshots,&error),error.c_str());}
    bool frame(unsigned n,unsigned phase,unsigned mode=1,bool available=true,bool visible=false,unsigned gate=0){
        gf::StartupDirectorState director;director.updates=n;director.phase=gf::u8(phase);if(!scene.before_actors(director,gf::u8(mode),available,&error))return false;
        auto a=startup_reveal_fixture::world(actors[0],float(n)),b=startup_reveal_fixture::world(actors[1],-float(n));
        return scene.after_actors({{{&actors[0],visible?nullptr:&a,n,gf::u8(visible?65:0)},{&actors[1],visible?nullptr:&b,n,gf::u8(visible?66:0)}}},gf::u8(gate),&sounds,&error);
    }
};
void life_cases(){
    Test t;check(!t.scene.model(3)->ready(),"not requested at load");check(t.frame(1,1),t.error.c_str());
    check(t.scene.states()->at(3).registered&&t.scene.states()->at(3).root_copies==1&&!t.scene.states()->at(3).visible,"constructor registers hidden copy; no early phase activation");
    check(t.frame(2,3)&&t.scene.states()->at(3).remaining==40&&t.scene.model(3)->animation().frame()==0,"threshold activation does not advance animation");
    for(unsigned n=3;n<=44;++n){check(t.frame(n,3,1,true,n>=10&&n<16),t.error.c_str());const auto& state=t.scene.states()->at(3);
        if(n==10)check(state.root.m[3]==9,"visible actor retains prior copied matrix, reads no missing world");
        if(n==16)check(state.root.m[3]==16,"hidden actor copies current bone1 again");
        if(n==41)check(state.visible&&state.remaining==1&&state.animation_updates==39,"exact last visible update");
        if(n==42)check(!state.visible&&state.phase==2&&state.registered&&state.root.m[3]==42,"expired hides but still registered/copying");
        if(n==43)check(state.phase==3&&state.registered&&state.root.m[3]==43,"phase2 to3 still registered");
        if(n==44)check(!state.allocated&&!state.registered&&state.root.m[3]==43,"release next callback; no new root read");
    }
    check(t.scene.states()->at(0).phase==1&&!t.scene.states()->at(0).visible,"late shine waits separately");
    for(unsigned n=45;n<=172;++n){check(t.frame(n,n<50?10:11),t.error.c_str());
        if(n==50)check(t.sounds.size()==3&&t.sounds[0].record==2&&t.sounds[1].record==1&&t.sounds[2].record==0&&t.sounds[0].id==377,"three ordered once-only sound requirements, not playback");
        else check(t.sounds.empty(),"no sound replay");
        if(n==169)check(t.scene.states()->at(0).visible&&t.scene.states()->at(0).remaining==1&&t.scene.states()->at(0).animation_updates==119,"late120-update lifetime distinct from AObj end");
        if(n==170)check(t.scene.states()->at(0).phase==2&&!t.scene.states()->at(0).visible,"late hide at expiration");
    }
    check(!t.scene.states()->at(0).allocated,"all late instances released");
    gf::StartupDirectorState d;d.updates=173;d.phase=12;check(t.scene.before_actors(d,2,false,&t.error)&&t.scene.after_actors({},0,&t.sounds,&t.error),"no actor reads after unregister/release");
    t.reload();check(t.frame(1,11)&&t.frame(2,11,2),"modechange dormant threshold still activates");check(t.scene.states()->at(0).visible,"source checks mode only on next active callback");check(t.frame(3,11,2)&&t.scene.states()->at(0).remaining==119&&!t.scene.states()->at(0).visible,"modechange decrements then hides without forced completion");
}
void guards(){
    Test t;gf::StartupDirectorState d;d.updates=1;d.phase=3;
    check(t.scene.before_actors(d,1,true,&t.error)&&!t.scene.states()&&!t.scene.model(0),"incomplete frame not published");
    check(!t.scene.before_actors(d,1,true,&t.error)&&!t.scene.ready(),"duplicate before stage refuses/latches");t.reload();
    check(!t.scene.after_actors({},0,&t.sounds,&t.error)&&!t.scene.ready(),"after without before");t.reload();
    check(!t.frame(2,3)&&t.scene.frames()==0,"skipped director frame");t.reload();
    check(t.scene.before_actors(d,1,true,&t.error),"late failure pre-stage");
    auto w=startup_reveal_fixture::world(t.actors[1],1),bad=startup_reveal_fixture::world(t.actors[0],2);bad.matrices[1].m[0]=0;
    t.sounds.push_back({377,4});check(!t.scene.after_actors({{{&t.actors[0],&bad,1,0},{&t.actors[1],&w,1,0}}},0,&t.sounds,&t.error)&&!t.scene.ready()&&t.sounds.empty()&&t.scene.frames()==0,"later bad root invalidates earlier copied draw and does not commit frame/output");
    gf::headless::Render backend;gf::RenderEngine render(backend);check(render.initialize({},nullptr,&t.error),"headless initialized");gf::Camera camera;check(!t.scene.draw(render,camera,{}),"no rejected-pose replay");t.reload();
    check(t.frame(1,3)&&t.frame(2,3),"fresh reload restores both stages");d.updates=3;check(t.scene.before_actors(d,1,true,&t.error),"foreign actor setup");
    check(!t.scene.after_actors({{{&t.actors[1],nullptr,3,65},{&t.actors[0],nullptr,3,66}}},0,&t.sounds,&t.error),"foreign neighboring actor even when visible");t.reload();
    check(t.scene.before_actors(gf::StartupDirectorState{.updates=1,.phase=3},1,true,&t.error),"stale frame setup");
    check(!t.scene.after_actors({{{&t.actors[0],&w,0,0},{&t.actors[1],&w,1,0}}},0,&t.sounds,&t.error),"stale actor snapshot");t.reload();
    check(t.scene.before_actors(gf::StartupDirectorState{.updates=1,.phase=3},1,true,&t.error),"reload generation setup");
    auto keep=t.actors[0].instance_binding();auto m=character_model_fixture::archive(),p=character_model_fixture::prefix(),r=character_model_fixture::profile();check(t.actors[0].load({m.data(),m.size()},{p.data(),p.size()},{r.data(),r.size()},&t.error),"same-model reload");
    check(!t.scene.after_actors({{{&t.actors[0],&w,1,0},{&t.actors[1],&w,1,0}}},0,&t.sounds,&t.error),"old generation held alive does not admit reload");t.reload();
    check(t.frame(1,3,1,false)&&t.scene.states()->at(3).phase==0,"resource absence waits, no forced creation");check(t.frame(2,3)&&t.scene.states()->at(3).phase==1&&!t.scene.states()->at(3).visible,"resource arrival initializes only");check(t.frame(3,3,1,true,false,1)&&t.scene.states()->at(3).visible,"scene display suppression does not stop callbacks");
    check(!t.scene.draw(render,camera,{}),"scene gate suppresses draw without reading actors");
    auto moved=std::move(t.scene);check(!t.scene.ready()&&moved.ready(),"move transfers owner, moved-from cannot publish");moved.release();check(!moved.ready()&&!moved.model(0),"release clears instances");
    auto bytes=startup_reveal_fixture::profile(t.bank,startup_reveal_fixture::hash(m));gf::StartupRevealProfile profile;
    for(unsigned size=0;size<64;++size)check(!gf::startup_reveal_load({bytes.data(),size},&profile,&t.error)&&!profile.loaded,"profile truncation");
    for(unsigned at:{0u,4u,8u,44u,48u,52u,54u,56u,57u,58u,59u,60u}){auto badp=bytes;badp[at]^=1;check(!gf::startup_reveal_load({badp.data(),badp.size()},&profile,&t.error)&&!profile.loaded,"profile source-policy guards");}
    {auto badp=bytes;badp[44]=0x80;check(!gf::startup_reveal_load({badp.data(),badp.size()},&profile,&t.error),"source positive-zero operand exact");}
    profile=t.profile;profile.bank_hash^=1;check(!t.scene.load(t.bank,profile,{&t.actors[0],&t.actors[1]},gf::StartupRevealMode::DeclaredNativeActorSnapshots,&t.error),"wrong source bank");
    check(!t.scene.load(t.bank,t.profile,{&t.actors[0],&t.actors[0]},gf::StartupRevealMode::DeclaredNativeActorSnapshots,&t.error),"duplicate actor scope");
    t.reload();check(t.frame(1,3)&&t.frame(2,3),"draw-time lifetime setup");
    auto old_generation=t.actors[0].instance_binding();check(t.actors[0].load({m.data(),m.size()},{p.data(),p.size()},{r.data(),r.size()},&t.error),"reload between publication and draw");
    check(!t.scene.draw(render,camera,{{{&t.actors[0],nullptr,2,0},{&t.actors[1],nullptr,2,0}}})&&!t.scene.ready(),"draw-time current generation recheck, even while old generation is held alive");
    t.reload();for(unsigned n=1;n<=8192;++n)if(!t.frame(n,1,1,false))check(false,"bounded waiting prefix");check(!t.frame(8193,1,1,false),"work budget refuses, never forces completion");
}
}
int main(){life_cases();guards();std::printf("{\"startup_reveal_checks\":%u,\"owned_only\":true,\"all_passed\":true}\n",checks);}
