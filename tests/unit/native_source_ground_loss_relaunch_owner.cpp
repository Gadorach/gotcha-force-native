#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground-loss relaunch: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem{
public:
    bool file_size(const std::string& n,gf::u64* o)override{std::error_code e;const auto s=std::filesystem::file_size(n,e);if(e||!o)return false;*o=s;return true;}
    bool read(const std::string& n,std::vector<gf::u8>& o)override{gf::u64 s=0;if(!file_size(n,&s)||s>(32u<<20))return false;std::ifstream f(n,std::ios::binary);o.assign(std::istreambuf_iterator<char>(f),{});return o.size()==s;}
    bool exists(const std::string& n)override{return std::filesystem::exists(n);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
struct AnglePlane final:gf::IBattleMvpTerrain{
    const gf::NativeBattleMvpView& view;gf::BattleMvpVec normal{};
    explicit AnglePlane(const gf::NativeBattleMvpView& v):view(v){gf::f32 x=0,y=0;gf::source_heading_xz(8192,&x,&y);normal={x,y,0};}
    bool height(gf::f32 x,gf::f32 z,gf::f32 y,gf::f32* out)const override{return view.height(x,z,y,out);}
    bool static_surface(gf::f32 x,gf::f32 z,gf::f32 y,gf::NativeBorgStaticSurface* out)const override{
        if(!view.static_surface(x,z,y,out))return false;
        out->normal=normal;return true;
    }
};
struct NoCoplanarProof final:gf::IBattleMvpTerrain{
    const gf::NativeBattleMvpView& view;
    explicit NoCoplanarProof(const gf::NativeBattleMvpView& v):view(v){}
    bool height(gf::f32 x,gf::f32 z,gf::f32 y,gf::f32* out)const override{return view.height(x,z,y,out);}
    bool static_surface(gf::f32 x,gf::f32 z,gf::f32 y,gf::NativeBorgStaticSurface* out)const override{return view.static_surface(x,z,y,out);}
};
struct RelabeledPlane final:gf::IBattleMvpTerrain{
    const gf::NativeBattleMvpView& view;
    explicit RelabeledPlane(const gf::NativeBattleMvpView& v):view(v){}
    bool height(gf::f32 x,gf::f32 z,gf::f32 y,gf::f32* out)const override{return view.height(x,z,y,out);}
    bool static_surface(gf::f32 x,gf::f32 z,gf::f32 y,gf::NativeBorgStaticSurface* out)const override{
        if(!view.static_surface(x,z,y,out))return false;
        if(out->triangle==1993)out->triangle=2008;
        return true;
    }
};
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground-loss relaunch: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"actual698 relaunch"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    view.source_practice_controls();
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"same qualified shared resource model");
    struct Route {unsigned press,hold,fixture,entry,fall,touchdown,floors;s16 meter;bool air=false,direction=false;};
    const Route routes[]={{122,20,0,123,143,199,0,7950},{122,450,0,123,223,327,0,0},
        {137,2,0,138,140,148,0,9750},{146,450,1,147,247,333,6,0},
        {143,2,0,144,146,187,0,8600,true},{122,20,0,123,143,180,0,7950,false,true},
        {146,11,1,147,158,162,6,8850},{146,5,1,147,152,152,1,9450}};
    auto bind=[&](unsigned fixture,NativeGRedGroundDashDemo::Mode mode){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={-1368.47f,fixture?-466.667f:-628.674f};config.spawn_query_y[0]=94.339f;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "declared original-stage grounded placement/query reference and4500 bounds; no midair/resource injection or rebind");
    };
    auto pad_at=[&](unsigned t,const Route& r){
        GamePadState pad;if(t>=5&&!source.ground_loss_entries()){pad.dpad_down=true;pad.dpad_right=true;}
        if(r.air&&t>=122&&t<r.press)pad.stick_x=127;
        pad.b=t>=r.press&&t<r.press+r.hold;
        if(r.direction&&pad.b)pad.dpad_up=true;
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"actual timer/relaunch/scalar acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world, &pose)&&view.update_source(world,pose,&error),
            "matching complete receipt before resource/VM/latch/timer/pose publication");
    };
    bind(0,NativeGRedGroundDashDemo::Mode::GroundLossAir);bool old_refused=false;
    for(unsigned t=0;t<140&&!old_refused;++t){
        const auto retained=pose;NativeBorgMotionOffer offer;
        if(!prepare(pad_at(t,routes[0]),&offer)){
            check(world.tick()==122&&source.ground_loss_timer698()==29&&!source.relaunches()&&source.resource()==10000&&
                retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "prior air mode still refuses ground-loss/positive698 relaunch before cost or publication");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"new relaunch permission does not widen the earlier mode");
    for(const auto& r:routes){
        bind(r.fixture,NativeGRedGroundDashDemo::Mode::GroundLossRelaunch);
        CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;unsigned entry=0,fall=0,touchdown=0,floors=0,positive_floor=0;
        bool neutral_over8=false,neutral_over25=false,directional_clamp=false;BattleMvpVec landed{};
        const unsigned frames=r.fixture&&r.hold<=11?200:600;
        for(unsigned t=0;t<frames;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto timer=source.ground_loss_timer698();const auto meter=source.resource();
            const auto latches=source.relaunch_stages();CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "common resource recovery uses previous accepted airborne cache");
            NativeBorgMotionOffer offer;
            check(prepare(pad_at(t,r),&offer)&&source.ground_loss_timer698()==timer&&source.resource()==meter&&
                source.relaunch_stages().lower_stage==latches.lower_stage&&source.relaunch_stages().upper_stage==latches.upper_stage,
                "input/VM/timer/cost/independent latches staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer through loss/air/relaunch/released fall/landing");
            const f32 current=offer.kind==NativeBorgMotionKind::GroundLossEntry?30.f:std::max(0.f,timer-1.f);
            check(offer.controller_timer698&&*offer.controller_timer698==current&&
                (!offer.relaunch_floor||offer.relaunch_floor->timer698==current)&&
                (!offer.surface_landing||offer.surface_landing->timer698==current),"same actual timer across all roles and both contact contracts");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==before.position&&
                    source.resource()==meter&&source.ground_loss_timer698()==timer&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.air_motion){
                if(offer.air_motion->phase==16)check(character_air_resource_consume(expected,input.costs[3],input.recovery_delay,1,&expected),"air50 before motion");
                if(offer.air_motion->phase==15&&!offer.air_motion->entry&&offer.air_motion->next_selector==16)
                    check(character_air_resource_consume(expected,input.costs[2],input.recovery_delay,1,&expected),"wind-up450 after motion before contact");
            }
            if(offer.relaunch_motion){
                const auto& e=*offer.relaunch_motion;
                check(e.resource_before==expected.value736&&character_air_resource_consume(expected,input.costs[e.entry?0:1],input.recovery_delay,1,&expected)&&
                    e.resource_after==expected.value736,"entry150/continued100 before relaunch movement/contact");
                if(e.entry){
                    entry=t+1;
                    check(e.vertical_before==f32(before.vertical_speed+15)&&offer.entry_vertical_delta==15&&
                        (!offer.relaunch_floor?offer.proposed_position[1]:offer.relaunch_floor->integrated_position[1])==f32(before.position[1]+e.vertical_before),
                        "entry adds15 to old velocity before displacement, never replaces it or clears698");
                    auto wrong=offer;wrong.controller_timer698=0;rejects(wrong,"positive relaunch cannot fake a timer clear");
                    wrong=offer;wrong.relaunch_motion.reset();rejects(wrong,"new admission cannot omit exact relaunch proof");
                    wrong=offer;wrong.relaunch_motion->vertical_before=15;rejects(wrong,"old velocity remains part of additive entry");
                    wrong=offer;wrong.relaunch_motion->resource_after+=150;rejects(wrong,"positive timer does not waive entry cost");
                }
                if(!e.direction&&e.move_speed>8){
                    neutral_over8=true;if(e.move_speed>25)neutral_over25=true;
                    check(e.planar_accel==0&&e.speed44==e.move_speed,"neutral wrapper does not apply the positive-acceleration8 clamp");
                    auto wrong=offer;wrong.relaunch_motion->speed44=8;rejects(wrong,"blanket8 clamp cannot replace actual retained scalar");
                }
                if(e.direction){directional_clamp=true;check(e.planar_accel>0&&e.speed44<=8,"positive planar acceleration retains source8 clamp");}
            }
            if(offer.relaunch_floor){
                ++floors;const auto& e=*offer.relaunch_floor;
                check(e.timer698==current&&e.angle<=(current>0?6144:10240)&&
                    offer.vertical_speed==f32(e.vertical_speed*(e.vertical_speed<0?bits_to_f32(0x3f4ccccdu):1.f))&&
                    !offer.static_push&&!offer.surface_landing,"null754 snap retains history/50/latches with negative-only .8 tail");
                 auto wrong=offer;wrong.relaunch_floor->timer698=current>0?0:1;rejects(wrong,"relaunch floor proof cannot choose another timer gate");
                wrong=offer;wrong.relaunch_floor->response_factor=e.response_factor<1.f?1.f:.8f;
                rejects(wrong,"the accepted vertical-speed sign decides damping, not the cue");
                if(current>0){
                    ++positive_floor;AnglePlane angled(view);wrong=offer;wrong.relaunch_floor->surface.normal=angled.normal;
                    check(source_surface_angle(angled.normal,&wrong.relaunch_floor->angle)&&wrong.relaunch_floor->angle>6144&&wrong.relaunch_floor->angle<=10240,
                        "numeric coherent inter-gate floor fixture");
                    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    check(!world.step({},false,&angled,nullptr,&frame,&wrong)&&world.tick()==t&&!retained.frame_scope.expired(),
                        "positive698 relaunch floor cannot borrow the zero-timer AC gate");
                }
            }
            if(offer.jump_motion&&offer.jump_motion->phase==6&&offer.jump_motion->entry&&offer.jump_motion->relaunch_origin){
                fall=t+1;auto wrong=offer;wrong.jump_motion->relaunch_origin=false;rejects(wrong,"release/depletion cannot borrow normal jump or ground-loss lineage");
                check(!offer.jump_motion->ground_loss&&!offer.jump_motion->air_dash_origin,"released21 lineage remains distinct after real ground-loss relaunch");
            }
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown;
            if(hit){touchdown=t+1;landed=offer.proposed_position;}
            accept(offer);model=expected;
            const auto shown=view.practice_readout(world);
            check(shown&&shown->relaunch_floor_damped==(offer.relaunch_floor?
                std::optional{offer.relaunch_floor->response_factor<1.f}:std::nullopt),
                "the snap response cue is present only on its accepted floor frame");
            if(r.hold==11&&t+1==156)check(offer.relaunch_floor&&offer.relaunch_floor->vertical_speed>0&&
                offer.relaunch_floor->response_factor==1.f&&shown->relaunch_floor_damped==false,
                "last snap keeps the newly positive vertical speed");
            if(r.hold==11&&t+1==157)check(offer.kind==NativeBorgMotionKind::RelaunchActive&&!offer.relaunch_floor&&
                !world.actors()[0].grounded&&!shown->relaunch_floor_damped&&source.resource()==8850,
                "brief held route lifts off without a snap cue or grounded refill");
            check(source.resource()==model.value736&&source.ground_loss_timer698()==current&&!source.surface_pushes(),"same shared resource/timer kernels; no push/refill shortcut");
            if(offer.relaunch_floor)check(!world.actors()[0].grounded&&source.cached_airborne87()==1&&!source.landings(),"relaunch snap is not landing or cached-ground history");
            if(offer.relaunch_motion){
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.relaunch_motion->upper_after=0;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals lower/upper latch and scalar proof");
            }
            if(hit)check(source.resource()==r.meter&&source.cached_airborne87()==1&&source.movement_programs()==std::array<u16,2>{21,21},
                "actual surface callback retains released21/cache1 and real spent meter");
            if(touchdown&&t+1==touchdown+1)check(source.cached_airborne87()==0&&source.resource()==r.meter&&
                source.movement_programs()==std::array<u16,2>{8,8},"separate landing8 before common recovery");
            if(touchdown&&t+1>touchdown)check(world.actors()[0].position==landed,"stable separate landing/idle endpoint");
        }
        check(entry==r.entry&&fall==r.fall&&touchdown==r.touchdown&&floors==r.floors&&source.relaunches()==1&&!source.ground_loss_landings()&&
            source.landings()==1&&source.resource()==10000&&source.ground_loss_timer698()==0&&source.selector()==0&&
            source.movement_programs()==std::array<u16,2>{0,0}&&!world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
             "eight genuine lift/release/depletion/floor/high-scalar/directional routes return idle");
        if(!r.direction)check(neutral_over8,"new neutral inherited scalar variant really exceeds8");
        if(r.air)check(neutral_over25,"air-to-neutral-relaunch and released lineage genuinely retain scalar above25");
        if(r.direction)check(directional_clamp,"directional source clamp branch exercised");
        if(r.floors)check(positive_floor==1,"actual positive698 relaunch floor receipt exercised, not a default-zero substitution");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"timer/relaunch/pose authority retirement");
    }
    using Kind=NativeBorgMotionKind;
    using Mode=NativeGRedGroundDashDemo::Mode;
    struct SnapDash {unsigned press,flick,snaps,touch;f32 preceding_timer;u32 face;s16 meter;bool positive_speed=false;};
    const SnapDash snap_dashes[]={{148,150,1,186,3.f,1116,9750},{147,152,1,188,1.f,900,9450},{147,157,6,193,0.f,900,8950,true}};
    auto snap_dash_pad=[&](unsigned tick,const SnapDash& route,bool hold_on_flick){
        GamePadState pad;
        if(tick>=6&&!source.ground_loss_entries())pad.dpad_down=pad.dpad_right=true;
        pad.b=tick>=route.press&&(tick<route.flick||(hold_on_flick&&tick==route.flick));
        if(tick==route.flick)pad.stick_x=pad.stick_y=127;
        return pad;
    };
    for(const auto& route:snap_dashes){
        bind(1,Mode::GroundLossRelaunch);
        NativeBorgMotionOffer previous;
        std::weak_ptr<const NativeBorgMotionProducer> producer;
        unsigned starts=0,paid=0,active=0,hit=0,land=0,air_snaps=0;
        for(unsigned tick=1;tick<=220;++tick){
            const auto before=world.actors()[0];const auto retained=pose;
            const auto old_timer=source.ground_loss_timer698();const auto meter=source.resource();
            NativeBorgMotionOffer offer;
            check(prepare(snap_dash_pad(tick,route,true),&offer)&&source.resource()==meter&&source.ground_loss_timer698()==old_timer,
                "release, enter or continue through the existing timed controller before publishing");
            if(tick==1)producer=offer.producer;
            check(!offer.producer.owner_before(producer)&&!producer.owner_before(offer.producer),
                "same producer through floor snap, wind-up, active dash and landing");
            const f32 timer=offer.kind==Kind::GroundLossEntry?30.f:std::max(0.f,old_timer-1.f);
            check(offer.controller_timer698==timer,"the timer naturally decrements across the handoff without reset");
            if(native_borg_motion_windup_entry(offer.kind)){
                ++starts;
                check(tick==route.flick&&old_timer==route.preceding_timer&&previous.relaunch_floor&&previous.relaunch_motion&&
                    (before.vertical_speed>0)==route.positive_speed&&!before.grounded&&
                    offer.kind==(route.positive_speed?Kind::AirDashWindupPositiveEntryFloor:Kind::AirDashWindupEntryFloor)&&
                    offer.air_dash_floor&&offer.air_motion&&!offer.relaunch_floor&&!offer.relaunch_motion&&!offer.static_push,
                    "one wind-up takes the accepted relaunch snap's timer and velocity domain, not a new grounded takeoff");
                const auto& e=*offer.air_motion;const auto& floor=*offer.air_dash_floor;
                check(floor.surface==previous.relaunch_floor->surface&&floor.surface.triangle==route.face&&floor.timer698==timer&&
                    floor.integrated_position==before.position&&offer.proposed_position==before.position&&offer.vertical_speed==0.f&&
                    e.vertical_before==before.vertical_speed*.5f&&e.vertical_before==floor.vertical_speed&&
                    e.speed_before==previous.relaunch_motion->speed44&&e.speed44==e.speed_before*.5f&&
                    e.resource_before==route.meter&&e.resource_after==route.meter&&!e.callback&&e.timer568==5.f,
                    "wind-up snap halves real inherited velocity before zeroing 48, with no step, gravity integration, cost or callback yet");
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                auto rejects=[&](const NativeBorgMotionOffer& wrong,const char* why){
                    const auto shown=view.practice_readout(world);
                    check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick-1&&
                        world.actors()[0].position==before.position&&world.motion_receipt_current(retained.receipt)&&
                        source.resource()==route.meter&&source.ground_loss_timer698()==old_timer&&!source.air_dash_starts()&&
                        shown&&shown->relaunch_floor_damped.has_value()&&!shown->windup_from_relaunch_snap,why);
                };
                auto wrong=offer;wrong.air_dash_floor->timer698=timer==0?1.f:0.f;
                rejects(wrong,"floor gate must use the actual next timer");
                wrong=offer;wrong.air_motion->vertical_before=0.f;
                rejects(wrong,"the zeroed post-contact speed cannot stand in for the remembered half-speed");
                wrong=offer;wrong.kind=route.positive_speed?Kind::AirDashWindupEntryFloor:Kind::AirDashWindupPositiveEntryFloor;
                rejects(wrong,"positive and negative entry snaps keep distinct kinds");
                wrong=offer;wrong.proposed_position[0]+=1.f;
                rejects(wrong,"a stationary entry may not advance the root");
                wrong=offer;wrong.air_motion->resource_after-=450;
                rejects(wrong,"wind-up entry cannot borrow the later charge");
            }
            if(offer.air_motion&&offer.air_motion->phase==15&&offer.air_motion->next_selector==16){
                paid=tick;check(tick==route.flick+5&&offer.air_motion->resource_after==route.meter-450,
                    "the separate accepted wind-up handoff pays 450");
            }
            if(native_borg_motion_active_entry(offer.kind)){
                active=tick;check(tick==paid+1&&offer.air_motion->resource_after==route.meter-500,"first active update then pays 50");
            }
            air_snaps+=bool(offer.air_dash_floor);
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            const auto shown=view.practice_readout(world);
            check(shown&&shown->windup_from_relaunch_snap==(tick==route.flick)&&source.ground_loss_timer698()==timer&&
                !source.surface_pushes(),"the snap-to-wind-up label is published on the matching accepted entry only");
            if(offer.air_dash_floor)check(!world.actors()[0].grounded&&source.cached_airborne87()&&!source.landings(),
                "neither wind-up nor active floor snaps are touchdown");
            if(tick==hit||tick==land)check(world.actors()[0].grounded&&source.resource()==route.meter-2000,
                "callback touchdown and the separate landing do not refill");
            previous=offer;
        }
        check(starts==1&&paid==route.flick+5&&active==paid+1&&air_snaps==36&&hit==route.touch&&land==hit+1&&
            source.resource()==10000&&source.landings()==1&&source.relaunch_floor_contacts()==route.snaps&&
            source.air_dash_starts()==1&&source.relaunches()==1&&source.selector()==0,
            "positive timer, expiry boundary and positive-speed routes all settle without any new contact permission");
    }
    bind(1,Mode::GroundLossRelaunch);
    for(unsigned tick=1;tick<=150;++tick){
        NativeBorgMotionOffer offer;
        check(prepare(snap_dash_pad(tick,snap_dashes[0],false),&offer),"same flick with Shift released reaches touchdown");
        if(tick==150)check(offer.kind==Kind::JumpTouchdown&&!offer.air_motion&&offer.controller_timer698==2.f,
            "release wins over the simultaneous flick and the ordinary fall reaches real contact");
        accept(offer);
    }
    const auto retained=pose;const auto stopped=world.actors()[0].position;NativeBorgMotionOffer denied;
    check(!prepare({},&denied)&&error.find("refuses buffered air/landing dash")!=std::string::npos&&
        world.tick()==150&&world.actors()[0].position==stopped&&source.resource()==9750&&source.ground_loss_timer698()==2.f&&
        !source.air_dash_starts()&&source.landings()==1&&retained.frame_scope.expired()&&!world.step({},false,&view),
        "the residual flick at landing still refuses, without changing touchdown or borrowing a land-dash permission");
    bind(1,Mode::GroundLossRelaunch);
    for(unsigned tick=1;tick<=190;++tick){
        auto pad=snap_dash_pad(tick,snap_dashes[0],false);pad.stick_x=pad.stick_y=0;
        NativeBorgMotionOffer offer;check(prepare(pad,&offer),"neutral release without the flick");
        if(tick==150||tick==151)check(offer.kind==(tick==150?Kind::JumpTouchdown:Kind::JumpLanding),
            "neutral release lands and accepts the following landing entry");
        accept(offer);
    }
    check(!source.air_dash_starts()&&source.landings()==1&&source.resource()==10000&&source.selector()==0,
        "neutral release settles normally without a pending dash");
    auto floor_windup_prefix=[&](Mode mode,const SnapDash& route){
        bind(1,mode);
        for(unsigned tick=1;tick<=route.flick+5;++tick){
            NativeBorgMotionOffer offer;
            check(prepare(snap_dash_pad(tick,route,true),&offer),"accepted snap and uninterrupted wind-up prefix");
            accept(offer);
        }
    };
    GamePadState relaunch_tap;relaunch_tap.b=true;
    floor_windup_prefix(Mode::GroundWalkExpiryDirectRelaunch,snap_dashes[0]);
    {
        NativeBorgMotionOffer offer;const auto retained=pose;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(!view.practice_readout(world)->floor_windup_relaunch_ready&&prepare(relaunch_tap,&offer)&&offer.kind==Kind::RelaunchEntry&&
            !world.step({},false,&view,nullptr,&frame,&offer)&&world.tick()==155&&source.resource()==9300&&
            world.motion_receipt_current(retained.receipt),"the same floor route in an older mode still cannot publish relaunch");
        source.reject();
    }
    floor_windup_prefix(Mode::GroundTimedSnapWindupRelaunch,snap_dashes[1]);
    {
        NativeBorgMotionOffer offer;
        check(!view.practice_readout(world)->floor_windup_relaunch_ready&&!prepare(relaunch_tap,&offer)&&world.tick()==157&&
            error.find("uninterrupted timed snap history")!=std::string::npos&&source.resource()==9000&&!world.step({},false,&view),
            "wind-up starting on a pending 1-to-0 cannot claim positive-entry ancestry later");
    }
    floor_windup_prefix(Mode::GroundTimedSnapWindupRelaunch,snap_dashes[0]);
    {
        auto direction=relaunch_tap;direction.dpad_up=true;
        NativeBorgMotionOffer offer;
        check(view.practice_readout(world)->floor_windup_relaunch_ready&&!prepare(direction,&offer)&&world.tick()==155&&
            error.find("plain neutral relaunch")!=std::string::npos&&source.resource()==9300&&!world.step({},false,&view),
            "the new permission is only for neutral relaunch, not steering or a ground fallback");
    }
    for(unsigned hold:{0u,1u,4u}){
        bind(1,Mode::GroundTimedSnapWindupRelaunch);
        unsigned floor_frames=0,hit=0,land=0,release=0;
        NativeBorgMotionOffer previous;
        std::weak_ptr<const NativeBorgMotionProducer> producer;
        for(unsigned tick=1;tick<=225;++tick){
            const auto before=world.actors()[0];const auto retained=pose;
            auto pad=snap_dash_pad(tick,snap_dashes[0],true);
            if(tick>=156&&tick<156+hold)pad.b=true;
            const auto meter=source.resource();const auto timer=source.ground_loss_timer698();
            NativeBorgMotionOffer offer;
            check(prepare(pad,&offer)&&source.resource()==meter&&source.ground_loss_timer698()==timer,
                "lineage, contact, timer and charges remain staged");
            if(tick==1)producer=offer.producer;
            check(!offer.producer.owner_before(producer)&&!producer.owner_before(offer.producer),
                "both choices keep one producer and one renderer through landing");
            const bool lineage=tick>=150&&tick<=155;
            check((offer.air_motion&&offer.air_motion->timed_relaunch_floor_windup)==lineage,
                "only six consecutive qualified floor wind-up frames carry lineage");
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            auto refuses=[&](const NativeBorgMotionOffer& wrong,const IBattleMvpTerrain& terrain,const char* why){
                const auto shown=view.practice_readout(world);
                check(!world.step({},false,&terrain,nullptr,&frame,&wrong)&&world.tick()==tick-1&&
                    world.actors()[0].position==before.position&&world.motion_receipt_current(retained.receipt)&&
                    source.resource()==meter&&source.ground_loss_timer698()==timer&&shown&&
                    shown->floor_windup_relaunch_ready==(tick==156),why);
            };
            if(lineage){
                ++floor_frames;
                check(offer.air_dash_floor&&!before.grounded&&offer.proposed_position[1]==before.position[1]&&
                    offer.controller_timer698==std::max(0.f,152.f-f32(tick)),
                    "floor continuity and natural expiry do not imply landing");
                auto wrong=offer;wrong.air_motion->timed_relaunch_floor_windup=false;
                refuses(wrong,view,"the current offer cannot edit the certified ancestry");
            }
            if(tick==151){
                NoCoplanarProof missing(view);
                check(previous.air_dash_floor&&previous.air_dash_floor->surface.triangle==1116&&
                    offer.air_dash_floor->surface.triangle==900,"the wind-up actually crosses the original flat seam");
                refuses(offer,missing,"an unverified seam cannot extend the timed floor lineage");
            }
            if(tick==155)check(offer.air_motion->resource_before==9750&&offer.air_motion->resource_after==9300&&
                offer.air_motion->next_selector==16,"completed floor wind-up spends its own 450");
            if(tick==156&&hold){
                check(offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&offer.relaunch_motion->entry&&!offer.relaunch_motion->direction&&
                    !before.grounded&&!before.vertical_speed&&offer.proposed_position[1]==before.position[1]+15.f&&offer.vertical_speed==15.8f&&
                    offer.facing==before.facing&&offer.controller_timer698==0.f&&offer.relaunch_motion->resource_before==9300&&
                    offer.relaunch_motion->resource_after==9150&&!offer.relaunch_floor&&!offer.air_dash_floor&&!offer.static_push,
                    "neutral rising entry from the same floor pays 150 but not active dash");
                auto wrong=offer;wrong.relaunch_motion->resource_before=9750;
                refuses(wrong,view,"a relaunch cannot undo the paid wind-up");
                wrong=offer;wrong.relaunch_motion->resource_after-=50;
                refuses(wrong,view,"a relaunch cannot charge for the active dash it replaced");
                wrong=offer;wrong.controller_timer698=1.f;
                refuses(wrong,view,"entry cannot revive the expired timer");
                AnglePlane other_plane(view);
                refuses(offer,other_plane,"native must independently select the same flat plane over the new entry");
            }
            if(tick==156&&!hold)check(offer.kind==Kind::AirDashActiveEntryFloor&&offer.air_motion&&
                offer.air_motion->resource_after==9250,"neutral input still selects the ordinary paid active dash");
            if(hold&&tick>156&&!release&&offer.kind==Kind::JumpAir)release=tick;
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            const auto shown=view.practice_readout(world);
            check(shown&&shown->floor_windup_relaunch_ready==(tick==155)&&!shown->plain_windup_relaunch_ready,
                "only the accepted paid floor wind-up gives the choice");
            if(tick==release||tick==hit||tick==land)check(source.resource()==(hold?9150-100*int(hold-1):7750),
                "no landing or release prematurely recovers meter");
            previous=offer;
        }
        check(floor_frames==6&&hit==(hold==1?190u:hold==4?200u:186u)&&land==hit+1&&
            release==(hold?156+hold:0)&&source.relaunches()==(hold?2u:1u)&&
            (source.air_dash_active_frames()==0)==bool(hold)&&source.resource()==10000&&source.landings()==1&&!source.selector(),
            "both the ordinary dash and neutral relaunch choices reach separate landing and idle");
    }
    struct ClockHandoff {
        f32 x,z,reference;unsigned walk_end,snap,paid,left,face,dash_touch,tap_touch,held_touch,end;
        bool lateral;Mode prior,enabled;
    };
    const ClockHandoff clock_routes[]{
        {2892.57739f,-2198.41479f,151.148178f,8,25,31,7,1543,62,66,76,110,false,
            Mode::GroundTimedSnapWindupRelaunch,Mode::GroundPositiveSnapWindupRelaunch},
        {3895.56787f,-1816.47241f,262.01004f,25,46,54,1,1542,85,89,99,135,true,
            Mode::GroundPositiveSnapWindupRelaunch,Mode::GroundExpiringSnapWindupRelaunch},
    };
    for(const auto& r:clock_routes){
        const auto handoff=r.paid+1;
        auto fixture=[&](Mode mode){
            BattleMvpConfig cfg;cfg.arena_half=4500;cfg.spawn[0]={r.x,r.z};cfg.spawn_query_y[0]=r.reference;
            check(world.start(cfg,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
                source.bind(files,art,pack,world,seed,&error,mode),"declared grounded floor-handoff fixture");
        };
        auto pad_at=[&](unsigned tick,unsigned hold){
            GamePadState pad;pad.dpad_up=tick>=6&&tick<=r.walk_end;
            pad.b=tick==r.snap||tick==r.snap+1||(tick>=handoff&&tick<handoff+hold);
            if(tick==r.snap+1){if(r.lateral)pad.stick_x=-127;else pad.stick_y=-127;}
            return pad;
        };
        fixture(r.prior);
        for(unsigned tick=1;tick<=r.paid;++tick){
            NativeBorgMotionOffer offer;check(prepare(pad_at(tick,0),&offer),"older mode accepts the same timed floor wind-up");
            if(tick==r.paid)check(offer.air_motion&&offer.air_motion->timed_relaunch_floor_windup&&offer.controller_timer698==f32(r.left),
                "the older policy has correct lineage but does not admit this countdown transition");
            accept(offer);
        }
        {
            NativeBorgMotionOffer refused;const auto retained=pose;const auto previous=world.actors()[0].position;
            check(!view.practice_readout(world)->floor_windup_relaunch_ready&&!prepare(pad_at(handoff,1),&refused)&&
                error.find("countdown")!=std::string::npos&&world.tick()==r.paid&&source.resource()==9400&&
                source.ground_loss_timer698()==f32(r.left)&&world.actors()[0].position==previous&&source.relaunches()==1&&
                retained.frame_scope.expired()&&!world.step({},false,&view),
                "older clock policy still refuses without consuming timer, meter or fallback");
        }
        for(unsigned hold:{0u,1u,4u}){
            fixture(r.enabled);
            unsigned hit=0,land=0,fall=0;
            for(unsigned tick=1;tick<=r.end;++tick){
                const auto before=world.actors()[0];const auto retained=pose;
                const auto timer=source.ground_loss_timer698();const auto meter=source.resource();
                NativeBorgMotionOffer offer;
                check(prepare(pad_at(tick,hold),&offer)&&source.ground_loss_timer698()==timer&&source.resource()==meter,
                    "motion, timer and cost are staged before native acceptance");
                const f32 next=offer.kind==Kind::GroundLossEntry?30.f:std::max(0.f,timer-1.f);
                check(offer.controller_timer698==next,"every role carries the actual countdown");
                if(tick==r.snap)check(offer.kind==Kind::RelaunchEntryFloor&&offer.relaunch_floor&&
                    offer.relaunch_floor->surface.triangle==r.face&&next==f32(r.left+r.paid-tick)&&!before.grounded,
                    "first real snap is a positive-timer relaunch entry, not touchdown");
                if(tick>r.snap&&tick<=r.paid)check(offer.air_dash_floor&&offer.air_dash_floor->surface.triangle==r.face&&
                    offer.air_motion&&offer.air_motion->timed_relaunch_floor_windup&&next==f32(r.left+r.paid-tick)&&
                    offer.air_motion->resource_after==(tick==r.paid?9400:9850)&&!before.grounded,
                    "the entire wind-up stays on its certified flat floor and pays 450 only at completion");
                if(tick==handoff&&hold){
                    check(offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&offer.relaunch_motion->entry&&!offer.relaunch_motion->direction&&
                        !before.grounded&&!before.vertical_speed&&next==f32(r.left-1)&&offer.proposed_position[1]==before.position[1]+15.f&&
                        offer.relaunch_motion->resource_before==9400&&offer.relaunch_motion->resource_after==9250&&
                        !offer.air_dash_floor&&!offer.relaunch_floor&&offer.facing==before.facing,
                        "neutral relaunch pays 150 and carries the real next timer without snapping again");
                    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    for(f32 wrong_timer:{f32(r.left),next==0.f?2.f:0.f}){
                        auto wrong=offer;wrong.controller_timer698=wrong_timer;
                        check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==r.paid&&source.resource()==9400&&
                            source.ground_loss_timer698()==f32(r.left)&&world.motion_receipt_current(retained.receipt)&&
                            view.practice_readout(world)->floor_windup_relaunch_ready,
                            "a forged countdown leaves the previously paid wind-up and cue live");
                    }
                }else if(tick==handoff)check(native_borg_motion_active_entry(offer.kind)&&offer.air_motion->resource_after==9350&&next==f32(r.left-1),
                    "neutral retains the ordinary active-entry cost and actual countdown");
                if(hold&&tick>handoff&&!fall&&offer.kind==Kind::JumpAir)fall=tick;
                if(offer.kind==Kind::JumpTouchdown)hit=tick;
                if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
                accept(offer);
                const auto shown=view.practice_readout(world);
                check(shown&&shown->floor_windup_relaunch_ready==(tick==r.paid)&&!shown->plain_windup_relaunch_ready,
                    "the chosen clock scope advertises an accepted paid wind-up only");
                if(tick==fall||tick==hit||tick==land)check(source.resource()==(hold?9250-100*int(hold-1):7850),
                    "grounded recovery waits for the original callback and separate landing");
            }
            check(hit==(hold==1?r.tap_touch:hold==4?r.held_touch:r.dash_touch)&&land==hit+1&&fall==(hold?handoff+hold:0)&&
                source.relaunches()==(hold?2u:1u)&&(source.air_dash_active_frames()==0)==bool(hold)&&
                world.actors()[0].grounded&&source.ground_loss_timer698()==0.f&&source.resource()==10000&&!source.selector(),
                "positive and expiring choices both reach separate landing and normal idle");
        }
    }
    auto active_fixture=[&](){
        const auto& r=clock_routes[0];
        BattleMvpConfig cfg;cfg.arena_half=4500;cfg.spawn[0]={r.x,r.z};cfg.spawn_query_y[0]=r.reference;
        check(world.start(cfg,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,Mode::GroundLossRelaunch),"same positive floor fixture in the earlier scope");
    };
    auto active_pad=[](unsigned tick,unsigned relaunch,bool up){
        GamePadState pad;pad.dpad_up=(tick>=6&&tick<=8)||(tick==relaunch&&up);
        pad.b=tick==25||tick==26||tick==relaunch;
        if(tick==26)pad.stick_y=-127;
        return pad;
    };
    active_fixture();
    {
        NativeBorgMotionOffer offer;
        for(unsigned tick=1;tick<=31;++tick){
            check(prepare(active_pad(tick,0,false),&offer),"existing owner accepts paid floor wind-up");
            accept(offer);
        }
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);const auto retained=pose;
        check(!view.practice_readout(world)->floor_windup_relaunch_ready&&prepare(active_pad(32,32,false),&offer)&&
            offer.kind==Kind::RelaunchEntry&&!world.step({},false,&view,nullptr,&frame,&offer)&&world.tick()==31&&
            source.resource()==9400&&source.ground_loss_timer698()==7.f&&world.motion_receipt_current(retained.receipt),
            "this mode still needs the actual active frame before allowing relaunch");
    }
    for(bool up:{false,true}){
        active_fixture();
        unsigned hit=0,land=0;NativeBorgMotionOffer previous;
        for(unsigned tick=1;tick<=110;++tick){
            const auto before=world.actors()[0];const auto retained=pose;
            const auto meter=source.resource();const auto timer=source.ground_loss_timer698();
            NativeBorgMotionOffer offer;
            check(prepare(active_pad(tick,33,up),&offer)&&source.resource()==meter&&source.ground_loss_timer698()==timer,
                "actual post-active relaunch and clock stay staged");
            check(offer.controller_timer698==(offer.kind==Kind::GroundLossEntry?30.f:std::max(0.f,timer-1.f)),"every role retains the real clock");
            if(tick==32)check(offer.kind==Kind::AirDashActiveEntryFloor&&offer.air_motion&&offer.air_dash_floor&&
                offer.air_motion->resource_before==9400&&offer.air_motion->resource_after==9350&&
                offer.controller_timer698==6.f&&!before.grounded,"separate active snap pays its own 50 before the new lift");
            if(tick==33){
                check(previous.air_motion&&previous.air_dash_floor&&offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&
                    offer.relaunch_motion->direction==up&&offer.controller_timer698==5.f&&!before.grounded&&
                    offer.relaunch_motion->speed_before==previous.air_motion->speed44&&offer.relaunch_motion->speed_before>29.f&&
                    offer.relaunch_motion->vertical_before==f32(before.vertical_speed+15.f)&&
                    offer.relaunch_motion->resource_before==9350&&offer.relaunch_motion->resource_after==9200&&
                    !offer.relaunch_floor&&!offer.static_push,"plain relaunch starts from the paid active frame without a new contact");
                const auto& motion=*offer.relaunch_motion;
                check(up?(motion.move_speed==motion.speed_before&&motion.speed44==8.f):
                    (motion.move_speed==f32(motion.speed_before*.98f)&&motion.speed44==motion.move_speed&&motion.speed44>29.f),
                    "only the directional acceleration caps retained speed; neither branch clamps movement before integration");
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                auto refuses=[&](const NativeBorgMotionOffer& wrong,const char* why){
                    check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==32&&
                        world.actors()[0].position==before.position&&source.resource()==9350&&source.ground_loss_timer698()==6.f&&
                        world.motion_receipt_current(retained.receipt)&&!view.practice_readout(world)->relaunch_from_active_snap,why);
                };
                auto wrong=offer;wrong.relaunch_motion->resource_before=9400;refuses(wrong,"cannot refund the active dash");
                wrong=offer;wrong.relaunch_motion->move_speed=8.f;refuses(wrong,"cannot cap the movement before integration");
                wrong=offer;wrong.relaunch_motion->vertical_before=15.f;refuses(wrong,"cannot discard the active snap's negative velocity");
                wrong=offer;wrong.controller_timer698=0.f;refuses(wrong,"cannot restart or erase the existing countdown");
            }
            if(tick==34)check(offer.kind==Kind::JumpAir&&offer.controller_timer698==4.f&&offer.jump_motion->relaunch_origin,
                "release uses the ordinary fall after relaunch, not another active update");
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            const auto shown=view.practice_readout(world);
            check(shown&&shown->relaunch_from_active_snap==(tick==33)&&!shown->floor_windup_relaunch_ready,
                "the marker is exclusive to the actual accepted post-active entry");
            if(tick==34||tick==hit||tick==land)check(source.resource()==9200,"release and landing preserve both earlier charges");
            previous=offer;
        }
        check(hit==(up?66u:67u)&&land==hit+1&&source.air_dash_active_frames()==1&&source.relaunches()==2&&
            source.resource()==10000&&world.actors()[0].grounded&&!source.selector(),
            "two genuine post-active choices finish in idle under existing permissions");
    }
    auto released_push_fixture=[&](Mode mode,f32 z){
        BattleMvpConfig cfg;cfg.arena_half=4500;cfg.spawn[0]={1000.65021f,z};cfg.spawn_query_y[0]=1549.25293f;
        check(world.start(cfg,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"declared grounded timed release fixture");
    };
    auto release_pad=[](unsigned tick,unsigned walk_end,unsigned tap){
        GamePadState pad;pad.dpad_right=tick>=6&&tick<=walk_end;pad.b=tick==tap;return pad;
    };
    released_push_fixture(Mode::GroundWalkTimedDirectUpRelaunch,-3365.34546f);
    for(unsigned tick=1;tick<=32;++tick){
        NativeBorgMotionOffer offer;
        check(prepare(release_pad(tick,8,23),&offer),"older policy accepts the same walking fall, relaunch tap and ordinary release");
        if(offer.jump_motion)check(!offer.jump_motion->timed_walk_relaunch_fall,"older policy does not adopt new history");
        accept(offer);
    }
    {
        NativeBorgMotionOffer offer;const auto retained=pose;const auto before=world.actors()[0].position;
        check(!prepare({},&offer)&&error.find("steep push/damping branch")!=std::string::npos&&world.tick()==32&&
            source.ground_loss_timer698()==7.f&&source.resource()==9850&&world.actors()[0].position==before&&
            retained.frame_scope.expired()&&!world.step({},false,&view),"older mode still refuses the same contact before publication");
    }
    released_push_fixture(Mode::GroundTimedWalkReleasePush,-3365.34546f);
    unsigned timed_pushes=0,hit=0,land=0;
    NativeBorgMotionOffer previous;
    for(unsigned tick=1;tick<=125;++tick){
        const auto before=world.actors()[0];const auto retained=pose;
        const auto timer=source.ground_loss_timer698();const auto meter=source.resource();
        NativeBorgMotionOffer offer;
        check(prepare(release_pad(tick,8,23),&offer)&&source.ground_loss_timer698()==timer&&source.resource()==meter,
            "relaunch, release and inter-gate response remain staged");
        const f32 next=offer.kind==Kind::GroundLossEntry?30.f:std::max(0.f,timer-1.f);
        check(offer.controller_timer698==next,"actual timer is preserved across every role");
        check((offer.relaunch_motion&&offer.relaunch_motion->timed_walk_relaunch_tap)==(tick==23)&&
            (offer.jump_motion&&offer.jump_motion->timed_walk_relaunch_fall)==(tick>=24&&tick<=33),
            "only the exact timed tap and neutral ordinary fall earn history, which ends after the first push");
        if(tick==23)check(previous.kind==Kind::GroundLossAir&&previous.walk_ledge&&previous.walk_ledge->walk_origin&&
            offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion->resource_after==9850&&next==16.f,
            "timed plain tap begins from an ordinary walking-ledge fall");
        if(tick==24)check(offer.kind==Kind::JumpAir&&offer.jump_motion&&offer.jump_motion->entry&&
            offer.jump_motion->relaunch_origin&&next==15.f,"release really enters ordinary falling before any new push");
        auto refuses=[&](const NativeBorgMotionOffer& wrong,const char* why){
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick-1&&
                world.actors()[0].position==before.position&&source.ground_loss_timer698()==timer&&source.resource()==meter&&
                world.motion_receipt_current(retained.receipt)&&view.practice_readout(world)->timed_release_push==(tick==34),why);
        };
        if(tick==23){
            auto wrong=offer;wrong.relaunch_motion->timed_walk_relaunch_tap=false;
            refuses(wrong,"native verifies the initial role instead of trusting the producer bit");
        }
        if(tick==32||tick==34){
            auto wrong=offer;wrong.jump_motion->timed_walk_relaunch_fall=tick==34;
            refuses(wrong,"neutral history is required before, and cannot be carried on after, the accepted push");
        }
        if(offer.kind==Kind::StaticPush){
            ++timed_pushes;
            check(tick==33&&previous.kind==Kind::JumpAir&&next==6.f&&offer.static_push&&
                offer.static_push->surface.triangle==1961&&offer.static_push->angle==8191&&before.vertical_speed<0.f&&
                !offer.jump_motion->entry&&!offer.jump_motion->drift_heading&&
                offer.vertical_speed==f32(offer.static_push->vertical_speed*.9f)&&!offer.surface_landing&&!offer.relaunch_motion,
                "first neutral timed push uses the selected inter-gate plane, exact .9 tail and no contact callback");
            auto wrong=offer;wrong.controller_timer698=0.f;refuses(wrong,"cannot borrow the ordinary expired contact gate");
            wrong=offer;wrong.static_push->angle=10241;refuses(wrong,"cannot borrow a steeper inter-gate response");
            wrong=offer;wrong.proposed_position[1]=offer.static_push->surface.height;refuses(wrong,"projected falling push is not a floor snap");
        }
        if(offer.kind==Kind::JumpTouchdown)hit=tick;
        if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
        accept(offer);
        const auto shown=view.practice_readout(world);
        check(shown&&shown->timed_release_push==(tick==33),"only accepted timed contact publishes the push cue");
        if(tick==33)check(!world.actors()[0].grounded&&source.cached_airborne87()==1&&source.resource()==9850&&!source.landings(),
            "neither the new push nor the remaining countdown grounds or refills G-Red");
        if(tick==hit||tick==land)check(source.resource()==9850,"touchdown and its separate landing preserve the entry cost");
        previous=offer;
    }
    check(timed_pushes==1&&hit==90&&land==91&&source.relaunches()==1&&source.relaunch_fall_pushes()==1&&
        !source.air_dash_starts()&&!source.walk_ledge_pushes()&&source.resource()==10000&&world.actors()[0].grounded&&!source.selector(),
        "one timed inter-gate response, separate landing, normal idle");
    released_push_fixture(Mode::GroundTimedWalkReleasePush,-3371.34546f);
    for(unsigned tick=1;tick<=35;++tick){
        NativeBorgMotionOffer offer;check(prepare(release_pad(tick,9,24),&offer),"nearby approach reaches a genuine first push");
        if(tick==35)check(offer.kind==Kind::StaticPush&&offer.static_push->surface.triangle==2008&&
            offer.controller_timer698==5.f,"this adjacent route earns the push itself, not the next step");
        accept(offer);
    }
    {
        NativeBorgMotionOffer offer;const auto retained=pose;const auto before=world.actors()[0].position;
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(prepare({},&offer)&&offer.kind==Kind::JumpAir&&!offer.jump_motion->timed_walk_relaunch_fall&&
            !world.step({},false,&view,nullptr,&frame,&offer)&&world.tick()==35&&world.actors()[0].position==before&&
            source.ground_loss_timer698()==5.f&&source.resource()==9850&&world.motion_receipt_current(retained.receipt),
            "a nearby accepted push still cannot bypass the next native travel check");
        source.reject();
        check(retained.frame_scope.expired()&&!world.step({},false,&view),"native failure does not grant a fallback");
    }
    auto two_push_fixture=[&](Mode mode,f32 x=-1298.54333f,f32 z=-3395.96802f){
        BattleMvpConfig cfg;cfg.arena_half=4500;cfg.spawn[0]={x,z};cfg.spawn_query_y[0]=1846.01367f;
        check(world.start(cfg,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"declared grounded same-face timed release fixture");
    };
    auto two_push_pad=[](unsigned tick){
        GamePadState pad;pad.dpad_down=pad.dpad_right=tick==6||tick==7;pad.b=tick==20;return pad;
    };
    two_push_fixture(Mode::GroundTimedWalkReleasePush);
    for(unsigned tick=1;tick<=33;++tick){
        NativeBorgMotionOffer offer;
        check(prepare(two_push_pad(tick),&offer),"the single-push policy accepts the full prefix");
        if(tick==33)check(offer.kind==Kind::StaticPush&&offer.controller_timer698==5.f&&
            offer.jump_motion->timed_walk_relaunch_fall,"single-push policy accepts the first contact on this route");
        accept(offer);
    }
    {
        NativeBorgMotionOffer offer;const auto before=world.actors()[0].position;
        check(!prepare({},&offer)&&error.find("steep push/damping branch")!=std::string::npos&&world.tick()==33&&
            source.ground_loss_timer698()==5.f&&source.resource()==9850&&world.actors()[0].position==before&&
            !world.step({},false,&view),"the old scope refuses this genuine second timed contact without a fallback");
    }
    two_push_fixture(Mode::GroundTimedWalkReleaseSecondPush);
    previous={};hit=land=timed_pushes=0;
    for(unsigned tick=1;tick<=125;++tick){
        const auto before=world.actors()[0];const auto retained=pose;
        const auto timer=source.ground_loss_timer698();const auto meter=source.resource();
        NativeBorgMotionOffer offer;
        check(prepare(two_push_pad(tick),&offer)&&source.ground_loss_timer698()==timer&&source.resource()==meter,
            "staging either contact cannot publish native state or spend resource");
        const auto next=offer.kind==Kind::GroundLossEntry?30.f:std::max(0.f,timer-1.f);
        check(offer.controller_timer698==next,"both pushes use the same actual countdown");
        const bool first=tick==33,second=tick==34;
        check((offer.jump_motion&&offer.jump_motion->second_timed_release_push)==second,
            "only the immediate same-face successor carries the second-contact tag");
        if(tick==21)check(offer.kind==Kind::JumpAir&&offer.jump_motion->entry&&offer.jump_motion->timed_walk_relaunch_fall&&
            next==17.f,"tap release starts ordinary falling at a positive clock");
        if(tick==35)check(offer.kind==Kind::JumpAir&&!offer.jump_motion->timed_walk_relaunch_fall&&next==3.f,
            "the permission is exhausted while the timer is still positive");
        auto refuses=[&](const NativeBorgMotionOffer& wrong,const char* why,const IBattleMvpTerrain* terrain=nullptr){
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,terrain?terrain:&view,nullptr,&frame,&wrong)&&world.tick()==tick-1&&
                world.actors()[0].position==before.position&&source.ground_loss_timer698()==timer&&source.resource()==meter&&
                world.motion_receipt_current(retained.receipt)&&view.practice_readout(world)->timed_release_push==(tick==34||tick==35)&&
                view.practice_readout(world)->second_timed_release_push==(tick==35),why);
        };
        if(offer.kind==Kind::StaticPush){
            ++timed_pushes;
            const auto& surface=offer.static_push->surface;
            check((first||second)&&next==(second?4.f:5.f)&&surface.triangle==1993&&offer.static_push->angle==8192&&
                (second?(previous.kind==Kind::StaticPush&&previous.static_push->surface.triangle==surface.triangle&&
                    previous.static_push->surface.normal==surface.normal):previous.kind==Kind::JumpAir)&&
                offer.jump_motion->timed_walk_relaunch_fall&&!offer.jump_motion->entry&&!offer.jump_motion->drift_heading&&
                before.vertical_speed<0.f&&offer.vertical_speed==f32(offer.static_push->vertical_speed*.9f)&&!offer.surface_landing,
                "successive descending responses retain one selected plane with existing damping, not a snap");
            auto wrong=offer;wrong.jump_motion->second_timed_release_push=!second;
            refuses(wrong,"native requires the second-contact tag exactly when the accepted predecessor qualifies");
            if(second){
                wrong=offer;wrong.controller_timer698=0.f;
                refuses(wrong,"a timed successor cannot borrow a zero timer");
                wrong=offer;wrong.static_push->surface.triangle=2008;
                refuses(wrong,"native reselects the actual triangle instead of accepting a substituted face");
                const RelabeledPlane relabeled(view);
                refuses(wrong,"even an independently selected face with identical geometry cannot replace the accepted first face",&relabeled);
                wrong=offer;wrong.proposed_position[1]+=1.f;
                refuses(wrong,"native checks the post-response projection independently of the matching face");
            }
        }
        if(tick==35){
            auto wrong=offer;wrong.jump_motion->timed_walk_relaunch_fall=true;wrong.jump_motion->second_timed_release_push=true;
            refuses(wrong,"fabricated history cannot extend the second-contact permission");
        }
        if(offer.kind==Kind::JumpTouchdown)hit=tick;
        if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
        accept(offer);
        const auto shown=view.practice_readout(world);
        check(shown&&shown->timed_release_push==(first||second)&&shown->second_timed_release_push==second,
            "both cues describe only the current accepted frame");
        if(first||second)check(!world.actors()[0].grounded&&source.cached_airborne87()==1&&source.resource()==9850&&!source.landings(),
            "neither response grounds, charges, or refills G-Red");
        if(tick==hit||tick==land)check(source.resource()==9850,"touchdown and landing still preserve the tap cost");
        previous=offer;
    }
    check(timed_pushes==2&&hit==101&&land==102&&source.relaunches()==1&&source.relaunch_fall_pushes()==2&&
        !source.air_dash_starts()&&!source.walk_ledge_pushes()&&source.resource()==10000&&world.actors()[0].grounded&&!source.selector(),
        "the two contacts lead to distinct touchdown, landing and full-meter idle");
    for(bool third:{false,true}){
        two_push_fixture(Mode::GroundTimedWalkReleaseSecondPush,third?-4080.66113f:-1298.54333f,
            third?-3463.83618f:-3398.46802f);
        const unsigned last=third?35:32;
        for(unsigned tick=1;tick<=last;++tick){
            auto pad=two_push_pad(tick);
            if(third){pad.dpad_down=false;pad.dpad_right=false;pad.dpad_up=pad.dpad_left=tick==6||tick==7;}
            NativeBorgMotionOffer offer;
            check(prepare(pad,&offer),"boundary route must reach its accepted contact prefix");
            if(tick==last)check(offer.kind==Kind::StaticPush&&offer.jump_motion->second_timed_release_push==third&&
                offer.static_push->surface.triangle==(third?2021u:1992u)&&offer.controller_timer698==(third?3.f:6.f),
                "last accepted contact is real, with a positive clock still available");
            accept(offer);
        }
        const auto before=world.actors()[0].position;const auto retained=pose;
        NativeBorgMotionOffer offer;
        check(!prepare({},&offer)&&error.find(third?"steep push/damping branch":"same selected face")!=std::string::npos&&
            world.tick()==last&&world.actors()[0].position==before&&source.resource()==9850&&
            source.ground_loss_timer698()==(third?3.f:6.f)&&retained.frame_scope.expired()&&!world.step({},false,&view),
            "actual different-face or third positive response refuses with no fallback or publication");
    }
    two_push_fixture(Mode::GroundTimedWalkReleaseThirdPush,-4080.66113f,-3463.83618f);
    timed_pushes=hit=land=0;previous={};
    for(unsigned tick=1;tick<=145;++tick){
        const auto before=world.actors()[0];const auto retained=pose;
        const auto timer=source.ground_loss_timer698();const auto meter=source.resource();
        GamePadState pad;pad.dpad_up=pad.dpad_left=tick==6||tick==7;pad.b=tick==20;
        NativeBorgMotionOffer offer;
        check(prepare(pad,&offer)&&source.ground_loss_timer698()==timer&&source.resource()==meter,
            "the third contact is staged without publishing clock, motion or resource");
        const bool first=tick==34,second=tick==35,third=tick==36;
        check(offer.controller_timer698==(offer.kind==Kind::GroundLossEntry?30.f:std::max(0.f,timer-1.f))&&
            (offer.jump_motion&&offer.jump_motion->second_timed_release_push)==second&&
            (offer.jump_motion&&offer.jump_motion->third_timed_release_push)==third,"actual countdown and each contact's ordinal are distinct");
        if(tick==21)check(offer.kind==Kind::JumpAir&&offer.jump_motion->entry&&offer.jump_motion->relaunch_origin,
            "ordinary released relaunch precedes the three neutral contacts");
        auto refuses=[&](const NativeBorgMotionOffer& wrong,const char* why){
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            const bool refused=!world.step({},false,&view,nullptr,&frame,&wrong);
            const auto shown=view.practice_readout(world);
            check(refused&&world.tick()==tick-1&&world.actors()[0].position==before.position&&
                source.ground_loss_timer698()==timer&&source.resource()==meter&&world.motion_receipt_current(retained.receipt)&&
                shown&&shown->third_timed_release_push==(tick==37),why);
        };
        if(offer.kind==Kind::StaticPush){
            ++timed_pushes;
            check((first||second||third)&&offer.controller_timer698==f32(38-tick)&&offer.static_push->surface.triangle==2021&&
                offer.static_push->angle==8192&&offer.jump_motion->timed_walk_relaunch_fall&&!offer.jump_motion->entry&&
                !offer.jump_motion->drift_heading&&before.vertical_speed<0.f&&!offer.surface_landing&&
                offer.vertical_speed==f32(offer.static_push->vertical_speed*.9f)&&
                (first?previous.kind==Kind::JumpAir:previous.kind==Kind::StaticPush&&
                    previous.static_push->surface.normal==offer.static_push->surface.normal),
                "three independent steep responses carry one face and plane, not a snap or callback");
            auto wrong=offer;wrong.jump_motion->third_timed_release_push=!third;
            refuses(wrong,"the third tag requires the actual accepted second contact");
            if(third){
                check(previous.jump_motion->second_timed_release_push&&timer==3.f,"second contact owns the exact timing and predecessor");
                wrong=offer;wrong.static_push->surface.triangle=1993;refuses(wrong,"another inter-gate face is not the witnessed successor");
                wrong=offer;wrong.controller_timer698=0.f;refuses(wrong,"the third cannot borrow an expired clock");
                wrong=offer;wrong.proposed_position[1]+=.5f;refuses(wrong,"the third projection is independently checked");
            }
        }
        if(tick==37){
            check(offer.kind==Kind::JumpAir&&!offer.jump_motion->timed_walk_relaunch_fall&&offer.controller_timer698==1.f,
                "permission is exhausted before the clock expires");
            auto wrong=offer;wrong.jump_motion->third_timed_release_push=true;wrong.jump_motion->timed_walk_relaunch_fall=true;
            refuses(wrong,"after three, fabricated history cannot reopen the positive-timer path");
        }
        if(offer.kind==Kind::JumpTouchdown)hit=tick;
        if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
        accept(offer);
        const auto shown=view.practice_readout(world);
        check(shown&&shown->timed_release_push==(first||second||third)&&shown->second_timed_release_push==second&&
            shown->third_timed_release_push==third,"cues describe only the current accepted ordinal");
        if(first||second||third)check(!world.actors()[0].grounded&&source.cached_airborne87()==1&&source.resource()==9850&&!source.landings(),
            "none of the three responses grounds, charges or refills");
        if(tick==hit||tick==land)check(source.resource()==9850,"distinct touchdown and landing keep the original tap charge");
        previous=offer;
    }
    check(timed_pushes==3&&hit==101&&land==102&&source.relaunches()==1&&source.relaunch_fall_pushes()==3&&
        !source.air_dash_starts()&&!source.walk_ledge_pushes()&&source.resource()==10000&&world.actors()[0].grounded&&!source.selector(),
        "the third verified response still reaches ordinary landing and full idle");
    auto third_prefix=[&](Mode mode){
        two_push_fixture(mode,-4080.66113f,-3463.83618f);
        for(unsigned tick=1;tick<=36;++tick){
            GamePadState pad;pad.dpad_up=pad.dpad_left=tick==6||tick==7;pad.b=tick==20;
            NativeBorgMotionOffer offer;
            check(prepare(pad,&offer),"actual neutral prefix through the third push");
            if(tick==36)check(offer.kind==Kind::StaticPush&&offer.jump_motion->third_timed_release_push&&
                offer.static_push->surface.triangle==2021&&offer.controller_timer698==2.f,"authentic third contact, not an expired frame");
            accept(offer);
        }
    };
    GamePadState down;down.dpad_down=true;
    third_prefix(Mode::GroundTimedWalkReleaseThirdPush);
    {
        NativeBorgMotionOffer offer;const auto retained=pose;const auto before=world.actors()[0].position;
        check(!prepare(down,&offer)&&error.find("steep push/damping branch")!=std::string::npos&&world.tick()==36&&
            source.ground_loss_timer698()==2.f&&source.resource()==9850&&world.actors()[0].position==before&&
            retained.frame_scope.expired()&&!world.step({},false,&view),"the three-push mode still refuses the exact Down request");
    }
    third_prefix(Mode::GroundTimedWalkReleaseDownPush);
    const auto drift_before=world.actors()[0];const auto drift_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    CharacterMovementHeadingFrame source_heading;
    check(native_borg_source_heading_frame(drift_frame,drift_before.facing,&source_heading),"current movement-camera proof");
    NativeGRedGroundInput mapped;
    check(source.map_input(down,&mapped,&error)&&mapped.x==0&&mapped.y==-32,"soft Down is the exact input");
    NativeBorgMotionOffer steered;
    check(prepare(down,&steered)&&steered.kind==Kind::StaticPush&&steered.jump_motion&&steered.jump_motion->down_timed_release_push&&
        !steered.jump_motion->third_timed_release_push&&!steered.jump_motion->second_timed_release_push&&
        steered.jump_motion->drift_heading==source_heading.camera5b0&&steered.static_push&&steered.static_push->surface.triangle==2011&&
        steered.static_push->angle==13653&&steered.controller_timer698==1.f&&steered.facing==drift_before.facing&&
        source.ground_loss_timer698()==2.f&&source.resource()==9850&&source.relaunch_fall_pushes()==3,
        "the staged Down contact is a separate selected plane and does not turn, ground, spend or publish");
    auto integrated=drift_before.position;f32 x=0,z=0;
    source_heading_xz(source_heading.camera5b0,&x,&z);
    integrated[0]=std::fma(4.f,x,integrated[0]);integrated[2]=std::fma(4.f,z,integrated[2]);
    const auto& fall=*steered.jump_motion;
    source_heading_xz(fall.travel5ae,&x,&z);
    integrated[0]=std::fma(1.f,f32(fall.move_speed*x),integrated[0]);
    integrated[1]=std::fma(drift_before.vertical_speed,1.f,integrated[1]);
    integrated[2]=std::fma(1.f,f32(fall.move_speed*z),integrated[2]);
    BattleMvpVec projected;
    check(steered.static_push->integrated_position==integrated&&steered.vertical_speed==f32(steered.static_push->vertical_speed*.9f)&&
        source_surface_push_position(integrated,steered.static_push->surface.normal,steered.static_push->surface.height,13653,&projected)&&
        projected==steered.proposed_position&&!steered.surface_landing,
        "four-unit drift precedes retained motion and projection; the .9 tail is not a snap");
    auto refuses_down=[&](const NativeBorgMotionOffer& wrong,const NativeBorgMovementFrame* frame,const char* why){
        const bool refused=!world.step({},false,&view,nullptr,frame,&wrong);const auto shown=view.practice_readout(world);
        check(refused&&world.tick()==36&&world.actors()[0].position==drift_before.position&&world.motion_receipt_current(pose.receipt)&&
            source.ground_loss_timer698()==2.f&&source.resource()==9850&&shown&&shown->third_timed_release_push&&!shown->down_timed_release_push,why);
    };
    refuses_down(steered,nullptr,"a fourth push needs a current camera proof");
    NativeBorgMovementFrame opposite;
    check(native_borg_movement_frame({-drift_frame.forward.x,-drift_frame.forward.z},&opposite),"opposite valid camera");
    refuses_down(steered,&opposite,"another valid camera does not prove this Down input");
    auto wrong=steered;wrong.jump_motion->drift_heading.reset();refuses_down(wrong,&drift_frame,"neutral is not the witnessed contact");
    wrong=steered;wrong.static_push->surface.triangle=2021;refuses_down(wrong,&drift_frame,"the old shallow face cannot be substituted");
    wrong=steered;wrong.static_push->angle=10240;refuses_down(wrong,&drift_frame,"the earlier inter-gate angle cannot substitute");
    wrong=steered;wrong.controller_timer698=0.f;refuses_down(wrong,&drift_frame,"cannot borrow the expiry gate");
    wrong=steered;wrong.proposed_position[1]+=.5f;refuses_down(wrong,&drift_frame,"post-response displacement is independently checked");
    accept(steered);
    check(view.practice_readout(world)->down_timed_release_push&&!view.practice_readout(world)->third_timed_release_push&&
        source.ground_loss_timer698()==1.f&&source.resource()==9850&&source.relaunch_fall_pushes()==4&&
        !world.actors()[0].grounded&&source.cached_airborne87()==1,"the matching accepted receipt publishes only the Down-contact cue");
    hit=land=0;
    while(world.tick()<145){
        const auto tick=world.tick()+1;NativeBorgMotionOffer offer;
        check(prepare({},&offer)&&offer.controller_timer698==0.f,"following neutral fall uses only existing rules at expiry");
        if(tick==38)check(offer.kind==Kind::JumpAir&&!offer.jump_motion->timed_walk_relaunch_fall,"no further timed push is inherited");
        if(offer.kind==Kind::JumpTouchdown)hit=tick;
        if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
        accept(offer);
        check(!view.practice_readout(world)->down_timed_release_push,"the steered cue clears after the one accepted contact");
        if(tick==hit||tick==land)check(source.resource()==9850,"landing remains separate from meter recovery");
    }
    check(hit==102&&land==103&&world.actors()[0].grounded&&!source.selector()&&source.resource()==10000&&
        source.relaunch_fall_pushes()==4&&!source.air_dash_starts()&&source.relaunches()==1,"fourth steered push leads to normal landing and idle");
    enum class ExpiryFollowup { DownOnExpiry,Neutral,HoldDiagonal,TurnDown,TurnDownLeft };
    for(auto followup:{ExpiryFollowup::DownOnExpiry,ExpiryFollowup::Neutral,ExpiryFollowup::HoldDiagonal,
                      ExpiryFollowup::TurnDown,ExpiryFollowup::TurnDownLeft}){
        const bool diagonal=followup!=ExpiryFollowup::DownOnExpiry;
        third_prefix(Mode::GroundTimedWalkReleaseDownPush);
        NativeBorgMotionOffer offer;
        check(prepare(down,&offer)&&offer.jump_motion->down_timed_release_push,"repeat the actual Down response");
        accept(offer);
        const auto before=world.actors()[0];const auto retained=pose;
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        auto steer=down;steer.dpad_right=diagonal;
        check(prepare(steer,&offer)&&offer.kind==Kind::StaticPush&&offer.static_push&&offer.jump_motion&&
            offer.static_push->surface.triangle==2011&&offer.static_push->angle==13653&&offer.controller_timer698==0.f&&
            !offer.jump_motion->timed_walk_relaunch_fall&&!offer.jump_motion->down_timed_release_push&&
            offer.jump_motion->drift_heading&&offer.facing==before.facing&&source.ground_loss_timer698()==1.f&&source.resource()==9850,
            "both inputs stage a zero-timer high-angle response, not an extra timed permission");
        auto travel=[&](const BattleMvpVec& end){
            const auto& cfg=world.config();const auto& from=world.actors()[0];const auto& other=world.actors()[1];
            NativeBorgContactResult result;
            NativeBorgContactRequest request{from.position,other.position,{end[0]-from.position[0],end[2]-from.position[2]},
                cfg.arena_half,cfg.actor_radius,cfg.actor_height,cfg.step_height,false,other.health!=0};
            request.vertical_end_y=end[1];
            check(native_borg_resolve_contact(request,&view,&result),"query the two native paths independently");
            return result;
        };
        const auto pre=travel(offer.static_push->integrated_position);const auto post=travel(offer.proposed_position);
        check(pre.terrain_blocked==!diagonal&&!post.terrain_blocked&&!pre.body_hit&&!post.body_hit&&!pre.bounds_hit&&!post.bounds_hit,
            "holding Down blocks only the approach; Down+Right passes both paths");
        if(!diagonal){
            check(!world.step({},false,&view,nullptr,&frame,&offer)&&world.tick()==37&&world.actors()[0].position==before.position&&
                world.motion_receipt_current(retained.receipt)&&source.ground_loss_timer698()==1.f&&source.resource()==9850&&
                view.practice_readout(world)->down_timed_release_push,
                "a clear projected endpoint cannot publish a blocked approach at expiry");
            source.reject();
            check(retained.frame_scope.expired()&&!world.step({},false,&view),"refusal cannot fall back or advance the clock");
            continue;
        }
        accept(offer);
        check(source.ground_loss_timer698()==0.f&&source.resource()==9850&&source.relaunch_fall_pushes()==5&&
            !view.practice_readout(world)->timed_release_push&&!view.practice_readout(world)->down_timed_release_push,
            "the accepted fifth response uses the existing zero-timer rule, not the timed cue");
        const auto expiry_position=world.actors()[0].position;
        const auto expiry_facing=world.actors()[0].facing;
        if(followup==ExpiryFollowup::TurnDown||followup==ExpiryFollowup::TurnDownLeft){
            auto turn=down;turn.dpad_left=followup==ExpiryFollowup::TurnDownLeft;
            const auto expiry_pose=pose;
            const auto expiry_frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(prepare(turn,&offer)&&offer.kind==Kind::StaticPush&&offer.static_push&&offer.jump_motion&&
                offer.static_push->surface.triangle==2011&&offer.static_push->angle==13653&&offer.controller_timer698==0.f&&
                offer.jump_motion->drift_heading&&!offer.jump_motion->timed_walk_relaunch_fall&&offer.facing==expiry_facing,
                "turning Down or Down+Left on 39 stages a sixth projection but not a timed contact");
            const auto approach=travel(offer.static_push->integrated_position);const auto projection=travel(offer.proposed_position);
            check(offer.static_push->surface.height>expiry_position[1]&&approach.terrain_blocked&&!projection.terrain_blocked&&
                !approach.body_hit&&!projection.body_hit&&!approach.bounds_hit&&!projection.bounds_hit,
                "a sixth staged projection clears its endpoint but its approach hits the raised side of face 2011");
            check(!world.step({},false,&view,nullptr,&expiry_frame,&offer)&&world.tick()==38&&
                world.actors()[0].position==expiry_position&&world.motion_receipt_current(expiry_pose.receipt)&&
                !expiry_pose.frame_scope.expired()&&source.ground_loss_timer698()==0.f&&source.resource()==9850&&
                source.cached_airborne87()==1&&source.relaunch_fall_pushes()==5,
                "native retains the accepted fifth push, pose and meter when the sixth approach is blocked");
            source.reject();
            check(expiry_pose.frame_scope.expired()&&!world.step({},false,&view),"the refused sixth approach cannot fall back");
            continue;
        }
        hit=land=0;
        while(world.tick()<145){
            const auto tick=world.tick()+1;
            const bool held_drift=followup==ExpiryFollowup::HoldDiagonal&&tick<=41;
            check(prepare(held_drift?steer:GamePadState{},&offer),"existing falling leaves the face and reaches ordinary landing");
            if(tick<=42)check(offer.kind==Kind::JumpAir&&!offer.static_push&&offer.jump_motion&&
                bool(offer.jump_motion->drift_heading)==held_drift&&offer.facing==expiry_facing&&
                offer.controller_timer698==0.f,"neutral or continued Down+Right drifts without another projection or a body turn");
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            if(tick==hit||tick==land)check(source.resource()==9850,"expiry response is not grounding or meter recovery");
        }
        check(hit==103&&land==104&&source.resource()==10000&&world.actors()[0].grounded&&!source.selector()&&
            source.relaunch_fall_pushes()==5&&source.relaunches()==1&&!source.air_dash_starts(),
            "the Down+Right continuation reaches distinct landing and full idle");
    }
    for(const auto& [press,hold]:std::array<std::pair<unsigned,unsigned>,5>{{{38,10},{39,10},{40,10},{41,1},{41,10}}}){
        third_prefix(Mode::GroundTimedWalkReleaseDownPush);
        NativeBorgMotionOffer offer;
        check(prepare(down,&offer)&&offer.jump_motion->down_timed_release_push,"second relaunch test starts with the genuine fourth push");
        accept(offer);
        const bool ready=press==41;
        unsigned second_entry=0,active=0;
        hit=land=0;
        for(unsigned tick=38;tick<=175;++tick){
            GamePadState pad;pad.b=tick>=press&&tick<press+hold;
            pad.dpad_down=pad.dpad_right=tick==38&&press!=38;
            const auto before=world.actors()[0];
            check(prepare(pad,&offer)&&offer.controller_timer698==0.f,"fresh Shift changes neither the expiry clock nor contact permissions");
            if(tick==38&&press!=38)check(offer.kind==Kind::StaticPush&&offer.static_push&&
                offer.static_push->surface.triangle==2011&&offer.static_push->angle==13653,"the second relaunch follows a real fifth contact");
            if(tick==38&&press==38)check(offer.kind==Kind::JumpAir&&!offer.static_push,
                "early neutral Shift does not replace the missing Down+Right drift at expiry");
            if(tick==41&&ready){
                check(offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&offer.relaunch_motion->entry&&
                    !offer.relaunch_motion->direction&&!offer.relaunch_motion->timed_walk_relaunch_tap&&
                    !offer.static_push&&!offer.relaunch_floor&&before.vertical_speed<0.f&&offer.facing==before.facing&&
                    offer.relaunch_motion->vertical_before==f32(before.vertical_speed+15.f)&&offer.vertical_speed>0.f&&
                    offer.relaunch_motion->resource_before==9850&&offer.relaunch_motion->resource_after==9700&&
                    source.resource()==9850&&source.relaunches()==1,"the fresh ready press stages plain neutral lift at the ordinary entry cost");
                second_entry=tick;
            }else check(!offer.relaunch_motion||!offer.relaunch_motion->entry,"early or held Shift never queues another relaunch entry");
            if(!active&&offer.kind==Kind::RelaunchActive)active=tick;
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            if(tick==41&&ready)check(source.resource()==9700&&source.relaunches()==2&&!world.actors()[0].grounded,
                "matching native acceptance publishes the second entry and its 150 meter charge");
            if(tick==50)check(source.resource()==(ready?(hold==1?9700:8800):9850),
                "nine held updates cost 100 each; an early hold or a released tap does not pay those costs");
            if(ready&&tick==press+hold)check(offer.kind==Kind::JumpAir&&offer.jump_motion&&offer.jump_motion->relaunch_origin,
                "releasing the second relaunch resumes ordinary falling");
        }
        check(source.relaunches()==(ready?2u:1u)&&second_entry==(ready?41u:0u)&&active==(ready&&hold==10?46u:0u)&&
            hit==(ready?(hold==1?118u:139u):(press==38?102u:103u))&&land==hit+1&&
            source.relaunch_fall_pushes()==(press==38?4u:5u)&&!source.air_dash_starts()&&
            world.actors()[0].grounded&&!source.selector()&&source.resource()==10000,
            "unbuffered early input, a fresh tap, and held ascent each reach their own complete landing and idle");
    }
    enum class LiftDashInput { HeldFlick,ReleaseFlick,SoftUp };
    for(auto input:{LiftDashInput::HeldFlick,LiftDashInput::ReleaseFlick,LiftDashInput::SoftUp}){
        third_prefix(Mode::GroundTimedWalkReleaseDownPush);
        NativeBorgMotionOffer offer,previous;
        check(prepare(down,&offer)&&offer.jump_motion->down_timed_release_push,"air dash handoff starts with the genuine fourth push");
        accept(offer);
        const bool released=input==LiftDashInput::ReleaseFlick,soft=input==LiftDashInput::SoftUp;
        const unsigned entry=47+released,handoff=entry+5,dash=handoff+1,recovery=dash+31,fall=recovery+29;
        hit=land=0;
        for(unsigned tick=38;tick<=190;++tick){
            GamePadState pad;pad.b=tick>=41&&tick<=(released?46u:47u);
            pad.dpad_down=pad.dpad_right=tick==38;
            if(tick==47){pad.dpad_up=soft;if(!soft)pad.stick_y=-80;}
            const auto before=world.actors()[0];
            check(prepare(pad,&offer)&&offer.controller_timer698==0.f,"lift or dash takes only its existing route with the timer at zero");
            if(tick==46)check(offer.kind==Kind::RelaunchActive&&offer.relaunch_motion&&offer.relaunch_motion->resource_after==9200,
                "the second relaunch reaches accepted powered ascent before the flick");
            if(tick==47&&released)check(offer.kind==Kind::JumpAir&&offer.jump_motion&&offer.jump_motion->entry&&
                offer.jump_motion->relaunch_origin&&!offer.air_motion&&source.resource()==9200,
                "same-frame Shift release takes ordinary falling before the buffered flick");
            if(tick==47&&soft)check(offer.kind==Kind::RelaunchActive&&!offer.air_motion&&offer.relaunch_motion&&
                offer.relaunch_motion->direction&&offer.relaunch_motion->resource_after==9100,
                "soft Up steers and pays active lift, not air dash");
            if(!soft){
                if(tick==entry){
                    const auto* air=offer.air_motion?&*offer.air_motion:nullptr;
                    const f32 speed=released?previous.jump_motion->speed44:previous.relaunch_motion->speed44;
                    check(offer.kind==Kind::AirDashWindupEntry&&air&&air->phase==15&&air->entry&&air->timer568==5.f&&
                        !offer.static_push&&!offer.air_dash_floor&&offer.proposed_position==before.position&&
                        air->speed_before==speed&&air->speed44==.5f*speed&&offer.vertical_speed==.5f*before.vertical_speed&&
                        air->resource_before==9200&&air->resource_after==9200&&source.resource()==9200&&!source.air_dash_starts(),
                        "direct or buffered entry is stationary and free, with half of the last accepted velocities");
                }
                if(tick==handoff)check(offer.kind==Kind::AirDashWindup&&offer.air_motion&&offer.air_motion->next_selector==16&&
                    offer.air_motion->timer568==0.f&&offer.air_motion->resource_after==8750,"the last wind-up update pays 450");
                if(tick==dash)check(offer.kind==Kind::AirDashActiveEntry&&offer.air_motion&&offer.air_motion->resource_after==8700,
                    "separate active entry pays 50 and starts the dash");
                if(tick==recovery)check(offer.kind==Kind::AirDashRecovery&&offer.air_motion&&offer.air_motion->entry&&
                    offer.air_motion->resource_after==7200&&source.air_dash_active_frames()==31,"neutral input recovers after the minimum active run");
                if(tick==fall)check(offer.kind==Kind::JumpAir&&offer.jump_motion&&offer.jump_motion->entry&&offer.jump_motion->air_dash_origin,
                    "completed recovery gives ordinary post-dash falling");
            }
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);previous=offer;
            if(!soft&&tick==entry)check(source.air_dash_starts()==1&&source.resource()==9200,
                "the matching entry receipt publishes one free wind-up, not another relaunch update");
        }
        check(hit==(soft?131u:154u+released)&&land==hit+1&&source.relaunches()==2&&source.relaunch_fall_pushes()==5&&
            source.air_dash_starts()==(soft?0u:1u)&&source.air_dash_active_frames()==(soft?0u:31u)&&
            world.actors()[0].grounded&&!source.selector()&&source.resource()==10000,
            "the direct, buffered and non-dash controls have distinct landing ticks and restore the meter normally");
    }
    enum class PaidRising { OldMode, EarlyWindup, Steered, Neutral, FiveModeSix, SixRight, SixLeft };
    for(auto run:{PaidRising::OldMode,PaidRising::EarlyWindup,PaidRising::Steered,PaidRising::Neutral,
                  PaidRising::FiveModeSix,PaidRising::SixRight,PaidRising::SixLeft}){
        const bool extended=run==PaidRising::SixRight||run==PaidRising::SixLeft;
        const bool diagonal=extended||run==PaidRising::FiveModeSix;
        const bool take_tap=extended||run==PaidRising::Neutral;
        const bool tracked=take_tap||run==PaidRising::Steered;
        const unsigned start=run==PaidRising::EarlyWindup?46:47;
        const unsigned handoff=start+(diagonal?6:5);
        third_prefix(extended?Mode::GroundRisingDiagonalWindupRelaunch:
            run==PaidRising::OldMode?Mode::GroundTimedWalkReleaseDownPush:Mode::GroundRisingPlainWindupRelaunch);
        NativeBorgMotionOffer offer;
        check(prepare(down,&offer)&&offer.jump_motion->down_timed_release_push,"paid handoff begins with the original fourth push");
        accept(offer);
        for(unsigned tick=38;tick<=handoff;++tick){
            GamePadState pad;pad.dpad_down=pad.dpad_right=tick==38;
            pad.b=tick>=41&&tick<=start;
            if(tick==start){pad.stick_y=-80;if(diagonal)pad.stick_x=run==PaidRising::SixLeft?-80:80;}
            check(prepare(pad,&offer)&&offer.controller_timer698==0.f,"verified prefix at zero ledge clock");
            if(tick>=start){
                check(offer.air_motion&&offer.air_motion->rising_relaunch_windup==tracked&&!offer.static_push&&!offer.air_dash_floor&&
                    offer.air_motion->timer568==f32(handoff-tick),"uninterrupted rising wind-up needs the opt-in for its actual length");
                if(tick==start||tick==handoff){
                    const auto from=world.actors()[0].position;const auto token=pose.receipt;
                    const auto camera=NativeBattleMvpView::movement_frame(world,1152,648);
                    auto forged=offer;forged.air_motion->rising_relaunch_windup=!tracked;
                    check(!world.step({},false,&view,nullptr,&camera,&forged)&&world.tick()==tick-1&&
                        world.actors()[0].position==from&&world.motion_receipt_current(token),
                        "native rejects missing, fabricated, or backfilled rising wind-up lineage");
                }
            }
            accept(offer);
            check(view.practice_readout(world)->plain_windup_relaunch_ready==(tracked&&tick==handoff),
                "the paid receipt alone publishes the new handoff cue");
        }
        const auto prior=world.actors()[0];const auto paid_pose=pose;
        const s16 paid_meter=start==47?8750:8850;
        check(offer.air_motion->phase==15&&offer.air_motion->next_selector==16&&offer.air_motion->resource_before-paid_meter==450&&
            source.resource()==paid_meter&&source.relaunches()==2&&source.air_dash_starts()==1&&!source.air_dash_active_frames(),
            "the real handoff has already spent 450 and has not paid an active dash update");
        GamePadState tap;tap.b=true;tap.dpad_up=run==PaidRising::Steered;
        if(!take_tap){
            check(!prepare(tap,&offer)&&error.find(run==PaidRising::Steered?"needs neutral Shift":"plain wind-up relaunch needs")!=std::string::npos&&
                world.tick()==handoff&&world.actors()[0].position==prior.position&&source.resource()==paid_meter&&
                source.relaunches()==2&&!source.air_dash_active_frames()&&paid_pose.frame_scope.expired()&&!world.step({},false,&view),
                "old mode, wrong duration, non-powered lineage, or steer cannot borrow the handoff");
            continue;
        }
        check(prepare(tap,&offer)&&offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&!offer.relaunch_motion->direction&&
            offer.relaunch_motion->vertical_before==f32(prior.vertical_speed+15.f)&&offer.vertical_speed==18.f&&
            offer.relaunch_motion->resource_before==8750&&offer.relaunch_motion->resource_after==8600&&offer.facing==prior.facing&&
            !offer.static_push&&!offer.relaunch_floor&&source.resource()==8750&&source.relaunches()==2,
            "neutral handoff stages the plain capped lift and the 150 cost without replaying wind-up or active entry");
        const auto camera=NativeBattleMvpView::movement_frame(world,1152,648);
        auto wrong=offer;wrong.relaunch_motion->direction=true;
        check(!world.step({},false,&view,nullptr,&camera,&wrong)&&world.tick()==handoff&&world.motion_receipt_current(paid_pose.receipt)&&
            source.resource()==8750&&view.practice_readout(world)->plain_windup_relaunch_ready,"native preserves the paid receipt on a fabricated steered entry");
        accept(offer);
        check(world.tick()==handoff+1&&source.resource()==8600&&source.relaunches()==3&&!source.air_dash_active_frames()&&
            !view.practice_readout(world)->plain_windup_relaunch_ready,"the matching receipt publishes the third relaunch once");
        hit=land=0;
        while(world.tick()<190){
            const auto tick=world.tick()+1;
            check(prepare({},&offer)&&offer.controller_timer698==0.f,"release after the paid handoff leads to ordinary landing");
            if(tick==handoff+2)check(offer.kind==Kind::JumpAir&&offer.jump_motion->relaunch_origin,"tap releases straight into ordinary falling");
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
        }
        check(hit==(diagonal?152u:151u)&&land==hit+1&&source.relaunches()==3&&source.relaunch_fall_pushes()==5&&source.air_dash_starts()==1&&
            !source.air_dash_active_frames()&&world.actors()[0].grounded&&!source.selector()&&source.resource()==10000,
            "the scoped paid handoff completes without buying any active dash updates");
    }
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLossRelaunch),"new relaunch scope requires GFMM9");
    std::printf("ground-loss/relaunch/actual698/conditional-clamp/latches/receipt: %u checks PASS; genuine relaunch and snap-to-wind-up routes plus old-policy/gate/lineage refusals; native proxies only\n",checks);
}
