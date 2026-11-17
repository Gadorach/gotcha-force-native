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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground-loss air: %s (%s)\n",why,error.c_str());std::abort();}}
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
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground-loss air: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"owned698 air controller"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"same exact shared resource profile");
    auto bind=[&](unsigned fixture,NativeGRedGroundDashDemo::Mode mode){
        BattleMvpConfig config;config.arena_half=4500;
        if(fixture==2)config.spawn[0]={1012.360657f,1403.599731f};
        else config.spawn[0]={-1368.47f,fixture?-466.667f:-628.674f};
        config.spawn_query_y[0]=fixture==2?482.747589f:94.339f;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "declared native grounded placement/reference on unchanged original stage; no midair/clock/resource injection");
    };
    auto pad_at=[&](unsigned t,unsigned fixture,unsigned flick,unsigned direction,unsigned hold){
        GamePadState pad;
        if(fixture==2){if(t==5)pad.stick_y=-127;}
        else if(t>=5&&!source.ground_loss_entries()){pad.dpad_down=true;pad.dpad_right=true;}
        if(t>=flick&&t<flick+hold&&!source.landings()){
            if(direction==0)pad.stick_y=-127;
            if(direction==1)pad.stick_y=127;
            if(direction==2)pad.stick_x=127;
            if(direction==3)pad.stick_x=-127;
        }return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"owned-timer air/contact acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "complete matching native receipt before VM/resource/timer/contact/pose publication");
    };
    bind(0,NativeGRedGroundDashDemo::Mode::GroundLossLanding);bool old_refused=false;
    for(unsigned t=0;t<140&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto retained=pose;
        if(!prepare(pad_at(t,0,122,2,50),&offer)){
            check(world.tick()==122&&source.ground_loss_timer698()==29&&!source.air_dash_starts()&&source.resource()==10000&&
                retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "prior landing-input mode still refuses positive698 air dash before spending/publication");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"new air permission does not widen old controller");
    bind(2,NativeGRedGroundDashDemo::Mode::GroundLossLanding);
    for(unsigned t=0;t<36;++t){NativeBorgMotionOffer offer;check(prepare(pad_at(t,2,36,0,37),&offer),"prior exact-expiry fixture");accept(offer);}
    NativeBorgMotionOffer boundary;const auto retained=pose;
    check(prepare(pad_at(36,2,36,0,37),&boundary)&&boundary.air_motion&&boundary.controller_timer698&&*boundary.controller_timer698==0,
        "earlier mode stages actual expiry but retains its conservative prior-frame refusal");
    const auto boundary_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&boundary_frame,&boundary)&&world.tick()==36&&!retained.frame_scope.expired(),
        "older native guard remains unchanged at previous1/current0 boundary");
    source.reject();
    struct Route {unsigned fixture,flick,direction,hold,entry,active,recovery,touchdown;s16 meter;};
    const Route routes[]={{0,122,2,50,123,130,173,185,7400},{0,122,3,50,123,132,173,183,7500},
        {0,122,2,37,123,130,161,176,8000},{0,124,2,50,125,132,175,186,7400},{2,36,0,37,37,43,0,73,8000}};
    for(const auto& r:routes){
        bind(r.fixture,NativeGRedGroundDashDemo::Mode::GroundLossAir);
        CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;unsigned entry=0,active=0,recovery=0,touchdown=0;bool positive_entry=false;BattleMvpVec landed{};
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto prior_pose=pose;const auto timer=source.ground_loss_timer698();const auto meter=source.resource();
            CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "common recovery uses previous accepted cache, not a newly selected flight/contact");
            NativeBorgMotionOffer offer;check(prepare(pad_at(t,r.fixture,r.flick,r.direction,r.hold),&offer)&&source.resource()==meter&&
                source.ground_loss_timer698()==timer,"air admission/VM/resource/timer/contact remain staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"same producer spans ground loss and air dash");
            const f32 current=offer.kind==NativeBorgMotionKind::GroundLossEntry?30.f:std::max(0.f,timer-1.f);
            check(offer.controller_timer698&&*offer.controller_timer698==current&&
                (!offer.air_dash_floor||offer.air_dash_floor->timer698==current)&&
                (!offer.surface_landing||offer.surface_landing->timer698==current),"every contact role uses the same actual timer witness");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==before.position&&
                    source.resource()==meter&&source.ground_loss_timer698()==timer&&!prior_pose.frame_scope.expired()&&world.motion_receipt_current(prior_pose.receipt),why);
            };
            if(offer.air_motion){
                const auto& e=*offer.air_motion;
                if(e.phase==15&&e.entry){
                    entry=t+1;positive_entry=current>0;
                    check(offer.proposed_position==before.position&&offer.vertical_speed==f32(before.vertical_speed*.5f)&&
                        e.speed_before>=e.speed44&&e.resource_before==10000&&e.resource_after==10000&&!offer.walk_ledge&&!e.callback,
                        "real accepted ground-loss history enters wind-up without moving/spending/resetting698");
                    auto wrong=offer;wrong.controller_timer698=current>0?0:1;rejects(wrong,"air selection cannot clear or mint the actual countdown");
                    wrong=offer;wrong.air_motion.reset();rejects(wrong,"new air permission cannot omit phase/scalar/resource evidence");
                    wrong=offer;wrong.proposed_position[1]+=offer.vertical_speed;rejects(wrong,"wind-up entry does not integrate half velocity");
                    wrong=offer;wrong.air_motion->resource_after-=450;rejects(wrong,"positive698 does not spend wind-up handoff cost early");
                }
                if(e.phase==16){
                    check(character_air_resource_consume(expected,input.costs[3],input.recovery_delay,1,&expected),"active50 before motion/contact");
                    if(e.entry)active=t+1;
                }
                if(e.phase==15&&!e.entry&&e.next_selector==16)
                    check(character_air_resource_consume(expected,input.costs[2],input.recovery_delay,1,&expected),"handoff450 after movement, before contact");
                if(e.phase==17&&!recovery)recovery=t+1;
                if(e.phase==17&&current>0)check(!e.drift_heading,"verified617E4 suppresses direct drift while698 is positive");
                check(e.resource_after==expected.value736,"native air proof follows the same exact shared resource kernel");
            }
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown;
            if(hit){touchdown=t+1;landed=offer.proposed_position;check(offer.surface_landing&&offer.surface_landing->timer698==current,"actual callback retains current timer/plane proof");}
            accept(offer);model=expected;
            check(source.resource()==model.value736&&source.ground_loss_timer698()==current&&!source.surface_pushes()&&!source.relaunches(),
                "matching receipt commits timer/resource without push/relaunch/fallback");
            if(offer.air_motion){
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.controller_timer698=current+1;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals actual timer during air VM/motion phases");
            }
            if(hit)check(world.actors()[0].grounded&&source.cached_airborne87()==1&&source.resource()==r.meter&&
                source.movement_programs()==std::array<u16,2>{u16(r.recovery?12:11),u16(r.recovery?12:11)},"real ready callback precedes accepted landing8/cache reset");
            if(touchdown&&t+1==touchdown+1)check(source.cached_airborne87()==0&&source.resource()==r.meter&&
                source.movement_programs()==std::array<u16,2>{8,8},"separate landing entry, no forced grounded refill");
            if(touchdown&&t+1>touchdown)check(world.actors()[0].position==landed,"stable later landing and idle");
        }
        check(entry==r.entry&&active==r.active&&recovery==r.recovery&&touchdown==r.touchdown&&positive_entry==(r.fixture!=2)&&
            source.air_dash_starts()==1&&source.ground_loss_entries()==1&&!source.ground_loss_landings()&&source.landings()==1&&
            source.resource()==10000&&source.ground_loss_timer698()==0&&source.selector()==0&&source.movement_programs()==std::array<u16,2>{0,0}&&
            !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "five600-update genuine positive/expiry-boundary air routes return idle without inventing ground-loss touchdown");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"air/timer/pose authority retirement");
    }
    //Actual positive-time snaps are verified on this bounded route. A later
    //raised-side entry is still fatal; do not flatten terrain to finish it.
    bind(1,NativeGRedGroundDashDemo::Mode::GroundLossAir);unsigned positive_snaps=0;bool side_refused=false;
    for(unsigned t=0;t<180&&!side_refused;++t){
        NativeBorgMotionOffer offer;const auto before=world.actors()[0];const auto prior=pose;const auto timer=source.ground_loss_timer698();
        if(!prepare(pad_at(t,1,147,0,50),&offer)){
            check(world.tick()==157&&positive_snaps==2&&source.ground_loss_timer698()==timer&&
                error.find("raised-surface/side-entry")!=std::string::npos&&prior.frame_scope.expired()&&!world.step({},false,&view),
                "later unknown raised-side response still refuses and retires authority after genuine positive snaps");
            side_refused=true;break;
        }
        if(offer.air_dash_floor&&offer.air_dash_floor->timer698>0){
            ++positive_snaps;const auto& e=*offer.air_dash_floor;
            check(offer.air_motion&&offer.air_motion->phase==15&&offer.vertical_speed==0&&e.timer698==*offer.controller_timer698&&
                e.angle<=6144&&!offer.air_motion->callback,"positive wind-up ret1 zero48 uses actual stricter AE gate, not landing");
            auto wrong=offer;wrong.air_dash_floor->timer698=0;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==before.position&&!prior.frame_scope.expired(),
                "floor evidence cannot downgrade positive698 to declared zero");
            AnglePlane angled(view);wrong=offer;wrong.air_dash_floor->surface.normal=angled.normal;
            check(source_surface_angle(angled.normal,&wrong.air_dash_floor->angle)&&wrong.air_dash_floor->angle>6144&&wrong.air_dash_floor->angle<=10240,
                "numeric inter-gate floor fixture");
            check(!world.step({},false,&angled,nullptr,&frame,&wrong)&&world.tick()==t&&!prior.frame_scope.expired(),
                "coherent45-degree null-floor metadata cannot borrow AC during positive698");
        }
        accept(offer);
    }
    check(side_refused,"new permission does not guess unsupported geometry");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLossAir),"timer-bound air dash requires GFMM9");
    std::printf("ground-loss/air/actual698/contact-gate/resource/receipt: %u checks PASS; five600-update genuine routes plus old-policy/expiry/positive-floor/side refusals; native proxies only\n",checks);
}
