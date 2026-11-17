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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground air owner: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground air owner: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"one ground/air controller"},nullptr,&error)&&view.load(files,art,render,&error),"shared renderer and original stage/models");
    std::vector<u8> bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"qualified shared costs and recovery");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode,const BattleMvpConfig& config=BattleMvpConfig{}){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "fresh native ground seed; no airborne teleport or replacement controller");
    };
    for(unsigned route=0;route<3;++route){
        BattleMvpConfig config;if(route==2)config.arena_half=4500; //declared long held-active/depletion fixture; no position/stage edit
        bind(NativeGRedGroundDashDemo::Mode::GroundAir,config);
        CharacterAirResourceState model;model.value736=input.resource_limit;
        std::weak_ptr<const NativeBorgMotionProducer> producer;
        unsigned entries=0,active_entries=0,holds=0,recovery_frames=0;u32 first_entry=0,first_active=0,touchdown=0,fall_after_air=0;
        bool depleted=false;BattleMvpVec landed{};
        for(unsigned t=0;t<600;++t){
            GamePadState pad;
            if(!route){pad.b=t>=5&&t<350;if((t>=45&&t<87)||(t>=90&&t<132))pad.stick_y=-127;}
            else {pad.b=t>=5&&t<(route==2?350u:150u);if(t>=75&&t<(route==2?155u:120u))pad.stick_y=-127;}
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);const auto before=world.actors()[0];const auto old_pose=pose;
            const auto resource=source.resource();const auto flags=source.movement_flags();const auto starts=source.air_dash_starts(),landings=source.landings();
            CharacterAirResourceState recovered;
            check(character_air_resource_recover(model,0,flags,source.cached_airborne87(),input.resource_limit,input.recovery_increment,&recovered),
                "shared recovery uses previous accepted cached87");
            NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;
            check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error)&&
                source.resource()==resource&&source.air_dash_starts()==starts&&source.landings()==landings,
                "air input/VM/motion/resource/phase/counters are staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer through jump/air dash/recovery/falling/landing");
            auto refuses=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto tick=world.tick();
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==before.position&&
                    source.resource()==resource&&source.air_dash_starts()==starts&&!old_pose.frame_scope.expired()&&world.motion_receipt_current(old_pose.receipt),why);
            };
            auto expected_resource=recovered;
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected_resource,
                input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected_resource),"exact jump resource candidate");
            if(offer.air_motion){
                const auto& e=*offer.air_motion;
                check(!offer.jump_motion&&!offer.walk_ledge&&!offer.ordered_ground&&!offer.static_push&&!offer.air_dash_floor&&!offer.relaunch_floor&&
                    e.resource_before==recovered.value736,"distinct air proof cannot borrow jump/ground/root/floor/push authority");
                if(e.phase==16)check(character_air_resource_consume(expected_resource,input.costs[3],input.recovery_delay,1,&expected_resource),"active cost before movement");
                if(e.phase==15&&!e.entry&&e.next_selector==16)
                    check(character_air_resource_consume(expected_resource,input.costs[2],input.recovery_delay,1,&expected_resource),"wind-up handoff cost after movement");
                check(e.resource_after==expected_resource.value736,"air proof resource matches shared signed source kernel");
                if(e.phase==15&&e.entry){
                    ++entries;if(!first_entry)first_entry=world.tick()+1;
                    check(offer.proposed_position==before.position&&offer.vertical_speed==f32(before.vertical_speed*.5f)&&
                        e.move_speed==f32(e.speed_before*.5f)&&!e.callback&&!e.planar_accel&&
                        e.resource_after==e.resource_before,"wind-up entry halves velocity without translation, gravity or deferred cost");
                    auto wrong=offer;wrong.air_motion.reset();refuses(wrong,"new air-dash policy requires its exact numeric proof");
                    wrong=offer;wrong.proposed_position[1]+=offer.vertical_speed;
                    refuses(wrong,"entry cannot integrate old/new vertical velocity early");
                    wrong=offer;wrong.air_motion->resource_after=std::max(0,int(e.resource_before)-450);
                    refuses(wrong,"wind-up cannot spend handoff resource early");
                    wrong=offer;wrong.kind=NativeBorgMotionKind::RelaunchEntry;
                    refuses(wrong,"air-dash permission does not inherit relaunch");
                }
                if(e.phase==16&&e.entry){
                    ++active_entries;if(!first_active)first_active=world.tick()+1;
                    check(e.move_speed==30&&e.speed44==std::fma(bits_to_f32(0xbd4ccccdu),1.f,30.f)&&
                        e.vertical_before==0&&offer.proposed_position[1]==before.position[1]&&!e.callback&&e.timer568==30,
                        "active entry resets source30/-0.05 and zero48 before integration; no ceiling/floor shortcut");
                    auto wrong=offer;wrong.air_motion->move_speed=29;refuses(wrong,"active scalar cannot be fabricated");
                    wrong=offer;wrong.air_motion->callback=true;refuses(wrong,"callback cannot become ready before timer568 expires");
                }
                if(e.phase==16&&!e.entry){++holds;
                    if(e.next_stage==1)check(!e.callback,"active timer gates callback even while physically airborne");}
                if(e.phase==17){
                    ++recovery_frames;check(e.callback&&e.vertical_accel==-1&&e.move_speed==f32(e.speed_before*bits_to_f32(0x3f7d70a4u)),
                        "recovery uses shared0.99/direct-drift/-1 path before real contact and lower completion");
                    if(e.next_selector==6){
                        auto wrong=offer;wrong.air_motion->lower_done=false;refuses(wrong,"recovery cannot claim falling before its source lower completion");
                    }
                }
                if(!e.resource_after)depleted=true;
            }
            if(offer.jump_motion&&offer.jump_motion->phase==6&&offer.jump_motion->air_dash_origin&&!fall_after_air){
                fall_after_air=world.tick()+1;
                check(offer.kind==NativeBorgMotionKind::JumpAir&&!offer.jump_motion->ground_loss&&source.ordered_root().updates==0,
                    "completed air recovery enters distinct program13 origin, not normal jump6 or ground-loss history");
                auto wrong=offer;wrong.jump_motion->air_dash_origin=false;
                refuses(wrong,"post-air scalar above8 cannot borrow ordinary jump lineage/clamp");
            }
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown;
            if(hit){touchdown=world.tick()+1;landed=offer.proposed_position;}
            const bool accepted=world.step({},false,&view,nullptr,&frame,&offer);
            if(!accepted)error="route="+std::to_string(route)+" tick="+std::to_string(world.tick()+1)+" kind="+
                std::to_string(unsigned(offer.kind))+" phase="+std::to_string(offer.air_motion?offer.air_motion->phase:0)+
                " pos="+std::to_string(offer.proposed_position[0])+"/"+std::to_string(offer.proposed_position[1])+"/"+std::to_string(offer.proposed_position[2]);
            check(accepted&&source.commit(world,view.source_binding(),&error)&&
                source.pose(world,&pose)&&view.update_source(world,pose,&error),"native phase/resource/endpoint/contact acceptance before source and pose publication");
            model=expected_resource;
            check(source.resource()==model.value736&&!source.ordered_root().updates,"no special refill or invented ground-root evaluation during air dash");
            if(hit)check(source.cached_airborne87()==1&&world.actors()[0].grounded,"actual touchdown keeps previous airborne cache until landing entry");
            if(touchdown&&world.tick()>touchdown)check(world.actors()[0].position==landed,"stable separate landing/idle on the selected original floor");
            if(offer.air_motion){auto wrong=offer;wrong.air_motion->timer568+=1;NativeBorgMotionReceipt receipt;
                check(!world.motion_receipt(wrong,&receipt)&&world.motion_receipt(offer,&receipt),"receipt matches complete air timer/phase/resource evidence");}
        }
        check(entries==(route?1u:2u)&&active_entries==entries&&source.air_dash_starts()==entries&&
            recovery_frames>0&&holds>0&&source.landings()==1&&touchdown&&source.selector()==0&&source.resource()==10000&&
            !source.relaunches()&&!source.surface_pushes()&&!world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "600-update two-dash/recovery-completion/depletion routes return idle without relaunch/floor/push/combat permission");
        if(!route)check(first_entry==46&&first_active==52&&touchdown==159&&!fall_after_air,"accepted two-air-dash demo timings and real recovery touchdown");
        else check(first_entry==76&&first_active==82&&fall_after_air==(route==2?185u:150u)&&touchdown==(route==2?196u:168u),
            "high jump preserves explicit post-recovery program13 falling and actual ground contact");
        if(route==2)check(depleted,"held active dash really depletes resource before recovery");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"air-mode retirement revokes source pose/receipt authority");
    }
    //61338 does not clear698. Do not infer a new combined floor/callback contract.
    BattleMvpConfig edge;edge.arena_half=4500;edge.spawn[0]={1012.360657f,1403.599731f};edge.spawn_query_y[0]=482.747589f;
    bind(NativeGRedGroundDashDemo::Mode::GroundAir,edge);
    for(unsigned t=0;t<7;++t){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);GamePadState pad;if(t==5)pad.stick_y=-127;
        NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;
        check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error)&&
            world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose),"fresh ground-loss timer fixture");
    }
    const auto position=world.actors()[0].position;NativeBorgMotionOffer unsupported;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!source.prepare(world,view.source_binding(),frame,60,0,1u<<16,&unsupported,&error)&&world.tick()==7&&world.actors()[0].position==position&&
        !source.air_dash_starts()&&pose.frame_scope.expired()&&!world.step({},false,&view),"positive ground-loss698 overlap refuses without actor/floor/push acknowledgement");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundAir),"new combined controller requires GFMM9");
    std::printf("one ground/air controller/phase/resource/lineage/contact/pose: %u checks PASS; three600-update original-stage routes plus timer refusal; native proxies only\n",checks);
}
