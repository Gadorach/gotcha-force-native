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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source relaunch-entry surface: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem{
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
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source relaunch-entry surface: optional qualified original pack absent");return 0;}
    std::vector<u8> bytes;CharacterMovementProfile profile;CharacterMovementInputProfile input;
    check(files.read(inputs+"/motion.gfmm",bytes)&&character_movement_load({bytes.data(),bytes.size()},&profile,&error),"unchanged qualified GFMM6");
    check(files.read(inputs+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"unchanged input/resource profile");
    auto triangle=stage_ground_fixture::triangle(0);const f32 length=std::sqrt(10.f);
    for(auto& v:triangle.vertices)v[1]=-3*v[0];
    triangle.plane={3/length,1/length,0,0};
    StihGrid floor,empty;auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto data=stage_ground_fixture::bytes(triangles);check(grid.load({data.data(),data.size()},&error),"owned numeric plane");
    };
    load(floor,{triangle});load(empty,{});
    for(bool direction:{false,true})for(s16 value:{s16(5000),s16(75)}){
        CharacterMovementMotionState before;before.position={-200,610,-200};before.speed44=4;before.vertical48=-30;before.vertical_change50=-.07f;
        GRedRelaunchPlan plan;
        check(gred_relaunch_plan(profile,{},before,&plan)&&plan.entry&&plan.request&&plan.program==14&&plan.cost_index==0&&
            plan.motion.vertical48==-15,"entry adds15 to old descending speed before motion; no wind-up half or continued reset");
        CharacterAirResourceState resource;resource.value736=value;
        check(character_air_resource_consume(resource,input.costs[0],input.recovery_delay,1,&resource)&&resource.value736==std::max(0,int(value)-150),
            "entry consumes150 once before motion/contact, not continuation100");
        GRedRelaunchState state;check(gred_relaunch_lower_finished(plan,true,&state)&&state.lower_stage==1,"entry request cannot count as lower completion");
        auto accelerated=plan.motion;
        check(gred_jump_acceleration(profile,direction,&accelerated),"source neutral/directional acceleration before clamped integration");
        CharacterMovementCeiling ceiling;CharacterMovementMotionState integrated;
        check(character_movement_air_step(profile,accelerated,1,1,true,ceiling,&integrated,&error)&&integrated.position[1]==595&&
            integrated.vertical48==f32(-15+accelerated.vertical_change50),"displacement uses additive old speed, then positive acceleration/clamp");
        CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
        ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=610;ground.query_height668=80;ground.step674=30;
        GRedSurfaceContact contact;
        check(gred_surface_contact(profile,ground,before,integrated,{0,0,true},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Pushed&&
            contact.motion.vertical48==f32(integrated.vertical48*.9f)&&contact.motion.vertical_change50==accelerated.vertical_change50&&
            contact.motion.planar_change4c==0,"entry push returns0, keeps jump acceleration and damped velocity; no landing/refill");
        GRedRelaunchUpperPlan upper;
        check(gred_relaunch_upper_plan(state,&upper)&&upper.request&&upper.program==14&&upper.next_stage==1,
            "upper entry requests14 independently after lower motion/contact, not an early15 handoff");
        CharacterAirResourceState recovered=resource;
        check(character_air_resource_recover(resource,0,0x40,1,input.resource_limit,input.recovery_increment,&recovered)&&recovered.value736==resource.value736,
            "depleted or nondepleted entry projection remains airborne for resource recovery");
        const auto retained=contact.motion.position;
        check(!gred_surface_contact(profile,ground,before,integrated,{0,0,false},&contact,&error)&&contact.motion.position==retained,
            "same operands cannot give entry projection authority to earlier mode");
    }
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source relaunch entry fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"qualified original stage and normal Borg models");
    BattleMvpConfig config;config.spawn[0]={-606.4511719f,-1373.5872803f};
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode,const std::string& pack){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "native player placement only, ordinary target/bounds/references and live source seed");
        check(!source.relaunches()&&!source.relaunch_surface_pushes()&&!source.relaunch_entry_surface_pushes(),"fresh committed entry counters");
    };
    auto pad_at=[&](unsigned t){
        GamePadState pad;pad.b=(t>=5&&t<31)||(t>=78&&!source.relaunch_surface_pushes());
        if(source.relaunch_entry_surface_pushes()&&source.selector()==8)pad.dpad_left=true;
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"matching entry-push native receipt before source cost/VM/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::RelaunchSurface,inputs);bool old_refused=false;
    for(unsigned t=0;t<150&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto selector=source.selector();const auto meter=source.resource();
        if(!prepare(pad_at(t),&offer)){
            check(tick==78&&selector==6&&world.tick()==tick&&source.resource()==meter&&!source.relaunches()&&!source.relaunch_entry_surface_pushes()&&
                !world.step({},false,&view),"older continuation mode refuses entry projection without charging or falling back");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"same GFMM6 does not widen earlier entry contact authority");
    const std::string combined_pack="local/content/source-gred-ground-v9";
    const bool has_combined=files.exists(combined_pack+"/motion.gfmm");
    if(has_combined){
        bind(NativeGRedGroundDashDemo::Mode::GroundRelaunchPush,combined_pack);
        while(world.tick()<78){NativeBorgMotionOffer offer;check(prepare(pad_at(world.tick()),&offer),"previous combined mode follows the approach");accept(offer);}
        NativeBorgMotionOffer refused;const auto current=pose;const auto meter=source.resource();
        check(!prepare(pad_at(78),&refused)&&world.tick()==78&&!source.relaunches()&&source.resource()==meter&&
            current.frame_scope.expired()&&!world.motion_receipt_current(current.receipt)&&!world.step({},false,&view),
            "previous combined mode refuses entry contact without committing or falling back");
    }
    for(bool combined:{false,true}){
    if(combined&&!has_combined)continue;
    bind(combined?NativeGRedGroundDashDemo::Mode::GroundRelaunchEntryPush:NativeGRedGroundDashDemo::Mode::RelaunchEntrySurface,
         combined?combined_pack:inputs);
    u32 entry_tick=0,touchdown_tick=0;s16 entry_meter=0;
    NativeBorgMotionOffer prior;
    for(unsigned t=0;t<240;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto meter=source.resource();const auto count=source.relaunches();
        const auto pushes=source.relaunch_entry_surface_pushes();
        const auto stages=source.relaunch_stages();const auto programs=source.movement_programs();const auto retained=pose;
        check(prepare(pad_at(t),&offer)&&source.resource()==meter&&source.relaunches()==count&&source.relaunch_entry_surface_pushes()==pushes&&
            source.movement_programs()==programs&&source.relaunch_stages().lower_stage==stages.lower_stage&&
            source.relaunch_stages().upper_stage==stages.upper_stage,"entry cost, latches and counters stay private before acceptance");
        if(offer.kind==NativeBorgMotionKind::RelaunchEntryPush){
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            const auto position=world.actors()[0].position;
            const auto rejects=[&](NativeBorgMotionOffer edited,const char* why){
                check(!world.step({},false,&view,nullptr,&frame,&edited)&&world.tick()==tick&&world.actors()[0].position==position&&
                    source.resource()==meter&&source.relaunches()==count&&source.relaunch_entry_surface_pushes()==pushes&&
                    source.movement_programs()==programs&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            auto edited=offer;edited.entry_vertical_delta=14;
            rejects(edited,"edited lift cannot spend resource or revoke the prior accepted pose");
            if(combined){
                check(offer.relaunch_motion&&prior.jump_motion&&offer.controller_timer698==0.f,
                    "combined entry proves the prior falling motion and zero timer");
                const auto& e=*offer.relaunch_motion;const auto& p=*offer.static_push;
                check(e.entry&&e.lower_before==0&&e.upper_before==0&&e.lower_after==1&&e.upper_after==1&&
                    e.lower_program==14&&e.upper_program==14&&e.speed_before==prior.jump_motion->speed44&&
                    e.vertical_before==f32(prior.vertical_speed+15)&&e.vertical_before<=0&&
                    p.vertical_speed==std::clamp(std::fma(e.vertical_accel,1.f,e.vertical_before),-30.f,18.f)&&
                    offer.vertical_speed==f32(p.vertical_speed*p.damping)&&e.resource_before==meter&&
                    e.resource_after==meter-input.costs[0]&&!offer.relaunch_floor,
                    "entry applies lift before integration, projects with .9 damping, and requests both lanes exactly once");
                edited=offer;edited.relaunch_motion->vertical_before=prior.vertical_speed;
                rejects(edited,"entry cannot integrate the unlifted velocity");
                edited=offer;edited.kind=NativeBorgMotionKind::RelaunchWindupPush;edited.entry_vertical_delta=0;
                rejects(edited,"an entry cannot borrow the continued push kind");
                edited=offer;edited.relaunch_motion->resource_after=meter-input.costs[1];
                rejects(edited,"entry cannot borrow continuation cost");
                edited=offer;edited.relaunch_motion->upper_after=2;
                rejects(edited,"entry cannot skip the first upper request");
                edited=offer;edited.static_push->damping=.8f;
                rejects(edited,"entry steep contact cannot use floor damping");
                edited=offer;edited.controller_timer698=1;
                rejects(edited,"positive698 remains unsupported");
            }
        }
        accept(offer);
        if(offer.kind==NativeBorgMotionKind::RelaunchEntryPush){
            entry_tick=world.tick();entry_meter=source.resource();const auto current=source.relaunch_stages();
            check(offer.static_push&&offer.entry_vertical_delta==15&&offer.static_push->surface.triangle==1320&&
                offer.static_push->relaunch_acceleration==profile.air[0]&&
                offer.static_push->integrated_position[1]==f32(offer.observed.position[1]+f32(offer.observed.vertical_speed+15))&&
                source.selector()==8&&source.movement_programs()==std::array<u16,2>{14,14}&&current.lower_stage==1&&current.upper_stage==1&&
                source.resource()==meter-input.costs[0]&&source.resource()==8400&&source.relaunches()==1&&source.relaunch_surface_pushes()==1&&
                source.relaunch_entry_surface_pushes()==1&&source.surface_pushes()==1&&!source.landings()&&!world.actors()[0].grounded&&
                world.actors()[0].facing==offer.observed.facing,"original entry adds before movement/acceleration/projection, pays150 once and keeps independent request14 latches");
            NativeBorgMotionReceipt receipt;auto changed=offer;changed.kind=NativeBorgMotionKind::RelaunchWindupPush;
            check(!world.motion_receipt(changed,&receipt),"entry receipt cannot be relabelled as continued relaunch");
            GamePadState pause;pause.start=true;const auto live=pose;
            check(world.step({pause,{}},false,&view)&&world.paused()&&world.tick()==entry_tick&&source.resource()==entry_meter&&
                source.pose(world,&pose)&&!live.frame_scope.expired(),"pause freezes accepted entry projection and spending");
            check(world.step({},false,&view)&&world.step({pause,{}},false,&view)&&!world.paused()&&world.tick()==entry_tick&&view.update_source(world,pose,&error),
                "unpause cannot repeat additive entry or hidden VM update");
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            touchdown_tick=world.tick();
            check(entry_tick&&touchdown_tick==entry_tick+1&&source.movement_programs()==std::array<u16,2>{21,21}&&source.selector()==7&&
                source.resource()==entry_meter&&world.actors()[0].grounded&&world.actors()[0].facing==offer.observed.facing,
                 "accepted soft drift and falling21 give a separate real touchdown, not a relaunch landing/refill or camera-forced heading");
            if(combined)check(offer.jump_motion&&prior.relaunch_motion&&offer.jump_motion->relaunch_origin&&
                offer.jump_motion->speed_before==prior.relaunch_motion->speed44&&offer.jump_motion->vertical_before==prior.vertical_speed,
                "release inherits the damped entry response");
        }
        prior=offer;
    }
    check(entry_tick==79&&touchdown_tick==80&&source.relaunches()==1&&source.relaunch_entry_surface_pushes()==1&&source.relaunch_surface_pushes()==1&&
        source.surface_pushes()==1&&source.landings()==1&&source.selector()==0&&source.resource()==10000&&!world.shots_spawned()&&
        world.actors()[0].health==100&&world.actors()[1].health==100,"bounded original entry-push route falls/lands/refills once through shared systems");
    const auto retained=pose;source.reject();
    check(retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt),"entry producer retirement revokes pose and receipt");
    std::printf("%s entry: entry=%u touchdown=%u meter=%d\n",combined?"combined":"legacy",entry_tick,touchdown_tick,entry_meter);
    }
    check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v5",world,seed,&error,NativeGRedGroundDashDemo::Mode::RelaunchEntrySurface)&&world.tick()==0,
        "legacy pack cannot fabricate entry-push operands");
    std::printf("source relaunch-entry additive/spend/lane/contact/pose: %u checks PASS; native proxies, no whole-frame equivalence\n",checks);
}
