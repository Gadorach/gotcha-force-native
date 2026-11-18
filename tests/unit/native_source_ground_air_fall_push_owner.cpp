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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"combined post-air fall push: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("combined post-air fall push: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"post-air falling selected-plane push"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"same qualified resource kernels");
    struct Route {f32 x,z;unsigned dash,release,jump,direction,drift,push,touchdown;};
    const Route routes[]={{-585,-3010,45,72,55,0,3,112,113},{-580,-3010,45,72,55,0,3,112,113},
        {-606.6735f,-1373.0029f,0,0,33,0,0,82,83}};
    auto bind=[&](const Route& r,NativeGRedGroundDashDemo::Mode mode=NativeGRedGroundDashDemo::Mode::GroundAirFallPush){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={r.x,r.z};
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "fresh native grounded placement and4500 bounds; no midair/velocity/resource injection or edited stage");
    };
    auto pad_at=[&](unsigned t,const Route& r){
        GamePadState pad;pad.b=t>=5&&t<r.jump;
        if(r.dash&&t>=r.dash&&t<r.release){if(r.direction==3)pad.stick_x=-127;else pad.stick_y=-127;}
        if(source.air_dash_starts()&&(source.selector()==6||(r.drift>8&&source.selector()==17))&&!source.landings()){
            const unsigned d=r.drift%8;pad.dpad_up=d==1;pad.dpad_left=d==3;pad.dpad_right=d==4;
        }
        return pad;
    };
    auto prepare=[&](unsigned t,const Route& r,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad_at(t,r),&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"post-air fall numeric/plane acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "matching receipt before source VM/motion/resource/contact/counters/pose publication");
    };
    bind(routes[0],NativeGRedGroundDashDemo::Mode::GroundFallPush);bool old_refused=false;
    for(unsigned t=0;t<130&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto meter=source.resource();const auto retained=pose;
        if(!prepare(t,routes[0],&offer)){
            check(world.tick()==111&&source.resource()==meter&&source.air_dash_starts()==1&&!source.surface_pushes()&&!source.landings()&&
                retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "ordinary-only push policy still refuses actual post-air steep response; no fallback or publication");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"new post-air permission does not widen earlier ordinary push mode");
    for(const auto& r:routes){
        bind(r);CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;NativeBorgMotionOffer prior;
        unsigned pushes=0,push_tick=0,touchdown=0,recovery=0;BattleMvpVec landed{};
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto meter=source.resource();const auto count=source.surface_pushes();
            CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "common recovery uses previous accepted cache, not contact coincidence");
            NativeBorgMotionOffer offer;check(prepare(t,r,&offer)&&source.resource()==meter&&source.surface_pushes()==count,"private staged VM/motion/resource/projection");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"same producer across all jump/air/recovery/fall/landing roles");
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected,input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected),"exact ascent cost");
            if(offer.air_motion){
                const auto& e=*offer.air_motion;
                if(e.phase==16)check(character_air_resource_consume(expected,input.costs[3],input.recovery_delay,1,&expected),"exact active cost");
                if(e.phase==15&&!e.entry&&e.next_selector==16)check(character_air_resource_consume(expected,input.costs[2],input.recovery_delay,1,&expected),"deferred wind-up handoff cost");
                check(e.resource_after==expected.value736,"same shared air resource kernel");
                if(e.phase==17)++recovery;
            }
            auto refuses=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==before.position&&
                    source.resource()==meter&&source.surface_pushes()==count&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.static_push){
                ++pushes;push_tick=t+1;const auto& p=*offer.static_push;const auto& j=*offer.jump_motion;BattleMvpVec projected;
                check(offer.kind==NativeBorgMotionKind::StaticPush&&j.phase==6&&j.air_dash_origin==bool(r.dash)&&!j.ground_loss&&!j.relaunch_origin&&
                    !offer.air_motion&&!offer.relaunch_motion&&!offer.walk_ledge&&!offer.ordered_ground&&!offer.surface_landing&&
                    *offer.controller_timer698==0&&p.angle==11582&&p.damping==bits_to_f32(0x3f666666u)&&p.vertical_speed==-30&&
                    offer.vertical_speed==-27&&j.vertical_accel==-1&&j.planar_accel==0&&j.speed44==f32(j.move_speed*p.damping)&&
                    source_surface_push_position(p.integrated_position,p.surface.normal,p.surface.height,p.angle,&projected)&&projected==offer.proposed_position,
                    "same exact fall integration/selected plane/.9 tail with explicit zero698 caller lineage");
                if(r.dash)check(j.entry&&prior.air_motion&&prior.air_motion->phase==17&&prior.air_motion->next_selector==6&&
                    prior.air_motion->lower_done&&prior.kind==NativeBorgMotionKind::AirDashRecovery&&recovery==29&&
                    j.speed_before==prior.air_motion->speed44&&j.move_speed>20&&j.speed44>8&&
                    j.move_speed==f32(j.speed_before*bits_to_f32(0x3f7d70a4u)),
                    "genuine lower recovery done receipt precedes immediate falling13 push; no ordinary speed8 clamp");
                auto wrong=offer;wrong.jump_motion.reset();refuses(wrong,"post-air projection requires exact numeric falling proof");
                wrong=offer;wrong.static_push.reset();refuses(wrong,"falling evidence alone cannot grant steep projection");
                wrong=offer;wrong.jump_motion->air_dash_origin=!j.air_dash_origin;refuses(wrong,"post-air/ordinary lineage cannot be swapped");
                wrong=offer;wrong.jump_motion->entry=!j.entry;refuses(wrong,"first falling entry is bound to accepted recovery completion");
                wrong=offer;wrong.jump_motion->ground_loss=true;refuses(wrong,"post-air permission does not enable ground-loss pushes");
                wrong=offer;wrong.jump_motion->relaunch_origin=true;refuses(wrong,"post-air permission does not enable released-relaunch pushes");
                wrong=offer;wrong.controller_timer698=1;refuses(wrong,"post-air push remains zero698 only");
                wrong=offer;wrong.static_push->integrated_position[2]+=1;refuses(wrong,"pre-response plane endpoint must match actual motion/drift");
                wrong=offer;wrong.static_push->surface.normal={0,1,0};refuses(wrong,"UP cannot replace selected plane");
                wrong=offer;++wrong.static_push->surface.triangle;refuses(wrong,"selected plane/triangle ownership");
                wrong=offer;wrong.static_push->damping=.8f;refuses(wrong,"null-floor .8 tail cannot replace push .9");
                wrong=offer;wrong.jump_motion->speed44=j.move_speed+1;refuses(wrong,"push retains exact damped scalar");
                wrong=offer;wrong.jump_motion->vertical_accel=0;refuses(wrong,"falling gravity is not wind-up zero or active -.07");
                wrong=offer;wrong.vertical_speed=0;refuses(wrong,"push is not landing/zero48");
                wrong=offer;wrong.proposed_position[1]+=.01f;refuses(wrong,"no arbitrary endpoint correction");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashRecoveryPush;refuses(wrong,"falling permission does not grant recovery callback push");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashActivePush;refuses(wrong,"falling permission does not grant active callback push");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupEntryPush;refuses(wrong,"falling permission does not grant wind-up entry push");
            }
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown;
            if(hit){touchdown=t+1;landed=offer.proposed_position;}
            accept(offer);model=expected;
            check(source.resource()==model.value736&&!source.relaunches()&&!source.ordered_root().updates,"same resource recovery; no invented root/relaunch");
            if(offer.static_push){
                const u16 program=r.dash?13:6;
                check(!world.actors()[0].grounded&&source.selector()==6&&source.cached_airborne87()==1&&!source.landings()&&
                    source.movement_programs()==std::array<u16,2>{program,program},"push keeps program13/6 and cache; no landing/refill");
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.jump_motion->air_dash_origin=!wrong.jump_motion->air_dash_origin;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals actual post-air lineage");
                wrong=offer;wrong.static_push->surface.height+=.01f;check(!world.motion_receipt(wrong,&receipt),"receipt also seals selected plane");
            }
            if(hit)check(world.actors()[0].grounded&&source.cached_airborne87()==1&&
                source.movement_programs()==std::array<u16,2>{u16(r.dash?13:6),u16(r.dash?13:6)},"later callback keeps captured falling program/cache");
            if(touchdown&&t+1==touchdown+1)check(source.cached_airborne87()==0&&source.movement_programs()==std::array<u16,2>{8,8},"separate accepted landing8");
            if(touchdown&&t+1>touchdown)check(world.actors()[0].position==landed,"stable separate landing/idle position");
            prior=offer;
        }
        check(pushes==1&&push_tick==r.push&&touchdown==r.touchdown&&source.surface_pushes()==1&&source.dash_fall_pushes()==unsigned(bool(r.dash))&&
            source.air_dash_starts()==unsigned(bool(r.dash))&&source.landings()==1&&source.selector()==0&&source.resource()==10000&&
            source.movement_programs()==std::array<u16,2>{0,0}&&!world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "two post-air and inherited ordinary600-update original-stage routes finish idle");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"producer retirement revokes post-air pose/receipt");
    }
    //A genuine continued-fall candidate projects below a different native floor.
    const Route penetration{-570,-3030,46,73,56,0,3,114,0};bind(penetration);
    for(unsigned t=0;t<113;++t){NativeBorgMotionOffer offer;check(prepare(t,penetration,&offer),"continued-fall penetration fixture");accept(offer);}
    NativeBorgMotionOffer unsupported;const auto retained=pose;const auto position=world.actors()[0].position;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(prepare(113,penetration,&unsupported)&&unsupported.static_push&&unsupported.jump_motion->air_dash_origin&&!unsupported.jump_motion->entry&&
        !world.step({},false,&view,nullptr,&frame,&unsupported)&&world.tick()==113&&world.actors()[0].position==position&&!source.surface_pushes()&&
        !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),"native post-projection floor/penetration refusal stays atomic");
    source.reject();check(retained.frame_scope.expired(),"explicit retirement revokes penetrating candidate authority");
    //The first post-air push is valid here; the following projection is not.
    //Its candidate must use the accepted .9 scalar/velocity, not pre-push values.
    const Route repeated{5,10,44,71,54,3,9,113,0};bind(repeated);
    NativeBorgMotionOffer pushed;
    for(unsigned t=0;t<113;++t){
        NativeBorgMotionOffer offer;check(prepare(t,repeated,&offer),"accepted-push/continued-penetration fixture");accept(offer);
        if(offer.static_push)pushed=offer;
    }
    const auto accepted_pose=pose;const auto accepted_position=world.actors()[0].position;
    const auto accepted_meter=source.resource();const auto repeated_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(pushed.static_push&&pushed.jump_motion->entry&&source.surface_pushes()==1&&source.dash_fall_pushes()==1&&
        source.movement_programs()==std::array<u16,2>{13,13}&&source.cached_airborne87()==1&&!source.landings(),
        "first genuine projection is accepted and remains post-air falling13");
    check(prepare(113,repeated,&unsupported)&&unsupported.static_push&&unsupported.jump_motion->air_dash_origin&&
        !unsupported.jump_motion->entry&&unsupported.jump_motion->speed_before==pushed.jump_motion->speed44&&
        unsupported.jump_motion->move_speed==f32(pushed.jump_motion->speed44*bits_to_f32(0x3f7d70a4u))&&
        unsupported.jump_motion->speed44==f32(unsupported.jump_motion->move_speed*bits_to_f32(0x3f666666u))&&
        unsupported.jump_motion->vertical_before==pushed.vertical_speed&&unsupported.static_push->vertical_speed==-28&&
        unsupported.vertical_speed==f32(-28.f*bits_to_f32(0x3f666666u)),
        "continued candidate integrates previously accepted damped scalar/velocity before another .9 tail");
    check(!world.step({},false,&view,nullptr,&repeated_frame,&unsupported)&&world.tick()==113&&
        world.actors()[0].position==accepted_position&&source.resource()==accepted_meter&&source.surface_pushes()==1&&
        source.dash_fall_pushes()==1&&!source.landings()&&!accepted_pose.frame_scope.expired()&&world.motion_receipt_current(accepted_pose.receipt),
        "rejecting second projection preserves first accepted push/counters/resource/current pose; no native fallback");
    source.reject();check(accepted_pose.frame_scope.expired(),"explicit retirement after second-projection refusal");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundAirFallPush),"post-air push requires GFMM9");
    std::printf("combined/post-air-fall/push/recovery-lineage/plane/resource/receipt: %u checks PASS; three600-update genuine routes plus old-policy/caller/penetration refusals; native proxies only\n",checks);
}
