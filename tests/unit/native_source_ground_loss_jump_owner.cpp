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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground-loss landing jump: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem{
public:
    bool file_size(const std::string& n,gf::u64* o)override{std::error_code e;const auto s=std::filesystem::file_size(n,e);if(e||!o)return false;*o=s;return true;}
    bool read(const std::string& n,std::vector<gf::u8>& o)override{gf::u64 s=0;if(!file_size(n,&s)||s>(32u<<20))return false;std::ifstream f(n,std::ios::binary);o.assign(std::istreambuf_iterator<char>(f),{});return o.size()==s;}
    bool exists(const std::string& n)override{return std::filesystem::exists(n);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground-loss landing jump: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"retained698 landing jump"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    view.source_practice_controls();
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"same qualified resource kernel");
    struct Route {f32 x,z,query;u8 direction;u32 first,jump,second;f32 jump_timer;};
    const Route routes[]={
        {-1368.47f,-628.674f,94.339f,10,139,141,194,10},
        {-1666.667f,-333.333f,94.339f,10,114,116,169,5},
        {-1368.47f,-466.667f,94.339f,10,148,150,203,2},
        {-3941.015f,3.646f,161.944f,9,93,95,148,0}};
    auto bind=[&](const Route& r,NativeGRedGroundDashDemo::Mode mode){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={r.x,r.z};config.spawn_query_y[0]=r.query;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "fresh grounded native placement/reference on unchanged original stage; no actor/clock injection");
    };
    auto pad_at=[&](unsigned t,const Route& r,bool jump){
        GamePadState pad;if(t>=5&&!source.ground_loss_entries()){
            pad.dpad_up=r.direction&1u;pad.dpad_down=r.direction&2u;pad.dpad_left=r.direction&4u;pad.dpad_right=r.direction&8u;
        }
        if(jump&&t>=r.jump-1&&t<r.jump+14)pad.b=true;
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"retained timer/landing jump acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "matching complete receipt before VM/timer/resource/pose publication");
    };
    bind(routes[0],NativeGRedGroundDashDemo::Mode::GroundLossSurface);bool old_refused=false;
    for(unsigned t=0;t<160&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto retained=pose;const auto timer=source.ground_loss_timer698();
        if(!prepare(pad_at(t,routes[0],true),&offer)){
            check(world.tick()==140&&source.ground_loss_timer698()==timer&&timer==11&&!source.landing_jumps()&&source.landings()==1&&
                retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "earlier ground-loss surface policy still refuses jump cancellation while698 is positive");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"new jump permission does not widen the previous policy");
    for(const auto& r:routes){
        bind(r,NativeGRedGroundDashDemo::Mode::GroundLossJump);
        CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;u32 jump_tick=0,first=0,second=0;bool expired=false;BattleMvpVec landed{};
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto timer=source.ground_loss_timer698();const auto meter=source.resource();
            CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "common recovery uses previous accepted cache, not newly selected jump");
            NativeBorgMotionOffer offer;
            check(prepare(pad_at(t,r,true),&offer)&&source.ground_loss_timer698()==timer&&source.resource()==meter,"countdown/input/VM/resource staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer spans both landings and jump");
            const f32 expected_timer=offer.kind==NativeBorgMotionKind::GroundLossEntry?30.f:std::max(0.f,timer-1.f);
            check(offer.controller_timer698&&*offer.controller_timer698==expected_timer&&
                (!offer.walk_ledge||offer.walk_ledge->timer698==expected_timer)&&
                (!offer.surface_landing||offer.surface_landing->timer698==expected_timer),
                "all movement roles carry the same timer even when ground-loss flag/ledger is absent");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==before.position&&
                    source.ground_loss_timer698()==timer&&source.resource()==meter&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected,input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected),"only actual ascent spends150/100");
            if(offer.landing_target){
                jump_tick=t+1;
                check(offer.landing_target==4&&jump_tick==r.jump&&*offer.controller_timer698==r.jump_timer&&
                    offer.kind==NativeBorgMotionKind::JumpTakeoff&&offer.jump_motion&&offer.jump_motion->entry&&!offer.jump_motion->ground_loss&&
                    !offer.walk_ledge&&!offer.surface_landing&&!offer.ordered_ground&&offer.proposed_position==before.position&&
                    offer.vertical_speed==0&&expected.value736==10000,"accepted landing entry precedes jump; no early launch/cost or fabricated timer reset");
                auto wrong=offer;wrong.controller_timer698.reset();rejects(wrong,"new mode cannot omit timer witness on jump roles");
                wrong=offer;wrong.controller_timer698=expected_timer>0?0:1;rejects(wrong,"jump selection cannot clear or mint698");
                wrong=offer;wrong.jump_motion->ground_loss=true;rejects(wrong,"fresh takeoff is distinct from ground-loss program13 lineage");
                wrong=offer;wrong.landing_target=12;rejects(wrong,"jump permission does not imply positive698 dash cancellation");
            }
            if(offer.jump_motion&&offer.jump_motion->phase==4&&expected_timer>0){
                auto wrong=offer;wrong.controller_timer698=0;rejects(wrong,"retained timer remains required throughout grounded takeoff");
                check(!offer.jump_motion->ground_loss&&!offer.walk_ledge,"timer is not inferred from a copied ground-loss origin");
            }
            if(jump_tick&&timer==1&&expected_timer==0){
                expired=true;check(offer.jump_motion&&offer.jump_motion->phase==4,"genuine timer expiry during takeoff, not on jump selection");
            }
            const bool hit=offer.kind==NativeBorgMotionKind::GroundLossTouchdown||offer.kind==NativeBorgMotionKind::JumpTouchdown;
            if(hit){
                if(!first)first=t+1;else second=t+1;
                landed=offer.proposed_position;
                check(offer.surface_landing&&offer.surface_landing->timer698==expected_timer,"every callback uses actual carried timer and paired plane");
                auto wrong=offer;wrong.surface_landing->timer698=expected_timer>0?0:1;rejects(wrong,"callback cannot choose another gate after role change");
            }
            accept(offer);model=expected;
            if(offer.kind==NativeBorgMotionKind::JumpTakeoff){
                const auto readout=view.practice_readout(world);
                const std::optional<u8> displayed=expected_timer>0?std::optional{u8(expected_timer)}:std::nullopt;
                check(readout&&readout->ledge_timer_ticks==displayed&&(!displayed||
                    NativeBattleMvpView::practice_ledge_cue(*readout)=="LEDGE TIMER "+std::to_string(*displayed)+" TICKS"),
                    "a live timer can carry into takeoff without falsely showing a fall drift cue");
            }
            check(source.ground_loss_timer698()==expected_timer&&source.resource()==model.value736&&!source.surface_pushes()&&
                !source.air_dash_starts()&&!source.relaunches()&&!source.ordered_root().updates,"same timer/resource kernels and no fabricated flight/root/push");
            if(offer.controller_timer698){
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.controller_timer698=expected_timer+1;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals timer across every role");
            }
            if(t+1==r.first){
                const auto pose_before=pose;GamePadState skip;skip.b=true;NativeGRedGroundInput mapped;
                check(!source.map_input(skip,&mapped,&error)&&world.tick()==r.first&&!pose_before.frame_scope.expired(),
                    "new permission still cannot skip accepted landing entry on touchdown");
            }
            if(t+1==r.jump)check(source.movement_programs()==std::array<u16,2>{2,2}&&source.cached_airborne87()==0&&source.landing_jumps()==1,
                "ordinary shared takeoff2/cache0, not direct air launch");
            if(offer.jump_motion&&offer.jump_motion->phase==5&&offer.jump_motion->entry)
                check(t+1==r.jump+12&&source.resource()==9850&&*offer.controller_timer698==0,"actual rising4 launch/cost after retained takeoff clock expires");
            if(t+1==r.second)check(source.movement_programs()==std::array<u16,2>{6,6}&&source.resource()==9650&&source.cached_airborne87()==1,
                "second touchdown retains ordinary falling6/cache1 and real spent meter");
            if(second&&t+1==second+1)check(source.movement_programs()==std::array<u16,2>{8,8}&&source.resource()==9650&&source.cached_airborne87()==0,
                "separate second landing8 before grounded recovery");
            if(second&&t+1>second)check(world.actors()[0].position==landed,"stable separate final landing/idle");
        }
        check(first==r.first&&jump_tick==r.jump&&second==r.second&&(r.jump_timer==0||expired)&&source.landings()==2&&
            source.ground_loss_landings()==1&&source.surface_landings()==2&&source.landing_jumps()==1&&source.resource()==10000&&
            source.ground_loss_timer698()==0&&source.selector()==0&&source.movement_programs()==std::array<u16,2>{0,0}&&
            !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "four600-update genuine retained-positive/expiry-boundary landing-jump routes return idle with two real callbacks");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"timer carrier lifetime retirement");
    }
    //The new positive-timer permission is jump-only, not target12/flick or flight.
    bind(routes[0],NativeGRedGroundDashDemo::Mode::GroundLossJump);
    for(unsigned t=0;t<140;++t){NativeBorgMotionOffer offer;check(prepare(pad_at(t,routes[0],false),&offer),"positive landing dash-refusal fixture");accept(offer);}
    NativeBorgMotionOffer unsupported;GamePadState flick;flick.stick_y=-127;const auto retained=pose;
    check(!prepare(flick,&unsupported)&&world.tick()==140&&source.ground_loss_timer698()==11&&!source.landing_dashes()&&
        retained.frame_scope.expired()&&!world.step({},false,&view),"genuine positive698 landing dash stays refused before root/VM/commit");
    BattleMvpConfig edge;edge.arena_half=4500;edge.spawn[0]={1012.360657f,1403.599731f};edge.spawn_query_y[0]=482.747589f;
    check(world.start(edge,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        source.bind(files,art,pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLossJump),"fresh positive air-overlap fixture");
    for(unsigned t=0;t<7;++t){GamePadState pad;if(t==5)pad.stick_y=-127;check(prepare(pad,&unsupported),"dash ledge fixture");accept(unsupported);}
    const auto position=world.actors()[0].position;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!source.prepare(world,view.source_binding(),frame,60,0,1u<<16,&unsupported,&error)&&world.tick()==7&&world.actors()[0].position==position&&
        !source.air_dash_starts()&&pose.frame_scope.expired()&&!world.step({},false,&view),"positive698 air dash stays refused despite new timer carrier");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLossJump),"new combined mode requires GFMM9");
    std::printf("ground-loss/landing-jump/retained698/all-role-receipt: %u checks PASS; four600-update genuine routes plus old-policy/touchdown/dash/air refusals; native proxies only\n",checks);
}
