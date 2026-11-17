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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground relaunch floor: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground relaunch floor: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"shared combined relaunch floor"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"shared qualified resource operands");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "ordinary native spawn/arena/stage/target; no airborne state injection or replacement producer");
    };
    auto pad_at=[](unsigned t,unsigned route){
        GamePadState pad;const unsigned end=route==0?80:route==1?243:82;
        pad.b=(t>=5&&t<31)||(t>=78&&t<end);
        if(route==2&&t>=78&&t<82)pad.dpad_up=true;
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"native combined floor acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "matching complete native receipt before VM/resource/latch/contact/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::GroundRelaunch);bool refused=false;
    for(unsigned t=0;t<100&&!refused;++t){
        NativeBorgMotionOffer offer;const auto retained=pose;const auto meter=source.resource();
        if(!prepare(pad_at(t,0),&offer)){
            check(world.tick()==78&&source.resource()==meter&&!source.relaunches()&&!source.relaunch_floor_contacts()&&!source.landings()&&
                retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "older combined relaunch still refuses floor entry before cost/commit and retires authority without fallback");
            refused=true;
        }else accept(offer);
    }
    check(refused,"new floor capability does not widen old relaunch policy");
    for(unsigned route=0;route<3;++route){
        bind(NativeGRedGroundDashDemo::Mode::GroundRelaunchFloor);
        CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;unsigned snaps=0,negative=0,positive=0;
        u32 entry=0,handoff=0,lift=0,falling=0,touchdown=0;BattleMvpVec landed{};
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto meter=source.resource();
            const auto count=source.relaunch_floor_contacts();const auto latches=source.relaunch_stages();
            CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "resource recovery follows previous accepted cache, never a null-callback snap");
            NativeBorgMotionOffer offer;
            check(prepare(pad_at(t,route),&offer)&&source.resource()==meter&&source.relaunch_floor_contacts()==count&&
                source.relaunch_stages().lower_stage==latches.lower_stage&&source.relaunch_stages().upper_stage==latches.upper_stage,
                "costs, both VM lane latches, counters and contact remain staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer through ground/jump/relaunch/floor/falling/landing");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);const auto tick=world.tick();
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==before.position&&
                    source.resource()==meter&&source.relaunch_floor_contacts()==count&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected,input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected),"shared jump costs");
            if(offer.relaunch_motion){
                const auto& e=*offer.relaunch_motion;
                check(e.resource_before==expected.value736&&character_air_resource_consume(expected,input.costs[e.entry?0:1],input.recovery_delay,1,&expected)&&
                    e.resource_after==expected.value736,"same exact150/100 relaunch cost before motion/contact; no floor discount");
                if(e.entry)entry=t+1;
                if(e.lower_program==15&&!handoff)handoff=t+1;
            }
            if(offer.relaunch_floor){
                ++snaps;const auto& floor=*offer.relaunch_floor;const auto& e=*offer.relaunch_motion;
                if(floor.vertical_speed<0)++negative;else ++positive;
                check(!offer.air_motion&&!offer.jump_motion&&!offer.walk_ledge&&!offer.ordered_ground&&!offer.static_push&&!offer.air_dash_floor&&
                    floor.integrated_position[1]<=floor.surface.height&&offer.proposed_position[1]==floor.surface.height&&
                    floor.acceleration==e.vertical_accel&&floor.response_factor==(floor.vertical_speed<0?bits_to_f32(0x3f4ccccdu):1.f)&&
                    offer.vertical_speed==f32(floor.vertical_speed*floor.response_factor),
                    "exact pre-response air integration/selected-floor proof; only negative48 gets .8 while50/scalar/latches persist");
                auto wrong=offer;wrong.relaunch_floor.reset();rejects(wrong,"floor kind cannot omit selected contact proof");
                wrong=offer;wrong.relaunch_motion.reset();rejects(wrong,"combined floor cannot borrow unsealed legacy relaunch numeric authority");
                wrong=offer;wrong.relaunch_floor->integrated_position[0]+=1;rejects(wrong,"floor pre-response endpoint must match exact relaunch integration");
                wrong=offer;wrong.relaunch_floor->vertical_speed-=1;rejects(wrong,"floor pre-response velocity must match exact relaunch integration");
                wrong=offer;wrong.relaunch_floor->acceleration=0;rejects(wrong,"floor must preserve acceleration50");
                wrong=offer;wrong.relaunch_floor->surface.normal={1,0,0};rejects(wrong,"selected height and normal cannot be split");
                wrong=offer;wrong.relaunch_floor->response_factor=.9f;rejects(wrong,"steep push damping cannot substitute for floor tail");
                wrong=offer;wrong.vertical_speed=0;rejects(wrong,"floor tail is not wind-up zero48 or landing reset");
                wrong=offer;wrong.relaunch_motion->resource_after+=100;rejects(wrong,"floor cannot refund continued resource");
                wrong=offer;wrong.relaunch_motion->lower_after=0;rejects(wrong,"floor cannot reset relaunch stage");
                wrong=offer;wrong.kind=NativeBorgMotionKind::JumpTouchdown;wrong.relaunch_floor.reset();rejects(wrong,"null callback cannot acknowledge landing");
                wrong=offer;wrong.kind=NativeBorgMotionKind::RelaunchEntryPush;rejects(wrong,"relaunch-floor permission does not enable steep pushes");
                if(floor.vertical_speed>0){wrong=offer;wrong.vertical_speed=f32(floor.vertical_speed*.8f);rejects(wrong,"positive crossing must not receive negative-only .8 tail");}
            }
            if(route==1&&snaps==8&&!offer.relaunch_floor&&offer.relaunch_motion&&!lift)lift=t+1;
            if(offer.jump_motion&&offer.jump_motion->phase==6&&offer.jump_motion->entry&&offer.jump_motion->relaunch_origin)falling=t+1;
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown;
            if(hit){touchdown=t+1;landed=offer.proposed_position;}
            accept(offer);model=expected;
            check(source.resource()==model.value736&&!source.ordered_root().updates&&!source.surface_pushes()&&!source.air_dash_starts()&&
                !source.windup_entry_floor_contacts(),"shared resource model with no invented root/push/entry-floor/air-dash shortcut");
            if(offer.relaunch_floor){
                check(!world.actors()[0].grounded&&source.cached_airborne87()==1&&source.selector()==8&&!source.landings()&&
                    source.relaunches()==1&&source.resource()==8400-s16(100*(t+1-79)),"every null snap keeps airborne history/latches and continued spending");
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.relaunch_floor->response_factor=0;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals full pre-response/caller-tail payload");
                wrong=offer;wrong.relaunch_motion->upper_after=0;check(!world.motion_receipt(wrong,&receipt),"receipt also seals both VM lane latches");
            }
            if(hit)check(source.cached_airborne87()==1&&world.actors()[0].grounded&&source.movement_programs()==std::array<u16,2>{21,21},
                "actual touchdown follows separate released/depleted relaunch21, not a null-callback floor snap");
            if(touchdown&&t+1==touchdown+1)check(source.cached_airborne87()==0&&source.resource()==(route==0?8300:route==1?0:8100)&&
                source.movement_programs()==std::array<u16,2>{8,8},"separate landing8 entry precedes common recovery; no forced refill");
            if(touchdown&&t+1>touchdown)check(world.actors()[0].position==landed,"stable separate landing/idle endpoint");
        }
        check(entry==79&&snaps==(route==0?2u:route==1?8u:4u)&&negative==(route==1?7u:snaps)&&positive==(route==1?1u:0u)&&
            falling==(route==0?81u:route==1?164u:83u)&&touchdown==(route==0?81u:route==1?242u:83u)&&
            source.relaunch_floor_contacts()==snaps&&source.relaunches()==1&&source.landings()==1&&source.selector()==0&&
            source.resource()==10000&&source.movement_programs()==std::array<u16,2>{0,0}&&!world.shots_spawned()&&
            world.actors()[0].health==100&&world.actors()[1].health==100,"three genuine600-update snap/release, held-lift/depletion and directional routes return idle");
        if(route==1)check(handoff==84&&lift==87,"floor preserves14→15 handoff and lifts only after positive-speed snap");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"retirement revokes floor pose/receipt authority");
    }
    for(unsigned flick:{80u,86u}){
        bind(NativeGRedGroundDashDemo::Mode::GroundRelaunchFloor);bool entry_refused=false;
        for(unsigned t=0;t<=flick&&!entry_refused;++t){
            GamePadState pad;pad.b=(t>=5&&t<31)||t>=78;if(t==flick)pad.stick_x=127;
            NativeBorgMotionOffer offer;const auto retained=pose;const auto meter=source.resource();
            if(!prepare(pad,&offer)){
                check(world.tick()==flick&&source.resource()==meter&&!source.air_dash_starts()&&!source.windup_entry_floor_contacts()&&
                    source.relaunch_floor_contacts()==flick-78&&!source.landings()&&retained.frame_scope.expired()&&!world.step({},false,&view)&&
                    error.find("wind-up entry floor contact is not enabled")!=std::string::npos,
                    "genuine negative/positive wind-up entry-floor routes remain explicitly refused, without borrowing floor authority");
                entry_refused=true;
            }else accept(offer);
        }
        check(entry_refused,"new combined floor route does not silently enable air-dash entry snaps");
    }
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundRelaunchFloor),"combined relaunch-floor controller requires GFMM9");
    std::printf("ground/air/relaunch-floor/pre-response/latches/resource/receipt: %u checks PASS; three600-update genuine routes, older-mode and both entry-floor refusals; native proxies only\n",checks);
}
