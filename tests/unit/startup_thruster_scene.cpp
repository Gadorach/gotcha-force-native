#include <cstdio>
#include <cstdlib>
#include <limits>
#include "tests/unit/startup_thruster_fixture.hpp"
#include "engine/core/headless_backends.hpp"
namespace {
unsigned checks=0;
void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"startup thruster: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;using startup_thruster_fixture::Fixture;std::string error;Fixture f;check(f.load(&error),"owned source-shaped assets");
    StartupActorFrame actor;StartupThrusterScene jets;StartupActorFrameOutput out;
    check(f.bind(actor,jets,&error)&&jets.held_parent708_plan()==5&&actor.state()->scalars.field708==0,"held mask05 plan does not write actual parent");
    check(!actor.state()->have_declared_position,"constructor has no invented accepted position");
    headless::Render backend;RenderEngine render(backend);check(render.initialize({640,480,false,true,"owned thrusters"},nullptr,&error),"render initialization");
    Camera camera;camera.eye[2]=100;camera.target[2]=0;camera.far_plane=1000;camera.fov_y_degrees=100;
    jets.upload_textures(render);check(jets.draw(actor,render,camera)==0,"no draw before accepted frame");
    StartupActorFrameInput input;input.isolated_external_mutations=input.placement_declared=true;input.position={3,4,5};
    check(actor.request(0,&out,&error)&&actor.state()->vm.state().actor.auxiliary==5,"request prefix sets both owned auxiliary bits before scalar preupdate");
    std::array<u8,6> expected{};std::array<StartupAuxiliaryState,2> previous{};
    for(unsigned frame=1;frame<=10;++frame){
        const auto bits=actor.state()->vm.state().actor.auxiliary;
        for(unsigned c=0;c<6;++c){if((5u&(1u<<c))&&(bits&(1u<<c)))expected[c]=2;if(expected[c])--expected[c];}
        input.position[0]+=1;
        check(jets.before_actor(actor,input,frame,&error)&&!jets.states()&&jets.draw(actor,render,camera)==0,"pre stage suppresses previous publication");
        check(actor.step(input,&out,&error)&&actor.state()->declared_position==input.position&&actor.state()->have_declared_position,"accepted placement belongs to current staged actor");
        check(jets.after_actors(actor,frame,0,&error)&&*jets.leases()==expected&&actor.state()->scalars.field708==0,"pre-VM lease snapshot and parent guard preserved");
        check(jets.motion()->current20==input.position&&jets.motion()->previous2c==input.position,"history is entry5685C copy, not previous accepted position/root");
        StartupAuxiliaryParent parent;parent.model94=0x615;parent.phase18=actor.state()->scalars.phase18;parent.bones=actor.state()->world.matrices;
        for(unsigned e=0;e<2;++e){
            if(frame==1){StartupAuxiliaryConstruction made;check(startup_auxiliary_construct(f.profile,e,parent,0,true,0,&made,&error),"component comparison construct");previous[e]=made.state;}
            parent.active_count=expected[f.profile.entries[e].channel];StartupAuxiliaryState next;
            check(startup_auxiliary_step_with_turning(f.profile,f.turn,e,parent,*jets.motion(),previous[e],&next,&error),"existing component comparison (not independent world proof)");
            const auto& actual=jets.states()->at(e);check(actual.phase==next.phase&&actual.counter==next.counter&&actual.cached_a8==next.cached_a8&&actual.previous_a9==next.previous_a9,"owner preserves component lifecycle/history ordering");
            for(unsigned n=0;n<2;++n){check(std::equal(std::begin(actual.matrices[n].m),std::end(actual.matrices[n].m),std::begin(next.matrices[n].m))&&actual.history[n]==next.history[n],"all component matrices/history exact compatibility");}
            previous[e]=next;
        }
        render.begin_frame({});render.reset_arena();check(jets.draw(actor,render,camera)==(expected[0]?4u:0u)+(expected[2]?4u:0u),"every owned mesh of each active instance drawn");render.end_frame();
    }
    check(expected[0]==0&&expected[2]==0,"cleared VM bits expire leases naturally");
    check(actor.request(0,&out,&error)&&jets.draw(actor,render,camera)==0&&!jets.ready(),"request invalidates parent publication; old jets not replayed");
    check(f.bind(actor,jets,&error)&&actor.request(0,&out,&error),"explicit fresh bind after refusal");
    check(jets.before_actor(actor,input,120,&error)&&actor.step(input,&out,&error)&&jets.after_actors(actor,120,1,&error)&&jets.draw(actor,render,camera)==0,"first global frame may be120; scene34 suppresses display only in explicit observation subset");
    check(!jets.before_actor(actor,input,122,&error)&&!jets.ready(),"global frame gap refuses");
    for(unsigned mode:{0u,5u})for(unsigned allocation=0;allocation<4;++allocation){
        const std::array<bool,2> available={bool(allocation&1),bool(allocation&2)};
        check(f.bind(actor,jets,&error,u8(mode),available),"source construction branch");
        check(jets.held_parent708_plan()==(mode==5?0:((available[0]?1:0)|(available[1]?4:0))),"only successful non-mode5 allocations add held channel plans");
        check(actor.request(0,&out,&error)&&jets.before_actor(actor,input,1,&error)&&actor.step(input,&out,&error)&&jets.after_actors(actor,1,0,&error),"allocation branch stages");
        for(unsigned e=0;e<2;++e)check(jets.states()->at(e).allocated==(mode!=5&&available[e]),"uncreated children not fabricated");
    }
    for(unsigned kind=0;kind<11;++kind){
        check(f.bind(actor,jets,&error)&&actor.request(0,&out,&error),"fresh refusal case");
        auto entry=input;
        if(kind==0){check(!jets.after_actors(actor,1,0,&error),"after without pre refuses");}
        if(kind==1){check(jets.before_actor(actor,entry,1,&error)&&!jets.before_actor(actor,entry,1,&error),"duplicate pre refuses");}
        if(kind==2){check(jets.before_actor(actor,entry,1,&error)&&!jets.after_actors(actor,1,0,&error),"unadvanced actor refuses");}
        if(kind==3){check(jets.before_actor(actor,entry,1,&error)&&actor.step(entry,&out,&error)&&!jets.after_actors(actor,2,0,&error),"wrong global frame refuses");}
        if(kind==4){check(jets.before_actor(actor,entry,1,&error),"mismatched position pre");entry.position[1]+=1;check(actor.step(entry,&out,&error)&&!jets.after_actors(actor,1,0,&error),"position input mismatch refuses");}
        if(kind==5){entry.placement_declared=false;check(!jets.before_actor(actor,entry,1,&error),"undeclared position refuses");}
        if(kind==6){entry.position[0]=std::numeric_limits<float>::quiet_NaN();check(!jets.before_actor(actor,entry,1,&error),"nonfinite position refuses");}
        if(kind==7){check(jets.before_actor(actor,entry,1,&error)&&actor.step(entry,&out,&error)&&actor.step(entry,&out,&error)&&!jets.after_actors(actor,1,0,&error),"skipped parent frame refuses");}
        if(kind==8){StartupActorFrame neighbor;check(neighbor.load(f.actor.assets(),StartupActorFrameMode::IsolatedRequirements,0,&out,&error)&&!jets.before_actor(neighbor,entry,1,&error),"same-model neighbor generation refuses");}
        if(kind==9){check(jets.before_actor(actor,entry,1,&error),"reload pre");check(actor.load(f.actor.assets(),StartupActorFrameMode::IsolatedRequirements,0,&out,&error)&&!jets.after_actors(actor,1,0,&error),"parent reload refuses");}
        if(kind==10){check(jets.before_actor(actor,entry,1,&error),"failed actor pre");entry.position[0]=1e8f;check(!actor.step(entry,&out,&error)&&!actor.state()->have_declared_position&&!jets.after_actors(actor,1,0,&error),"failed actor never commits placement or auxiliary state");}
        check(!jets.ready()&&!jets.states()&&jets.draw(actor,render,camera)==0,"all refusals latch and clear all draws");
    }
    check(f.bind(actor,jets,&error)&&actor.request(0,&out,&error),"late failure setup");
    check(jets.before_actor(actor,input,1,&error)&&actor.step(input,&out,&error)&&jets.after_actors(actor,1,0,&error),"initial valid publication");
    const auto committed=jets.parent_frames();auto bad=input;bad.position[0]=1e8f;
    check(!actor.step(bad,&out,&error)&&actor.state()->declared_position==input.position&&jets.draw(actor,render,camera)==0&&!jets.ready()&&jets.parent_frames()==committed,"late parent failure retains accepted position but invalidates jet publication");
    check(f.bind(actor,jets,&error),"move setup");auto moved=std::move(jets);check(!jets.ready()&&moved.ready()&&actor.request(0,&out,&error)&&moved.before_actor(actor,input,1,&error)&&actor.step(input,&out,&error)&&moved.after_actors(actor,1,0,&error),"heap-stable owner move");
    auto moved_actor=std::move(actor);check(!actor.ready()&&moved.bound(moved_actor),"heap-stable parent move");moved.release();check(!moved.ready(),"explicit release");
    check(f.bind(actor,jets,&error),"invalid bind setup");auto wrong=f.profile;wrong.entries[0].parts[0].bone=3;
    check(!jets.bind(actor,{f.model.data(),f.model.size()},wrong,f.turn,StartupThrusterMode::DeclaredIsolatedConstructionAndPlacement,0,{true,true},&error),"actual bone3 not silently remapped for two-bone owned rig");
    check(!jets.bind(actor,{f.model.data(),f.model.size()},f.profile,f.turn,static_cast<StartupThrusterMode>(99),0,{true,true},&error),"unknown scope refuses");
    check(f.bind(actor,jets,&error),"disabled actor prefix setup");
    for(unsigned frame=1;frame<=120;++frame){
        check(jets.before_actors(actor,input,frame,false,&error)&&jets.after_actors(actor,frame,0,&error),"group18 updates without actor VM updates");
        check(actor.state()->frames==0&&!actor.state()->have_declared_position&&!jets.motion()->source_positions_declared&&jets.states()->at(0).counter==s16(frame-1)&&jets.draw(actor,render,camera)==0,"no position/root read or fake actor frame; auxiliary counter advances");
    }
    check(actor.request(0,&out,&error)&&jets.before_actor(actor,input,121,&error)&&actor.step(input,&out,&error)&&jets.after_actors(actor,121,0,&error)&&jets.states()->at(0).counter==120,"first actor frame does not reset independently running auxiliary clock");
    check(jets.before_actors(actor,input,122,false,&error)&&jets.after_actors(actor,122,0,&error)&&jets.leases()->at(0)==1&&actor.state()->frames==1,"disabled parent freezes lease but not auxiliary callback");
    auto late=f;for(auto& part:late.profile.entries[1].parts)part.position[0]=10000;
    check(late.bind(actor,jets,&error)&&actor.request(0,&out,&error),"second-child late failure setup");
    auto far=input;far.position={9990000,0,0};
    check(jets.before_actor(actor,far,1,&error)&&actor.step(far,&out,&error),"accepted actor before derived auxiliary-domain failure");
    check(!jets.after_actors(actor,1,0,&error)&&!jets.ready()&&jets.parent_frames()==0&&actor.state()->frames==1&&jets.draw(actor,render,camera)==0,"second child failure clears both publications and leaves auxiliary counters uncommitted, not separately accepted parent");
    std::printf("{\"startup_thruster_owned_checks\":%u,\"parent708_written\":false,\"audio_fulfilled\":false,\"full_startup\":false,\"all_passed\":true}\n",checks);
}
