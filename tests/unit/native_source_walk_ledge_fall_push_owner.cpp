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
    if(!ok){std::fprintf(stderr,"walk ledge-fall push: %s (%s)\n",why,error.c_str());std::abort();}
}
class Files final:public gf::IFileSystem {
public:
    bool change_contact_operand=false;
    bool file_size(const std::string& path,gf::u64* out)override{
        std::error_code e;const auto size=std::filesystem::file_size(path,e);
        if(e||!out)return false;
        *out=size;return true;
    }
    bool read(const std::string& path,std::vector<gf::u8>& out)override{
        gf::u64 size=0;if(!file_size(path,&size)||size>(32u<<20))return false;
        std::ifstream in(path,std::ios::binary);out.assign(std::istreambuf_iterator<char>(in),{});
        if(change_contact_operand&&path.ends_with("/motion.gfmm")&&out.size()==376)out[349]^=1;
        return out.size()==size;
    }
    bool exists(const std::string& path)override{return std::filesystem::exists(path);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
// Preserve the selected plane. Reject just one of the two independent
// native travel requests, so passing plane math alone cannot grant travel.
class RaisedTravel final:public gf::IBattleMvpTerrain {
    const gf::NativeBattleMvpView& view_;
    gf::BattleMvpVec endpoint_;
public:
    RaisedTravel(const gf::NativeBattleMvpView& view,gf::BattleMvpVec endpoint):view_(view),endpoint_(endpoint){}
    bool height(gf::f32 x,gf::f32 z,gf::f32 current_y,gf::f32* out)const override{
        if(out&&x==endpoint_[0]&&z==endpoint_[2]){*out=current_y+1;return true;}
        return view_.height(x,z,current_y,out);
    }
    bool static_surface(gf::f32 x,gf::f32 z,gf::f32 current_y,gf::NativeBorgStaticSurface* out)const override{
        return view_.static_surface(x,z,current_y,out);
    }
};
}

int main(){
    using namespace gf;
    using Kind=NativeBorgMotionKind;
    using Mode=NativeGRedGroundDashDemo::Mode;
    Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("walk ledge-fall push: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"Walk ledge-fall push"},nullptr,&error)&&view.load(files,art,render,&error),"qualified stage and models");
    view.source_practice_controls();
    struct Route {f32 x,z,reference;u8 direction;};
    const Route inter_gate{1000.65021f,-3362.34546f,1549.25293f,8};
    const Route complete{1000.65021f,-3363.34546f,1549.25293f,8};
    const Route expires{-1250.96863f,-3313.80225f,1846.01367f,4};
    const Route raised{-1502.66699f,-97.3330078f,94.339f,10};
    auto bind=[&](const Route& r,Mode mode){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={r.x,r.z};config.spawn_query_y[0]=r.reference;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"fresh grounded original-stage fixture");
    };
    auto prepare=[&](const Route& r,NativeBorgMotionOffer* offer,GamePadState pad={}){
        if(world.tick()>=5&&!source.ground_departures()){
            pad.dpad_up=r.direction&1u;pad.dpad_down=r.direction&2u;pad.dpad_left=r.direction&4u;pad.dpad_right=r.direction&8u;
        }
        NativeGRedGroundInput input;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&input,&error)&&source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact native acceptance publishes pose/timer and resource");
    };
    NativeBorgMotionOffer offer;
    bind(inter_gate,Mode::GroundInclinedDashLedge);
    while(world.tick()<22){check(prepare(inter_gate,&offer),"previous-policy approach");accept(offer);}
    check(!prepare(inter_gate,&offer)&&error.find("steep push")!=std::string::npos&&world.tick()==22&&pose.frame_scope.expired()&&
        !world.step({},false,&view),"the old walking/ordered-dash policy still refuses and never falls back");

