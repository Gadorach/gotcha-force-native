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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground wind-up entry floor: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground wind-up entry floor: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"combined wind-up entry floor"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"shared exact resource operands");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode,const BattleMvpConfig& config=BattleMvpConfig{}){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "native ground seed; no airborne injection, teleport/rebind or stage/target edit");
    };
    auto pad_at=[](unsigned t,bool positive){
        const unsigned flick=positive?86:80;
        GamePadState pad;pad.b=(t>=5&&t<31)||(t>=78&&t<=flick);
        if(t>=flick&&t<flick+39)pad.stick_x=127; //release on actual touchdown, before landing-entry input
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"native combined entry/air acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "complete matching native receipt before VM/resource/timer/counter/pose publication");
    };
    for(bool positive:{false,true}){
        const unsigned flick=positive?86:80;const s16 entry_meter=positive?7700:8300;
        bind(NativeGRedGroundDashDemo::Mode::GroundRelaunchFloor);bool refused=false;
        for(unsigned t=0;t<=flick&&!refused;++t){
            const auto retained=pose;NativeBorgMotionOffer offer;
            if(!prepare(pad_at(t,positive),&offer)){
                check(world.tick()==flick&&source.resource()==entry_meter&&!source.air_dash_starts()&&!source.windup_entry_floor_contacts()&&
                    source.relaunch_floor_contacts()==flick-78&&!source.landings()&&retained.frame_scope.expired()&&
                    !world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                    "previous combined relaunch-floor policy still refuses both entry snaps and retires authority without fallback");
                refused=true;
            }else accept(offer);
        }
        check(refused,"new entry capability does not widen older policy");
        bind(NativeGRedGroundDashDemo::Mode::GroundWindupEntryFloor);
        CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;u32 entry=0,handoff=0,active=0,touchdown=0;
        unsigned windup_floors=0,active_floors=0;BattleMvpVec landed{};
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto meter=source.resource();
            const auto entries=source.windup_entry_floor_contacts();const auto starts=source.air_dash_starts();
            CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "resource recovery follows accepted cached87, not floor coincidence or new entry selection");
            NativeBorgMotionOffer offer;
            check(prepare(pad_at(t,positive),&offer)&&source.resource()==meter&&source.windup_entry_floor_contacts()==entries&&source.air_dash_starts()==starts,
                "entry motion/timer/VM/resource/counters remain staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer through ground/jump/relaunch/floor/air/landing");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);const auto tick=world.tick();
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==before.position&&
                    source.resource()==meter&&source.air_dash_starts()==starts&&source.windup_entry_floor_contacts()==entries&&
                    !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected,input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected),"shared jump costs");
            if(offer.relaunch_motion)check(character_air_resource_consume(expected,input.costs[offer.relaunch_motion->entry?0:1],input.recovery_delay,1,&expected),"shared relaunch costs");
            if(offer.air_motion){
                const auto& e=*offer.air_motion;
                if(e.phase==16)check(character_air_resource_consume(expected,input.costs[3],input.recovery_delay,1,&expected),"active50 before movement/contact");
                if(e.phase==15&&!e.entry&&e.next_selector==16){
                    handoff=t+1;check(character_air_resource_consume(expected,input.costs[2],input.recovery_delay,1,&expected),"handoff450 after movement, before contact");
                }
                check(e.resource_after==expected.value736,"air numeric proof agrees with exact shared resource kernel");
            }
            if(native_borg_motion_windup_floor_entry(offer.kind)){
                entry=t+1;const auto& floor=*offer.air_dash_floor;const auto& e=*offer.air_motion;
                check(offer.kind==(positive?NativeBorgMotionKind::AirDashWindupPositiveEntryFloor:NativeBorgMotionKind::AirDashWindupEntryFloor)&&
                    (before.vertical_speed>0)==positive&&e.entry&&e.phase==15&&e.next_selector==15&&e.next_stage==1&&!e.callback&&
                    offer.proposed_position==before.position&&floor.integrated_position==before.position&&offer.vertical_speed==0&&
                    floor.vertical_speed==f32(before.vertical_speed*.5f)&&e.vertical_before==floor.vertical_speed&&
                    e.move_speed==f32(e.speed_before*.5f)&&e.speed44==e.move_speed&&e.planar_accel==0&&
                    e.vertical_accel==bits_to_f32(0xbd8f5c29u)&&floor.acceleration==e.vertical_accel&&
                    e.resource_before==entry_meter&&e.resource_after==entry_meter&&e.timer568==7&&
                    !offer.relaunch_motion&&!offer.jump_motion&&!offer.static_push&&!offer.relaunch_floor&&!offer.ordered_ground,
                    "entry halves44/48 then null-floor zero48, without translation/integration/clamp/cost/landing");
                auto wrong=offer;wrong.air_dash_floor->vertical_speed+=floor.acceleration;rejects(wrong,"entry cannot apply future gravity before contact");
                wrong=offer;wrong.air_motion->vertical_before=0;rejects(wrong,"zeroed caller response cannot replace the pre-response half-speed");
                wrong=offer;wrong.proposed_position[1]+=floor.vertical_speed;rejects(wrong,"entry cannot move by its positive or negative half velocity");
                wrong=offer;wrong.air_dash_floor->integrated_position[0]+=1;rejects(wrong,"entry cannot invent pre-contact translation");
                wrong=offer;wrong.air_motion->resource_after=std::max(0,int(entry_meter)-450);rejects(wrong,"entry cannot spend deferred handoff cost early");
                wrong=offer;wrong.air_motion->timer568=0;rejects(wrong,"entry cannot skip the verified source turn timer");
                wrong=offer;wrong.air_motion->callback=true;rejects(wrong,"entry cannot install landing callback");
                wrong=offer;wrong.air_motion.reset();rejects(wrong,"combined entry needs exact air numeric proof, not legacy contact alone");
                wrong=offer;wrong.air_dash_floor.reset();rejects(wrong,"entry-floor kind needs paired selected contact proof");
                wrong=offer;wrong.kind=positive?NativeBorgMotionKind::AirDashWindupEntryFloor:NativeBorgMotionKind::AirDashWindupPositiveEntryFloor;
                rejects(wrong,"positive and nonascending entry tags cannot borrow each other's velocity domain");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupEntryPush;rejects(wrong,"entry snap permission does not enable steep push");
                wrong=offer;wrong.kind=NativeBorgMotionKind::JumpTouchdown;wrong.air_dash_floor.reset();rejects(wrong,"null entry contact is not landing");
            }
            if(offer.air_dash_floor){
                if(offer.air_motion->phase==15)++windup_floors;else ++active_floors;
            }
            if(offer.air_motion&&offer.air_motion->phase==16&&offer.air_motion->entry)active=t+1;
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown;
            if(hit){touchdown=t+1;landed=offer.proposed_position;}
            accept(offer);model=expected;
            check(source.resource()==model.value736&&!source.ordered_root().updates&&!source.surface_pushes(),"no invented root/push/refill, exact shared spending/recovery");
            if(native_borg_motion_windup_floor_entry(offer.kind)){
                check(source.windup_entry_floor_contacts()==1&&source.positive_windup_floor_contacts()==unsigned(positive)&&source.air_dash_starts()==1&&
                    !source.landings()&&!world.actors()[0].grounded&&source.cached_airborne87()==1&&source.resource()==entry_meter&&
                    source.movement_programs()==std::array<u16,2>{10,10},"single accepted entry/counter, shared10/cache1, no early refill");
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.air_dash_floor->vertical_speed=0;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals exact nonzero pre-response half-speed");
                wrong=offer;wrong.air_motion->entry=false;check(!world.motion_receipt(wrong,&receipt),"receipt also seals entry phase");
            }
            if(offer.air_dash_floor)check(!world.actors()[0].grounded&&source.cached_airborne87()==1&&!source.landings(),"all wind-up/early-active snaps remain airborne");
            if(hit)check(source.resource()==entry_meter-2000&&source.cached_airborne87()==1&&world.actors()[0].grounded&&
                source.movement_programs()==std::array<u16,2>{11,11},"only later minimum active timer callback produces touchdown, captured11/cache1");
            if(touchdown&&t+1==touchdown+1)check(source.cached_airborne87()==0&&source.resource()==entry_meter-2000&&
                source.movement_programs()==std::array<u16,2>{8,8},"separate landing8 entry before common grounded recovery");
            if(touchdown&&t+1>touchdown)check(world.actors()[0].position==landed,"stable separate landing/idle endpoint");
        }
        check(entry==flick+1&&handoff==flick+8&&active==flick+9&&touchdown==flick+39&&windup_floors==8&&active_floors==30&&
            source.windup_entry_floor_contacts()==1&&source.positive_windup_floor_contacts()==unsigned(positive)&&
            source.relaunch_floor_contacts()==flick-78&&source.relaunches()==1&&source.air_dash_starts()==1&&source.landings()==1&&
            source.selector()==0&&source.resource()==10000&&source.movement_programs()==std::array<u16,2>{0,0}&&!world.shots_spawned()&&
            world.actors()[0].health==100&&world.actors()[1].health==100,"two genuine600-update negative/positive entry routes, separate costs/contact/landing, stable idle");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"retiring entry owner revokes pose/receipt authority");
    }
    BattleMvpConfig edge;edge.arena_half=4500;edge.spawn[0]={1012.360657f,1403.599731f};edge.spawn_query_y[0]=482.747589f;
    bind(NativeGRedGroundDashDemo::Mode::GroundWindupEntryFloor,edge);
    for(unsigned t=0;t<7;++t){GamePadState pad;if(t==5)pad.stick_y=-127;NativeBorgMotionOffer offer;check(prepare(pad,&offer),"ground-loss timer fixture");accept(offer);}
    const auto position=world.actors()[0].position;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer unsupported;
    check(!source.prepare(world,view.source_binding(),frame,60,0,1u<<16,&unsupported,&error)&&world.tick()==7&&world.actors()[0].position==position&&
        !source.air_dash_starts()&&pose.frame_scope.expired()&&!world.step({},false,&view),"entry-floor capability does not authorize unresolved positive698 overlap");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundWindupEntryFloor),"combined entry-floor controller requires GFMM9");
    std::printf("ground/wind-up-entry-floor/half-speed/no-motion/resource/receipt: %u checks PASS; two600-update genuine routes, both older-mode and timer refusals; native proxies only\n",checks);
}
