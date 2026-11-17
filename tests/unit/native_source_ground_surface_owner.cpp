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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground surface: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem{
public:
    bool file_size(const std::string& n,gf::u64* o)override{std::error_code e;const auto s=std::filesystem::file_size(n,e);if(e||!o)return false;*o=s;return true;}
    bool read(const std::string& n,std::vector<gf::u8>& o)override{gf::u64 s=0;if(!file_size(n,&s)||s>(32u<<20))return false;std::ifstream f(n,std::ios::binary);o.assign(std::istreambuf_iterator<char>(f),{});return o.size()==s;}
    bool exists(const std::string& n)override{return std::filesystem::exists(n);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
struct HeightOnly final:gf::IBattleMvpTerrain{
    const gf::NativeBattleMvpView& view;
    explicit HeightOnly(const gf::NativeBattleMvpView& v):view(v){}
    bool height(gf::f32 x,gf::f32 z,gf::f32 y,gf::f32* out)const override{return view.height(x,z,y,out);}
};
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground surface: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"combined paired surface landing"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"shared resource operands");
    //A native proxy snap must not hide an intermediate body sweep. Numeric
    //contact fixture only, not an injected source actor or original solid ACK.
    NativeBorgContactRequest sweep;sweep.position={-100,400,0};sweep.other_position={0,0,0};sweep.delta={100,0};
    sweep.height=200;sweep.grounded=false;sweep.other_active=true;sweep.vertical_end_y=100;NativeBorgContactResult checked;
    check(native_borg_resolve_contact(sweep,nullptr,&checked)&&checked.body_hit,"pre-snap downward path can hit a body");
    sweep.vertical_end_y=300;
    check(native_borg_resolve_contact(sweep,nullptr,&checked)&&!checked.body_hit,"higher final snap endpoint cannot substitute for the pre-snap sweep");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode,const BattleMvpConfig& config=BattleMvpConfig{}){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "fresh native ground seed on original stage; no airborne injection or replacement producer");
    };
    auto pad_at=[](unsigned t,unsigned route){
        GamePadState pad;pad.b=t>=5&&t<(route<2?21u:31u);
        if(route<2&&t>=48&&t<(route==0?108u:85u))pad.stick_y=127;
        if(route==4&&t>=60&&t<100)pad.b=true;
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"native paired surface acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "matching complete receipt before VM/resource/contact/counter/pose publication");
    };
    for(unsigned route=0;route<2;++route){
        bind(NativeGRedGroundDashDemo::Mode::GroundWindupEntryFloor);bool refused=false;
        for(unsigned t=0;t<140&&!refused;++t){
            const auto retained=pose;const auto position=world.actors()[0].position;const auto meter=source.resource();
            NativeBorgMotionOffer offer;
            if(!prepare(pad_at(t,route),&offer)){
                check(route==1&&world.tick()==104&&source.resource()==meter&&!source.landings()&&
                    retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                    "older recovery path still refuses nonflat contact in source and retires authority without fallback");
                refused=true;
            }else if(!route&&offer.kind==NativeBorgMotionKind::JumpTouchdown){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&offer)&&world.tick()==107&&world.actors()[0].position==position&&
                    source.resource()==meter&&!source.landings()&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),
                    "older active callback stages contact but native flat-only policy rejects it without retiring the prior frame");
                source.reject();check(retained.frame_scope.expired(),"explicit rejection retires the old pending producer");
                refused=true;
            }else accept(offer);
        }
        check(refused,"new surface permission does not widen previous controller");
    }
    for(unsigned route=0;route<5;++route){
        BattleMvpConfig config;if(route==2||route==4)config.spawn[0]={-619.2617f,169.4453f}; //declared original shallow triangle, fresh grounded seed
        bind(NativeGRedGroundDashDemo::Mode::GroundSurface,config);
        CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;u32 touchdown=0;BattleMvpVec landed{};
        const u32 expected_tick=route==0?108:route==1?105:route==4?171:79;
        const s16 expected_angle=route==0?7187:route==1?562:route==3?0:4448;
        const s16 expected_meter=route==0?6600:route==1?7550:route==4?4500:8550;
        const u16 captured_program=route==0?11:route==1?12:route==4?21:6;
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto meter=source.resource();
            const auto landings=source.landings();CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "common recovery follows accepted airborne cache, not proposed landing");
            NativeBorgMotionOffer offer;
            check(prepare(pad_at(t,route),&offer)&&source.resource()==meter&&source.landings()==landings,
                "surface selection, VM, resource and landing counters staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer through every recovered movement/contact phase");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);const auto tick=world.tick();
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==before.position&&
                    source.resource()==meter&&source.landings()==landings&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected,input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected),"exact jump costs");
            if(offer.relaunch_motion)check(character_air_resource_consume(expected,input.costs[offer.relaunch_motion->entry?0:1],input.recovery_delay,1,&expected),"exact relaunch costs");
            if(offer.air_motion){
                if(offer.air_motion->phase==16)check(character_air_resource_consume(expected,input.costs[3],input.recovery_delay,1,&expected),"active cost before movement/contact");
                if(offer.air_motion->phase==15&&!offer.air_motion->entry&&offer.air_motion->next_selector==16)
                    check(character_air_resource_consume(expected,input.costs[2],input.recovery_delay,1,&expected),"handoff cost after movement, before contact");
            }
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown;
            check(bool(offer.surface_landing)==hit,"new policy requires complete selected surface metadata even for flat callback landing");
            if(hit){
                touchdown=t+1;landed=offer.proposed_position;const auto& e=*offer.surface_landing;NativeBorgStaticSurface selected;
                check(touchdown==expected_tick&&e.angle==expected_angle&&e.angle>=0&&e.angle<=10240&&
                    view.static_surface(e.integrated_position[0],e.integrated_position[2],before.position[1],&selected)&&selected==e.surface&&
                    e.integrated_position[1]<=selected.height&&before.position[1]>=selected.height-.01f&&before.vertical_speed<0&&
                    offer.proposed_position==BattleMvpVec{e.integrated_position[0],selected.height,e.integrated_position[2]}&&offer.vertical_speed==0&&
                    !offer.static_push&&!offer.air_dash_floor&&!offer.relaunch_floor,"exact original selected height/normal/triangle, callback angle and pre-response integration");
                if(!route){NativeBorgStaticSurface previous;check(view.static_surface(before.position[0],before.position[2],before.position[1],&previous)&&
                    previous.height!=selected.height&&!native_borg_flat_surface(selected)&&std::abs(selected.height-82.003227f)<.0001f,
                    "genuine active dash reaches unequal-height nonflat plane rather than borrowing old UP/equal-height test");}
                auto wrong=offer;wrong.surface_landing.reset();rejects(wrong,"missing selected surface cannot fall back to old flat authority");
                wrong=offer;wrong.surface_landing->surface.normal={0,1,0};if(expected_angle)rejects(wrong,"guessed UP normal cannot replace the selected plane");
                wrong=offer;++wrong.surface_landing->surface.triangle;rejects(wrong,"height-only or mismatched triangle cannot certify landing");
                wrong=offer;wrong.surface_landing->surface.height+=1;rejects(wrong,"selected height cannot be edited independently");
                wrong=offer;++wrong.surface_landing->angle;rejects(wrong,"quantized source angle must match the selected normal");
                wrong=offer;wrong.surface_landing->angle=16384;rejects(wrong,"steep response cannot be relabelled as landing");
                wrong=offer;wrong.surface_landing->integrated_position[0]+=1;rejects(wrong,"pre-contact endpoint must match actual jump/air integration");
                wrong=offer;wrong.surface_landing->vertical_speed=0;rejects(wrong,"callback zero cannot substitute for pre-response gravity/clamp");
                wrong=offer;wrong.surface_landing->acceleration=0;rejects(wrong,"pre-response acceleration must match the actual phase");
                wrong=offer;wrong.proposed_position[1]+=1;rejects(wrong,"callback must snap exactly to selected height");
                wrong=offer;wrong.kind=NativeBorgMotionKind::JumpLanding;rejects(wrong,"surface callback cannot skip separate landing entry");
                wrong=offer;wrong.kind=NativeBorgMotionKind::StaticPush;rejects(wrong,"surface landing permission does not grant steep push");
                if(offer.air_motion){wrong=offer;wrong.air_motion->callback=false;rejects(wrong,"surface contact cannot bypass air callback readiness");}
                HeightOnly height_only(view);const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&height_only,nullptr,&frame,&offer)&&world.tick()==t&&!retained.frame_scope.expired(),
                    "height-only terrain cannot mint paired-surface callback authority");
            }
            accept(offer);model=expected;
            check(source.resource()==model.value736&&!source.surface_pushes()&&!source.ordered_root().updates,"same shared costs/recovery and no invented root/push");
            if(hit){
                check(world.actors()[0].grounded&&source.cached_airborne87()==1&&source.resource()==expected_meter&&source.surface_landings()==1&&
                    source.last_surface_angle()==expected_angle&&source.movement_programs()==std::array<u16,2>{captured_program,captured_program},
                    "actual accepted callback touchdown keeps captured air/fall program and old cache/resource");
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.surface_landing->surface.height+=.01f;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals complete paired plane and pre-response metadata");
                NativeBattleMvp copy=world;check(!copy.motion_receipt_current(receipt),"copied world cannot borrow surface receipt");
            }
            if(touchdown&&t+1==touchdown+1)check(source.cached_airborne87()==0&&source.resource()==expected_meter&&
                source.movement_programs()==std::array<u16,2>{8,8},"separate accepted landing8 before grounded resource recovery");
            if(touchdown&&t+1>touchdown)check(world.actors()[0].position==landed,"stable shallow/flat landing and idle endpoint");
        }
        check(touchdown==expected_tick&&source.landings()==1&&source.surface_landings()==1&&source.selector()==0&&source.resource()==10000&&
            source.movement_programs()==std::array<u16,2>{0,0}&&source.air_dash_starts()==unsigned(route<2)&&source.relaunches()==unsigned(route==4)&&
            !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "five600-update genuine active/recovery/jump/relaunch/flat routes finish in shared idle with one real surface landing");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"retirement revokes surface pose/receipt leases");
    }
    BattleMvpConfig steep;steep.spawn[0]={-606.6735f,-1373.0029f};bind(NativeGRedGroundDashDemo::Mode::GroundSurface,steep);bool steep_refused=false;
    for(unsigned t=0;t<100&&!steep_refused;++t){
        GamePadState pad;pad.b=t>=5&&t<33;NativeBorgMotionOffer offer;const auto retained=pose;const auto meter=source.resource();
        if(!prepare(pad,&offer)){
            check(world.tick()==81&&source.resource()==meter&&!source.landings()&&!source.surface_pushes()&&
                retained.frame_scope.expired()&&!world.step({},false,&view),"genuine steep triangle remains refused before push/landing/refill/fallback");
            steep_refused=true;
        }else accept(offer);
    }
    check(steep_refused,"surface permission does not enable legacy steep response");
    BattleMvpConfig edge;edge.arena_half=4500;edge.spawn[0]={1012.360657f,1403.599731f};edge.spawn_query_y[0]=482.747589f;
    bind(NativeGRedGroundDashDemo::Mode::GroundSurface,edge);
    for(unsigned t=0;t<7;++t){GamePadState pad;if(t==5)pad.stick_y=-127;NativeBorgMotionOffer offer;check(prepare(pad,&offer),"ground-loss timer fixture");accept(offer);}
    const auto position=world.actors()[0].position;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer unsupported;
    check(!source.prepare(world,view.source_binding(),frame,60,0,1u<<16,&unsupported,&error)&&world.tick()==7&&world.actors()[0].position==position&&
        !source.air_dash_starts()&&pose.frame_scope.expired()&&!world.step({},false,&view),"surface permission does not resolve positive ground-loss698 overlap");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundSurface),"combined surface controller requires GFMM9");
    std::printf("ground/surface-callback/paired-plane/pre-response/resource/receipt: %u checks PASS; five600-update genuine routes plus old-policy/steep/timer refusals; native proxies only\n",checks);
}
