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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground air floor: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground air floor: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"shared ground/air floor"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"shared qualified resource operands");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode,const BattleMvpConfig& config=BattleMvpConfig{}){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "fresh ground seed and one source owner; no airborne teleport/rebind");
    };
    auto pad_at=[](unsigned t,unsigned flick){
        GamePadState pad;pad.b=t>=5&&t<21;if(t>=flick&&t<flick+37)pad.stick_y=-127;return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"exact native floor/air acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "matching native receipt before VM/resource/contact/counter/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::GroundAir);bool refused=false;
    for(unsigned t=0;t<100&&!refused;++t){
        NativeBorgMotionOffer offer;const auto retained=pose;const auto meter=source.resource();
        if(!prepare(pad_at(t,60),&offer)){
            check(world.tick()==63&&source.resource()==meter&&!source.windup_floor_contacts()&&!source.landings()&&
                retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "older combined-air policy still refuses null-callback floor and retires authority without fallback");
            refused=true;
        }else accept(offer);
    }
    check(refused,"previous combined controller was not widened");
    for(unsigned route=0;route<3;++route){
        bind(NativeGRedGroundDashDemo::Mode::GroundAirFloor);
        const unsigned flick=route==0?60:route==1?61:58;
        CharacterAirResourceState model;model.value736=input.resource_limit;
        unsigned windup=0,active=0;u32 first_floor=0,handoff=0,active_entry=0,touchdown=0;BattleMvpVec landed{};
        std::weak_ptr<const NativeBorgMotionProducer> producer;
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto meter=source.resource();
            const auto floor_count=source.windup_floor_contacts()+source.active_floor_contacts();
            CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "resource recovery uses previous accepted cache, not floor coincidence");
            NativeBorgMotionOffer offer;
            check(prepare(pad_at(t,flick),&offer)&&source.resource()==meter&&source.windup_floor_contacts()+source.active_floor_contacts()==floor_count,
                "all input/VM/timer/resource/floor effects staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer through ground/jump/floor/landing/idle");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);const auto tick=world.tick();
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==before.position&&
                    source.resource()==meter&&source.windup_floor_contacts()+source.active_floor_contacts()==floor_count&&
                    !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected,input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected),"shared jump costs");
            if(offer.air_motion){
                const auto& air=*offer.air_motion;
                if(air.phase==16)check(character_air_resource_consume(expected,input.costs[3],input.recovery_delay,1,&expected),"active50 before motion/contact");
                if(air.phase==15&&!air.entry&&air.next_selector==16){
                    check(character_air_resource_consume(expected,input.costs[2],input.recovery_delay,1,&expected),"handoff450 after wind-up motion/contact");
                    handoff=t+1;
                }
                if(air.phase==16&&air.entry)active_entry=t+1;
                check(air.resource_after==expected.value736,"air payload and shared exact resource model agree");
            }
            if(offer.air_dash_floor){
                const auto& floor=*offer.air_dash_floor;const auto& air=*offer.air_motion;
                if(!first_floor)first_floor=t+1;
                const bool is_windup=air.phase==15;
                if(is_windup)++windup;else ++active;
                check(!air.callback&&!offer.static_push&&!offer.relaunch_floor&&!offer.jump_motion&&!offer.walk_ledge&&!offer.ordered_ground&&
                    offer.proposed_position[1]==floor.surface.height&&floor.acceleration==bits_to_f32(0xbd8f5c29u)&&
                    floor.integrated_position[1]<=floor.surface.height&&
                    offer.vertical_speed==(is_windup?0.f:floor.vertical_speed)&&floor.vertical_speed<0,
                    "wind-up ret1 zero48 versus active no-tail negative48; selected height/normal and pre-response evidence retained");
                auto wrong=offer;wrong.air_dash_floor.reset();rejects(wrong,"air floor kind cannot omit selected contact proof");
                wrong=offer;wrong.air_motion.reset();rejects(wrong,"combined floor kind cannot borrow legacy unsealed air phase");
                wrong=offer;wrong.air_dash_floor->vertical_speed-=.25f;rejects(wrong,"floor pre-response velocity must match exact air integration");
                wrong=offer;wrong.air_dash_floor->integrated_position[0]+=1;rejects(wrong,"floor pre-response endpoint must match actual air movement");
                wrong=offer;wrong.air_dash_floor->surface.normal={1,0,0};rejects(wrong,"floor normal cannot be replaced independently of selected result");
                wrong=offer;wrong.air_motion->callback=true;rejects(wrong,"floor snap cannot install callback early");
                wrong=offer;wrong.air_motion->next_selector=7;rejects(wrong,"snap is not a landing selector");
                wrong=offer;wrong.kind=NativeBorgMotionKind::JumpTouchdown;wrong.air_dash_floor.reset();rejects(wrong,"no cached-history/landing shortcut before readiness");
                wrong=offer;wrong.kind=NativeBorgMotionKind::RelaunchWindupFloor;rejects(wrong,"floor permission does not inherit relaunch");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashActivePush;rejects(wrong,"floor permission does not inherit steep push");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupEntryFloor;rejects(wrong,"combined mode does not borrow legacy wind-up entry-floor authority");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupPositiveEntryFloor;rejects(wrong,"positive wind-up entry remains separately refused");
                if(!is_windup){wrong=offer;wrong.vertical_speed=0;rejects(wrong,"active floor does not zero accumulated48");}
                else {wrong=offer;wrong.vertical_speed=floor.vertical_speed;rejects(wrong,"wind-up floor must apply separate caller zero48");}
            }
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown;
            if(hit){touchdown=t+1;landed=offer.proposed_position;
                check(offer.air_motion&&offer.air_motion->phase==16&&offer.air_motion->callback&&offer.air_motion->next_stage==2&&!offer.air_dash_floor,
                    "only minimum active timer installs real landing callback");
            }
            accept(offer);model=expected;
            check(source.resource()==model.value736&&!source.ordered_root().updates&&!source.relaunches()&&!source.surface_pushes(),
                "resource follows shared source kernel; no invented root/relaunch/push/refill");
            if(offer.air_dash_floor){
                check(!world.actors()[0].grounded&&source.cached_airborne87()==1&&!source.landings()&&
                    source.resource()<=9550&&source.selector()!=7,"null callback keeps native phase2 and prior airborne cache");
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.air_dash_floor->integrated_position[1]-=1;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals pre-response floor evidence");
                wrong=offer;wrong.air_motion->timer568+=1;check(!world.motion_receipt(wrong,&receipt),"receipt also seals air phase/timer evidence");
            }
            if(hit)check(world.actors()[0].grounded&&source.cached_airborne87()==1&&source.landings()==1,"actual touchdown still precedes landing cache reset");
            if(touchdown&&t+1==touchdown+1)check(source.cached_airborne87()==0&&source.resource()==7550&&
                source.movement_programs()==std::array<u16,2>{8,8},"separate accepted landing8 entry; no touchdown refill");
            if(touchdown&&t+1>touchdown)check(world.actors()[0].position==landed,"stable landing/idle endpoint");
        }
        check(first_floor==(route==0?64u:route==1?63u:87u)&&windup==(route==0?3u:route==1?5u:0u)&&
            active==(route==2?8u:30u)&&handoff==flick+6&&active_entry==flick+7&&touchdown==flick+37&&
            source.windup_floor_contacts()==windup&&source.active_floor_contacts()==active&&source.air_dash_starts()==1&&
            !source.windup_entry_floor_contacts()&&!source.positive_windup_floor_contacts()&&source.landings()==1&&
            source.resource()==10000&&source.selector()==0&&source.movement_programs()==std::array<u16,2>{0,0}&&
            !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "three genuine600-update routes: wind-up/active snaps, exact resource/timer ordering, one later landing/idle; no entry/relaunch/push/combat");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"retiring owner revokes source pose and receipt");
    }
    BattleMvpConfig edge;edge.arena_half=4500;edge.spawn[0]={1012.360657f,1403.599731f};edge.spawn_query_y[0]=482.747589f;
    bind(NativeGRedGroundDashDemo::Mode::GroundAirFloor,edge);
    for(unsigned t=0;t<7;++t){GamePadState pad;if(t==5)pad.stick_y=-127;NativeBorgMotionOffer offer;check(prepare(pad,&offer),"ground-loss fixture");accept(offer);}
    const auto position=world.actors()[0].position;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer unsupported;
    check(!source.prepare(world,view.source_binding(),frame,60,0,1u<<16,&unsupported,&error)&&world.tick()==7&&world.actors()[0].position==position&&
        !source.air_dash_starts()&&pose.frame_scope.expired()&&!world.step({},false,&view),"floor permission does not resolve positive ground-loss698 overlap");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundAirFloor),"combined floor controller requires GFMM9");
    std::printf("ground/air floor/phase/contact/resource/receipt/pose: %u checks PASS; three600-update genuine routes, older-mode and timer refusals; native proxies only\n",checks);
}
