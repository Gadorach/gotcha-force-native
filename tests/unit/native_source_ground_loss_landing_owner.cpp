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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground-loss landing input: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem{
public:
    bool file_size(const std::string& n,gf::u64* o)override{std::error_code e;const auto s=std::filesystem::file_size(n,e);if(e||!o)return false;*o=s;return true;}
    bool read(const std::string& n,std::vector<gf::u8>& o)override{gf::u64 s=0;if(!file_size(n,&s)||s>(32u<<20))return false;std::ifstream f(n,std::ios::binary);o.assign(std::istreambuf_iterator<char>(f),{});return o.size()==s;}
    bool exists(const std::string& n)override{return std::filesystem::exists(n);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
class WithoutInclineJoin final:public gf::IBattleMvpTerrain{
    const gf::NativeBattleMvpView& view_;
public:
    explicit WithoutInclineJoin(const gf::NativeBattleMvpView& view):view_(view){}
    bool height(gf::f32 x,gf::f32 z,gf::f32 y,gf::f32* out)const override{return view_.height(x,z,y,out);}
    bool static_surface(gf::f32 x,gf::f32 z,gf::f32 y,gf::NativeBorgStaticSurface* out)const override{
        return view_.static_surface(x,z,y,out);
    }
};
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground-loss landing input: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"root-owned retained698 landing dash"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    view.source_practice_controls();
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={-1368.47f,-466.667f};config.spawn_query_y[0]=94.339f;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "declared native grounded placement/reference on original terrain,4500 bounds; no midair injection/rebind");
    };
    auto pad_at=[&](unsigned t,unsigned hold,bool jump=false){
        GamePadState pad;if(t>=5&&!source.ground_loss_entries()){pad.dpad_down=true;pad.dpad_right=true;}
        if(t>=149&&t<149+hold){if(jump)pad.b=true;else pad.stick_y=127;}
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"retained timer/root/landing acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "complete matching native receipt before VM/root/timer/resource/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::GroundLossJump);bool old_refused=false;
    for(unsigned t=0;t<170&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto retained=pose;
        if(!prepare(pad_at(t,1),&offer)){
            check(world.tick()==149&&source.ground_loss_timer698()==3&&!source.landing_dashes()&&!source.ordered_root().updates&&
                source.landings()==1&&retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&
                error.find("positive ground-loss timer698")!=std::string::npos&&!world.step({},false,&view),
                "previous jump-only policy refuses genuine flat positive698 dash before root publication");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"new dash permission does not widen earlier mode");
    for(unsigned route=0;route<3;++route){
        bind(NativeGRedGroundDashDemo::Mode::GroundLossLanding);
        const bool jump=route==2;const unsigned hold=jump?15:route==1?8:1;
        std::weak_ptr<const NativeBorgMotionProducer> producer;std::weak_ptr<const NativeBorgOrderedRootScope> previous_root;
        u32 target=0,recovery=0,idle=0;unsigned roots=0,root_frames=0;bool zero_control=false,done_only=false;BattleMvpVec final{};
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto timer=source.ground_loss_timer698();
            const auto count=source.landing_dashes();const auto updates=source.ordered_root().updates;NativeBorgMotionOffer offer;
            check(prepare(pad_at(t,hold,jump),&offer)&&source.ground_loss_timer698()==timer&&source.landing_dashes()==count&&
                source.ordered_root().updates==updates,"admission, VM/root samples, scalar, timer and counters staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"same producer across walk/loss/landing/owned dash or jump");
            const f32 expected=offer.kind==NativeBorgMotionKind::GroundLossEntry?30.f:std::max(0.f,timer-1.f);
            check(offer.controller_timer698&&*offer.controller_timer698==expected&&
                (!offer.walk_ledge||offer.walk_ledge->timer698==expected),"current timer carried independently of selected movement role");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==before.position&&
                    source.ground_loss_timer698()==timer&&source.landing_dashes()==count&&source.ordered_root().updates==updates&&
                    !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            if(offer.kind==NativeBorgMotionKind::GroundLossTouchdown){
                check(t+1==148&&expected==4&&source.cached_airborne87()==1,"real callback still precedes landing input");
                auto wrong=offer;wrong.landing_target=12;rejects(wrong,"physical touchdown does not authorize dash cancellation");
            }
            if(offer.landing_target){
                target=t+1;check(target==150&&expected==2&&before.grounded&&before.vertical_speed==0,"accepted landing8 before positive698 interruption");
                check(view.practice_readout(world)&&!view.practice_readout(world)->landing_dash_started,
                    "staging does not replace the accepted landing readout");
                auto wrong=offer;wrong.landing_target=0;rejects(wrong,"missing landing provenance cannot bypass pending landing completion");
                wrong=offer;wrong.landing_target=255;rejects(wrong,"unknown landing target refused");
                wrong=offer;wrong.controller_timer698=0;rejects(wrong,"dash or jump cannot clear retained698");
                check(view.practice_readout(world)&&!view.practice_readout(world)->landing_dash_started,
                    "a refused replacement leaves the landing readout unchanged");
                if(!jump){
                    const auto& e=*offer.ordered_ground;
                    check(offer.landing_target==12&&offer.kind==NativeBorgMotionKind::Ground&&e.phase==12&&e.move_speed==25&&e.retained_speed==24.5f&&
                        e.root_sampled&&e.root_evaluated&&!e.root_scope.expired()&&e.root_updates==1&&offer.walk_ledge&&
                        !offer.jump_motion&&!offer.surface_landing,"genuine landing dash uses immediate ordered25 motion and privately owned post-VM root");
                    wrong=offer;wrong.ordered_ground->root_scope.reset();rejects(wrong,"landing/timer permission alone cannot mint root authority");
                    wrong=offer;wrong.ordered_ground->move_speed=24.5f;rejects(wrong,"entry moves25 before retaining24.5, not a rendered-displacement guess");
                }else check(offer.landing_target==4&&offer.kind==NativeBorgMotionKind::JumpTakeoff&&offer.proposed_position==before.position&&
                    !offer.ordered_ground,"inherited positive jump remains ordinary no-motion takeoff2");
            }
            if(offer.ordered_ground){
                const auto& e=*offer.ordered_ground;++root_frames;roots=unsigned(e.root_updates);
                if(e.phase==14&&!recovery)recovery=t+1;
                if(e.phase==14&&!e.control_after)zero_control=true;
                if(e.phase==14&&e.root_sampled&&!e.root_evaluated)done_only=true;
                if(e.root_sampled){
                    auto wrong=offer;wrong.ordered_ground->root_scope=previous_root;
                    if(!previous_root.expired())rejects(wrong,"last frame's root lease cannot bind a new post-VM observation");
                    previous_root=e.root_scope;
                }
                if(t+1>150){auto wrong=offer;wrong.landing_target=12;rejects(wrong,"landing-entry provenance cannot be replayed during active/recovery dash");}
            }
            if(!jump&&target&&!offer.ordered_ground&&!idle){idle=t+1;final=offer.proposed_position;}
            accept(offer);
            const auto shown=view.practice_readout(world);
            check(shown&&shown->landing_dash_started==(offer.landing_target==12),
                "dash-from-landing cue is published only on the accepted target12 frame");
            check(source.ground_loss_timer698()==expected&&!source.surface_pushes()&&!source.air_dash_starts()&&!source.relaunches(),
                "matching acceptance commits the actual timer and no push/flight capability");
            NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.controller_timer698=expected+1;
            check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"complete receipt seals timer even on root/VM hold frames");
            if(t+1==150)check(source.resource()==10000&&source.ground_loss_timer698()==2&&source.cached_airborne87()==0,
                "positive landing interruption does not invent an air cost/cache/refill");
            if(!jump&&idle&&t+1>idle)check(world.actors()[0].position==final,"stable reset/idle after owned recovery tail");
        }
        check(target==150&&source.landings()==(jump?2u:1u)&&source.landing_dashes()==unsigned(!jump)&&
            source.landing_jumps()==unsigned(jump)&&source.ground_loss_landings()==1&&source.resource()==10000&&source.ground_loss_timer698()==0&&
            source.selector()==0&&source.movement_programs()==std::array<u16,2>{0,0}&&!world.shots_spawned()&&
            world.actors()[0].health==100&&world.actors()[1].health==100,"three600-update release/held-root/jump routes finish in shared idle");
        if(!jump)check(recovery==(route==1?159u:157u)&&idle==(route==1?178u:176u)&&roots==25&&source.ordered_root().updates==25&&
            root_frames>25&&zero_control&&done_only,"actual AObj successors/holds, zero-control retained-root reads and motion-before-done/reset preserved");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"retirement revokes timer/root/pose receipts");
    }
    //The angle/geometry refusal is independent of this admission capability.
    bind(NativeGRedGroundDashDemo::Mode::GroundLossLanding);
    for(unsigned t=0;t<149;++t){NativeBorgMotionOffer offer;check(prepare(pad_at(t,0),&offer),"nonflat destination fixture");accept(offer);}
    GamePadState forward;forward.stick_y=-127;NativeBorgMotionOffer unsupported;const auto retained=pose;
    check(!prepare(forward,&unsupported)&&world.tick()==149&&source.ground_loss_timer698()==3&&!source.landing_dashes()&&!source.ordered_root().updates&&
        retained.frame_scope.expired()&&error.find("unresolved/nonflat destination")!=std::string::npos&&!world.step({},false,&view),
        "new landing permission does not silently authorize nonflat ordered-dash terrain or publish a staged root");
    BattleMvpConfig edge;edge.arena_half=4500;edge.spawn[0]={1012.360657f,1403.599731f};edge.spawn_query_y[0]=482.747589f;
    check(world.start(edge,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        source.bind(files,art,pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLossLanding),"fresh positive698 air-overlap fixture");
    for(unsigned t=0;t<7;++t){GamePadState pad;if(t==5)pad.stick_y=-127;NativeBorgMotionOffer offer;check(prepare(pad,&offer),"dash ledge");accept(offer);}
    const auto position=world.actors()[0].position;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!source.prepare(world,view.source_binding(),frame,60,0,1u<<16,&unsupported,&error)&&world.tick()==7&&world.actors()[0].position==position&&
        !source.air_dash_starts()&&pose.frame_scope.expired()&&!world.step({},false,&view),"positive698 air dash stays refused independently of ground dash");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLossLanding),"retained-timer owned dash requires GFMM9");
    using Mode=NativeGRedGroundDashDemo::Mode;
    using Kind=NativeBorgMotionKind;
    BattleMvpConfig incline;incline.arena_half=4500;incline.spawn[0]={-1719.38672f,-508.356781f};incline.spawn_query_y[0]=343.13092f;
    auto bind_incline=[&](Mode mode){
        check(world.start(incline,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"declared native grounded ledge above original incline");
    };
    auto incline_pad=[](unsigned tick,unsigned flick){
        GamePadState pad;pad.dpad_left=tick>=6&&tick<=9;
        if(tick==flick)pad.stick_x=-127;
        return pad;
    };
    {
        bind_incline(Mode::GroundLossLanding);
        for(unsigned tick=1;tick<=33;++tick){
            NativeBorgMotionOffer offer;check(prepare(incline_pad(tick,34),&offer),"old mode reaches the same entered landing");accept(offer);
        }
        const auto retained=pose;const auto before=world.actors()[0].position;
        check(!prepare(incline_pad(34,34),&unsupported)&&world.tick()==33&&world.actors()[0].position==before&&
            source.ground_loss_timer698()==6.f&&!source.ordered_root().updates&&!source.landing_dashes()&&
            error.find("ordered dash: ground motion")!=std::string::npos&&
            retained.frame_scope.expired()&&!world.step({},false,&view),
            "the older flat-only policy still refuses this incline before publishing the dash root");
    }
    bind_incline(Mode::GroundSlopeTransitionDash);
    for(unsigned tick=1;tick<=70;++tick){
        NativeBorgMotionOffer offer;check(prepare(incline_pad(tick,33),&offer)&&!offer.landing_target&&!offer.ordered_ground,
            "a lone flick before the accepted landing entry does not cancel or queue a dash");
        if(tick==33||tick==34)check(offer.kind==Kind::JumpLanding,"early flick leaves the landing callbacks intact");
        accept(offer);
    }
    check(!source.landing_dashes()&&!source.ordered_root().updates&&!source.selector(),
        "early flick leaves a completed landing, not a delayed dash");
    bind_incline(Mode::GroundSlopeTransitionDash);
    unsigned resets=0;
    std::weak_ptr<const NativeBorgMotionProducer> incline_producer;
    for(unsigned tick=1;tick<=70;++tick){
        const auto before=world.actors()[0];const auto retained=pose;const auto prior_timer=source.ground_loss_timer698();
        const auto roots=source.ordered_root().updates;
        NativeBorgMotionOffer offer;check(prepare(incline_pad(tick,34),&offer)&&source.ground_loss_timer698()==prior_timer&&
            source.ordered_root().updates==roots,"prepare uses the same accepted clock, producer and staged root");
        if(tick==1)incline_producer=offer.producer;
        check(!offer.producer.owner_before(incline_producer)&&!incline_producer.owner_before(offer.producer),
            "one producer through ground loss, landing, incline dash and idle");
        const f32 timer=tick>=9&&tick<39?f32(39-tick):0.f;
        check(offer.controller_timer698==timer&&(!offer.walk_ledge||offer.walk_ledge->timer698==timer),
            "timer remains independent of planar slope motion and decays through 1 to 0");
        if(tick==32)check(offer.kind==Kind::GroundLossTouchdown&&offer.surface_landing&&
            offer.surface_landing->surface.triangle==1226&&offer.surface_landing->angle>0&&offer.surface_landing->angle<=6144&&
            !offer.landing_target&&timer==7.f,"positive-timer contact selects the genuine shallow incline, not an UP floor");
        if(tick==33)check(offer.kind==Kind::JumpLanding&&offer.jump_motion&&offer.jump_motion->entry&&timer==6.f&&
            !offer.landing_target,"landing entry is its own update before the dash");
        if(tick==34){
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            NativeBorgStaticSurface from;
            check(before.grounded&&before.vertical_speed==0&&view.static_surface(before.position[0],before.position[2],before.position[1],&from)&&
                from.triangle==1226&&offer.walk_ledge&&offer.walk_ledge->surface.triangle==1014&&
                !native_borg_flat_surface(from)&&!native_borg_flat_surface(offer.walk_ledge->surface)&&
                view.gentle_slope_crossing(1226,1014,before.position,offer.proposed_position)&&
                offer.proposed_position[1]==offer.walk_ledge->surface.height&&offer.proposed_position[1]<before.position[1]&&
                offer.kind==Kind::Ground&&offer.landing_target==12&&timer==5.f&&offer.ordered_ground&&
                offer.ordered_ground->phase==12&&offer.ordered_ground->move_speed==25&&offer.ordered_ground->retained_speed==24.5f&&
                offer.ordered_ground->root_updates==1&&!offer.ordered_ground->root_scope.expired(),
                "the first owned dash step crosses the shallow crease with its real heights, timer and fresh root");
            WithoutInclineJoin unverified(view);
            auto wrong=offer;wrong.controller_timer698=0.f;
            check(!world.step({},false,&unverified,nullptr,&frame,&offer)&&!world.step({},false,&view,nullptr,&frame,&wrong)&&
                world.tick()==33&&world.actors()[0].position==before.position&&world.motion_receipt_current(retained.receipt)&&
                !source.ordered_root().updates&&!source.landing_dashes()&&source.ground_loss_timer698()==6.f&&
                view.practice_readout(world)&&!view.practice_readout(world)->landing_dash_started,
                "missing native seam evidence or a forged timer leaves the accepted landing, root and cue untouched");
        }
        if(offer.ordered_ground&&!offer.ordered_ground->next_selector){
            ++resets;check(tick==59&&offer.ordered_ground->phase==14&&offer.ordered_ground->root_updates==25,
                "the dash finishes real ordered recovery before returning to idle");
        }
        accept(offer);
        const auto shown=view.practice_readout(world);
        check(shown&&shown->landing_dash_started==(tick==34)&&source.ground_loss_timer698()==timer&&source.resource()==10000&&
            !source.surface_pushes()&&!source.air_dash_starts()&&!source.relaunches(),
            "only accepted target12 publishes a cue; no air spend or steep-contact authority is added");
        if(tick>=32)check(world.actors()[0].grounded,"slope traversal is grounded after the distinct touchdown");
    }
    check(resets==1&&source.ground_departures()==1&&source.ground_loss_landings()==1&&source.landing_dashes()==1&&
        source.landings()==1&&source.selector()==0&&source.ordered_root().updates==25,"complete incline route returns to normal idle");
    std::printf("ground-loss/landing-input/root/timer/receipt: %u checks PASS; flat and incline routes with old-policy/nonflat/air refusals; native proxies only\n",checks);
}
