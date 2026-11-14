// Owned untextured billboard regression: real source orbs use this geometry
// class. Default marker filtering is kept; only the qualified orb owner opts in.
#include <cstdio>
#include <cstdlib>
#include "tests/unit/startup_reveal_fixture.hpp"
#include "engine/core/headless_backends.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"orb billboard: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;std::string error;auto raw=startup_reveal_fixture::archive();
    for(unsigned record=3;record<=4;++record)for(unsigned j=0;j<3;++j)store_be32(raw.data()+32+0x1000+record*0x100+j*64+4,0x80200);
    HsdRecordAnimationBank bank;check(bank.load({raw.data(),raw.size()},&error),"owned untextured billboard records");
    headless::Render backend;RenderEngine render(backend);check(render.initialize({64,64,false,true,"owned orb"},nullptr,&error),"render init");Camera camera;camera.eye[2]=20;camera.far_plane=100;camera.viewport_width=camera.viewport_height=64;
    AnimatedModelScene legacy;check(legacy.load(bank,3,3,0,0,&error)&&legacy.request(0,1,&error),"requested owned record");legacy.set_enabled(true);
    auto draw=[&](){render.begin_frame({});render.reset_arena();const auto n=legacy.draw(render,camera);render.end_frame();return n;};
    check(draw()==0,"legacy default still filters untextured billboard markers");legacy.set_untextured_billboards(true);check(draw()==3,"explicit authored effect emits all three untextured billboard meshes");legacy.set_untextured_billboards(false);check(draw()==0,"explicit opt-in can be revoked");
    auto model=character_model_fixture::archive(),prefix=character_model_fixture::prefix(),rig=character_model_fixture::profile();std::array<CharacterModelScene,2> actors;for(auto& a:actors)check(a.load({model.data(),model.size()},{prefix.data(),prefix.size()},{rig.data(),rig.size()},&error),"owned actor resource");auto profile_bytes=startup_reveal_fixture::profile(bank,startup_reveal_fixture::hash(model));StartupRevealProfile profile;check(startup_reveal_load({profile_bytes.data(),profile_bytes.size()},&profile,&error),"owned reveal profile");StartupRevealScene scene;check(scene.load(bank,profile,{&actors[0],&actors[1]},StartupRevealMode::DeclaredNativeActorSnapshots,&error),"qualified source orb owner");
    const auto w0=startup_reveal_fixture::world(actors[0],0),w1=startup_reveal_fixture::world(actors[1],0);
    for(unsigned frame=1;frame<=3;++frame){StartupDirectorState d;d.updates=frame;d.phase=3;check(scene.before_actors(d,1,true,&error),"early callback before actors");std::vector<StartupRevealSound> cues;std::array<StartupRevealActor,2> snapshots={StartupRevealActor{&actors[0],&w0,frame,0},StartupRevealActor{&actors[1],&w1,frame,0}};check(scene.after_actors(snapshots,0,&cues,&error),"hidden accepted actor copy");render.begin_frame({});render.reset_arena();const auto n=scene.draw_model(3,render,camera,snapshots)+scene.draw_model(4,render,camera,snapshots);render.end_frame();check(n==(frame==1?0u:6u),"both orb records emit their untextured billboard geometry when source-visible");check(cues.empty(),"orb fix does not invent sound completion");}
    std::printf("{\"orb_billboard_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
