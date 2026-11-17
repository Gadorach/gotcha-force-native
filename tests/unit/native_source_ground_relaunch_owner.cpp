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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground relaunch: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground relaunch: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"one ground/air/relaunch controller"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"shared qualified resource operands");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "ordinary native spawn/bounds/stage/target; no actor injection or replacement controller");
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"native relaunch/lineage acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "exact receipt before VM/resource/latch/contact/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::GroundAirFloor);
    for(unsigned t=0;t<60;++t){GamePadState pad;pad.b=t>=5&&t<31;NativeBorgMotionOffer offer;check(prepare(pad,&offer),"older controller jump fixture");accept(offer);}
    NativeGRedGroundInput mapped;GamePadState jump;jump.b=true;
    check(!source.map_input(jump,&mapped,&error)&&world.tick()==60&&!source.relaunches()&&!pose.frame_scope.expired(),
        "older combined floor mode still refuses new airborne jump input without retiring a live accepted frame");
    for(unsigned route=0;route<5;++route){
        bind(NativeGRedGroundDashDemo::Mode::GroundRelaunch);
        CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;
        u32 entry=0,active=0,falling=0,touchdown=0;unsigned relaunch_frames=0;bool depleted=false,redashed=false;
        for(unsigned t=0;t<600;++t){
            GamePadState pad;pad.b=(t>=5&&t<31)||(t>=60&&t<(route==2?243u:100u));
            if(route==1&&t>=60&&t<100)pad.dpad_up=true;
            if(route==3){pad.b=(t>=5&&t<75)||(t>=80&&t<120);if(t>=45&&t<120)pad.stick_y=-127;}
            if(route==4&&t>=80&&t<110)pad.stick_y=-127;
            const auto before=world.actors()[0];const auto retained=pose;const auto meter=source.resource();const auto launches=source.relaunches();
            const auto stages=source.relaunch_stages();CharacterAirResourceState expected;
            check(character_air_resource_recover(model,0,source.movement_flags(),source.cached_airborne87(),input.resource_limit,input.recovery_increment,&expected),
                "recovery follows previous accepted cache rather than relaunch/input state");
            NativeBorgMotionOffer offer;
            check(prepare(pad,&offer)&&source.resource()==meter&&source.relaunches()==launches&&
                source.relaunch_stages().lower_stage==stages.lower_stage&&source.relaunch_stages().upper_stage==stages.upper_stage,
                "input/resource/lower-upper latches and counters are staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer through ground/jump/air/relaunch/falling/landing");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);const auto tick=world.tick();
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==before.position&&
                    source.resource()==meter&&source.relaunches()==launches&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected,input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected),"shared jump costs");
            if(offer.air_motion){
                const auto& e=*offer.air_motion;
                if(e.phase==16)check(character_air_resource_consume(expected,input.costs[3],input.recovery_delay,1,&expected),"active50 before motion");
                if(e.phase==15&&!e.entry&&e.next_selector==16)check(character_air_resource_consume(expected,input.costs[2],input.recovery_delay,1,&expected),"wind-up450 after motion");
                if(route==4&&e.phase==15&&e.entry){redashed=true;check(t==80&&e.resource_before==6500,"air dash can interrupt accepted relaunch without inheriting its latches");}
            }
            if(offer.relaunch_motion){
                ++relaunch_frames;const auto& e=*offer.relaunch_motion;
                check(!offer.air_motion&&!offer.jump_motion&&!offer.ordered_ground&&!offer.walk_ledge&&!offer.static_push&&!offer.relaunch_floor&&!offer.air_dash_floor&&
                    e.resource_before==expected.value736,"distinct relaunch proof; no old air/ground/root/floor/push acknowledgement");
                check(character_air_resource_consume(expected,input.costs[e.entry?0:1],input.recovery_delay,1,&expected)&&e.resource_after==expected.value736,
                    "entry150/continued100 before motion, no relaunch-specific refill");
                if(e.entry){
                    entry=t+1;
                    check(e.vertical_before==f32(before.vertical_speed+15)&&offer.entry_vertical_delta==15&&
                        offer.proposed_position[1]==f32(before.position[1]+e.vertical_before)&&e.lower_before==0&&e.upper_before==0,
                        "entry adds15 before displacement; it neither replaces nor halves old velocity");
                }
                if(e.lower_program==15&&!active)active=t+1;
                check(e.lower_after>=1&&e.lower_after<=3&&e.upper_after>=1&&e.upper_after<=e.lower_after&&
                    e.speed44<=8&&offer.vertical_speed<=18&&offer.vertical_speed>=-30,"independent lane latches and normal source clamps");
                auto wrong=offer;wrong.relaunch_motion.reset();rejects(wrong,"combined relaunch cannot omit its exact proof");
                wrong=offer;wrong.relaunch_motion->vertical_before=15;rejects(wrong,"entry/continuation cannot replace old vertical speed by a fixed15");
                wrong=offer;wrong.relaunch_motion->resource_after+=1;rejects(wrong,"relaunch cannot borrow an unspent/refilled meter");
                wrong=offer;wrong.relaunch_motion->move_speed+=1;rejects(wrong,"retained scalar is not a displacement-derived speed");
                wrong=offer;wrong.relaunch_motion->lower_after=0;rejects(wrong,"lower completion cannot reset relaunch latches");
                wrong=offer;wrong.relaunch_motion->upper_program=e.upper_program==14?15:14;rejects(wrong,"upper program must follow its own latch, not a guessed lower mirror");
                wrong=offer;wrong.relaunch_motion->coarse57d=4;rejects(wrong,"unknown released-relaunch animation family refused");
                wrong=offer;wrong.kind=NativeBorgMotionKind::RelaunchEntryFloor;rejects(wrong,"airborne relaunch permission does not grant floor snaps");
                wrong=offer;wrong.kind=NativeBorgMotionKind::RelaunchEntryPush;rejects(wrong,"airborne relaunch permission does not grant steep push");
                if(e.lower_after==2){wrong=offer;wrong.relaunch_motion->lower_done=false;rejects(wrong,"lower stage2 needs its actual completion update");}
            }
            if(offer.jump_motion&&offer.jump_motion->phase==6&&offer.jump_motion->entry&&offer.jump_motion->relaunch_origin){
                falling=t+1;auto wrong=offer;wrong.jump_motion->relaunch_origin=false;
                rejects(wrong,"released/depleted relaunch cannot borrow normal jump6 or post-air13 lineage");
                check(!offer.jump_motion->air_dash_origin&&!offer.jump_motion->ground_loss&&offer.jump_motion->relaunch_coarse57d==0,
                    "explicit normal-G-Red released-relaunch21 lineage remains distinct");
            }
            if(offer.kind==NativeBorgMotionKind::JumpTouchdown)touchdown=t+1;
            accept(offer);model=expected;
            check(source.resource()==model.value736&&!source.ordered_root().updates&&!source.surface_pushes()&&!source.relaunch_floor_contacts()&&
                !source.windup_entry_floor_contacts(),"shared exact resource model; no fabricated root/push/entry-floor capability");
            if(offer.relaunch_motion){
                check(!world.actors()[0].grounded&&source.cached_airborne87()==1&&source.selector()==8,"relaunch preserves airborne cache");
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.relaunch_motion->upper_after=0;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals independent lane latches");
                if(!source.resource())depleted=true;
            }
            if(falling&&t+1==falling)check(source.movement_programs()==std::array<u16,2>{21,21},"release/depletion requests verified coarse0 program21");
        }
        const unsigned expected_touchdown=route==0?171:route==1?170:route==2?243:route==3?200:151;
        check(entry==(route==3?81u:61u)&&active==(route==3?86u:66u)&&touchdown==expected_touchdown&&
            source.relaunches()==1&&source.landings()==1&&source.air_dash_starts()==(route>=3?1u:0u)&&
            source.selector()==0&&source.resource()==10000&&source.movement_programs()==std::array<u16,2>{0,0}&&
            !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100&&relaunch_frames,
            "five genuine600-update release/steer/deplete/air-interruption/redash routes return idle with one landing");
        if(route<4)check(falling==(route==2?146u:route==3?121u:101u),"exact release/depletion boundary after accepted relaunch");
        if(route==2)check(depleted,"held relaunch really consumes the last resource before falling");
        if(route==4)check(redashed&&!falling,"air dash interrupts relaunch and restores distinct post-air falling lineage");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"retirement revokes relaunch pose/receipt authority");
    }
    bind(NativeGRedGroundDashDemo::Mode::GroundRelaunch);bool floor_refused=false;
    for(unsigned t=0;t<100&&!floor_refused;++t){
        GamePadState pad;pad.b=(t>=5&&t<31)||(t>=78&&t<82);NativeBorgMotionOffer offer;const auto meter=source.resource();const auto retained=pose;
        if(!prepare(pad,&offer)){
            check(world.tick()==78&&source.resource()==meter&&!source.relaunches()&&!source.relaunch_floor_contacts()&&!source.landings()&&
                retained.frame_scope.expired()&&!world.step({},false,&view),"genuine relaunch-floor entry refuses before spending, publication or implicit fallback");
            floor_refused=true;
        }else accept(offer);
    }
    check(floor_refused,"combined relaunch does not borrow the older floor lab");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundRelaunch),"combined relaunch requires GFMM9");
    std::printf("ground/air/relaunch/scalar/resource/independent-latches/lineage/receipt: %u checks PASS; five600-update genuine routes, older-mode and floor refusals; native proxies only\n",checks);
}
