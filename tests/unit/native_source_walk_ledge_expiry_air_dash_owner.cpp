#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"

namespace {
unsigned checks=0;
std::string error;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"Expiry air dash: %s (%s)\n",why,error.c_str());std::abort();}
}
class Files final:public gf::IFileSystem {
public:
    bool file_size(const std::string& path,gf::u64* out)override{
        std::error_code e;const auto size=std::filesystem::file_size(path,e);
        if(e||!out)return false;
        *out=size;return true;
    }
    bool read(const std::string& path,std::vector<gf::u8>& out)override{
        gf::u64 size=0;if(!file_size(path,&size)||size>(32u<<20))return false;
        std::ifstream in(path,std::ios::binary);out.assign(std::istreambuf_iterator<char>(in),{});return out.size()==size;
    }
    bool exists(const std::string& path)override{return std::filesystem::exists(path);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}

int main(){
    using namespace gf;
    using Kind=NativeBorgMotionKind;
    using Mode=NativeGRedGroundDashDemo::Mode;
    Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("Expiry air dash: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"Expiry air dash"},nullptr,&error)&&view.load(files,art,render,&error),
        "qualified stage and model assets");
    view.source_practice_controls();
    auto bind=[&](Mode mode){
        BattleMvpConfig cfg;cfg.arena_half=4500;cfg.spawn[0]={-1250.96863f,-3313.80225f};cfg.spawn_query_y[0]=1846.01367f;
        check(world.start(cfg,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"fresh grounded placement on the original stage");
    };
    auto prepare=[&](bool flick,NativeBorgMotionOffer* offer,bool lift=false,bool up=false){
        GamePadState pad;
        if(world.tick()>=5&&!source.ground_departures())pad.dpad_left=true;
        if(flick)pad.stick_x=-127;
        pad.b=lift;pad.dpad_up=up;
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeGRedGroundInput input;
        return source.map_input(pad,&input,&error)&&source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"native receipt precedes source and renderer publication");
    };
    NativeBorgMotionOffer offer;
    auto prefix=[&](unsigned tick){
        while(world.tick()<tick){check(prepare(false,&offer),"accepted walking ledge prefix");accept(offer);}
    };
    bind(Mode::GroundWalkLedgeExpiryPush);prefix(56);
    check(offer.kind==Kind::GroundLossPush&&offer.walk_ledge->walk_origin&&offer.walk_ledge->timer698==0&&
        prepare(true,&offer)&&offer.kind==Kind::AirDashWindupEntry,"older mode can stage the candidate but lacks the handoff");
    const auto old_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&old_frame,&offer)&&world.tick()==56&&
        source.resource()==10000&&!source.air_dash_starts()&&world.motion_receipt_current(pose.receipt),
        "older mode publishes no motion, timer, spend or pose on the refused handoff");
    source.reject();check(!world.step({},false,&view),"older refusal does not fall back");

    bind(Mode::GroundWalkLedgeExpiryAirDash);prefix(55);
    check(offer.kind==Kind::GroundLossPush&&source.ground_loss_timer698()==1&&
        !prepare(true,&offer)&&error.find("accepted zero-timer expiry push")!=std::string::npos&&world.tick()==55&&
        source.ground_loss_timer698()==1&&!source.air_dash_starts()&&source.resource()==10000&&!view.practice_readout(world),
        "the next decrement cannot substitute for a previously accepted expiry push");

    bind(Mode::GroundWalkLedgeDirectTimedAirDash);prefix(55);
    check(offer.kind==Kind::GroundLossPush&&source.ground_loss_timer698()==1&&
        !prepare(true,&offer)&&error.find("still-positive walking-push countdown")!=std::string::npos&&world.tick()==55&&
        source.ground_loss_timer698()==1&&!source.air_dash_starts()&&source.resource()==10000,
        "direct timed mode cannot borrow a pending timer expiry either");

    bind(Mode::GroundWalkLedgeExpiryAirDash);prefix(56);
    check(offer.kind==Kind::GroundLossPush&&offer.walk_ledge->walk_origin&&offer.walk_ledge->timer698==0,"exact expiry predecessor");
    unsigned entry=0,handoff=0,active=0,fall=0,touchdown=0,landing=0;
    while(world.tick()<300){
        const auto tick=world.tick()+1;const auto before=world.actors()[0];
        check(prepare(tick==57,&offer),"single left flick and complete post-expiry air-dash route");
        if(native_borg_motion_windup_entry(offer.kind)){
            entry=tick;const auto& a=*offer.air_motion;
            check(tick==57&&offer.kind==Kind::AirDashWindupEntry&&a.entry&&a.phase==15&&offer.controller_timer698==0&&
                offer.proposed_position==before.position&&a.vertical_before==f32(before.vertical_speed*.5f)&&
                a.resource_before==10000&&a.resource_after==10000&&!offer.air_dash_floor&&!offer.static_push,
                "plain entry halves velocity without displacement, contact, timer reset or cost");
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            auto rejects=[&](const NativeBorgMotionOffer& wrong,const char* why){
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==56&&
                    world.actors()[0].position==before.position&&source.resource()==10000&&!source.air_dash_starts()&&
                    world.motion_receipt_current(pose.receipt),why);
            };
            auto wrong=offer;wrong.controller_timer698=1;rejects(wrong,"timer substitution");
            wrong=offer;wrong.proposed_position[0]+=1;rejects(wrong,"pre-contact displacement on entry");
            wrong=offer;wrong.air_motion->resource_after-=450;rejects(wrong,"early handoff cost");
            wrong=offer;wrong.kind=Kind::AirDashWindupEntryFloor;wrong.air_dash_floor=NativeBorgAirDashFloor{};
            rejects(wrong,"contact response cannot borrow plain expiry entry");
        }
        if(offer.air_motion&&offer.air_motion->phase==15&&offer.air_motion->next_selector==16){
            handoff=tick;
            check(tick==62&&offer.air_motion->resource_before==10000&&offer.air_motion->resource_after==9550,
                "handoff pays 450 on its own update");
        }
        if(native_borg_motion_active_entry(offer.kind)){
            active=tick;
            check(tick==63&&offer.air_motion->resource_before==9550&&offer.air_motion->resource_after==9500,
                "separate active entry pays 50, not the wind-up cost again");
        }
        if(offer.kind==Kind::JumpAir&&!fall){fall=tick;check(source.resource()==8000,"recovered active costs are retained on release");}
        if(offer.kind==Kind::JumpTouchdown)touchdown=tick;
        if(touchdown&&!landing&&offer.kind==Kind::JumpLanding)landing=tick;
        accept(offer);
    }
    check(entry==57&&handoff==62&&active==63&&fall==123&&touchdown==150&&landing==151&&
        source.walk_ledge_pushes()==2&&source.air_dash_starts()==1&&!source.relaunches()&&
        source.resource()==10000&&world.actors()[0].grounded&&source.selector()==0,"complete accepted route returns to idle and refills normally");

    auto expiry_windup_prefix=[&](Mode mode){
        bind(mode);prefix(56);
        const bool scoped=mode==Mode::GroundExpiryPlainWindupRelaunch;
        while(world.tick()<62){
            const unsigned tick=world.tick();
            const auto before=world.actors()[0];
            check(prepare(tick==56,&offer)&&offer.air_motion&&offer.air_motion->phase==15&&
                offer.air_motion->walk_expiry_windup==scoped&&!offer.air_dash_floor&&!offer.static_push,
                "only the new uninterrupted plain wind-up carries expiry lineage");
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            auto wrong=offer;wrong.air_motion->walk_expiry_windup=!scoped;
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&
                world.actors()[0].position==before.position&&source.resource()==10000&&world.motion_receipt_current(pose.receipt)&&
                !view.practice_readout(world)->plain_windup_relaunch_ready,
                "expiry history cannot be fabricated in the older mode or erased from a scoped frame");
            accept(offer);
            check(view.practice_readout(world)->plain_windup_relaunch_ready==(scoped&&world.tick()==62),
                "the prompt requires the accepted, completed expiry wind-up and the matching policy");
        }
        check(offer.kind==Kind::AirDashWindup&&offer.air_motion->next_selector==16&&offer.air_motion->next_stage==0&&
            !offer.air_motion->timer568&&offer.controller_timer698==0&&source.resource()==9550&&
            !source.air_dash_active_frames(),"plain expiry wind-up completed and paid its own charge");
    };
    expiry_windup_prefix(Mode::GroundTimedPlainWindupRelaunch);
    check(!prepare(false,&offer,true)&&error.find("still-positive ledge countdown")!=std::string::npos&&
        world.tick()==62&&source.resource()==9550&&!source.relaunches(),
        "positive-timer handoff mode still refuses the completed zero-timer wind-up");

    bind(Mode::GroundExpiryPlainWindupRelaunch);prefix(56);
    check(!view.practice_readout(world)->direct_expiry_relaunch_ready,"older modes do not promise a direct entry");
    check(prepare(false,&offer,true,true)&&offer.kind==Kind::RelaunchEntry,
        "older mode stages the same soft-Up candidate but cannot authorize it");
    const auto direct_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&direct_frame,&offer)&&world.motion_receipt_current(pose.receipt)&&
        !view.practice_readout(world)->direct_expiry_relaunch,"older policy still rejects the verified direct relaunch");
    source.reject();
    check(world.tick()==56&&source.resource()==10000&&!source.relaunches()&&!world.step({},false,&view),
        "expiry relaunch needs a completed wind-up and does not fall back");

    for(const bool held:{false,true}){
        expiry_windup_prefix(Mode::GroundExpiryPlainWindupRelaunch);
        const auto before=world.actors()[0];const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(prepare(false,&offer,true,held)&&offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&
            offer.controller_timer698==0&&offer.entry_vertical_delta==15&&
            offer.relaunch_motion->vertical_before==f32(before.vertical_speed+15.f)&&offer.relaunch_motion->vertical_before>0&&
            offer.relaunch_motion->resource_before==9550&&offer.relaunch_motion->resource_after==9400&&
            !offer.static_push&&!offer.relaunch_floor&&!offer.air_motion,"qualified expiry history admits plain rising relaunch");
        auto refuses=[&](const NativeBorgMotionOffer& wrong,const char* why){
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==62&&world.actors()[0].position==before.position&&
                source.resource()==9550&&!source.relaunches()&&world.motion_receipt_current(pose.receipt),why);
        };
        auto wrong=offer;wrong.controller_timer698=1;refuses(wrong,"expiry handoff cannot invent a positive timer");
        wrong=offer;wrong.relaunch_motion->resource_before=10000;refuses(wrong,"expiry handoff cannot refund the wind-up");
        wrong=offer;wrong.relaunch_motion->resource_after-=50;refuses(wrong,"expiry handoff cannot borrow the active-entry charge");
        wrong=offer;wrong.kind=Kind::RelaunchEntryFloor;wrong.relaunch_floor=NativeBorgRelaunchFloor{};
        refuses(wrong,"expiry lineage does not authorize a relaunch floor response at entry");
        accept(offer);
        check(source.movement_programs()==std::array<u16,2>{14,14}&&source.resource()==9400&&source.relaunches()==1&&
            !source.air_dash_active_frames()&&!view.practice_readout(world)->plain_windup_relaunch_ready,
            "accepted handoff runs relaunch, not active dash, and clears the prompt");
        unsigned active_tick=0,fall_tick=0,hit=0,land=0;
        while(world.tick()<210){
            const unsigned tick=world.tick()+1;
            const bool rise=held&&tick<=74;
            check(prepare(false,&offer,rise,rise),"expiry handoff settles through ordinary fall and landing");
            if(!active_tick&&native_borg_motion_relaunch_active(offer.kind))active_tick=tick;
            if(!fall_tick&&offer.kind==Kind::JumpAir)fall_tick=tick;
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            if(tick==fall_tick||tick==hit||tick==land)check(source.resource()==(held?8300:9400),"release and landing do not refill early");
        }
        check((held?(active_tick==68&&fall_tick==75&&hit==146):(!active_tick&&fall_tick==64&&hit==126))&&
            land==hit+1&&source.landings()==1&&source.resource()==10000&&world.actors()[0].grounded&&!source.selector()&&
            source.air_dash_starts()==1&&!source.air_dash_active_frames()&&source.relaunches()==1,
            "tap and held expiry handoffs both reach idle without active dash");
    }
    bind(Mode::GroundWalkExpiryDirectRelaunch);prefix(55);
    check(!view.practice_readout(world)->direct_expiry_relaunch_ready,"a timer of one does not publish the next-input hint");
    check(offer.kind==Kind::GroundLossPush&&source.ground_loss_timer698()==1.f&&
        !prepare(false,&offer,true,true)&&world.tick()==55&&error.find("accepted zero-timer walking push")!=std::string::npos&&
        source.ground_loss_timer698()==1.f&&source.resource()==10000&&!source.relaunches()&&!world.step({},false,&view),
        "one-tick-early Shift cannot substitute a pending decrement for the accepted expiry push");
    bind(Mode::GroundWalkExpiryDirectRelaunch);prefix(56);
    check(view.practice_readout(world)->direct_expiry_relaunch_ready,"only the accepted expiry push offers Up+Shift");
    check(!prepare(false,&offer,true)&&world.tick()==56&&error.find("needs soft Up")!=std::string::npos&&
        source.resource()==10000&&!source.relaunches()&&!world.step({},false,&view),
        "neutral direct relaunch is not the verified directional route");

    bind(Mode::GroundWalkExpiryDirectRelaunch);prefix(56);
    check(view.practice_readout(world)->direct_expiry_relaunch_ready&&prepare(false,&offer)&&offer.kind==Kind::GroundLossAir&&
        view.practice_readout(world)->direct_expiry_relaunch_ready,"neutral input stages an ordinary fall but cannot replace the accepted hint yet");
    accept(offer);
    check(!view.practice_readout(world)->direct_expiry_relaunch_ready&&!view.practice_readout(world)->direct_expiry_relaunch,
        "an ordinary zero-timer fall explicitly closes the direct window");

    for(const bool held:{false,true}){
        bind(Mode::GroundWalkExpiryDirectRelaunch);prefix(56);
        const auto previous=offer;const auto before=world.actors()[0];const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(previous.kind==Kind::GroundLossPush&&previous.walk_ledge&&previous.walk_ledge->walk_origin&&
            previous.walk_ledge->timer698==0.f&&previous.static_push&&previous.static_push->surface.triangle==1999&&
            previous.controller_timer698==0.f&&!before.grounded&&before.vertical_speed<0.f&&
            prepare(false,&offer,true,true)&&offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&
            offer.controller_timer698==0.f&&offer.entry_vertical_delta==15.f&&
            offer.relaunch_motion->vertical_before==f32(before.vertical_speed+15.f)&&offer.vertical_speed<0.f&&
            offer.relaunch_motion->speed_before==previous.jump_motion->speed44&&
            offer.relaunch_motion->resource_before==10000&&offer.relaunch_motion->resource_after==9850&&
            !offer.static_push&&!offer.relaunch_floor&&!offer.air_motion,
            "direct soft-Up entry starts from the accepted projected speed and spends just 150 while still descending");
        auto refuses=[&](const NativeBorgMotionOffer& wrong,const NativeBorgMovementFrame* camera,const char* why){
            check(!world.step({},false,&view,nullptr,camera,&wrong)&&world.tick()==56&&world.actors()[0].position==before.position&&
                source.resource()==10000&&!source.relaunches()&&world.motion_receipt_current(pose.receipt)&&
                !view.practice_readout(world)->direct_expiry_relaunch&&view.practice_readout(world)->direct_expiry_relaunch_ready,why);
        };
        refuses(offer,nullptr,"native requires a current camera-frame turn proof");
        NativeBorgMovementFrame backwards;
        check(native_borg_movement_frame({-frame.forward.x,-frame.forward.z},&backwards),"valid opposite camera frame");
        refuses(offer,&backwards,"a different valid camera does not prove the accepted Up turn");
        auto wrong=offer;wrong.facing=before.facing;refuses(wrong,&frame,"entry cannot keep a fabricated unturned body");
        wrong=offer;wrong.controller_timer698=1;refuses(wrong,&frame,"direct expiry cannot invent a positive countdown");
        wrong=offer;wrong.relaunch_motion->resource_before=9550;refuses(wrong,&frame,"direct relaunch did not pay a wind-up cost");
        wrong=offer;wrong.kind=Kind::RelaunchEntryFloor;wrong.relaunch_floor=NativeBorgRelaunchFloor{};
        refuses(wrong,&frame,"direct expiry cannot authorize a floor response on entry");
        wrong=offer;wrong.kind=Kind::RelaunchEntryPush;wrong.static_push=previous.static_push;
        refuses(wrong,&frame,"direct expiry cannot borrow an old steep response");
        accept(offer);
        check(!world.actors()[0].grounded&&source.movement_programs()==std::array<u16,2>{14,14}&&source.resource()==9850&&
            source.relaunches()==1&&view.practice_readout(world)->direct_expiry_relaunch&&!view.practice_readout(world)->direct_expiry_relaunch_ready,
            "matching acceptance publishes relaunch and the cue, not touchdown or wind-up");
        unsigned active_tick=0,fall_tick=0,hit=0,land=0,lift_tick=0;
        while(world.tick()<170){
            const unsigned tick=world.tick()+1;
            const bool rise=held&&tick<=76;
            const auto y=world.actors()[0].position[1];
            check(prepare(false,&offer,rise,rise),"tap or held direct relaunch through ordinary fall and landing");
            if(!active_tick&&native_borg_motion_relaunch_active(offer.kind))active_tick=tick;
            if(!lift_tick&&rise&&offer.proposed_position[1]>y)lift_tick=tick;
            if(!fall_tick&&offer.kind==Kind::JumpAir)fall_tick=tick;
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            check(!view.practice_readout(world)->direct_expiry_relaunch&&source.ground_loss_timer698()==0.f,
                "entry cue does not persist, and the timer does not restart");
            if(tick==fall_tick||tick==hit||tick==land)check(source.resource()==(held?7950:9850),
                "release, touchdown and landing retain the spent meter");
        }
        check((held?(active_tick==62&&lift_tick==76&&fall_tick==77&&hit==136):(!active_tick&&!lift_tick&&fall_tick==58&&hit==110))&&
            land==hit+1&&source.landings()==1&&source.walk_ledge_pushes()==2&&!source.air_dash_starts()&&!source.relaunch_floor_contacts()&&
            source.resource()==10000&&world.actors()[0].grounded&&!source.selector(),
            "both complete routes use no paid dash or further contact response and return to idle");
    }
    auto timed_prefix=[&](Mode mode){
        BattleMvpConfig cfg;cfg.arena_half=4500;cfg.spawn[0]={-4066.18164f,-3400.83032f};cfg.spawn_query_y[0]=1846.01367f;
        check(world.start(cfg,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"declared grounded timed push fixture");
        while(world.tick()<24){
            const auto tick=world.tick()+1;
            check(prepare(false,&offer,false,tick>=6&&tick<=7),"walk off the original rim, then fall without new input");
            if(tick>=22)check(offer.kind==Kind::GroundLossPush&&offer.walk_ledge&&offer.walk_ledge->walk_origin&&
                offer.controller_timer698==f32(38-tick)&&offer.static_push&&offer.static_push->surface.triangle==(tick==24?2011u:2021u)&&
                offer.static_push->angle==(tick==24?13653:8192)&&offer.vertical_speed==f32(offer.static_push->vertical_speed*.9f),
                "three verified descending walking pushes retain actual selected faces, damping and timer");
            accept(offer);
        }
    };
    timed_prefix(Mode::GroundExpiringSnapWindupRelaunch);
    {
        const auto retained=pose;const auto before=world.actors()[0].position;
        check(!prepare(false,&offer,true)&&error.find("accepted zero-timer walking push")!=std::string::npos&&world.tick()==24&&
            source.ground_loss_timer698()==14.f&&source.resource()==10000&&world.actors()[0].position==before&&
            !source.relaunches()&&retained.frame_scope.expired()&&!world.step({},false,&view),
            "previous policies still refuse this same direct press before any timer, meter or fallback changes");
    }
    timed_prefix(Mode::GroundWalkTimedDirectRelaunch);
    check(!prepare(false,&offer,true,true)&&error.find("plain neutral lift")!=std::string::npos&&world.tick()==24&&
        source.ground_loss_timer698()==14.f&&source.resource()==10000&&!source.relaunches()&&!world.step({},false,&view),
        "directional lift does not borrow the new neutral entry permission");
    timed_prefix(Mode::GroundWalkTimedDirectUpRelaunch);
    {
        GamePadState diagonal;diagonal.b=diagonal.dpad_up=diagonal.dpad_right=true;
        NativeGRedGroundInput input;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(source.map_input(diagonal,&input,&error)&&
            !source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,&offer,&error)&&
            error.find("plain neutral or soft-Up lift")!=std::string::npos&&world.tick()==24&&
            source.resource()==10000&&source.ground_loss_timer698()==14.f&&!world.step({},false,&view),
            "diagonal entry does not borrow a cardinal Up proof");
    }
    bind(Mode::GroundWalkTimedDirectUpRelaunch);prefix(55);
    check(offer.kind==Kind::GroundLossPush&&source.ground_loss_timer698()==1.f&&
        !prepare(false,&offer,true,true)&&error.find("still-positive countdown")!=std::string::npos&&world.tick()==55&&
        source.resource()==10000&&!source.relaunches()&&!world.step({},false,&view),
        "pending expiry still cannot stand in for positive or previously expired walking-push history");

    for(bool up:{false,true})for(unsigned hold:{1u,8u}){
        timed_prefix(up?Mode::GroundWalkTimedDirectUpRelaunch:Mode::GroundWalkTimedDirectRelaunch);
        const auto prior=offer;const auto before=world.actors()[0];const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(prepare(false,&offer,true,up)&&offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&
            offer.controller_timer698==13.f&&!before.grounded&&before.vertical_speed<0.f&&
            offer.relaunch_motion->vertical_before==f32(before.vertical_speed+15.f)&&offer.relaunch_motion->vertical_before>0.f&&
            offer.proposed_position[1]>before.position[1]&&(offer.facing!=before.facing)==up&&offer.relaunch_motion->direction==up&&
            offer.relaunch_motion->speed_before==prior.jump_motion->speed44&&offer.relaunch_motion->speed_before<prior.jump_motion->move_speed&&
            offer.relaunch_motion->resource_before==10000&&offer.relaunch_motion->resource_after==9850&&
            !offer.relaunch_floor&&!offer.static_push&&!offer.air_motion,
            "plain upward entry uses the projected speed rather than the previous pre-response scalar and keeps 13");
        const auto& lift=*offer.relaunch_motion;
        f32 sine=0,cosine=0;source_heading_xz(8192,&sine,&cosine);
        check(up?(native_gred_relaunch_up_turn(frame,before.facing,lift.travel5ae,offer.facing)&&lift.move_speed==lift.speed_before&&
                lift.planar_accel==f32(.8f*sine)&&lift.vertical_accel==std::fma(.8f,cosine,0.f)&&lift.speed44>lift.move_speed):
                (lift.move_speed==f32(lift.speed_before*.98f)&&!lift.planar_accel&&lift.vertical_accel==.8f),
            "soft Up uses the exact turn and directional acceleration rather than neutral planar damping");
        auto refuses=[&](const NativeBorgMotionOffer& wrong,const NativeBorgMovementFrame* camera,const char* why){
            check(!world.step({},false,&view,nullptr,camera,&wrong)&&world.tick()==24&&world.actors()[0].position==before.position&&
                world.motion_receipt_current(pose.receipt)&&source.ground_loss_timer698()==14.f&&source.resource()==10000&&
                !view.practice_readout(world)->direct_timed_relaunch,why);
        };
        auto wrong=offer;wrong.controller_timer698=0.f;refuses(wrong,&frame,"new entry cannot borrow an expiry timer");
        wrong=offer;wrong.relaunch_motion->speed_before=prior.jump_motion->move_speed;refuses(wrong,&frame,"pre-projection speed cannot replace accepted speed");
        wrong=offer;wrong.relaunch_motion->direction=!up;refuses(wrong,&frame,"entry cannot change the exact acceleration branch");
        wrong=offer;wrong.facing={-before.facing.x,-before.facing.z};refuses(wrong,&frame,"no camera-directed body reset");
        if(up){
            NativeBorgMovementFrame backwards;
            check(native_borg_movement_frame({-frame.forward.x,-frame.forward.z},&backwards),"valid opposite camera frame");
            refuses(offer,nullptr,"soft Up needs a current camera-relative turn proof");
            refuses(offer,&backwards,"a different valid camera cannot certify the actual Up turn");
        }
        wrong=offer;wrong.kind=Kind::RelaunchEntryFloor;wrong.relaunch_floor=NativeBorgRelaunchFloor{};
        refuses(wrong,&frame,"timed lift is not permission for an entry floor response");
        accept(offer);
        check(view.practice_readout(world)->direct_timed_relaunch&&!view.practice_readout(world)->direct_expiry_relaunch&&
            view.practice_readout(world)->direct_timed_relaunch_up==up&&
            source.resource()==9850&&source.ground_loss_timer698()==13.f&&!world.actors()[0].grounded,
            "the timed cue is published only by the accepted plain lift");
        unsigned active_tick=0,fall_tick=0,hit=0,land=0;
        while(world.tick()<150){
            const unsigned tick=world.tick()+1;const auto timer=source.ground_loss_timer698();
            const bool rising=tick<25+hold;
            check(prepare(false,&offer,rising,rising&&up)&&offer.controller_timer698==std::max(0.f,timer-1.f),
                "actual positive countdown continues across relaunch and release");
            if(up&&hold==8&&tick==29)check(offer.relaunch_motion&&offer.relaunch_motion->move_speed<8.f&&
                offer.relaunch_motion->speed44==8.f,"held soft Up crosses the existing directional cap at tick 29");
            if(!active_tick&&native_borg_motion_relaunch_active(offer.kind))active_tick=tick;
            if(!fall_tick&&offer.kind==Kind::JumpAir)fall_tick=tick;
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            check(!view.practice_readout(world)->direct_timed_relaunch&&!view.practice_readout(world)->direct_timed_relaunch_up,
                "no stale direct entry cue in continuation");
            if(tick==fall_tick||tick==hit||tick==land)check(source.resource()==(hold==1?9850:9150),
                "release and landing retain spent meter until ordinary grounded recovery");
        }
        check(active_tick==(hold==1?0u:30u)&&fall_tick==25+hold&&hit==(hold==1?101u:up?114u:117u)&&land==hit+1&&
            source.walk_ledge_pushes()==3&&source.relaunches()==1&&!source.air_dash_starts()&&!source.relaunch_floor_contacts()&&
            source.resource()==10000&&world.actors()[0].grounded&&!source.selector(),
            "timed direct tap and hold both reach distinct touchdown/landing and full-meter idle");
    }
    auto expiring_prefix=[&](Mode mode){
        BattleMvpConfig cfg;cfg.arena_half=4500;cfg.spawn[0]={-1080.47473f,-3357.9165f};cfg.spawn_query_y[0]=1846.01367f;
        check(world.start(cfg,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"fresh grounded start for the expiring plain entry");
        prefix(37);
        check(offer.kind==Kind::GroundLossPush&&offer.walk_ledge->walk_origin&&offer.walk_ledge->timer698==1.f&&
            offer.static_push&&offer.static_push->surface.triangle==2009&&offer.static_push->angle==8192&&
            offer.vertical_speed==-27.f&&source.resource()==10000,"real integrated timer-one walking push, not an expired sample");
    };
    expiring_prefix(Mode::GroundTimedWalkReleaseSecondPush);
    check(!view.practice_readout(world)->direct_expiring_relaunch_ready&&!prepare(false,&offer,true,true)&&
        error.find("still-positive countdown")!=std::string::npos&&source.ground_loss_timer698()==1.f&&
        world.tick()==37&&source.resource()==10000&&!world.step({},false,&view),"older mode refuses the same prefix and requested transition");
    expiring_prefix(Mode::GroundWalkExpiringDirectRelaunch);
    check(!prepare(false,&offer,true)&&error.find("needs soft Up")!=std::string::npos&&world.tick()==37&&
        source.ground_loss_timer698()==1.f&&source.resource()==10000&&!world.step({},false,&view),
        "neutral Shift alone does not qualify");
    bind(Mode::GroundWalkExpiringDirectRelaunch);prefix(55);
    check(!prepare(false,&offer,true,true)&&error.find("plain descending entry without contact response")!=std::string::npos&&
        world.tick()==55&&source.ground_loss_timer698()==1.f&&source.resource()==10000&&!world.step({},false,&view),
        "a real nearby steep entry response is not admitted by the plain expiry-step permission");
    expiring_prefix(Mode::GroundWalkExpiringDirectRelaunch);
    const auto prior=offer;const auto before=world.actors()[0];const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(view.practice_readout(world)->direct_expiring_relaunch_ready&&!view.practice_readout(world)->direct_expiry_relaunch_ready&&
        prepare(false,&offer,true,true)&&offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&offer.controller_timer698==0.f&&
        source.ground_loss_timer698()==1.f&&source.resource()==10000&&!source.relaunches(),
        "timer and entry cost do not publish when staging Up+Shift");
    const auto& e=*offer.relaunch_motion;
    check(e.entry&&e.direction&&native_gred_relaunch_up_turn(frame,before.facing,e.travel5ae,offer.facing)&&
        offer.entry_vertical_delta==15.f&&e.vertical_before==-12.f&&e.speed_before==prior.jump_motion->speed44&&
        e.move_speed==e.speed_before&&offer.proposed_position[1]==f32(before.position[1]-12.f)&&
        offer.vertical_speed==std::fma(e.vertical_accel,1.f,-12.f)&&e.resource_before==10000&&e.resource_after==9850&&
        !offer.air_motion&&!offer.static_push&&!offer.relaunch_floor&&!offer.surface_landing,
        "plain descending entry adds 15 before integration, turns from the accepted facing and pays 150");
    auto refuses_expiring=[&](const NativeBorgMotionOffer& wrong,const NativeBorgMovementFrame* camera,const char* why){
        const bool refused=!world.step({},false,&view,nullptr,camera,&wrong);
        const auto shown=view.practice_readout(world);
        check(refused&&world.tick()==37&&world.actors()[0].position==before.position&&
            world.motion_receipt_current(pose.receipt)&&source.ground_loss_timer698()==1.f&&source.resource()==10000&&
            shown&&shown->direct_expiring_relaunch_ready&&!shown->direct_expiring_relaunch,why);
    };
    refuses_expiring(offer,nullptr,"native needs the qualified camera-relative Up turn");
    NativeBorgMovementFrame backwards;
    check(native_borg_movement_frame({-frame.forward.x,-frame.forward.z},&backwards),"opposite valid camera");
    refuses_expiring(offer,&backwards,"another valid camera cannot prove this turn");
    auto wrong=offer;wrong.controller_timer698=1.f;refuses_expiring(wrong,&frame,"cannot keep the old clock");
    wrong=offer;wrong.controller_timer698.reset();refuses_expiring(wrong,&frame,"no implicit zero clock");
    wrong=offer;wrong.entry_vertical_delta=0.f;refuses_expiring(wrong,&frame,"must add 15 before moving");
    wrong=offer;wrong.proposed_position[0]+=1.f;refuses_expiring(wrong,&frame,"native independently recomputes the endpoint");
    wrong=offer;wrong.relaunch_motion->resource_after=10000;refuses_expiring(wrong,&frame,"cannot erase the entry cost");
    wrong=offer;wrong.kind=Kind::RelaunchEntryPush;wrong.static_push=prior.static_push;
    refuses_expiring(wrong,&frame,"projection cannot borrow the plain entry gate");
    wrong=offer;wrong.kind=Kind::RelaunchEntryFloor;wrong.relaunch_floor=NativeBorgRelaunchFloor{};
    refuses_expiring(wrong,&frame,"snap cannot borrow the plain entry gate");
    accept(offer);
    check(view.practice_readout(world)->direct_expiring_relaunch&&!view.practice_readout(world)->direct_expiring_relaunch_ready&&
        !view.practice_readout(world)->direct_expiry_relaunch&&source.ground_loss_timer698()==0.f&&source.resource()==9850&&
        !world.actors()[0].grounded&&source.movement_programs()==std::array<u16,2>{14,14},
        "accepted relaunch publishes only its 1 to 0 cue, with a descending airborne frame");
    unsigned touched=0,landed=0;
    while(world.tick()<125){
        const auto tick=world.tick()+1;
        check(prepare(false,&offer)&&offer.controller_timer698==0.f,"ordinary release and landing keep the expired clock");
        if(tick==39)check(offer.kind==Kind::JumpAir&&offer.jump_motion&&offer.jump_motion->entry&&offer.jump_motion->relaunch_origin,
            "a one-update tap releases into ordinary falling");
        if(offer.kind==Kind::JumpTouchdown)touched=tick;
        if(touched&&!landed&&offer.kind==Kind::JumpLanding)landed=tick;
        accept(offer);
        check(!view.practice_readout(world)->direct_expiring_relaunch&&!view.practice_readout(world)->direct_expiring_relaunch_ready,
            "no stale expiry-step cue while falling or landing");
        if(tick==39||tick==touched||tick==landed)check(source.resource()==9850,"release and landing preserve the charge");
    }
    check(touched==91&&landed==92&&world.actors()[0].grounded&&!source.selector()&&source.resource()==10000&&
        source.walk_ledge_pushes()==1&&source.relaunches()==1&&!source.relaunch_surface_pushes()&&!source.relaunch_floor_contacts()&&
        !source.air_dash_starts(),"one plain entry, one walking push, separate landing, then full idle");
    std::printf("Walking push air followups: %u checks PASS, timed and expiry entries with separate costs and landing\n",checks);
}
