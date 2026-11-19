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
    auto prepare=[&](const Route& r,NativeBorgMotionOffer* offer){
        GamePadState pad;
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
