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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source active surface: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source active surface: optional qualified original pack absent");return 0;}
    std::vector<u8> bytes;CharacterMovementProfile profile;CharacterMovementInputProfile input;
    check(files.read(inputs+"/motion.gfmm",bytes)&&character_movement_load({bytes.data(),bytes.size()},&profile,&error),"unchanged qualified GFMM6");
    check(files.read(inputs+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"unchanged policy1 input/resource profile");
    auto triangle=stage_ground_fixture::triangle(0);const f32 length=std::sqrt(10.f);
    for(auto& v:triangle.vertices)v[1]=-3*v[0];
    triangle.plane={3/length,1/length,0,0};
    StihGrid floor,empty;auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto data=stage_ground_fixture::bytes(triangles);check(grid.load({data.data(),data.size()},&error),"owned numeric plane");
    };
    load(floor,{triangle});load(empty,{});
    for(unsigned gate=0;gate<3;++gate)for(s16 value:{s16(5000),s16(25),s16(0)})for(bool done:{false,true}){
        GRedAirDashState state;state.selector=16;state.entered=true;state.stage=gate==2?2:1;state.timer568=gate==0?4:gate==1?1:0;
        state.air_ready588=state.contact_callback=gate==2;
        CharacterMovementMotionState before;before.position={-200,610,-200};before.speed44=10;before.vertical48=-15;
        before.planar_change4c=gate==2?0:profile.air[6];before.vertical_change50=profile.air[11];
        GRedAirDashPlan plan;
        check(gred_air_dash_plan(profile,state,before,&plan)&&plan.advance&&!plan.request&&plan.program==11&&plan.cost_index==3&&!plan.cost_after_move&&
            plan.state.selector==16&&plan.state.stage==(gate==0?1:2)&&plan.state.air_ready588==(gate!=0),
            "continued active keeps program11; policy1 timer enables readiness, never forces recovery or follows clip completion");
        CharacterAirResourceState resource;resource.value736=value;
        check(character_air_resource_consume(resource,input.costs[plan.cost_index],input.recovery_delay,1,&resource)&&
            gred_air_dash_spent(plan,resource.value736,&state),"active spend and depletion decision precede steering/movement/contact");
        const auto expected=std::max(0,int(value)-50);
        check(resource.value736==expected&&state.selector==(expected?16:17)&&state.air_ready588==(gate!=0),
            "depletion selects future recovery while preserving source readiness, without spending twice or inventing a flight count");
        CharacterMovementMotionState integrated;CharacterMovementCeiling ceiling;
        check(character_movement_air_step(profile,plan.motion,1,1,false,ceiling,&integrated,&error)&&integrated.position[1]==595&&
            integrated.position[2]==-190&&integrated.vertical48==f32(-15+profile.air[11]),
            "active motion integrates old speeds without wind-up half-speed or recovery .99/-1 rules");
        CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
        ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=610;ground.query_height668=80;ground.step674=30;
        GRedSurfaceContact contact;GRedAirDashContactTail tail;
        check(gred_surface_contact(profile,ground,before,integrated,{0,0,true},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Pushed&&
            contact.motion.speed44==f32(integrated.speed44*.9f)&&contact.motion.vertical48==f32(integrated.vertical48*.9f)&&
            contact.motion.planar_change4c==0&&contact.motion.vertical_change50==profile.air[11]&&
            gred_air_dash_contact_tail(16,state.contact_callback,done,false,&tail)&&!tail.landing&&!tail.falling,
            "active push returns0 even before landing callback is ready or after depletion; no zero velocity, landing, refill or early fall");
        GRedAirDashPlan next;
        check(gred_air_dash_plan(profile,state,contact.motion,&next)&&
            (expected?(next.program==11&&!next.request&&next.motion.speed44==contact.motion.speed44&&next.motion.planar_change4c==0):
                      (next.program==12&&next.request&&next.motion.speed44==contact.motion.speed44&&next.motion.vertical_change50==-1)),
            "after push, continue damped active speed without reapplying entry drag/reset, or enter separate recovery at zero meter");
        CharacterAirResourceState recovered=resource;
        check(character_air_resource_recover(resource,0,0x40,1,input.resource_limit,input.recovery_increment,&recovered)&&recovered.value736==resource.value736,
            "active push preserves airborne resource history; no grounded refill at a projected plane");
        const auto retained=contact.motion.position;
        check(!gred_surface_contact(profile,ground,before,integrated,{0,0,false},&contact,&error)&&contact.motion.position==retained,
            "same GFMM6 cannot grant active push authority to earlier mode");
    }
    CharacterMovementMotionState under;under.position={-200,595,-200};under.speed44=8;under.vertical48=-10;
    GRedAirDashState entry;entry.selector=16;GRedAirDashPlan plan;
    check(gred_air_dash_plan(profile,entry,under,&plan)&&plan.request&&plan.program==11&&plan.motion.speed44==30&&plan.motion.vertical48==0,
        "active entry remains a separate launch reset, not a continued-frame projection");
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
    ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=595;ground.query_height668=80;ground.step674=30;GRedSurfaceContact contact;
    check(!gred_surface_contact(profile,ground,under,plan.motion,{0,0,false},&contact,&error),"unaccepted below-plane entry cannot borrow continued active push");
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source active surface fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"qualified shared original stage and normal Borg models");
    BattleMvpConfig config;config.arena_half=4500;config.spawn={BattlePoint{295.518066406f,1631.279907227f},BattlePoint{-2850,3000}};
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode,const std::string& pack){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "declared native demo bounds/placements, actual selected floors and same-world source seed");
        check(!source.active_pushes()&&!source.surface_pushes()&&!source.windup_pushes()&&!source.recovery_pushes(),
            "fresh source binding clears committed push counters");
    };
    auto pad_at=[&](unsigned t){GamePadState pad;pad.b=t>=5&&t<40;if(t>=38&&!source.active_pushes())pad.stick_x=127;return pad;};
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact native active/contact receipt before VM/resource/counter/pose commit");
    };
    bind(NativeGRedGroundDashDemo::Mode::WindupSurface,inputs);bool old_refused=false;
    for(unsigned t=0;t<200&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto selector=source.selector();const auto programs=source.movement_programs();
        if(!prepare(pad_at(t),&offer)){
            check(tick==156&&selector==16&&programs==std::array<u16,2>{11,11}&&world.tick()==tick&&!source.active_pushes()&&
                !world.step({},false,&view),"older wind-up mode refuses original active steep contact, without spending or legacy fallback");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"same GFMM6 does not widen older active-contact authority");
    const std::string combined_pack="local/content/source-gred-ground-v9";
    const bool has_combined=files.exists(combined_pack+"/motion.gfmm");
    if(has_combined){
        bind(NativeGRedGroundDashDemo::Mode::GroundWindupPush,combined_pack);
        while(world.tick()<156){NativeBorgMotionOffer offer;check(prepare(pad_at(world.tick()),&offer),"previous combined mode follows the approach");accept(offer);}
        NativeBorgMotionOffer refused;const auto current=pose;const auto meter=source.resource();
        check(!prepare(pad_at(156),&refused)&&world.tick()==156&&!source.active_pushes()&&source.resource()==meter&&
            current.frame_scope.expired()&&!world.motion_receipt_current(current.receipt)&&!world.step({},false,&view),
            "previous combined mode still refuses active contact without committing or falling back");
    }
    for(const bool combined:{false,true}){
    if(combined&&!has_combined)continue;
    const auto mode=combined?NativeGRedGroundDashDemo::Mode::GroundActivePush:NativeGRedGroundDashDemo::Mode::ActiveSurface;
    const auto& pack=combined?combined_pack:inputs;
    bind(mode,pack);u32 active_tick=0,recovery_tick=0,touchdown_tick=0;s16 active_meter=0;
    NativeBorgMotionOffer prior;
    for(unsigned t=0;t<360;++t){
        NativeBorgMotionOffer offer;const auto meter=source.resource();const auto count=source.active_pushes();const auto programs=source.movement_programs();
        const auto stage=source.air_dash_stages();const auto tick=world.tick();const auto retained=pose;
        check(prepare(pad_at(t),&offer)&&source.resource()==meter&&source.active_pushes()==count&&source.movement_programs()==programs&&
            source.air_dash_stages().timer568==stage.timer568,"active spending, readiness, VM and counters stay staged before acceptance");
        if(offer.kind==NativeBorgMotionKind::AirDashActivePush){
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            const auto position=world.actors()[0].position;
            const auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==position&&
                    source.resource()==meter&&source.active_pushes()==count&&source.movement_programs()==programs&&
                    !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            auto edited=offer;++edited.static_push->surface.triangle;
            rejects(edited,"spoofed plane cannot advance world, spend staged meter or revoke the previous accepted frame");
            if(combined){
                check(offer.air_motion&&prior.air_motion&&offer.controller_timer698==0.f,"combined path requires accepted motion and timer records");
                const auto& air=*offer.air_motion;const auto& contact=*offer.static_push;
                check(air.phase==16&&!air.entry&&air.stage_before==2&&air.next_stage==2&&air.next_selector==16&&air.callback&&
                    air.timer_before==prior.air_motion->timer568&&air.timer568==air.timer_before&&air.speed_before==prior.air_motion->speed44&&
                    air.move_speed==air.speed_before&&air.planar_accel==0.f&&air.vertical_accel==profile.air[11]&&
                    air.speed44==f32(air.move_speed*contact.damping)&&offer.vertical_speed==f32(contact.vertical_speed*contact.damping)&&
                    air.resource_before==meter&&air.resource_after==meter-input.costs[3],
                    "continued ready dash retains timer, charges one cost, and damps integrated motion without a launch reset");
                edited=offer;edited.air_motion->entry=true;rejects(edited,"active entry cannot borrow continued authority");
                edited=offer;edited.air_motion->callback=false;rejects(edited,"contact cannot change callback readiness");
                edited=offer;edited.air_motion->timer568-=1;rejects(edited,"contact cannot change the settled timer");
                edited=offer;edited.air_motion->resource_after=0;rejects(edited,"contact cannot fabricate depletion");
                edited=offer;edited.air_motion->next_selector=17;rejects(edited,"contact cannot force early recovery");
                edited=offer;edited.air_motion->speed44=air.move_speed;rejects(edited,"contact must damp retained scalar");
                edited=offer;edited.controller_timer698=1;rejects(edited,"positive698 remains unsupported");
                edited=offer;edited.kind=NativeBorgMotionKind::AirDashWindupEntryPush;rejects(edited,"wind-up entry remains unsupported");
            }
        }
        accept(offer);
        if(offer.kind==NativeBorgMotionKind::AirDashActivePush){
            active_tick=world.tick();active_meter=source.resource();
            check(offer.static_push&&offer.static_push->surface.triangle==1320&&offer.static_push->angle==11582&&
                source.selector()==16&&source.movement_programs()==std::array<u16,2>{11,11}&&source.last_active_push_programs()==std::array<u16,2>{11,11}&&
                source.resource()==meter-input.costs[3]&&source.resource()==1850&&source.air_dash_stages().stage==2&&
                source.air_dash_stages().air_ready588&&source.air_dash_stages().contact_callback&&source.air_dash_active_frames()==111&&
                source.active_pushes()==1&&source.surface_pushes()==1&&!source.landings()&&!world.actors()[0].grounded,
                "original active push keeps program11/readiness and pays exactly50, with no entry reset, landing or refill");
            NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupPush;
            check(!world.motion_receipt(wrong,&receipt),"wind-up receipt cannot replace accepted active physics");
            GamePadState pause;pause.start=true;const auto current=pose;
            check(world.step({pause,{}},false,&view)&&world.paused()&&world.tick()==active_tick&&source.resource()==active_meter&&
                source.pose(world,&pose)&&!current.frame_scope.expired(),"pause freezes active pose, cost and accepted frame");
            check(world.step({},false,&view)&&world.step({pause,{}},false,&view)&&!world.paused()&&world.tick()==active_tick&&
                view.update_source(world,pose,&error),"unpause does not execute a hidden active update");
        }
        if(offer.kind==NativeBorgMotionKind::AirDashRecoveryPush){
            recovery_tick=world.tick();
            check(active_tick&&recovery_tick==active_tick+1&&source.selector()==17&&source.movement_programs()==std::array<u16,2>{12,12}&&
                source.last_active_push_programs()==std::array<u16,2>{11,11}&&source.resource()==active_meter&&source.recovery_pushes()==1&&
                 !source.landings()&&!world.actors()[0].grounded,"neutral direction after ready enters separate recovery, resets acceleration and preserves active receipt history/meter");
            if(combined)check(offer.air_motion&&prior.air_motion&&offer.air_motion->entry&&
                offer.air_motion->speed_before==prior.air_motion->speed44&&offer.air_motion->vertical_before==prior.vertical_speed&&
                offer.air_motion->planar_accel==0.f&&offer.air_motion->vertical_accel==-1.f,
                "separate recovery consumes accepted damped speed and velocity, with its own gravity");
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            touchdown_tick=world.tick();
            check(recovery_tick&&touchdown_tick==recovery_tick+1&&source.selector()==7&&source.resource()==active_meter&&
                 world.actors()[0].grounded,"later real lower-floor touchdown is not an active push or instant refill");
        }
        prior=offer;
    }
    check(active_tick==157&&recovery_tick==158&&touchdown_tick==159&&source.active_pushes()==1&&source.recovery_pushes()==1&&source.surface_pushes()==2&&
        source.windup_pushes()==0&&source.air_dash_starts()==1&&source.landings()==1&&source.selector()==0&&source.resource()==10000&&
        !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
        "bounded original active-contact route recovers, lands and refills through the existing source owner and renderer");
    const auto retained=pose;source.reject();
    check(retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt),"active producer retirement revokes retained pose and receipt");
    bind(mode,pack);bool relaunch=false;
    for(unsigned t=0;t<180&&!relaunch;++t){
        auto pad=pad_at(t);const bool after_push=source.active_pushes()&&source.selector()==16;
        if(after_push)pad.b=true;
        NativeBorgMotionOffer offer;check(prepare(pad,&offer),"real source candidate at active-push/relaunch boundary");accept(offer);
        if(after_push){
            check(offer.kind==NativeBorgMotionKind::RelaunchEntry&&offer.entry_vertical_delta==15&&source.relaunches()==1&&
                source.movement_programs()==std::array<u16,2>{14,14}&&source.active_pushes()==1&&source.resource()==1700,
                "already-supported relaunch input enters a separate additive/request14 contract after active push");
            relaunch=true;
        }
    }
    check(relaunch,"actual source/native relaunch boundary after active contact");source.reject();
    std::printf("%s active surface: active=%u recovery=%u touchdown=%u meter=%d\n",combined?"combined":"legacy",
        active_tick,recovery_tick,touchdown_tick,active_meter);
    }
    check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v5",world,seed,&error,NativeGRedGroundDashDemo::Mode::ActiveSurface)&&world.tick()==0,
        "legacy pack cannot fabricate active steep operands");
    std::printf("source active spend/readiness/contact/depletion/pose: %u checks PASS; native proxies, no whole-frame equivalence\n",checks);
}
