#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <type_traits>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"owned ordered ground: %s (%s)\n",why,error.c_str());std::abort();}}
class Files:public gf::IFileSystem{
public:
    bool edit_motions=false;
    bool file_size(const std::string& n,gf::u64* o)override{std::error_code e;const auto size=std::filesystem::file_size(n,e);if(e||!o)return false;*o=size;return true;}
    bool read(const std::string& n,std::vector<gf::u8>& o)override{gf::u64 s=0;if(!file_size(n,&s)||s>(32u<<20))return false;std::ifstream f(n,std::ios::binary);o.assign(std::istreambuf_iterator<char>(f),{});if(edit_motions&&n.ends_with("/g-red.motions")&&!o.empty())o.back()^=1;return o.size()==s;}
    bool exists(const std::string& n)override{return std::filesystem::exists(n);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}
int main(){
    using namespace gf;static_assert(!std::is_default_constructible_v<NativeBorgOrderedRootScope>);
    static_assert(!std::is_copy_constructible_v<NativeBorgOrderedRootScope>);
    Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("owned ordered ground: optional qualified GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world,neighbor;
    NativeBattleMvpView view,neighbor_view;NativeGRedGroundDashDemo source,foreign;NativeBorgAnimationSeed seed;
    check(render.initialize({1152,648,false,true,"owned ordered dash"},nullptr,&error)&&view.load(files,art,render,&error)&&
        neighbor_view.load(files,art,render,&error),"shared renderer/models");
    auto bind=[&](const BattleMvpConfig& config={}){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::OrderedGround),"fresh owned source bind");
    };
    bind();
    check(neighbor.start({},&neighbor_view)&&neighbor_view.update(neighbor,&error)&&neighbor_view.source_seed(neighbor,&seed)&&
        foreign.bind(files,art,pack,neighbor,seed,&error,NativeGRedGroundDashDemo::Mode::OrderedGround),"neighbor uses its own world/rig/library binding");
    NativeBorgMotionOffer foreign_entry;
    for(unsigned t=0;t<=5;++t){
        const auto frame=NativeBattleMvpView::movement_frame(neighbor,1152,648);NativeBorgMotionOffer offer;
        check(foreign.prepare(neighbor,neighbor_view.source_binding(),frame,0,t==5?-60:0,t==5?4u<<16:0,&offer,&error),"neighbor stages same numeric route");
        if(t==5)foreign_entry=offer;
        else check(neighbor.step({},false,&neighbor_view,nullptr,&frame,&offer)&&foreign.commit(neighbor,neighbor_view.source_binding(),&error),"neighbor idle accepted");
    }
    check(foreign_entry.ordered_ground&&!foreign_entry.ordered_ground->root_scope.expired(),"neighbor's own authentic entry lease is live");
    for(auto policy:{NativeBorgMotionPolicy::GroundOnly,NativeBorgMotionPolicy::WalkLedge,NativeBorgMotionPolicy::OrderedGround}){
        NativeBattleMvp unowned;check(unowned.start({},&view),"fresh neighboring native world");
        auto producer=unowned.new_motion_producer(policy);NativeBorgMotionObservation observation;
        check(producer&&unowned.observe_motion(&observation),"independent producer observation");
        NativeBorgMotionOffer borrowed{observation,observation.position,observation.facing,BattleMvpAction::GroundDash,1,producer};
        borrowed.ordered_ground=foreign_entry.ordered_ground;
        check(!unowned.step({},false,&view,nullptr,nullptr,&borrowed)&&!unowned.tick(),
            "older policies and unqualified new-policy producers cannot borrow sealed ordered samples");
    }
    NativeBorgSourcePose pose;std::weak_ptr<const NativeBorgOrderedRootScope> previous_root,last_root;
    BattleMvpVec stopped{};BattlePoint stopped_facing{};u32 first_entry=0,first_recovery=0,latch=0,zero=0,clear=0,reset=0;
    unsigned held_frames=0,evaluations=0,retained_done_reads=0;
    for(unsigned t=0;t<600;++t){
        auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer offer;
        if(t>=80&&t<100)check(native_borg_movement_frame({1,0},&frame),"native camera rotation fixture");
        const auto old_root=source.ordered_root();const auto old_entries=source.ordered_entries();const auto old_pose=pose;
        const bool held=t>=5&&t<17;
        check(source.prepare(world,view.source_binding(),frame,0,held?-60:0,held?4u<<16:0,&offer,&error)&&
            source.ordered_root().updates==old_root.updates&&source.ordered_root().speed1dac==old_root.speed1dac&&
            source.ordered_entries()==old_entries,"root observations, counters, VM and phase stay staged");
        auto refuses=[&](NativeBorgMotionOffer wrong,const char* why){
            const auto position=world.actors()[0].position;const auto tick=world.tick();
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==position&&
                source.ordered_root().updates==old_root.updates&&!old_pose.frame_scope.expired()&&
                world.motion_receipt_current(old_pose.receipt),why);
        };
        if(offer.ordered_ground){
            const auto& e=*offer.ordered_ground;
            if(e.phase==12){
                first_entry=world.tick()+1;
                auto wrong=offer;wrong.ordered_ground.reset();refuses(wrong,"ordered dash cannot strip numeric/lease evidence");
                wrong=offer;wrong.ordered_ground->root_scope.reset();refuses(wrong,"float root speed without sealed same-operation lease refuses");
                wrong=offer;wrong.ordered_ground->root_scope=foreign_entry.ordered_ground->root_scope;
                refuses(wrong,"identical numeric sample from a neighboring producer/world refuses");
                wrong=offer;wrong.ordered_ground->root_speed=1;refuses(wrong,"root scalar cannot be edited independently of its sealed observation");
                wrong=offer;++wrong.ordered_ground->root_updates;refuses(wrong,"root evaluation counter cannot be edited");
                wrong=offer;++wrong.ordered_ground->control_after;refuses(wrong,"VM control result must match the sealed operation");
                wrong=offer;wrong.ordered_ground->program_done=true;refuses(wrong,"sealed VM result cannot be relabelled completed");
                wrong=offer;wrong.proposed_position=wrong.observed.position;refuses(wrong,"legacy request-only entry cannot be acknowledged as ordered motion");
                wrong=offer;wrong.proposed_position[1]+=.5f;refuses(wrong,"ordered ground cannot manufacture lift or a new floor");
                wrong=offer;wrong.kind=NativeBorgMotionKind::JumpTakeoff;refuses(wrong,"root lease grants no airborne phase");
                wrong=offer;wrong.ordered_ground->phase=14;refuses(wrong,"recovery cannot skip source entry/active sequence");
                check(e.move_speed==25&&e.retained_speed==24.5f&&e.root_speed==0&&e.root_evaluated&&
                    !e.root_scope.expired(),"first entry moves immediately and seals actual request-derived zero1DAC");
            }
            if(e.phase==13&&!e.root_sampled){
                ++held_frames;check(!e.root_evaluated&&e.root_updates==old_root.updates&&e.root_speed==old_root.speed1dac&&
                    e.root_scope.expired(),"control-bit1 hold retains prior scalar without fabricating a new root evaluation");
            }
            if(e.phase==14&&!first_recovery){
                first_recovery=world.tick()+1;
                check(e.move_speed==f32(22.f*.9f)&&e.root_speed==12.5f&&e.root_sampled&&e.root_evaluated,
                    "first recovery advances actual AObj, then damps22 while keeping observed root12.5");
                auto wrong=offer;wrong.ordered_ground->root_scope=previous_root;refuses(wrong,"earlier accepted AObj lease is not the current post-VM read");
                wrong=offer;wrong.ordered_ground->move_speed=22;refuses(wrong,"legacy recovery scalar is not accepted");
                wrong=offer;wrong.ordered_ground->retained_speed=0;refuses(wrong,"recovery cannot reset before its done tail");
            }
            if(e.phase==14&&e.recovery546==1&&!latch){latch=world.tick()+1;check(e.move_speed==12.5f,"phase546 latches to actual root speed");}
            if(e.phase==14&&e.move_speed==0&&!zero){zero=world.tick()+1;stopped=offer.proposed_position;stopped_facing=offer.facing;}
            if(e.phase==14&&e.control_after==3)clear=world.tick()+1;
            if(clear&&world.tick()+1==clear+1)check(e.control_before==0,"one-time scoped61160 acknowledgement cleared1D0F after the real VM control3");
            if(e.program_done){
                reset=world.tick()+1;++retained_done_reads;
                check(!e.root_evaluated&&e.root_sampled&&e.root_updates==old_root.updates&&e.next_selector==0,
                    "done-only VM update seals a fresh retained-root read, not an invented root advance");
                auto wrong=offer;wrong.ordered_ground->root_scope=previous_root;
                refuses(wrong,"done read needs its own current-frame VM observation even with unchanged root speed/counter");
            }
            evaluations+=e.root_evaluated;
            if(e.root_sampled){previous_root=e.root_scope;last_root=e.root_scope;}
        }
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"matching native geometry/phase/root lease before source/pose publication");
        if(offer.ordered_ground){
            const auto& e=*offer.ordered_ground;
            check(source.ordered_root().updates==e.root_updates&&source.ordered_root().speed1dac==e.root_speed&&
                source.ordered_root().position==BattleMvpVec{},"root-speed ownership never applies a second actor-position update");
            auto edited=offer;++edited.ordered_ground->control_after;NativeBorgMotionReceipt receipt;
            check(!world.motion_receipt(edited,&receipt)&&world.motion_receipt(offer,&receipt),"receipts match every evidence field, not just position");
        }
        if(t>=28)check(world.actors()[0].position==stopped&&world.actors()[0].facing==stopped_facing,
            "zero root speed and idle do not slide or let camera rotation force body heading");
        if(t>=37)check(source.selector()==0&&source.movement_programs()==std::array<u16,2>{0,0},"next idle dispatch uses shared source VM");
    }
    check(first_entry==6&&first_recovery==19&&latch==24&&zero==29&&clear==32&&reset==37&&held_frames==6&&
        evaluations==25&&retained_done_reads==1&&source.ordered_entries()==1&&source.ordered_active_frames()==12&&
        source.ordered_recovery_frames()==19&&source.ordered_resets()==1&&source.ordered_root().updates==25&&
        source.resource()==10000&&!world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
        "bounded600-frame original-stage ordered route and stable idle; no combat or resource fabrication");
    GamePadState jump;jump.b=true;NativeGRedGroundInput mapped;const auto tick=world.tick();
    check(!source.map_input(jump,&mapped,&error)&&world.tick()==tick&&!pose.frame_scope.expired(),"jump mapper still refuses without advancing/revoking current frame");
    source.reject();check(last_root.expired()&&pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),
        "producer retirement revokes retained root and pose/frame leases");
    BattleMvpConfig edge;edge.arena_half=4500;edge.spawn[0]={1012.360657f,1403.599731f};edge.spawn_query_y[0]=482.747589f;bind(edge);
    for(unsigned t=0;t<5;++t){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer offer;
        check(source.prepare(world,view.source_binding(),frame,0,0,0,&offer,&error)&&world.step({},false,&view,nullptr,&frame,&offer)&&
            source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose),"fresh native ground spawn near original roof edge, no airborne teleport");
    }
    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer offer;const auto position=world.actors()[0].position;
    check(!source.prepare(world,view.source_binding(),frame,0,60,8u<<16,&offer,&error)&&world.tick()==5&&
        world.actors()[0].position==position&&!source.ordered_entries()&&!source.ordered_root().updates&&pose.frame_scope.expired()&&
        !world.step({},false,&view),"dash-to-ledge remains fatal refusal with no root/VM/counter mutation or fallback");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::OrderedGround),
        "new live permission requires distinct GFMM9 capability, not GFMM8's equal .9 operand");
    files.edit_motions=true;
    check(!source.bind(files,art,pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::OrderedGround)&&
        error.find("qualified original G-Red motion bank")!=std::string::npos,
        "edited motion-bank bytes cannot mint original AObj/root-speed authority; source file remains read-only");
    std::printf("owned ordered ground/root/VM/receipt/pose: %u checks PASS; entry=%u recovery=%u root-latch=%u stop=%u clear=%u reset=%u; native proxies only\n",
        checks,first_entry,first_recovery,latch,zero,clear,reset);
}
