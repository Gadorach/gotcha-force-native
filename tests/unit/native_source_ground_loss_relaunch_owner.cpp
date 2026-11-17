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
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground-loss relaunch: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"actual698 relaunch"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"same qualified shared resource model");
    struct Route {unsigned press,hold,fixture,entry,fall,touchdown,floors;s16 meter;bool air=false,direction=false;};
    const Route routes[]={{122,20,0,123,143,199,0,7950},{122,450,0,123,223,327,0,0},
        {137,2,0,138,140,148,0,9750},{146,450,1,147,247,333,6,0},
        {143,2,0,144,146,187,0,8600,true},{122,20,0,123,143,180,0,7950,false,true}};
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
        for(unsigned t=0;t<600;++t){
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
            "six600-update genuine lift/release/depletion/floor/high-scalar/directional routes return idle");
        if(!r.direction)check(neutral_over8,"new neutral inherited scalar variant really exceeds8");
        if(r.air)check(neutral_over25,"air-to-neutral-relaunch and released lineage genuinely retain scalar above25");
        if(r.direction)check(directional_clamp,"directional source clamp branch exercised");
        if(r.floors)check(positive_floor==1,"actual positive698 relaunch floor receipt exercised, not a default-zero substitution");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"timer/relaunch/pose authority retirement");
    }
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLossRelaunch),"new relaunch scope requires GFMM9");
    std::printf("ground-loss/relaunch/actual698/conditional-clamp/latches/receipt: %u checks PASS; six600-update genuine routes plus old-policy/gate/lineage refusals; native proxies only\n",checks);
}
