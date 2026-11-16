#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source falling surface: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem {
public:
    bool file_size(const std::string& name,gf::u64* out)override{std::error_code e;const auto size=std::filesystem::file_size(name,e);if(e||!out)return false;*out=size;return true;}
    bool read(const std::string& name,std::vector<gf::u8>& out)override{gf::u64 size=0;if(!file_size(name,&size)||size>(32u<<20))return false;std::ifstream f(name,std::ios::binary);out.assign(std::istreambuf_iterator<char>(f),{});return out.size()==size;}
    bool exists(const std::string& name)override{return std::filesystem::exists(name);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",inputs="local/content/source-gred-ground-v6";
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source falling surface: optional qualified original pack absent");return 0;}
    std::vector<u8> bytes;CharacterMovementProfile profile;
    check(files.read(inputs+"/motion.gfmm",bytes)&&character_movement_load({bytes.data(),bytes.size()},&profile,&error),"qualified unchanged GFMM6 operands");
    //Both history choices use the recovered SAME selector6 reset/integration,
    //but retain their own animation. Synthetic steep plane, not edited stage.
    auto triangle=stage_ground_fixture::triangle(0);const f32 length=std::sqrt(10.f);
    for(auto& v:triangle.vertices)v[1]=-3*v[0];
    triangle.plane={3/length,1/length,0,0};
    StihGrid floor,empty;auto load=[&](StihGrid& grid,const std::vector<StihTriangle>& triangles){
        auto data=stage_ground_fixture::bytes(triangles);check(grid.load({data.data(),data.size()},&error),"owned numeric grid");
    };
    load(floor,{triangle});load(empty,{});
    for(unsigned history=1;history<=2;++history)for(u8 coarse=0;coarse<4;++coarse){
        GRedVerticalJumpState state{6,1,false};state.falling_after_dash=history==1;state.falling_after_relaunch=history==2;state.falling_coarse57d=coarse;
        CharacterMovementMotionState before;before.position={-200,610,-200};before.vertical48=-15;before.vertical_change50=.8f;before.planar_change4c=2;
        GRedVerticalJumpPlan plan;
        check(gred_vertical_jump_plan(profile,state,before,5000,0,&plan,true)&&plan.request&&plan.program==(history==1?13:21+coarse)&&
            plan.motion.vertical_change50==-1&&plan.motion.planar_change4c==0&&plan.state.cached87==1,
            "history-dependent falling entry preserves program, resets50/4C before motion and keeps airborne cache");
        CharacterMovementCeiling ceiling;CharacterMovementMotionState integrated;
        check(character_movement_air_step(profile,plan.motion,1,1,false,ceiling,&integrated,&error)&&integrated.position[1]==595&&integrated.vertical48==-16,
            "history falling uses old velocity then reset acceleration, not active/relaunch motion");
        CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
        ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=610;ground.query_height668=80;ground.step674=30;
        GRedSurfaceContact contact;
        check(gred_surface_contact(profile,ground,before,integrated,{0,0,true},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Pushed&&
            contact.motion.vertical48==f32(-16*.9f)&&contact.motion.vertical_change50==-1&&
            gred_vertical_jump_finish(plan,false,false,&state)&&state.selector==6&&state.cached87==1,
            "post-dash/relaunch numeric projection is not landing, refill or active-callback reset");
    }
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source falling surface fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"qualified shared original stage/models");
    BattleMvpConfig config;config.spawn[0]={-606.4511719f,-1373.5872803f};
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,inputs,world,seed,&error,mode),
            "native spawn and same-world source pose binding");
        check(!source.surface_pushes()&&!source.dash_fall_pushes()&&!source.relaunch_fall_pushes(),"fresh binding clears commit-only history counters");
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact native receipt before shared source pose publication");
    };
    auto pad_at=[](unsigned t){GamePadState pad;pad.b=(t>=5&&t<33)||(t>=45&&t<60);return pad;};
    bind(NativeGRedGroundDashDemo::Mode::Steep);bool older_refused=false;
    for(unsigned t=0;t<240&&!older_refused;++t){
        const auto tick=world.tick();NativeBorgMotionOffer offer;
        if(!prepare(pad_at(t),&offer)){
            check(world.tick()==tick&&source.relaunches()==1&&!source.surface_pushes()&&!world.step({},false,&view),
                "older mode still refuses actual post-relaunch steep contact without gravity fallback");
            older_refused=true;
        }else accept(offer);
    }
    check(older_refused,"loading GFMM6 does not widen ordinary-fall mode");
    bind(NativeGRedGroundDashDemo::Mode::FallingSurface);
    u32 push_tick=0;bool falling_program=false;
    for(unsigned t=0;t<240;++t){
        NativeBorgMotionOffer offer;const auto meter=source.resource();const auto selector=source.selector();
        const auto count=source.relaunch_fall_pushes();const auto programmes=source.movement_programs();
        check(prepare(pad_at(t),&offer)&&source.resource()==meter&&source.selector()==selector&&source.relaunch_fall_pushes()==count&&
            source.movement_programs()==programmes,"history VM, resource and counters stay private until acceptance");
        accept(offer);
        if(source.selector()==6&&source.movement_programs()==std::array<u16,2>{21,21})falling_program=true;
        if(offer.kind==NativeBorgMotionKind::StaticPush){
            push_tick=world.tick();
            check(offer.static_push&&offer.static_push->surface.triangle==1320&&source.movement_programs()==std::array<u16,2>{21,21}&&
                source.selector()==6&&!world.actors()[0].grounded&&source.resource()==meter&&meter<10000&&!source.landings()&&
                source.relaunches()==1&&source.relaunch_fall_pushes()==1&&!source.dash_fall_pushes(),
                "actual original steep projection retains relaunch-fall program21 and airborne resource, not ordinary6 or landing");
        }
    }
    check(falling_program&&push_tick==123&&source.surface_pushes()==1&&source.relaunch_fall_pushes()==1&&source.landings()==1&&source.selector()==0&&
        source.resource()==10000&&std::abs(world.actors()[0].position[1]-10)<.001f&&!world.shots_spawned()&&world.actors()[0].health==100,
        "original post-relaunch fall pushes once, then uses separate landing/refill on lower original floor");
    const auto retained=pose;source.reject();
    check(retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt),"history producer retirement revokes pose and receipt");
    //A late relaunch reaches the steep face IN selector8: it must still refuse.
    bind(NativeGRedGroundDashDemo::Mode::FallingSurface);bool active_refused=false;
    for(unsigned t=0;t<120&&!active_refused;++t){
        GamePadState pad;pad.b=(t>=5&&t<33)||t>=81;NativeBorgMotionOffer offer;const auto selector=source.selector();const auto tick=world.tick();
        if(!prepare(pad,&offer)){
            check(selector==8&&source.relaunches()==1&&!source.surface_pushes()&&world.tick()==tick&&!world.step({},false,&view),
                "active relaunch contact cannot borrow post-relaunch falling push authority");
            active_refused=true;
        }else accept(offer);
    }
    check(active_refused,"actual original early relaunch steep contact remains unsupported");
    //Retain an ordinary stage route through air-dash recovery and program13.
    config={};bind(NativeGRedGroundDashDemo::Mode::FallingSurface);bool dash_fall=false;
    for(unsigned t=0;t<480;++t){
        //Enough initial altitude for recovery's VM to finish BEFORE floor
        //contact, so selector6/program13 actually executes on this route.
        GamePadState pad;pad.b=t>=5&&t<90;if(t>=80&&t<122)pad.stick_y=-127;
        NativeBorgMotionOffer offer;check(prepare(pad,&offer),"existing dash/recovery movement candidate");accept(offer);
        dash_fall|=source.selector()==6&&source.movement_programs()==std::array<u16,2>{13,13};
    }
    check(dash_fall&&source.air_dash_starts()==1&&source.landings()==1&&source.selector()==0&&source.resource()==10000,
        "same source owner retains post-dash program13 continuation and ordinary landing/refill");
    std::printf("source falling-history surface/VM/resource/pose: %u checks PASS; push tick=%u, native proxies, no whole-frame equivalence\n",checks,push_tick);
}
