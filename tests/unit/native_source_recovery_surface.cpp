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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source recovery surface: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source recovery surface: optional qualified original pack absent");return 0;}
    std::vector<u8> bytes;CharacterMovementProfile profile;CharacterMovementInputProfile input;
    check(files.read(inputs+"/motion.gfmm",bytes)&&character_movement_load({bytes.data(),bytes.size()},&profile,&error),"qualified unchanged GFMM6");
    check(files.read(inputs+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"qualified unchanged resource/input profile");
    auto triangle=stage_ground_fixture::triangle(0);const f32 length=std::sqrt(10.f);
    for(auto& v:triangle.vertices)v[1]=-3*v[0];
    triangle.plane={3/length,1/length,0,0};
    StihGrid floor,empty;auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto data=stage_ground_fixture::bytes(triangles);check(grid.load({data.data(),data.size()},&error),"owned numeric plane grid");
    };
    load(floor,{triangle});load(empty,{});
    for(bool entered:{false,true})for(bool done:{false,true}){
        GRedAirDashState state;state.selector=17;state.entered=entered;state.stage=entered?1:0;state.contact_callback=entered;state.air_ready588=entered?1:0;
        CharacterMovementMotionState before;before.position={-200,610,-200};before.speed44=10;before.vertical48=-15;before.vertical_change50=entered?-1:-.07f;before.planar_change4c=-.05f;
        GRedAirDashPlan plan;
        check(gred_air_dash_plan(profile,state,before,&plan)&&plan.program==12&&plan.state.contact_callback&&plan.motion.vertical_change50==-1,
            "airborne recovery installs callback and reset acceleration without substituting falling program");
        CharacterMovementHeading neutral;auto motion=plan.motion;
        check(gred_jump_falling_drift(profile,neutral,&motion),"recovery keeps shared .99 scalar damping before source integration");
        CharacterMovementMotionState integrated;CharacterMovementCeiling ceiling;
        check(character_movement_air_step(profile,motion,1,1,true,ceiling,&integrated,&error),"recovery uses clamped source movement, not dash reset");
        CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
        ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=610;ground.query_height668=80;ground.step674=30;
        GRedSurfaceContact contact;GRedAirDashContactTail tail;
        check(gred_surface_contact(profile,ground,before,integrated,{0,0,true},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Pushed&&
            contact.motion.vertical_change50==-1&&contact.motion.planar_change4c==0&&contact.motion.speed44==f32(integrated.speed44*.9f)&&
            contact.motion.vertical48==f32(integrated.vertical48*.9f)&&
            gred_air_dash_contact_tail(17,plan.state.contact_callback,done,false,&tail)&&!tail.landing&&tail.falling==done,
            "recovery contact is projection first, then lower completion: stays12 or enters6, never landing/refill");
        CharacterAirResourceState resource;resource.value736=5000;
        check(character_air_resource_recover(resource,0,0x40,1,input.resource_limit,input.recovery_increment,&resource)&&resource.value736==5000,
            "cached airborne recovery push cannot gain grounded resource refill");
    }
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source recovery surface fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"qualified shared original stage and Borg models");
    BattleMvpConfig config;config.spawn={BattlePoint{44.928558350f,-332.878112793f},BattlePoint{1200,1200}};
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode,const std::string& pack){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "native-only demo placements, fresh same-world source actor/renderer binding");
        check(!source.surface_pushes()&&!source.recovery_pushes()&&!source.dash_fall_pushes()&&!source.relaunch_fall_pushes(),
            "fresh binding clears all committed push counters");
    };
    auto pad_at=[](unsigned t){GamePadState pad;pad.b=t>=5&&t<22;if(t>=20&&t<61)pad.stick_y=127;return pad;};
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact native recovery receipt before source state/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::FallingSurface,inputs);bool old_refused=false;
    for(unsigned t=0;t<180&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto selector=source.selector();const auto programs=source.movement_programs();
        if(!prepare(pad_at(t),&offer)){
            check(tick==72&&selector==17&&programs==std::array<u16,2>{12,12}&&world.tick()==tick&&!source.recovery_pushes()&&
                !world.step({},false,&view),"older falling mode refuses original recovery steep contact without fallback or borrowed authority");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"GFMM6 does not widen older selector6-only contact mode");
    const std::string combined_pack="local/content/source-gred-ground-v9";
    if(files.exists(combined_pack+"/motion.gfmm")){
        bind(NativeGRedGroundDashDemo::Mode::GroundRelaunchFallPush,combined_pack);
        while(world.tick()<72){NativeBorgMotionOffer offer;check(prepare(pad_at(world.tick()),&offer),"combined predecessor follows the same recovery route");accept(offer);}
        NativeBorgMotionOffer refused;const auto retained=pose;
        check(!prepare(pad_at(72),&refused)&&world.tick()==72&&!source.surface_pushes()&&
            retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
            "preceding combined mode still refuses contact inside recovery and cannot fall back");
    }
    for(bool combined:{false,true}){
    if(combined&&!files.exists(combined_pack+"/motion.gfmm")){std::puts("source recovery surface: optional qualified GFMM9 absent");continue;}
    bind(combined?NativeGRedGroundDashDemo::Mode::GroundRecoveryPush:NativeGRedGroundDashDemo::Mode::RecoverySurface,
        combined?combined_pack:inputs);u32 push_tick=0,touchdown_tick=0;s16 push_resource=0;
    NativeBorgMotionOffer prior;
    for(unsigned t=0;t<240;++t){
        NativeBorgMotionOffer offer;const auto resource=source.resource();const auto pushes=source.recovery_pushes();const auto programs=source.movement_programs();
        check(prepare(pad_at(t),&offer)&&source.resource()==resource&&source.recovery_pushes()==pushes&&source.movement_programs()==programs,
             "recovery resource/VM/counter changes remain staged until native acceptance");
        if(combined&&offer.kind==NativeBorgMotionKind::AirDashRecoveryPush){
            const auto& motion=*offer.air_motion;const auto& push=*offer.static_push;
            const auto before=world.actors()[0];const auto retained=pose;
            check(offer.controller_timer698==0.f&&prior.air_motion&&prior.air_motion->phase==17&&
                motion.phase==17&&!motion.entry&&!motion.lower_done&&motion.next_selector==17&&motion.callback&&
                motion.speed_before==prior.air_motion->speed44&&motion.move_speed==f32(motion.speed_before*bits_to_f32(0x3f7d70a4u))&&
                motion.speed44==f32(std::max(0.f,std::fma(motion.planar_accel,1.f,motion.move_speed))*push.damping)&&
                motion.vertical_before==before.vertical_speed&&motion.vertical_accel==-1&&
                push.vertical_speed==std::max(-30.f,f32(before.vertical_speed-1.f))&&
                push.damping==bits_to_f32(0x3f666666u)&&offer.vertical_speed==f32(push.vertical_speed*push.damping)&&
                motion.resource_before==resource&&motion.resource_after==resource,
                "combined recovery proves .99 motion, .9 response, unchanged meter and unfinished lower phase");
            const auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==before.position&&
                    source.resource()==resource&&source.recovery_pushes()==pushes&&source.movement_programs()==programs&&
                    !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            auto wrong=offer;wrong.air_motion.reset();rejects(wrong,"recovery push requires the air-motion record");
            wrong=offer;wrong.static_push.reset();rejects(wrong,"recovery push requires the plane record");
            wrong=offer;wrong.controller_timer698=1;rejects(wrong,"positive698 does not grant recovery push");
            wrong=offer;wrong.air_motion->next_selector=6;rejects(wrong,"contact cannot advance unfinished lower recovery");
            wrong=offer;wrong.air_motion->lower_done=true;wrong.air_motion->next_selector=6;
            rejects(wrong,"a forged done flag cannot promote unfinished captured recovery");
            wrong=offer;wrong.recovery_completion.reset();rejects(wrong,"combined recovery push requires its actual lower-lane proof");
            wrong=offer;wrong.air_motion->resource_after=10000;rejects(wrong,"projection cannot refill in air");
            wrong=offer;wrong.air_motion->speed44=motion.move_speed;rejects(wrong,"projection must damp retained scalar");
            wrong=offer;wrong.air_motion->control=1;rejects(wrong,"grounded recovery control is unsupported");
            wrong=offer;++wrong.static_push->surface.triangle;rejects(wrong,"projection requires the selected triangle");
            wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashActivePush;rejects(wrong,"active dash cannot borrow recovery push");
            wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupPush;rejects(wrong,"wind-up cannot borrow recovery push");
        }
        accept(offer);
        if(offer.kind==NativeBorgMotionKind::AirDashRecoveryPush){
            push_tick=world.tick();push_resource=source.resource();
            check(offer.static_push&&offer.static_push->surface.triangle==1320&&offer.static_push->angle==11582&&
                source.selector()==17&&source.movement_programs()==std::array<u16,2>{12,12}&&source.last_push_programs()==std::array<u16,2>{12,12}&&
                world.actors()[0].action==BattleMvpAction::AirDash&&world.actors()[0].facing==offer.observed.facing&&!world.actors()[0].grounded&&!source.landings()&&
                source.resource()==resource&&resource<10000&&source.recovery_pushes()==1&&source.surface_pushes()==1&&
                !source.dash_fall_pushes()&&!source.relaunch_fall_pushes(),"original recovery push stays program12/selector17, not falling, landing, resource refill or relaunch history");
            auto wrong=offer;wrong.kind=NativeBorgMotionKind::StaticPush;wrong.action=BattleMvpAction::Jump;NativeBorgMotionReceipt receipt;
            check(!world.motion_receipt(wrong,&receipt),"recovery receipt cannot be relabelled as selector6 falling");
            const auto retained=pose;GamePadState pause;pause.start=true;const auto tick=world.tick();
            check(world.step({pause,{}},false,&view)&&world.paused()&&world.tick()==tick&&source.pose(world,&pose)&&source.resource()==resource&&
                source.recovery_pushes()==1&&!retained.frame_scope.expired(),"pause freezes accepted recovery push without hidden VM, motion or resource update");
            check(world.step({},false,&view)&&world.step({pause,{}},false,&view)&&!world.paused()&&world.tick()==tick&&view.update_source(world,pose,&error),
                "unpause preserves exact current recovery pose/receipt until next source tick");
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            touchdown_tick=world.tick();
            check(push_tick&&touchdown_tick==push_tick+1&&source.selector()==7&&world.actors()[0].grounded&&source.resource()==push_resource&&
                 source.recovery_pushes()==1,"next original lower floor gives real touchdown, no instant refill or replayed push");
            if(combined)check(offer.air_motion&&offer.surface_landing&&prior.air_motion&&offer.air_motion->phase==17&&
                !offer.air_motion->lower_done&&offer.air_motion->planar_accel==0.f&&
                offer.air_motion->speed_before==prior.air_motion->speed44&&
                offer.air_motion->vertical_before==prior.vertical_speed,
                "separate touchdown inherits accepted damped motion and cleared planar acceleration");
        }
        prior=offer;
    }
    check(push_tick==73&&touchdown_tick==74&&source.recovery_pushes()==1&&source.surface_pushes()==1&&source.air_dash_starts()==1&&
        source.landings()==1&&source.selector()==0&&source.resource()==10000&&world.actors()[0].health==100&&world.actors()[1].health==100&&
        !world.shots_spawned(),"qualified original recovery route pushes once, then lands/refills through existing source systems");
    const auto retained=pose;source.reject();
    check(retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt),"recovery producer retirement revokes retained source pose and receipt");
    std::printf("%s recovery surface: push tick=%u touchdown=%u resource=%d\n",combined?"combined":"legacy",push_tick,touchdown_tick,push_resource);
    }
    check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
         !source.bind(files,art,"local/content/source-gred-ground-v5",world,seed,&error,NativeGRedGroundDashDemo::Mode::RecoverySurface)&&world.tick()==0,
         "older contact pack cannot fabricate recovery projection operands");
    if(files.exists(combined_pack+"/motion.gfmm")){
        struct Route {f32 x,z;unsigned drift;u32 triangle;s16 angle;};
        const Route right_of_seam{-575,-2990,110,1321,12972};
        auto scheduled=[](unsigned t,const Route& r){
            GamePadState pad;pad.b=t>=5&&t<38;
            if(t>=45&&t<72)pad.stick_y=-127;
            pad.dpad_left=t>=r.drift&&t<112;
            return pad;
        };
        using Kind=NativeBorgMotionKind;
        using Mode=NativeGRedGroundDashDemo::Mode;
        view.source_practice_controls();
        config={};config.arena_half=4500;config.spawn[0]={right_of_seam.x,right_of_seam.z};
        bind(Mode::GroundRelaunchFallPush,combined_pack);
        NativeBorgMotionOffer offer;
        while(world.tick()<110){check(prepare(scheduled(world.tick(),right_of_seam),&offer),"narrower mode reaches the completion approach");accept(offer);}
        check(!prepare(scheduled(110,right_of_seam),&offer)&&world.tick()==110&&source.resource()==5850&&
            !source.recovery_pushes()&&!source.landings()&&!world.step({},false,&view),
            "finishing animation does not let the older mode borrow steep recovery contact");

        for(const Route r:{right_of_seam,Route{-595,-2986,108,1320,11582}}){
            config.spawn[0]={r.x,r.z};bind(Mode::GroundRecoveryPush,combined_pack);
            NativeBorgMotionOffer prior;
            while(world.tick()<180){
                const auto prior_pose=pose;const auto before=world.actors()[0];const auto tick=world.tick()+1;
                check(prepare(scheduled(tick-1,r),&offer),"real grounded approach through completion push and landing");
                if(tick==111){
                    check(offer.kind==Kind::AirDashRecoveryPush&&offer.air_motion&&offer.static_push&&
                        offer.controller_timer698==0&&!offer.recovery_completion.expired()&&prior.kind==Kind::AirDashRecovery,
                        "owned contact proposal and sealed lower recovery completion");
                    const auto& air=*offer.air_motion;const auto& push=*offer.static_push;
                    check(air.phase==17&&!air.entry&&air.lower_done&&air.next_selector==6&&air.callback&&air.drift_heading&&
                        air.resource_before==5850&&air.resource_after==5850&&air.vertical_accel==-1&&
                        push.surface.triangle==r.triangle&&push.angle==r.angle&&push.vertical_speed==-30&&
                        offer.vertical_speed==-27&&air.speed44==f32(air.move_speed*bits_to_f32(0x3f666666u))&&
                        !offer.surface_landing&&offer.facing==before.facing,
                        "projection dampens motion without landing or spending; program 12 can finish after the push");
                    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    auto refuse=[&](const NativeBorgMotionOffer& wrong,const char* why){
                        check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==110&&
                            world.actors()[0].position==before.position&&source.resource()==5850&&!source.recovery_pushes()&&
                            world.motion_receipt_current(prior_pose.receipt)&&!view.practice_readout(world)->recovery_push_complete,why);
                    };
                    auto wrong=offer;wrong.air_motion->lower_done=false;wrong.air_motion->next_selector=17;
                    refuse(wrong,"a finished recovery cannot be falsely kept alive despite identical contact");
                    wrong=offer;wrong.recovery_completion.reset();refuse(wrong,"matching projection without lower-lane proof is insufficient");
                    wrong=offer;++wrong.static_push->surface.triangle;refuse(wrong,"matching completion without selected-plane proof is insufficient");
                }
                if(tick==112)check(offer.kind==Kind::JumpTouchdown&&offer.jump_motion&&offer.jump_motion->entry&&
                    offer.jump_motion->air_dash_origin&&offer.jump_motion->vertical_before==-27&&
                    offer.jump_motion->speed_before==prior.air_motion->speed44&&!offer.air_motion,
                    "the first program 13 update inherits damped recovery momentum and performs the real touchdown");
                accept(offer);
                const auto shown=view.practice_readout(world);
                check(shown&&shown->recovery_push_complete==(tick==111),"only the accepted completing push shows the cue");
                if(tick==111)check(!world.actors()[0].grounded&&source.selector()==6&&source.cached_airborne87()&&
                    !source.landings()&&source.resource()==5850&&source.movement_programs()==std::array<u16,2>{12,12},
                    "the completing push retains the recovery pose and airborne cache; it is not touchdown");
                if(tick==112)check(world.actors()[0].grounded&&source.landings()==1&&source.resource()==5850&&
                    source.movement_programs()==std::array<u16,2>{13,13},"touchdown still uses the falling program");
                if(tick==113)check(offer.kind==Kind::JumpLanding&&source.resource()==5850&&
                    source.movement_programs()==std::array<u16,2>{8,8},"separate landing entry has not yet refilled");
                if(tick==114)check(source.resource()==7850,"refill begins after the accepted landing entry");
                prior=offer;
            }
            check(source.recovery_pushes()==1&&source.surface_pushes()==1&&!source.dash_fall_pushes()&&source.landings()==1&&
                source.resource()==10000&&world.actors()[0].grounded&&!source.selector(),
                "owned completion route finishes idle with one steep push and one landing");
        }
        const Route partial{-585,-2990,110,1320,11582};
        config.spawn[0]={partial.x,partial.z};bind(Mode::GroundRecoveryPush,combined_pack);
        while(world.tick()<111){
            check(prepare(scheduled(world.tick(),partial),&offer),"nearby route reaches an accepted completion push");
            accept(offer);
        }
        check(offer.kind==Kind::AirDashRecoveryPush&&offer.static_push&&offer.static_push->surface.triangle==partial.triangle&&
            view.practice_readout(world)->recovery_push_complete&&source.recovery_pushes()==1,
            "a valid push and completion alone do not prove the next contact");
        const auto last_position=world.actors()[0].position;const auto last_pose=pose;
        check(!prepare(scheduled(111,partial),&offer)&&error.find("raised-surface/side-entry")!=std::string::npos&&
            world.tick()==111&&world.actors()[0].position==last_position&&source.resource()==5850&&
            source.recovery_pushes()==1&&!source.landings()&&last_pose.frame_scope.expired()&&
            !world.step({},false,&view),"the nearby raised-side route still stops before touchdown without fallback");

        const Route fork{-580,-2990,110,1320,11582};
        auto completion_prefix=[&](const Route& r){
            config.spawn[0]={r.x,r.z};bind(Mode::GroundRecoveryPush,combined_pack);
            while(world.tick()<111){
                check(prepare(scheduled(world.tick(),r),&offer),"common deterministic approach to completing push");
                accept(offer);
            }
            check(offer.kind==Kind::AirDashRecoveryPush&&offer.static_push&&offer.static_push->surface.triangle==r.triangle&&
                offer.air_motion&&offer.air_motion->lower_done&&offer.air_motion->next_selector==6&&
                source.resource()==5850&&!source.landings()&&!view.practice_readout(world)->recovery_push_exit_drift,
                "the shared prefix ends in one accepted push, not its follow-up");
        };
        completion_prefix(fork);
        const auto same_push=offer;
        check(!prepare(scheduled(111,fork),&offer)&&error.find("raised-surface/side-entry")!=std::string::npos&&
            world.tick()==111&&world.actors()[0].position==same_push.proposed_position&&source.resource()==5850&&
            !source.landings()&&!world.step({},false,&view),"Left alone still stops the comparison route");

        completion_prefix(fork);
        check(offer.static_push==same_push.static_push&&offer.air_motion==same_push.air_motion&&
            offer.proposed_position==same_push.proposed_position&&offer.vertical_speed==same_push.vertical_speed,
            "changing the following input does not change the accepted push");
        const auto from=world.actors()[0];const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        auto diagonal=scheduled(111,fork);diagonal.dpad_up=true;
        check(prepare(diagonal,&offer)&&offer.kind==Kind::JumpTouchdown&&offer.jump_motion&&offer.surface_landing&&
            offer.jump_motion->entry&&offer.jump_motion->phase==6&&offer.jump_motion->air_dash_origin&&
            offer.jump_motion->speed_before==same_push.air_motion->speed44&&offer.jump_motion->vertical_before==-27&&
            offer.facing==from.facing&&!view.practice_readout(world)->recovery_push_exit_drift,
            "Up+Left offers ordinary falling touchdown with the same prior damping and no staged cue");
        const auto& jump=*offer.jump_motion;
        s16 forward=0;f32 dx=0,dz=0,tx=0,tz=0;
        check(source_heading_s16(frame.forward.x,frame.forward.z,&forward)&&
            jump.drift_heading==source_heading_wrap(s32(forward)+0x2000),"diagonal drift is relative to the camera, not body heading");
        source_heading_xz(*jump.drift_heading,&dx,&dz);source_heading_xz(jump.travel5ae,&tx,&tz);
        check(offer.proposed_position[0]==std::fma(1.f,f32(jump.move_speed*tx),std::fma(4.f,dx,from.position[0]))&&
            offer.proposed_position[2]==std::fma(1.f,f32(jump.move_speed*tz),std::fma(4.f,dz,from.position[2])),
            "the existing four-unit drift runs before retained falling momentum");
        auto wrong=offer;wrong.jump_motion->drift_heading.reset();
        check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==111&&
            world.actors()[0].position==from.position&&source.resource()==5850&&!source.landings()&&
            world.motion_receipt_current(pose.receipt)&&!view.practice_readout(world)->recovery_push_exit_drift,
            "erasing the drift from the offered endpoint cannot publish touchdown or the cue");
        accept(offer);
        check(view.practice_readout(world)->recovery_push_exit_drift&&world.actors()[0].grounded&&source.landings()==1&&
            source.resource()==5850&&source.movement_programs()==std::array<u16,2>{13,13},
            "accepted diagonal touchdown shows the exit cue without an early refill");
        while(world.tick()<180){
            check(prepare({},&offer),"separate landing and idle after the diagonal exit");accept(offer);
            check(!view.practice_readout(world)->recovery_push_exit_drift,"no exit cue carries into landing");
        }
        check(source.resource()==10000&&!source.selector()&&source.recovery_pushes()==1&&source.landings()==1&&
            !source.relaunches()&&source.air_dash_starts()==1,"diagonal exit completes without a new action");

        completion_prefix(partial);
        GamePadState flick;flick.stick_x=-127;
        check(prepare(flick,&offer)&&offer.kind==Kind::AirDashWindupEntry&&offer.proposed_position==world.actors()[0].position,
            "a plain flick entry can be staged after the completing push");
        accept(offer);
        check(!prepare({},&offer)&&world.tick()==112&&source.air_dash_starts()==2&&source.resource()==5850&&
            !source.landings()&&!world.step({},false,&view),
            "one accepted wind-up entry does not bypass the raised travel on the next update");
    }
    std::printf("source recovery surface/VM/completion/resource/pose: %u checks PASS; native proxies, no whole-frame equivalence\n",checks);
}
