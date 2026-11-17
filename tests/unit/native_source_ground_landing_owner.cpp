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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground landing owner: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground landing owner: optional qualified GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"accepted landing-entry controller"},nullptr,&error)&&view.load(files,art,render,&error),"shared renderer/models/original stage");
    std::vector<u8> input_bytes;CharacterMovementInputProfile input;
    check(files.read(pack+"/movement.gfmi",input_bytes)&&character_movement_input_load({input_bytes.data(),input_bytes.size()},&input,&error),
        "qualified source resource costs/recovery operands, not a landing refill heuristic");
    auto bind=[&](bool ledge,NativeGRedGroundDashDemo::Mode mode){
        BattleMvpConfig config;
        if(ledge){config.arena_half=4500;config.spawn[0]={1012.360657f,1403.599731f};config.spawn_query_y[0]=482.747589f;}
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "fresh actual native world/model/VM/producer binding; no airborne placement");
    };
    for(unsigned route=0;route<3;++route){
        const bool ledge=route==1,second_jump=route!=2;bind(ledge,NativeGRedGroundDashDemo::Mode::GroundLanding);
        CharacterAirResourceState resource_model;resource_model.value736=input.resource_limit;
        u32 jump_start=0,dash_start=0,first_touch=0,first_entry=0;bool checked_touch=false,checked_root=false,checked_stale=false;
        std::weak_ptr<const NativeBorgMotionProducer> producer;BattleMvpVec first_floor{};
        for(unsigned t=0;t<600;++t){
            const bool eligible=source.landing_entry_accepted();const auto before_resource=source.resource();
            const auto roots=source.ordered_root().updates,jumps=source.landing_jumps(),dashes=source.landing_dashes();
            const auto before=world.actors()[0];const auto old_pose=pose;GamePadState pad;
            CharacterAirResourceState recovered;
            check(character_air_resource_recover(resource_model,0,source.movement_flags(),source.cached_airborne87(),
                input.resource_limit,input.recovery_increment,&recovered),"previous accepted cached87 controls common recovery");
            if(ledge){if(t>=5&&(!source.ground_loss_entries()||source.ground_loss_timer698()>0)&&source.landings()==0)pad.stick_y=-127;}
            else pad.b=t>=5&&t<35;
            if(second_jump&&!jump_start&&eligible&&source.landings()==1)jump_start=t;
            if(jump_start&&t<jump_start+20)pad.b=true;
            if(!dash_start&&eligible&&source.landings()>=(second_jump?2u:1u))dash_start=t;
            if(dash_start&&t<dash_start+12)pad.stick_y=127;
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;
            check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error)&&
                source.resource()==before_resource&&source.ordered_root().updates==roots&&source.landing_jumps()==jumps&&source.landing_dashes()==dashes,
                "landing admission, VM, root/resource/counters remain staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"same producer through jump/ledge/landing interruptions");
            auto refuses=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto tick=world.tick();
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==before.position&&
                    source.resource()==before_resource&&source.landing_jumps()==jumps&&source.landing_dashes()==dashes&&
                    !old_pose.frame_scope.expired()&&world.motion_receipt_current(old_pose.receipt),why);
            };
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown||offer.kind==NativeBorgMotionKind::GroundLossTouchdown;
            if(hit&&!first_touch){
                first_touch=world.tick()+1;first_floor=offer.proposed_position;
                auto wrong=offer;wrong.landing_target=4;refuses(wrong,"touchdown is not an accepted landing-entry capability");
                wrong=offer;wrong.landing_target=12;refuses(wrong,"no dash may borrow the physical touchdown result");
            }
            if(offer.landing_target){
                check(eligible&&!before.vertical_speed&&before.grounded,"only previously accepted landing entry may interrupt");
                auto wrong=offer;wrong.landing_target=0;refuses(wrong,"missing landing provenance cannot skip pending landing completion");
                wrong=offer;wrong.landing_target=255;refuses(wrong,"unknown landing target refuses");
                wrong=offer;wrong.landing_target=offer.landing_target==4?12:4;refuses(wrong,"target must match actual takeoff/dash role");
                if(offer.landing_target==4){
                    check(offer.kind==NativeBorgMotionKind::JumpTakeoff&&offer.jump_motion&&offer.jump_motion->entry&&
                        offer.proposed_position==before.position&&!offer.vertical_speed&&!offer.ordered_ground,
                        "landing jump selects takeoff2 without old landing update, movement or root advance");
                }else{
                    check(offer.kind==NativeBorgMotionKind::Ground&&offer.ordered_ground&&offer.ordered_ground->phase==12&&
                        offer.ordered_ground->move_speed==25&&offer.ordered_ground->retained_speed==24.5f&&
                        offer.ordered_ground->root_sampled&&!offer.ordered_ground->root_scope.expired(),
                        "landing dash uses fresh qualified request/root and immediate ordered ground motion");
                    wrong=offer;wrong.ordered_ground->root_scope.reset();refuses(wrong,"landing permission alone cannot mint dash/root authority");
                    checked_root=true;
                }
            }
            if(dash_start&&t>dash_start&&offer.ordered_ground&&!checked_stale){
                auto wrong=offer;wrong.landing_target=12;
                refuses(wrong,"earlier landing target cannot be replayed during the subsequent dash");
                checked_stale=true;
            }
            auto expected_resource=recovered;
            if(offer.jump_motion&&offer.jump_motion->phase==5)check(character_air_resource_consume(expected_resource,
                input.costs[offer.jump_motion->entry?0:1],input.recovery_delay,1,&expected_resource),"exact accepted jump entry/continued cost candidate");
            check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
                source.pose(world,&pose)&&view.update_source(world,pose,&error),"native exact receipt before landing VM/pose/resource publication");
            resource_model=expected_resource;
            check(source.resource()==resource_model.value736,"resource follows shared cached-history recovery and exact jump costs, never an interruption shortcut");
            if(hit&&world.tick()==first_touch){
                check(!source.landing_entry_accepted()&&source.cached_airborne87()==1&&source.movement_programs()==std::array<u16,2>{u16(ledge?13:6),u16(ledge?13:6)},
                    "touchdown keeps captured falling program/cache; it is not landing entry");
                GamePadState early;early.b=true;NativeGRedGroundInput rejected;
                check(!source.map_input(early,&rejected,&error)&&!pose.frame_scope.expired(),"fresh jump on touchdown refuses before simulation");
                early.b=false;early.stick_y=127;
                check(source.map_input(early,&rejected,&error)&&std::abs(rejected.y)<=32,
                    "touchdown mapping does not manufacture a full airborne/landing flick");
                checked_touch=true;
            }
            if(first_touch&&!first_entry&&world.tick()==first_touch+1){
                first_entry=world.tick();
                check(source.landing_entry_accepted()&&source.cached_airborne87()==0&&source.movement_programs()==std::array<u16,2>{8,8}&&
                    world.actors()[0].position==first_floor,"accepted60228 landing entry precedes any interruption");
            }
            if(offer.landing_target==4)check(source.landing_jumps()==jumps+1&&source.resource()==recovered.value736&&
                source.ordered_root().updates==roots&&source.movement_programs()==std::array<u16,2>{2,2},
                "takeoff interruption counter publishes once, after common recovery with no extra root/launch cost");
            if(offer.landing_target==12)check(source.landing_dashes()==dashes+1&&source.resource()==recovered.value736&&
                source.ordered_root().updates==roots+1,"dash interruption publishes actual new root operation, not an old landing scalar");
            if(offer.landing_target){
                auto wrong=offer;wrong.landing_target=0;NativeBorgMotionReceipt receipt;
                check(!world.motion_receipt(wrong,&receipt)&&world.motion_receipt(offer,&receipt),"receipt binds landing target as well as root/geometry/velocity");
            }
        }
        check(checked_touch&&checked_root&&checked_stale&&first_entry==first_touch+1&&dash_start&&
            source.landing_jumps()==u64(second_jump)&&source.landing_dashes()==1&&source.landings()==(second_jump?2u:1u)&&
            source.selector()==0&&source.resource()==10000&&source.cached_airborne87()==0&&!source.air_dash_starts()&&!source.relaunches()&&
            !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "600-update normal/ground-loss landing jump/dash routes return idle without unrelated permissions");
        if(route==0)check(first_touch==85&&first_entry==86&&jump_start+1==87&&dash_start+1==152,
            "accepted-entry-driven demo milestones, not guessed clip-end timers");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"retirement revokes new landing-mode leases");
    }
    //Raw APIs cannot skip the landing callback merely because physical contact occurred.
    for(auto mode:{NativeGRedGroundDashDemo::Mode::GroundLanding,NativeGRedGroundDashDemo::Mode::GroundJump}){
        bind(false,mode);
        for(unsigned t=0;t<86;++t){
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);GamePadState pad;pad.b=t>=5&&t<35;
            NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;
            check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error)&&
                world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose),"landing refusal fixture");
            if(t==84&&mode==NativeGRedGroundDashDemo::Mode::GroundLanding){
                const auto position=world.actors()[0].position;NativeBorgMotionOffer bad;
                check(!source.prepare(world,view.source_binding(),frame,0,0,0x100u,&bad,&error)&&world.tick()==85&&
                    world.actors()[0].position==position&&pose.frame_scope.expired()&&!world.step({},false,&view),
                    "raw fresh jump cannot skip touchdown-to-landing entry and publishes no partial state");
                break;
            }
        }
        if(mode==NativeGRedGroundDashDemo::Mode::GroundJump){
            GamePadState jump;jump.b=true;NativeGRedGroundInput mapped;
            check(source.landing_entry_accepted()&&!source.map_input(jump,&mapped,&error),"older ground-jump mode remains landing-cancel refusing");
        }
        source.reject();
    }
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLanding),
        "new accepted-entry policy requires GFMM9 and owned ordered-root capability");
    std::printf("accepted landing-entry/target/root/VM/resource/pose: %u checks PASS; three600-frame original-stage routes plus refusal fixtures; native proxies only\n",checks);
}
