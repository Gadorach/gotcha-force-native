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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source relaunch surface: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source relaunch surface: optional qualified original pack absent");return 0;}
    std::vector<u8> bytes;CharacterMovementProfile profile;CharacterMovementInputProfile input;
    check(files.read(inputs+"/motion.gfmm",bytes)&&character_movement_load({bytes.data(),bytes.size()},&profile,&error),"unchanged qualified GFMM6");
    check(files.read(inputs+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"unchanged resource/input profile");
    auto triangle=stage_ground_fixture::triangle(0);const f32 length=std::sqrt(10.f);
    for(auto& v:triangle.vertices)v[1]=-3*v[0];
    triangle.plane={3/length,1/length,0,0};
    StihGrid floor,empty;auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto data=stage_ground_fixture::bytes(triangles);check(grid.load({data.data(),data.size()},&error),"owned numeric plane");
    };
    load(floor,{triangle});load(empty,{});
    for(u8 stage:{u8(1),u8(2),u8(3)})for(bool direction:{false,true})for(bool done:{false,true})for(s16 value:{s16(5000),s16(50)}){
        GRedRelaunchState state{stage,stage};CharacterMovementMotionState before;before.position={-200,610,-200};
        before.speed44=4;before.vertical48=-15;before.vertical_change50=-.07f;
        GRedRelaunchPlan plan;
        check(gred_relaunch_plan(profile,state,before,&plan)&&!plan.entry&&plan.cost_index==1&&plan.program==(stage==1?14:15)&&
            plan.motion.vertical48==before.vertical48,"continued relaunch retains old velocity and continuation cost, not additive entry");
        CharacterAirResourceState resource;resource.value736=value;
        check(character_air_resource_consume(resource,input.costs[plan.cost_index],input.recovery_delay,1,&resource)&&resource.value736==std::max(0,int(value)-100),
            "continued relaunch spends100 before its motion/contact, including program15 request");
        check(gred_relaunch_lower_finished(plan,done,&state),"lower completion latch");
        auto accelerated=plan.motion;
        check(gred_jump_acceleration(profile,direction,&accelerated)&&accelerated.vertical_change50>0&&
            (!direction?accelerated.speed44==f32(4*profile.air_damping[0]):accelerated.speed44==4),
            "neutral/directional jump acceleration and neutral scalar damping remain distinct from dash/recovery");
        CharacterMovementCeiling ceiling;CharacterMovementMotionState integrated;
        check(character_movement_air_step(profile,accelerated,1,1,true,ceiling,&integrated,&error)&&integrated.position[1]==595&&
            integrated.vertical48==f32(before.vertical48+accelerated.vertical_change50),"old velocity is integrated before reviewed positive acceleration/clamp");
        CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
        ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=610;ground.query_height668=80;ground.step674=30;
        GRedSurfaceContact contact;
        check(gred_surface_contact(profile,ground,before,integrated,{0,0,true},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Pushed&&
            contact.motion.vertical48==f32(integrated.vertical48*.9f)&&contact.motion.speed44==f32(integrated.speed44*.9f)&&
            contact.motion.vertical_change50==accelerated.vertical_change50&&contact.motion.planar_change4c==0,
            "relaunch return0 retains positive acceleration and damped speeds, without callback/landing/refill");
        GRedRelaunchUpperPlan upper;
        check(gred_relaunch_upper_plan(state,&upper)&&upper.program==(stage==1?14:15)&&
            (stage!=1||!done||(!upper.request&&upper.next_stage==2)),
            "upper still updates14 while lower completion schedules a later15 request; push cannot synchronize early");
        CharacterAirResourceState recovered=resource;
        check(character_air_resource_recover(resource,0,0x40,1,input.resource_limit,input.recovery_increment,&recovered)&&recovered.value736==resource.value736,
            "relaunch projection preserves airborne meter, even when the continuation spend depleted it");
        if(!resource.value736){
            CharacterMovementInputState held;held.previous_held=0x100;GRedMovementAdmissionFields fields;
            fields.selector=8;fields.flags5e0=0x40;fields.air_ready588=1;fields.prior_handlers_declined=true;GRedMovementAdmission admission;
            check(gred_movement_admit(input,held,resource,fields,&admission,&error)&&admission.changed&&admission.selector==6,
                "depletion is admitted as falling on next input pass, not an invented recovery or refill in the contact tail");
        }
        const auto retained=contact.motion.position;
        check(!gred_surface_contact(profile,ground,before,integrated,{0,0,false},&contact,&error)&&contact.motion.position==retained,
            "same GFMM6 cannot widen earlier relaunch contact authority");
    }
    CharacterMovementMotionState under;under.position={-200,595,-200};under.vertical48=-30;GRedRelaunchPlan entry;
    check(gred_relaunch_plan(profile,{},under,&entry)&&entry.entry&&entry.motion.vertical48==-15,"entry is still a separate additive15 step");
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
    ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=595;ground.query_height668=80;ground.step674=30;GRedSurfaceContact contact;
    check(!gred_surface_contact(profile,ground,under,entry.motion,{0,0,false},&contact,&error),"unaccepted below-plane entry cannot borrow continued relaunch permission");
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source relaunch surface fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"qualified shared original stage and normal Borg models");
    BattleMvpConfig config;config.spawn[0]={-606.4511719f,-1373.5872803f};
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,inputs,world,seed,&error,mode),
            "native player placement, unchanged ordinary bounds/target and real source binding");
        check(!source.surface_pushes()&&!source.relaunch_surface_pushes()&&!source.relaunches(),"fresh relaunch counters");
    };
    auto pad_at=[&](unsigned t){GamePadState pad;pad.b=(t>=5&&t<33)||(t>=81&&!source.relaunch_surface_pushes());return pad;};
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact native relaunch receipt before private VM/resource/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::ActiveSurface);bool old_refused=false;
    for(unsigned t=0;t<160&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto selector=source.selector();const auto programs=source.movement_programs();
        if(!prepare(pad_at(t),&offer)){
            check(tick==82&&selector==8&&programs==std::array<u16,2>{14,14}&&world.tick()==tick&&!source.relaunch_surface_pushes()&&
                !world.step({},false,&view),"older active mode refuses original continued relaunch contact without commit or native fallback");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"loading the same GFMM6 does not widen earlier relaunch contact authority");
    bind(NativeGRedGroundDashDemo::Mode::RelaunchSurface);u32 entry_tick=0,push_tick=0,touchdown_tick=0;s16 push_meter=0;
    for(unsigned t=0;t<240;++t){
        NativeBorgMotionOffer offer;const auto meter=source.resource();const auto pushes=source.relaunch_surface_pushes();
        const auto stages=source.relaunch_stages();const auto programs=source.movement_programs();const auto tick=world.tick();const auto retained=pose;
        check(prepare(pad_at(t),&offer)&&source.resource()==meter&&source.relaunch_surface_pushes()==pushes&&source.movement_programs()==programs&&
            source.relaunch_stages().lower_stage==stages.lower_stage&&source.relaunch_stages().upper_stage==stages.upper_stage,
            "relaunch spend, lower/upper latches and push counters remain staged before acceptance");
        if(native_borg_motion_relaunch_push(offer.kind)){
            auto wrong=offer;wrong.static_push->relaunch_acceleration=profile.air[11];
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&source.resource()==meter&&!source.relaunch_surface_pushes()&&
                !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),
                "borrowed dash acceleration cannot spend staged relaunch cost, advance a world or revoke prior accepted pose");
        }
        accept(offer);
        if(offer.kind==NativeBorgMotionKind::RelaunchEntry){
            entry_tick=world.tick();
            check(entry_tick==82&&!offer.static_push&&offer.entry_vertical_delta==15&&source.resource()==meter-input.costs[0]&&
                world.actors()[0].position[1]==f32(offer.observed.position[1]+f32(offer.observed.vertical_speed+15))&&source.relaunches()==1,
                "separate entry still adds15 to old velocity/displacement and pays150 once, without projection");
        }
        if(native_borg_motion_relaunch_push(offer.kind)){
            push_tick=world.tick();push_meter=source.resource();const auto current=source.relaunch_stages();
            check(offer.kind==NativeBorgMotionKind::RelaunchWindupPush&&offer.static_push&&offer.static_push->surface.triangle==1320&&
                offer.static_push->relaunch_acceleration==profile.air[0]&&offer.entry_vertical_delta==0&&
                source.selector()==8&&source.movement_programs()==std::array<u16,2>{14,14}&&source.last_relaunch_push_programs()==std::array<u16,2>{14,14}&&
                current.lower_stage==1&&current.upper_stage==1&&!world.actors()[0].grounded&&!source.landings()&&
                world.actors()[0].facing==offer.observed.facing&&source.resource()==meter-input.costs[1]&&source.resource()==8100&&source.relaunch_surface_pushes()==1,
                "original program14 push retains separate latches/positive acceleration and pays100, not another additive entry or landing refill");
            NativeBorgMotionReceipt receipt;auto changed=offer;changed.static_push->relaunch_acceleration=0;
            check(!world.motion_receipt(changed,&receipt),"changed relaunch acceleration cannot borrow exact accepted receipt");
            GamePadState pause;pause.start=true;const auto live=pose;
            check(world.step({pause,{}},false,&view)&&world.paused()&&world.tick()==push_tick&&source.resource()==push_meter&&
                source.pose(world,&pose)&&!live.frame_scope.expired(),"pause freezes relaunch latches/spending and accepted pose");
            check(world.step({},false,&view)&&world.step({pause,{}},false,&view)&&!world.paused()&&world.tick()==push_tick&&
                view.update_source(world,pose,&error),"unpause does not advance a hidden relaunch update");
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            touchdown_tick=world.tick();
            check(push_tick&&touchdown_tick==push_tick+1&&source.selector()==7&&world.actors()[0].grounded&&source.resource()==push_meter&&
                source.movement_programs()==std::array<u16,2>{21,21},"release after push executes post-relaunch falling21 and a separate real touchdown, no instant refill");
        }
    }
    check(entry_tick==82&&push_tick==83&&touchdown_tick==84&&source.relaunches()==1&&source.relaunch_surface_pushes()==1&&source.surface_pushes()==1&&
        source.landings()==1&&source.selector()==0&&source.resource()==10000&&!world.shots_spawned()&&
        world.actors()[0].health==100&&world.actors()[1].health==100,"bounded original relaunch push route falls, lands and refills through shared source/native/render systems");
    const auto retained=pose;source.reject();
    check(retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt),"relaunch producer retirement revokes retained pose and receipt");
    check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v5",world,seed,&error,NativeGRedGroundDashDemo::Mode::RelaunchSurface)&&world.tick()==0,
        "legacy pack cannot fabricate relaunch steep operands");
    std::printf("source continued relaunch/lane/spend/contact/pose: %u checks PASS; entry=%u push=%u touchdown=%u meter=%d; native proxies, no whole-frame equivalence\n",
        checks,entry_tick,push_tick,touchdown_tick,push_meter);
}
