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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"combined ordinary fall push: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("combined ordinary fall push: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"ordinary fall selected-plane push"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"same qualified resource kernels");
    struct Route {unsigned release,push,touchdown;f32 x,z;bool moving;};
    const Route routes[]={{33,82,83,-606.6735f,-1373.0029f,false},{38,90,91,-606.6735f,-1373.0029f,false},
        {33,82,83,-626.6735f,-1393.0029f,true}};
    auto bind=[&](const Route& r,NativeGRedGroundDashDemo::Mode mode){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={r.x,r.z};
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "fresh native grounded original-stage placement and4500 bounds; no midair/velocity injection");
    };
    auto pad_at=[](unsigned t,const Route& r){
        GamePadState pad;pad.b=t>=5&&t<r.release;if(r.moving&&t==32)pad.dpad_up=true;return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"ordinary push numeric/plane acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "matching receipt before source motion/VM/resource/contact/counter/pose commit");
    };
    bind(routes[0],NativeGRedGroundDashDemo::Mode::GroundLossRelaunch);bool old_refused=false;
    for(unsigned t=0;t<100&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto meter=source.resource();const auto retained=pose;
        if(!prepare(pad_at(t,routes[0]),&offer)){
            check(world.tick()==81&&source.resource()==meter&&!source.surface_pushes()&&!source.landings()&&
                retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "prior combined relaunch policy still refuses ordinary steep response before publication/fallback");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"new push permission does not widen earlier controller");
    for(const auto& r:routes){
        bind(r,NativeGRedGroundDashDemo::Mode::GroundFallPush);
        CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;unsigned push_tick=0,touchdown=0,pushes=0;bool nonzero=false;BattleMvpVec landed{};
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto meter=source.resource();const auto count=source.surface_pushes();
            CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "common resource recovery uses previous cache, not projection/landing coincidence");
            NativeBorgMotionOffer offer;
            check(prepare(pad_at(t,r),&offer)&&source.resource()==meter&&source.surface_pushes()==count,"VM/motion/plane/resource/counters staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer through jump/fall/push/callback/landing");
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected,input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected),"exact actual ascent costs");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==before.position&&
                    source.resource()==meter&&source.surface_pushes()==count&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.static_push){
                ++pushes;push_tick=t+1;const auto& e=*offer.static_push;const auto& j=*offer.jump_motion;BattleMvpVec projected;
                check(offer.kind==NativeBorgMotionKind::StaticPush&&j.phase==6&&!j.ground_loss&&!j.air_dash_origin&&!j.relaunch_origin&&
                    offer.controller_timer698&&*offer.controller_timer698==0&&!offer.walk_ledge&&!offer.air_motion&&!offer.relaunch_motion&&
                    !offer.surface_landing&&!offer.air_dash_floor&&!offer.relaunch_floor&&e.angle==11582&&e.angle>10240&&
                    e.damping==bits_to_f32(0x3f666666u)&&!e.relaunch_acceleration&&e.vertical_speed==-30&&offer.vertical_speed==-27&&
                    j.vertical_accel==-1&&j.planar_accel==0&&j.speed44==f32(j.move_speed*e.damping)&&
                    source_surface_push_position(e.integrated_position,e.surface.normal,e.surface.height,e.angle,&projected)&&projected==offer.proposed_position,
                    "ordinary zero698 selected-plane projection/.9 scalar-velocity tail preserves gravity and distinct caller lineage");
                nonzero=j.move_speed>0;
                auto wrong=offer;wrong.static_push.reset();rejects(wrong,"projection requires paired pre-response evidence");
                wrong=offer;wrong.jump_motion.reset();rejects(wrong,"combined projection requires actual fall numeric proof");
                wrong=offer;wrong.static_push->integrated_position[0]+=1;rejects(wrong,"plane response cannot fabricate pre-motion endpoint");
                wrong=offer;wrong.static_push->vertical_speed=-29;rejects(wrong,"post-gravity velocity must precede .9 response");
                wrong=offer;wrong.static_push->damping=.8f;rejects(wrong,"floor .8 tail cannot replace steep .9");
                wrong=offer;wrong.static_push->surface.normal={0,1,0};rejects(wrong,"guessed UP normal cannot mint selected-plane projection");
                wrong=offer;++wrong.static_push->surface.triangle;rejects(wrong,"selected triangle identity stays paired");
                wrong=offer;wrong.proposed_position[0]+=.01f;rejects(wrong,"projection is exact, not arbitrary native clipping");
                wrong=offer;wrong.vertical_speed=0;rejects(wrong,"projection is not landing or wind-up zero48");
                wrong=offer;wrong.jump_motion->air_dash_origin=true;rejects(wrong,"ordinary permission does not grant post-air falling pushes");
                wrong=offer;wrong.jump_motion->relaunch_origin=true;rejects(wrong,"ordinary permission does not grant released-relaunch pushes");
                wrong=offer;wrong.jump_motion->ground_loss=true;rejects(wrong,"ordinary permission does not grant ground-loss pushes");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashRecoveryPush;rejects(wrong,"ordinary permission does not grant recovery push");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashActivePush;rejects(wrong,"ordinary permission does not grant active push");
                wrong=offer;wrong.kind=NativeBorgMotionKind::RelaunchEntryPush;rejects(wrong,"ordinary permission does not grant relaunch push");
                wrong=offer;wrong.controller_timer698=1;rejects(wrong,"new push permission is zero698 only");
                if(nonzero){wrong=offer;wrong.jump_motion->speed44=j.move_speed;rejects(wrong,"scalar .9 damping cannot be omitted while vertical damping matches");}
            }
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown;
            if(hit){touchdown=t+1;landed=offer.proposed_position;}
            accept(offer);model=expected;
            check(source.resource()==model.value736&&!source.air_dash_starts()&&!source.relaunches()&&!source.ordered_root().updates,
                "same shared resource kernel and no invented flight/relaunch/dash root");
            if(offer.static_push){
                check(!world.actors()[0].grounded&&source.cached_airborne87()==1&&!source.landings()&&source.selector()==6&&
                    source.movement_programs()==std::array<u16,2>{6,6},"accepted projection stays falling6/airborne; no callback/refill/cache reset");
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.static_push->damping=.8f;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals plane and response");
                wrong=offer;wrong.jump_motion->speed44+=.01f;check(!world.motion_receipt(wrong,&receipt),"receipt also seals actual retained fall scalar");
            }
            if(hit)check(world.actors()[0].grounded&&source.cached_airborne87()==1&&source.movement_programs()==std::array<u16,2>{6,6},
                "later real callback remains separate from projection");
            if(touchdown&&t+1==touchdown+1)check(source.cached_airborne87()==0&&source.movement_programs()==std::array<u16,2>{8,8},"separate accepted landing8");
            if(touchdown&&t+1>touchdown)check(world.actors()[0].position==landed,"stable separate landing/idle endpoint");
        }
        check(pushes==1&&push_tick==r.push&&touchdown==r.touchdown&&nonzero==r.moving&&source.surface_pushes()==1&&source.landings()==1&&
            source.selector()==0&&source.resource()==10000&&source.movement_programs()==std::array<u16,2>{0,0}&&
            !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "three600-update genuine neutral/longer-rise/nonzero-scalar ordinary pushes return idle");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"ordinary-push pose/receipt retirement");
    }
    //The source can plan this projection, but the native post-plane/penetration
    //boundary must reject it and preserve the previously accepted frame.
    Route penetration{31,79,0,-606.6735f,-1373.0029f,false};bind(penetration,NativeGRedGroundDashDemo::Mode::GroundFallPush);
    for(unsigned t=0;t<78;++t){NativeBorgMotionOffer offer;check(prepare(pad_at(t,penetration),&offer),"penetrating projection fixture");accept(offer);}
    NativeBorgMotionOffer unsupported;const auto retained=pose;const auto position=world.actors()[0].position;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(prepare(pad_at(78,penetration),&unsupported)&&unsupported.static_push&&
        !world.step({},false,&view,nullptr,&frame,&unsupported)&&world.tick()==78&&world.actors()[0].position==position&&
        !source.surface_pushes()&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),
        "post-projection native floor/penetration refusal remains atomic; no forced UP floor or hidden clipping");
    source.reject();check(retained.frame_scope.expired(),"explicit retirement revokes rejected projection authority");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundFallPush),"combined ordinary push requires GFMM9");
    std::printf("combined/ordinary-fall/push/numeric-plane/resource/receipt: %u checks PASS; three600-update genuine routes plus old-policy/caller/penetration refusals; native proxies only\n",checks);
}