    bind(inter_gate,Mode::GroundWalkLedgeFallPush);
    while(world.tick()<25){
        const auto tick=world.tick();const auto before=world.actors()[0];
        check(prepare(inter_gate,&offer),"real walk/fall and three inter-gate push offers");
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        auto rejects=[&](const NativeBorgMotionOffer& wrong,const IBattleMvpTerrain& terrain,const char* why){
            const auto shown=view.practice_readout(world);const auto pushes=source.walk_ledge_pushes();
            check(!world.step({},false,&terrain,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==before.position&&
                world.motion_receipt_current(pose.receipt)&&!pose.frame_scope.expired()&&source.walk_ledge_pushes()==pushes&&
                source.resource()==10000&&shown&&view.practice_readout(world)->ledge_timer_ticks==shown->ledge_timer_ticks,why);
        };
        if(offer.kind==Kind::GroundDeparture){
            check(tick==7&&offer.walk_ledge->walk_origin&&!offer.ordered_ground,"walking departure seals its own lineage");
            auto wrong=offer;wrong.walk_ledge->walk_origin=false;rejects(wrong,view,"cannot strip walk identity at departure");
        }
        if(offer.kind==Kind::GroundLossPush){
            const auto& push=*offer.static_push;
            check(tick>=22&&tick<=24&&offer.walk_ledge->timer698==f32(38-tick)&&offer.walk_ledge->walk_origin&&
                !offer.walk_ledge->drift_heading&&push.angle==8191&&push.surface.triangle==1953&&!offer.surface_landing&&
                offer.vertical_speed==f32(push.vertical_speed*bits_to_f32(0x3f666666u))&&offer.jump_motion->next_selector==6,
                "positive owned timer, selected inter-gate face, damped continued fall, no callback");
            auto wrong=offer;wrong.walk_ledge->walk_origin=false;rejects(wrong,view,"cannot borrow non-walking lineage");
            wrong=offer;wrong.walk_ledge->timer698=0;wrong.controller_timer698=0;rejects(wrong,view,"matching forged timer expiry refuses");
            wrong=offer;wrong.static_push->surface.triangle++;rejects(wrong,view,"height coincidence is not a selected-plane receipt");
            wrong=offer;wrong.static_push->angle=6144;rejects(wrong,view,"landing gate cannot be relabelled as a push");
            wrong=offer;wrong.jump_motion->resource_before--;rejects(wrong,view,"cannot spend meter or invent a refill on a push");
            wrong=offer;wrong.jump_motion->move_speed+=1;rejects(wrong,view,"must integrate undamped .99 approach before .9 response");
            RaisedTravel pre(view,push.integrated_position),post(view,offer.proposed_position);
            rejects(offer,pre,"projection cannot skip blocked pre-contact travel");
            rejects(offer,post,"accepted approach cannot skip blocked post-projection travel");
        }
        accept(offer);
        check(source.resource()==10000&&!source.air_dash_starts()&&!source.relaunches()&&!source.ordered_root().updates,
            "walk-fall contact has no spend, root or borrowed air-dash/relaunch");
    }
    check(source.walk_ledge_pushes()==3&&source.movement_programs()==std::array<u16,2>{13,13}&&
        !world.actors()[0].grounded&&!source.landings()&&source.ground_loss_timer698()==14,
        "accepted projection stays in program13 with its real countdown");
    check(!prepare(inter_gate,&offer)&&error.find("inter-gate angle range")!=std::string::npos&&world.tick()==25&&
        source.walk_ledge_pushes()==3&&source.ground_loss_timer698()==14&&!view.practice_readout(world)&&
        !world.step({},false,&view),"next out-of-range face still refuses without a fallback or displayed candidate");

    bind(inter_gate,Mode::GroundWalkLedgeHighAnglePush);
    unsigned high_pushes=0,high_touchdown=0,high_landing=0;
    while(world.tick()<120){
        const auto tick=world.tick()+1;const auto previous=world.actors()[0].position;
        check(prepare(inter_gate,&offer),"replay the formerly stopped walk fall and recover");
        if(offer.kind==Kind::GroundLossPush&&offer.static_push->angle>10240){
            ++high_pushes;
            const auto& push=*offer.static_push;NativeBorgStaticSurface selected;
            check((tick==26||tick==27)&&push.angle==13653&&push.surface.triangle==1949&&
                view.static_surface(push.integrated_position[0],push.integrated_position[2],previous[1],&selected)&&
                selected==push.surface&&selected.height<=previous[1]&&offer.proposed_position[1]>push.integrated_position[1]&&
                offer.walk_ledge->timer698==f32(39-tick)&&offer.walk_ledge->walk_origin&&!offer.surface_landing&&
                offer.vertical_speed==f32(push.vertical_speed*bits_to_f32(0x3f666666u)),
                "steep face must have a matching descending approach and keep the positive walking timer");
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            const RaisedTravel pre(view,push.integrated_position),post(view,offer.proposed_position);
            const auto shown=view.practice_readout(world);
            check(!world.step({},false,&pre,nullptr,&frame,&offer)&&!world.step({},false,&post,nullptr,&frame,&offer)&&
                world.tick()+1==tick&&world.actors()[0].position==previous&&world.motion_receipt_current(pose.receipt)&&
                shown&&shown->ledge_timer_ticks==u8(40-tick)&&
                view.practice_readout(world)->ledge_timer_ticks==shown->ledge_timer_ticks&&source.resource()==10000,
                "steeper authorization cannot use the projection to bypass either native travel test or publish early");
        }
        if(offer.kind==Kind::GroundLossTouchdown){
            high_touchdown=tick;
            check(offer.walk_ledge->timer698==0&&offer.proposed_position[1]==10,"later ordinary flat touchdown after timer expiry");
        }
        if(high_touchdown&&!high_landing&&offer.kind==Kind::JumpLanding)high_landing=tick;
        accept(offer);
        check(source.resource()==10000&&!source.ordered_root().updates&&!source.air_dash_starts()&&!source.relaunches(),
            "steeper walk-fall has no borrowed resource, root, dash, or relaunch");
    }
    check(high_pushes==2&&source.walk_ledge_pushes()==5&&source.walk_departures()==1&&!source.dash_departures()&&
        high_touchdown==80&&high_landing==81&&world.actors()[0].grounded&&!source.selector(),
        "explicit high-angle policy adds two pushes and reaches the separate landing and idle");

    bind(complete,Mode::GroundWalkLedgeFallPush);
    unsigned departure=0,fall=0,touchdown=0,landing=0;
    while(world.tick()<120){
        const auto tick=world.tick()+1;
        check(prepare(complete,&offer),"complete qualified push-and-landing route");
        if(offer.kind==Kind::GroundDeparture)departure=tick;
        if(offer.kind==Kind::GroundLossEntry)fall=tick;
        if(offer.kind==Kind::GroundLossPush)check((tick==24||tick==25)&&offer.static_push->surface.triangle==1953&&
            offer.walk_ledge->timer698==f32(39-tick)&&!offer.surface_landing,"two scoped pushes on accepted ticks 24/25");
        if(offer.kind==Kind::GroundLossTouchdown)touchdown=tick;
        if(touchdown&&!landing&&offer.kind==Kind::JumpLanding)landing=tick;
        accept(offer);
        check(source.resource()==10000&&!source.ordered_root().updates,"real route retains its meter and never runs a dash root");
        if(tick==landing)check(source.movement_programs()==std::array<u16,2>{8,8},"landing8 remains separate from touchdown");
    }
    check(departure==8&&fall==9&&touchdown==77&&landing==78&&source.walk_ledge_pushes()==2&&
        source.walk_departures()==1&&!source.dash_departures()&&source.landings()==1&&
        world.actors()[0].grounded&&world.actors()[0].position[1]==10&&!source.selector(),
        "original stage completes both pushes and the independent touchdown/landing/idle sequence");

    for(const bool use_dash:{false,true}){
        bind(complete,Mode::GroundWalkLedgeFallPush);
        while(world.tick()<25){check(prepare(complete,&offer),"qualified positive-push prefix");accept(offer);}
        check(offer.kind==Kind::GroundLossPush&&source.ground_loss_timer698()==14,"last push still owns a positive timer");
        GamePadState input;if(use_dash)input.stick_y=-127;else input.b=true;
        const auto position=world.actors()[0].position;
        if(prepare(complete,&offer,input)){
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&frame,&offer)&&world.motion_receipt_current(pose.receipt),
                "a prepared action cannot immediately follow the positive push");
            source.reject();
        }
        check(world.tick()==25&&world.actors()[0].position==position&&source.ground_loss_timer698()==14&&
            !source.air_dash_starts()&&!source.relaunches()&&source.resource()==10000&&!world.step({},false,&view),
            "direct action leaves the positive push unchanged and does not fall back");
    }

    NativeBorgMotionOffer shared_fall;
    for(const bool use_dash:{false,true}){
        bind(complete,Mode::GroundWalkLedgeFallPush);
        NativeBorgMotionOffer prior;
        unsigned action_entry=0,active_entry=0,ordinary_fall=0,action_touchdown=0,action_landing=0;
        while(world.tick()<180){
            const unsigned tick=world.tick()+1;const auto before=world.actors()[0];
            GamePadState pad;
            if(use_dash)pad.stick_y=tick==30?-127:0;
            else pad.b=tick>=30&&tick<=34;
            check(prepare(complete,&offer,pad),"timed action travels and lands under the existing walking-push policy");
            if(tick>=26&&tick<=29){
                check(offer.kind==Kind::GroundLossAir&&offer.walk_ledge&&offer.walk_ledge->walk_origin&&
                    offer.walk_ledge->timer698==f32(39-tick)&&!offer.static_push&&!offer.walk_ledge->drift_heading,
                    "four accepted ordinary falls, each with its own positive timer, follow the last push");
                if(tick==29){
                    if(use_dash)check(offer.proposed_position==shared_fall.proposed_position&&
                        offer.vertical_speed==shared_fall.vertical_speed&&offer.walk_ledge==shared_fall.walk_ledge&&
                        offer.jump_motion==shared_fall.jump_motion,"both actions start from exactly the same accepted falling state");
                    else shared_fall=offer;
                }
            }
            if(tick==30){
                check(prior.kind==Kind::GroundLossAir&&source.ground_loss_timer698()==10&&offer.controller_timer698==9,
                    "entry uses the next decrement of the already accepted falling timer");
                if(use_dash)check(offer.kind==Kind::AirDashWindupEntry&&offer.air_motion&&
                    offer.air_motion->vertical_before==f32(before.vertical_speed*.5f)&&offer.proposed_position==before.position&&
                    offer.air_motion->resource_before==10000&&offer.air_motion->resource_after==10000,
                    "timed wind-up halves descending speed before any motion or charge");
                else check(offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&
                    offer.relaunch_motion->vertical_before==f32(before.vertical_speed+15.f)&&offer.relaunch_motion->vertical_before<0&&
                    offer.relaunch_motion->resource_before==10000&&offer.relaunch_motion->resource_after==9850,
                    "timed relaunch applies the entry lift once and spends 150 without a timer reset");
                ++action_entry;
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                auto wrong=offer;wrong.controller_timer698=0;
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==29&&
                    world.actors()[0].position==before.position&&source.resource()==10000&&
                    source.ground_loss_timer698()==10&&world.motion_receipt_current(pose.receipt)&&
                    view.practice_readout(world)->ledge_timer_ticks==10,
                    "forged expiry cannot advance the motion, timer, meter or pose");
            }
            if(use_dash&&tick==35)check(offer.air_motion&&offer.air_motion->next_selector==16&&
                offer.controller_timer698==4&&offer.air_motion->resource_before==10000&&offer.air_motion->resource_after==9550,
                "wind-up handoff costs 450 while the timer is positive");
            if(native_borg_motion_active_entry(offer.kind)){
                active_entry=tick;
                check(use_dash&&tick==36&&offer.controller_timer698==3&&offer.air_motion->resource_after==9500,
                    "separate active dash entry spends 50 and retains the countdown");
            }
            if(tick==39)check(offer.controller_timer698==0&&offer.kind==(use_dash?Kind::AirDashActive:Kind::JumpAir),
                "ordinary timer expiry does not interrupt the running action or the fall");
            if(tick>30&&offer.kind==Kind::JumpAir&&!ordinary_fall)ordinary_fall=tick;
            if(offer.kind==Kind::JumpTouchdown)action_touchdown=tick;
            if(action_touchdown&&!action_landing&&offer.kind==Kind::JumpLanding)action_landing=tick;
            accept(offer);
            if(tick==30)check(view.practice_readout(world)->ledge_timer_ticks==9&&source.resource()==(use_dash?10000:9850),
                "the retained timer and cost are visible only after the accepted action entry");
            if(tick==ordinary_fall||tick==action_touchdown||tick==action_landing)
                check(source.resource()==(use_dash?8000:9450),"release, touchdown and landing entry do not refill");
            prior=offer;
        }
        check(action_entry==1&&source.walk_ledge_pushes()==2&&source.landings()==1&&source.resource()==10000&&
            world.actors()[0].grounded&&!source.selector()&&
            (use_dash?(active_entry==36&&ordinary_fall==96&&action_touchdown==122&&action_landing==123&&
                source.air_dash_starts()==1&&!source.relaunches()):
                (!active_entry&&ordinary_fall==35&&action_touchdown==96&&action_landing==97&&
                !source.air_dash_starts()&&source.relaunches()==1)),
            "both timed follow-ups finish a real landing and reach full-meter idle");
    }

    auto high_push_prefix=[&](Mode mode){
        bind(inter_gate,mode);
        while(world.tick()<27){check(prepare(inter_gate,&offer),"five positive-timer walking pushes");accept(offer);}
        check(offer.kind==Kind::GroundLossPush&&offer.walk_ledge->walk_origin&&offer.walk_ledge->timer698==12&&
            source.walk_ledge_pushes()==5&&source.resource()==10000,"the direct entry predecessor is an accepted walking push");
    };
    GamePadState diagonal;diagonal.stick_x=diagonal.stick_y=-127;
    high_push_prefix(Mode::GroundWalkLedgeHighAnglePush);
    check(prepare(inter_gate,&offer,diagonal)&&offer.kind==Kind::AirDashWindupEntry,"older mode can stage the genuine diagonal candidate");
    const auto old_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&old_frame,&offer)&&world.tick()==27&&
        world.motion_receipt_current(pose.receipt)&&source.resource()==10000&&!source.air_dash_starts(),
        "older high-angle mode still refuses immediate positive-timer dash");
    source.reject();
    check(!world.step({},false,&view),"there is no fallback on an older direct-timed refusal");

    high_push_prefix(Mode::GroundWalkLedgeExpiryAirDash);
    check(!prepare(inter_gate,&offer,diagonal)&&error.find("zero-timer expiry push")!=std::string::npos&&world.tick()==27&&
        source.ground_loss_timer698()==12&&source.resource()==10000,"expiry opt-in still refuses positive timer entry");

    high_push_prefix(Mode::GroundWalkLedgeDirectTimedAirDash);
    GamePadState lift;lift.b=true;
    if(prepare(inter_gate,&offer,lift)){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(!world.step({},false,&view,nullptr,&frame,&offer),"direct timed air dash does not authorize relaunch");
        source.reject();
    }
    check(world.tick()==27&&source.ground_loss_timer698()==12&&!source.relaunches()&&source.resource()==10000,
        "direct relaunch still cannot publish against the positive push");

    for(const bool high:{true,false}){
        const auto& route=high?inter_gate:complete;
        const unsigned start=high?28:26,active_tick=high?35:34;
        const unsigned before_timer=high?12:14;
        bind(route,Mode::GroundWalkLedgeDirectTimedAirDash);
        unsigned entry=0,active=0,fall_tick=0,hit=0,land=0;
        NativeBorgMotionOffer prior;
        while(world.tick()<200){
            const auto tick=world.tick()+1;const auto before=world.actors()[0];
            GamePadState pad;
            if(tick==start){
                if(high)pad=diagonal;else pad.stick_x=127;
            }
            if(!high&&tick>active_tick&&tick<=active_tick+18)pad.dpad_up=true;
            check(prepare(route,&offer,pad),"complete direct-timed air dash on the real stage");
            if(tick==start){
                check(prior.kind==Kind::GroundLossPush&&prior.walk_ledge->walk_origin&&prior.walk_ledge->timer698==f32(before_timer)&&
                    offer.kind==Kind::AirDashWindupEntry&&offer.air_motion&&offer.air_motion->entry&&
                    offer.controller_timer698==f32(before_timer-1)&&!offer.static_push&&!offer.air_dash_floor&&
                    offer.proposed_position==before.position&&offer.facing==before.facing&&
                    offer.air_motion->vertical_before==f32(before.vertical_speed*.5f)&&
                    offer.air_motion->resource_before==10000&&offer.air_motion->resource_after==10000,
                    "direct entry keeps the countdown and starts stationary wind-up without cost or contact");
                ++entry;
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                auto refuse=[&](const NativeBorgMotionOffer& wrong,const char* why){
                    check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==start-1&&
                        world.actors()[0].position==before.position&&source.resource()==10000&&
                        source.ground_loss_timer698()==f32(before_timer)&&world.motion_receipt_current(pose.receipt)&&
                        view.practice_readout(world)->ledge_timer_ticks==before_timer,why);
                };
                auto wrong=offer;wrong.controller_timer698=0;refuse(wrong,"fabricated expiry cannot borrow the known route");
                wrong=offer;wrong.controller_timer698=f32(before_timer);refuse(wrong,"entry cannot pause the positive countdown");
                wrong=offer;wrong.proposed_position[0]+=1;refuse(wrong,"direct entry has no pre-motion displacement");
                wrong=offer;wrong.air_motion->resource_after=9550;refuse(wrong,"direct entry cannot charge handoff early");
                wrong=offer;wrong.kind=Kind::AirDashWindupEntryFloor;wrong.air_dash_floor=NativeBorgAirDashFloor{};
                refuse(wrong,"a floor response cannot borrow the new plain-entry lineage");
            }
            if(tick==active_tick-1)check(offer.air_motion&&offer.air_motion->phase==15&&offer.air_motion->next_selector==16&&
                offer.controller_timer698==f32(40-active_tick)&&offer.air_motion->resource_after==9550,
                "wind-up handoff spends 450 while timer698 is still positive");
            if(native_borg_motion_active_entry(offer.kind)){
                active=tick;
                check(tick==active_tick&&offer.controller_timer698==f32(39-active_tick)&&
                    offer.air_motion->resource_before==9550&&offer.air_motion->resource_after==9500,
                    "active entry spends 50 on a separate positive-countdown tick");
            }
            if(tick==39)check(offer.controller_timer698==0&&native_borg_motion_active_update(offer.kind),
                "positive countdown expires in the running dash, not on the push or wind-up");
            if(entry&&offer.kind==Kind::JumpAir&&!fall_tick)fall_tick=tick;
            if(entry&&offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            if(tick==start)check(view.practice_readout(world)->ledge_timer_ticks==before_timer-1&&source.resource()==10000,
                "HUD only reports the decrement after the entry is accepted");
            if(tick==fall_tick||tick==hit||tick==land)check(source.resource()==8000,"recovery, touchdown and landing entry preserve the dash costs");
            prior=offer;
        }
        check(entry==1&&active==active_tick&&fall_tick==(high?95:94)&&hit==(high?123:122)&&land==hit+1&&source.landings()==1&&
            source.walk_ledge_pushes()==(high?5u:2u)&&source.air_dash_starts()==1&&!source.relaunches()&&
            source.resource()==10000&&world.actors()[0].grounded&&!source.selector(),
            "diagonal route and separately steered inter-gate route both complete through landing and idle");
    }

    bind(inter_gate,Mode::GroundWalkLedgeDirectTimedAirDash);
    while(world.tick()<34){
        GamePadState pad;if(world.tick()==27)pad.stick_y=-127;
        check(prepare(inter_gate,&offer,pad),"W alone shares the push and enters the dash, but not the complete route");
        accept(offer);
    }
    check(native_borg_motion_active_entry(offer.kind)&&source.ground_loss_timer698()==5&&source.resource()==9500,
        "the cardinal attempt genuinely reached active entry");
    const auto cardinal_position=world.actors()[0].position;
    check(!prepare(inter_gate,&offer)&&error.find("raised-surface/side-entry")!=std::string::npos&&world.tick()==34&&
        world.actors()[0].position==cardinal_position&&source.ground_loss_timer698()==5&&source.resource()==9500&&
        !source.landings()&&!world.step({},false,&view),
        "plain entry does not let the next active update cross a raised face or spend another 50");

    const Route dash{2582.89014f,-2846.36206f,116.592773f,0};
    bind(dash,Mode::GroundWalkLedgeFallPush);
    while(world.tick()<7){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        GamePadState pad;if(world.tick()==5)pad.stick_x=127;
        NativeGRedGroundInput input;
        check(source.map_input(pad,&input,&error)&&
            source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,&offer,&error),"scoped mode still accepts the previously verified dash ledge");
        if(world.tick()==5||world.tick()==6){
            check(offer.kind==(world.tick()==5?Kind::GroundDeparture:Kind::GroundLossEntry)&&
                offer.walk_ledge&&!offer.walk_ledge->walk_origin,"a real dash departure cannot seed walking provenance");
            auto wrong=offer;wrong.walk_ledge->walk_origin=true;
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.motion_receipt_current(pose.receipt),
                "forged walking origin cannot be published on either dash-loss frame");
        }
        accept(offer);
    }

    bind(expires,Mode::GroundWalkLedgeHighAnglePush);
    while(world.tick()<55){check(prepare(expires,&offer),"approach and one push on the final timer tick");accept(offer);}
    check(source.walk_ledge_pushes()==1&&source.ground_loss_timer698()==1&&
        !prepare(expires,&offer)&&world.tick()==55&&source.ground_loss_timer698()==1&&source.walk_ledge_pushes()==1,
        "the high-angle policy still refuses the next zero-timer push without publishing that decrement");

    bind(expires,Mode::GroundWalkLedgeExpiryPush);
    departure=fall=touchdown=landing=0;unsigned boundary_push=0;
    std::optional<NativeBorgStaticPush> expiry_packet;
    while(world.tick()<180){
        const auto tick=world.tick()+1;const auto before=world.actors()[0];
        check(prepare(expires,&offer),"complete walking route across the natural timer expiry");
        if(offer.kind==Kind::GroundDeparture)departure=tick;
        if(offer.kind==Kind::GroundLossEntry)fall=tick;
        if(offer.kind==Kind::GroundLossPush){
            const auto& push=*offer.static_push;
            check((tick==55&&offer.walk_ledge->timer698==1&&push.surface.triangle==2009&&push.angle==8192)||
                (tick==56&&offer.walk_ledge->timer698==0&&push.surface.triangle==1999&&push.angle==13653),
                "the positive-timer and expiry pushes use their own actual selected plane and gate");
            if(tick==56){
                ++boundary_push;expiry_packet=push;
                check(source.ground_loss_timer698()==1&&source.walk_ledge_pushes()==1&&offer.walk_ledge->walk_origin&&
                    !offer.walk_ledge->drift_heading&&!offer.surface_landing,
                    "only the immediately previous accepted timer-one walking push licenses the staged zero");
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                auto rejects=[&](const NativeBorgMotionOffer& wrong,const IBattleMvpTerrain& terrain,const char* why){
                    const auto shown=view.practice_readout(world);
                    check(!world.step({},false,&terrain,nullptr,&frame,&wrong)&&world.tick()==55&&
                        world.actors()[0].position==before.position&&world.motion_receipt_current(pose.receipt)&&
                        source.ground_loss_timer698()==1&&source.walk_ledge_pushes()==1&&source.resource()==10000&&
                        shown&&shown->ledge_timer_ticks==1&&view.practice_readout(world)->ledge_timer_ticks==1,why);
                };
                const RaisedTravel pre(view,push.integrated_position),post(view,offer.proposed_position);
                rejects(offer,pre,"expiry does not allow a raised approach");
                rejects(offer,post,"expiry does not allow a blocked projection");
                auto wrong=offer;wrong.walk_ledge->drift_heading=0;wrong.jump_motion->drift_heading=0;
                rejects(wrong,view,"zero-timer steering cannot be folded into the witnessed expiry push");
                wrong=offer;wrong.controller_timer698=1;wrong.walk_ledge->timer698=1;
                rejects(wrong,view,"cannot keep the positive timer/angle gate for one more update");
            }
        }
        if(tick==57){
            check(offer.kind==Kind::GroundLossAir&&offer.walk_ledge->timer698==0&&expiry_packet,"ordinary zero-timer fall follows the boundary push");
            auto wrong=offer;wrong.kind=Kind::GroundLossPush;wrong.static_push=expiry_packet;
            wrong.walk_ledge->speed44=wrong.jump_motion->speed44=f32(wrong.jump_motion->move_speed*bits_to_f32(0x3f666666u));
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==56&&world.motion_receipt_current(pose.receipt),
                "a zero-timer continued fall cannot replay the 1-to-0 permission");
        }
        if(offer.kind==Kind::GroundLossTouchdown)touchdown=tick;
        if(touchdown&&!landing&&offer.kind==Kind::JumpLanding)landing=tick;
        accept(offer);
        check(source.resource()==10000&&!source.ordered_root().updates&&!source.air_dash_starts()&&!source.relaunches(),
            "expiry transition keeps the same meter and does not borrow a movement handler");
        if(tick==56||tick==57){
            const auto shown=view.practice_readout(world);
            check(shown&&shown->ledge_timer_ticks==0&&NativeBattleMvpView::practice_ledge_cue(*shown)==
                (tick==56?"LEDGE TIMER 0 TICKS":"DRIFT UNLOCKED"),"neutral expiry push and unlocked fall report distinct cues");
        }
    }
    check(departure==25&&fall==26&&boundary_push==1&&source.walk_ledge_pushes()==2&&touchdown==103&&landing==104&&
        source.walk_departures()==1&&!source.dash_departures()&&world.actors()[0].grounded&&
        world.actors()[0].position[1]==10&&source.selector()==0,
        "real timer expiry route lands independently and reaches idle with no meter spend");
    const auto neutral=world.actors()[0].position;

    bind(expires,Mode::GroundWalkLedgeExpiryPush);
    unsigned drifted=0;touchdown=landing=0;
    while(world.tick()<180){
        const auto tick=world.tick()+1;const auto before=world.actors()[0];
        GamePadState pad;pad.dpad_up=tick>=57&&tick<=92;
        check(prepare(expires,&offer,pad),"complete expiry route with live directional drift");
        if(pad.dpad_up){
            const auto& ledge=*offer.walk_ledge;const auto& jump=*offer.jump_motion;
            check(offer.kind==Kind::GroundLossAir&&ledge.timer698==0&&ledge.drift_heading&&
                jump.drift_heading==ledge.drift_heading&&offer.facing==before.facing&&!offer.static_push,
                "zero-timer drift changes travel without turning the body or pushing");
            f32 dx=0,dz=0,tx=0,tz=0;
            source_heading_xz(*ledge.drift_heading,&dx,&dz);source_heading_xz(ledge.travel5ae,&tx,&tz);
            check(offer.proposed_position[0]==std::fma(1.f,f32(ledge.speed44*tx),std::fma(4.f,dx,before.position[0]))&&
                offer.proposed_position[2]==std::fma(1.f,f32(ledge.speed44*tz),std::fma(4.f,dz,before.position[2])),
                "actual four-unit direct drift precedes the exact retained momentum");
            if(!drifted){
                auto wrong=offer;wrong.walk_ledge->drift_heading.reset();wrong.jump_motion->drift_heading.reset();
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                const auto prior=view.practice_readout(world);
                check(prior&&!prior->ledge_drift_active&&!world.step({},false,&view,nullptr,&frame,&wrong)&&
                    world.tick()==56&&world.motion_receipt_current(pose.receipt)&&!view.practice_readout(world)->ledge_drift_active,
                    "staging or erasing the first drift proof cannot publish a drift cue");
            }
            ++drifted;
        }
        if(offer.kind==Kind::GroundLossTouchdown)touchdown=tick;
        if(touchdown&&!landing&&offer.kind==Kind::JumpLanding)landing=tick;
        accept(offer);
        const auto shown=view.practice_readout(world);
        check(shown&&shown->ledge_drift_active==pad.dpad_up&&source.resource()==10000,"only committed drift is visible; meter stays unchanged");
        if(pad.dpad_up)check(NativeBattleMvpView::practice_ledge_cue(*shown)=="DRIFT ACTIVE","steering cue");
    }
    check(drifted==36&&touchdown==103&&landing==104&&world.actors()[0].position[2]>neutral[2]+140&&
        source.walk_ledge_pushes()==2&&world.actors()[0].grounded&&source.selector()==0,
        "drift shifts the landing on the original stage and recovers normally without another push");

    bind(expires,Mode::GroundWalkLedgeExpiryAirDash);
    while(world.tick()<56){check(prepare(expires,&offer),"accepted prefix before the immediate expiry relaunch refusal");accept(offer);}
    const auto before_relaunch=world.actors()[0].position;
    GamePadState relaunch;relaunch.b=true;
    check(offer.kind==Kind::GroundLossPush&&prepare(expires,&offer,relaunch)&&offer.kind==Kind::RelaunchEntry&&
        offer.relaunch_motion&&offer.relaunch_motion->resource_after==9850,"expiry push can stage a candidate but not launch");
    const auto expiry_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&expiry_frame,&offer)&&world.tick()==56&&world.actors()[0].position==before_relaunch&&
        source.resource()==10000&&!source.relaunches()&&world.motion_receipt_current(pose.receipt)&&
        view.practice_readout(world)->kind==Kind::GroundLossPush,"expiry air-dash permission does not admit relaunch or publish its cost/pose");
    source.reject();check(!world.step({},false,&view),"unsupported expiry launch does not fall back");

    for(const unsigned held_ticks:{1u,20u}){
        bind(expires,Mode::GroundWalkLedgeExpiryPush);
        unsigned entries=0,active_tick=0;fall=touchdown=landing=0;
        const s16 release_meter=held_ticks==1?9850:7950;
        while(world.tick()<200){
            const auto tick=world.tick()+1;const auto before=world.actors()[0];
            check(prepare(expires,&offer,tick>=58&&tick<58+held_ticks?relaunch:GamePadState{}),
                "accepted expiry route with a one-frame neutral gap before relaunch");
            if(tick==57)check(offer.kind==Kind::GroundLossAir&&offer.walk_ledge->timer698==0&&
                !offer.static_push&&!offer.walk_ledge->drift_heading&&source.resource()==10000,
                "neutral fall, not a push or a spend, supplies the relaunch predecessor");
            if(native_borg_motion_relaunch_entry(offer.kind)){
                ++entries;const auto& e=*offer.relaunch_motion;
                check(tick==58&&offer.kind==Kind::RelaunchEntry&&offer.controller_timer698==0&&offer.entry_vertical_delta==15&&
                    e.vertical_before==f32(before.vertical_speed+15.f)&&e.vertical_before<0&&
                    offer.proposed_position[1]==std::fma(1.f,e.vertical_before,before.position[1])&&
                    offer.vertical_speed>e.vertical_before&&offer.facing==before.facing&&e.lower_program==14&&e.upper_program==14&&
                    e.resource_before==10000&&e.resource_after==9850&&!offer.static_push&&!offer.relaunch_floor,
                    "one lift slows descent before acceleration, with two program14 requests and no floor response");
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                auto rejects=[&](const NativeBorgMotionOffer& wrong,const char* why){
                    check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==57&&
                        world.actors()[0].position==before.position&&source.resource()==10000&&!source.relaunches()&&
                        world.motion_receipt_current(pose.receipt)&&view.practice_readout(world)->kind==Kind::GroundLossAir,why);
                };
                auto wrong=offer;wrong.entry_vertical_delta=0;rejects(wrong,"entry cannot omit its additive lift");
                wrong=offer;wrong.relaunch_motion->resource_after=9900;rejects(wrong,"entry cannot pay the continued cost");
            }else if(native_borg_motion_relaunch(offer.kind)){
                const auto& e=*offer.relaunch_motion;
                check(!e.entry&&!offer.entry_vertical_delta&&e.resource_before==source.resource()&&e.resource_after==e.resource_before-100,
                    "continued relaunch charges per tick without repeating its entry lift");
                if(!active_tick&&native_borg_motion_relaunch_active(offer.kind)){
                    active_tick=tick;
                    check(e.lower_program==15&&e.upper_program==15&&e.resource_after==9350,"the held route enters active program15");
                }
            }
            if(offer.kind==Kind::JumpAir&&!fall)fall=tick;
            if(offer.kind==Kind::JumpTouchdown)touchdown=tick;
            if(touchdown&&offer.kind==Kind::JumpLanding&&!landing)landing=tick;
            accept(offer);
            if(tick==fall||tick==touchdown||tick==landing)check(source.resource()==release_meter,
                "release, touchdown and landing entry neither spend nor refill");
        }
        check(entries==1&&source.relaunches()==1&&!source.air_dash_starts()&&
            (held_ticks==1?(active_tick==0&&fall==59&&touchdown==110&&landing==111):
                (active_tick==63&&fall==78&&touchdown==141&&landing==142))&&
            source.landings()==1&&source.walk_ledge_pushes()==2&&world.actors()[0].grounded&&
            source.resource()==10000&&!source.selector(),"tap and held relaunch both settle and refill normally");
    }

    bind(expires,Mode::GroundWalkLedgeExpiryPush);
    while(world.tick()<56){check(prepare(expires,&offer),"accepted prefix near the restricted opposite drift");accept(offer);}
    const auto before_down=world.actors()[0].position;
    GamePadState down;down.dpad_down=true;
    check(!prepare(expires,&offer,down)&&error.find("steep push")!=std::string::npos&&world.tick()==56&&
        world.actors()[0].position==before_down&&source.walk_ledge_pushes()==2&&!view.practice_readout(world)&&!world.step({},false,&view),
        "opposite input into a new steep contact does not inherit expiry-push permission or show drift");

    bind(inter_gate,Mode::GroundWalkLedgeExpiryPush);
    while(world.tick()<23){check(prepare(inter_gate,&offer),"accepted prefix of the attempted post-push relaunch");accept(offer);}
    check(prepare(inter_gate,&offer,relaunch)&&offer.kind==Kind::RelaunchEntry&&source.walk_ledge_pushes()==1,
        "a staged relaunch candidate can exist without a post-push admission");
    const auto relaunch_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&relaunch_frame,&offer)&&world.tick()==23&&
        source.resource()==10000&&source.relaunches()==0&&world.motion_receipt_current(pose.receipt),
        "native refuses the post-push handoff without launch, meter or pose publication");
    source.reject();check(!world.step({},false,&view),"unsupported launch does not fall back");

    const Route dash_push_candidate{726.917969f,3344.20435f,487.747589f,0};
    bind(dash_push_candidate,Mode::GroundWalkLedgeExpiryPush);
    auto prepare_dash_candidate=[&](){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        GamePadState pad;if(world.tick()==5)pad.stick_y=-127;
        NativeGRedGroundInput input;
        return source.map_input(pad,&input,&error)&&
            source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,&offer,&error);
    };
    while(world.tick()<13){check(prepare_dash_candidate(),"accepted prefix of the genuine dash-origin contact candidate");accept(offer);}
    check(offer.walk_ledge&&!offer.walk_ledge->walk_origin&&source.dash_departures()==1&&
        !prepare_dash_candidate()&&error.find("steep push")!=std::string::npos&&world.tick()==13&&
        !source.walk_ledge_pushes()&&!view.practice_readout(world)&&!world.step({},false,&view),
        "dash-origin candidate remains unsupported even in the expiry mode");

    bind(raised,Mode::GroundWalkLedgeExpiryPush);
    while(world.tick()<140){check(prepare(raised,&offer),"known raised-face approach");accept(offer);}
    check(prepare(raised,&offer)&&offer.kind==Kind::GroundLossPush&&offer.walk_ledge->timer698==28&&
        offer.static_push->surface.height>world.actors()[0].position[1],"source can compute a raised-face candidate");
    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&frame,&offer)&&world.tick()==140&&source.ground_loss_timer698()==29&&
        !source.walk_ledge_pushes()&&world.motion_receipt_current(pose.receipt),
        "native still refuses the real raised face before publishing pose or timer");

    files.change_contact_operand=true;
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,pack,world,seed,&error,Mode::GroundWalkLedgeHighAnglePush)&&
        error.find("qualified normal source profiles required")!=std::string::npos,
        "the qualified-pack loader refuses a modified source angle before the new mode acquires a producer");
    std::printf("walk ledge-fall push: %u checks PASS, scoped positive-timer continuation and refusal boundaries\n",checks);
}
