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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground-loss surface: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground-loss surface: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"timer-bound ground-loss surface"},nullptr,&error)&&view.load(files,art,render,&error),"shared original stage/models/renderer");
    struct Route {f32 x,z,query;u8 direction;u32 departure,touchdown;f32 timer;s16 angle;bool hold_timer=false;};
    const Route routes[]={
        {-1368.47f,-628.674f,94.339f,10,120,139,12,3911},
        {-1666.667f,-333.333f,94.339f,10,90,114,7,3676,true},
        {-3941.015f,3.646f,161.944f,9,64,93,2,2038},
        {-1586.996f,-157.15f,94.339f,10,103,135,0,354},
        {-1368.47f,-466.667f,94.339f,10,121,148,4,0},
        {1010.166015625f,2315.240722656f,482.747558594f,1,85,117,0,0}};
    auto bind=[&](const Route& r,NativeGRedGroundDashDemo::Mode mode){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={r.x,r.z};config.spawn_query_y[0]=r.query;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "declared original-stage grounded placement/query reference and4500 bounds; no midair teleport/rebind");
    };
    auto pad_at=[&](unsigned t,const Route& r){
        GamePadState pad;if(t>=5&&(!source.ground_loss_entries()||(r.hold_timer&&source.ground_loss_timer698()>0))){
            pad.dpad_up=r.direction&1u;pad.dpad_down=r.direction&2u;pad.dpad_left=r.direction&4u;pad.dpad_right=r.direction&8u;
        }return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"timer-bound native surface acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&view.update_source(world,pose,&error),
            "complete receipt before frame history/timer/VM/contact/resource/pose publication");
    };
    for(unsigned index:{0u,3u}){
        const auto& r=routes[index];bind(r,NativeGRedGroundDashDemo::Mode::GroundSurface);bool refused=false;
        for(unsigned t=0;t<200&&!refused;++t){
            NativeBorgMotionOffer offer;const auto retained=pose;const auto meter=source.resource();const auto timer=source.ground_loss_timer698();
            if(!prepare(pad_at(t,r),&offer)){
                check(world.tick()==r.touchdown-1&&source.resource()==meter&&source.ground_loss_timer698()==timer&&!source.landings()&&
                    retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                    "previous ordinary-surface policy still refuses nonflat ground-loss contact for positive and zero timer");
                refused=true;
            }else accept(offer);
        }
        check(refused,"new timer-bound permission does not widen older controller");
    }
    for(const auto& r:routes){
        bind(r,NativeGRedGroundDashDemo::Mode::GroundLossSurface);
        std::weak_ptr<const NativeBorgMotionProducer> producer;u32 departed=0,entered=0,touchdown=0;BattleMvpVec landed{};
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;const auto timer=source.ground_loss_timer698();
            const auto entries=source.ground_loss_entries(),landings=source.ground_loss_landings();NativeBorgMotionOffer offer;
            check(prepare(pad_at(t,r),&offer)&&source.ground_loss_timer698()==timer&&source.ground_loss_entries()==entries&&
                source.ground_loss_landings()==landings&&source.resource()==10000,"frame history/countdown/contact/counters and meter remain staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer through walk/departure/falling/contact/landing/idle");
            const bool hit=offer.kind==NativeBorgMotionKind::GroundLossTouchdown;
            check(bool(offer.surface_landing)==hit&&offer.walk_ledge&&offer.walk_ledge->frame_start==before.position,
                "exact native frame-start ledger; callback proof only at actual ground-loss touchdown");
            if(offer.kind==NativeBorgMotionKind::GroundDeparture)departed=t+1;
            if(offer.kind==NativeBorgMotionKind::GroundLossEntry)entered=t+1;
            if(native_borg_motion_ground_loss(offer.kind)&&offer.walk_ledge->timer698>0)
                check(!offer.walk_ledge->drift_heading&&offer.facing==before.facing,"positive698 suppresses held direct drift and does not force body heading");
            auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==before.position&&
                    source.ground_loss_timer698()==timer&&source.ground_loss_landings()==landings&&!retained.frame_scope.expired()&&
                    world.motion_receipt_current(retained.receipt),why);
            };
            if(hit){
                touchdown=t+1;landed=offer.proposed_position;const auto& e=*offer.surface_landing;
                check(touchdown==r.touchdown&&e.timer698==r.timer&&offer.walk_ledge->timer698==e.timer698&&e.angle==r.angle&&
                    e.angle<=(e.timer698>0?6144:10240)&&e.surface==offer.walk_ledge->surface&&
                    e.integrated_position[1]<=e.surface.height&&e.acceleration==-1&&
                    e.vertical_speed==std::max(-30.f,f32(before.vertical_speed-1.f))&&offer.vertical_speed==0&&
                    offer.jump_motion&&offer.jump_motion->ground_loss&&offer.jump_motion->phase==6&&offer.jump_motion->next_selector==7,
                    "selected plane and actual timer are jointly bound to ground-loss integration/program13 lineage");
                auto wrong=offer;wrong.surface_landing.reset();rejects(wrong,"new ground-loss callback cannot borrow old flat-only acceptance");
                wrong=offer;wrong.surface_landing->timer698=e.timer698?0:1;rejects(wrong,"surface packet cannot choose another timer/gate");
                wrong=offer;wrong.walk_ledge->timer698=e.timer698?0:1;rejects(wrong,"native accepted countdown cannot be replaced by caller fields");
                wrong=offer;wrong.surface_landing->timer698=0;wrong.walk_ledge->timer698=0;
                if(e.timer698>0)rejects(wrong,"matching forged zero timers cannot evade owned previous-frame decay");
                wrong=offer;wrong.walk_ledge->frame_start[1]+=1;rejects(wrong,"rendered/old history cannot substitute for native frame start");
                wrong=offer;wrong.surface_landing->surface.normal={0,1,0};if(e.angle)rejects(wrong,"guessed UP normal cannot replace paired selected plane");
                wrong=offer;++wrong.surface_landing->surface.triangle;rejects(wrong,"selected triangle identity remains required");
                wrong=offer;wrong.surface_landing->integrated_position[0]+=1;rejects(wrong,"pre-snap position must match actual damped travel");
                wrong=offer;wrong.surface_landing->vertical_speed=0;rejects(wrong,"post-callback zero cannot replace pre-response velocity");
                wrong=offer;wrong.kind=NativeBorgMotionKind::JumpTouchdown;rejects(wrong,"ordinary jump landing cannot borrow ground-loss history/timer");
                wrong=offer;wrong.kind=NativeBorgMotionKind::StaticPush;rejects(wrong,"timer-bound surface landing does not enable push");
                if(e.timer698>0){
                    AnglePlane angled(view);wrong=offer;wrong.surface_landing->surface.normal=angled.normal;
                    wrong.walk_ledge->surface.normal=angled.normal;
                    check(source_surface_angle(angled.normal,&wrong.surface_landing->angle)&&wrong.surface_landing->angle>6144&&
                        wrong.surface_landing->angle<=10240,"numeric inter-gate plane fixture");
                    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    check(!world.step({},false,&angled,nullptr,&frame,&wrong)&&world.tick()==t&&!retained.frame_scope.expired(),
                        "coherent45-degree plane metadata cannot borrow ordinary AC gate while timer698 is positive");
                }
            }
            accept(offer);
            check(source.resource()==10000&&!source.surface_pushes()&&!source.air_dash_starts()&&!source.relaunches()&&!source.ordered_root().updates,
                "walking ground loss invents no resource spending/refill/root/flight/push");
            if(hit){
                check(source.ground_loss_timer698()==r.timer&&source.cached_airborne87()==1&&source.movement_programs()==std::array<u16,2>{13,13}&&
                    world.actors()[0].grounded&&source.surface_landings()==1,"callback retains timer/captured13/cache1; no touchdown landing-entry shortcut");
                NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.surface_landing->timer698+=1;
                check(world.motion_receipt(offer,&receipt)&&!world.motion_receipt(wrong,&receipt),"receipt seals timer-bound contact as well as scalar/history");
            }
            if(touchdown&&t+1==touchdown+1)check(source.cached_airborne87()==0&&source.movement_programs()==std::array<u16,2>{8,8}&&
                source.ground_loss_timer698()==std::max(0.f,r.timer-1),"separate landing8 and continuing timer decay, not a fabricated698 reset");
            if(touchdown&&t+1>touchdown)check(world.actors()[0].position==landed,"stable nonflat/flat landing and idle");
        }
        check(departed==r.departure&&entered==departed+1&&touchdown==r.touchdown&&source.ground_departures()==1&&
            source.ground_loss_entries()==1&&source.ground_loss_landings()==1&&source.landings()==1&&source.surface_landings()==1&&
            source.selector()==0&&source.ground_loss_timer698()==0&&source.movement_programs()==std::array<u16,2>{0,0}&&
            !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "six genuine600-update positive/zero timer, inclined/flat ground-loss routes return idle");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"timer-bound pose/receipt retirement");
    }
    //698 is not reset by touchdown. Neither old target4/12 evidence nor a new
    //press may authorize an unreviewed landing interruption while it persists.
    const auto& r=routes[0];bind(r,NativeGRedGroundDashDemo::Mode::GroundLossSurface);
    for(unsigned t=0;t<140;++t){NativeBorgMotionOffer offer;check(prepare(pad_at(t,r),&offer),"positive timer landing fixture");accept(offer);}
    NativeBorgMotionOffer offer;check(prepare({},&offer),"staged continuing landing");
    auto wrong=offer;wrong.kind=NativeBorgMotionKind::JumpTakeoff;wrong.action=BattleMvpAction::Jump;wrong.landing_target=4;wrong.walk_ledge.reset();
    wrong.jump_motion=NativeBorgJumpMotion{};wrong.jump_motion->entry=true;wrong.jump_motion->held_jump=true;wrong.jump_motion->resource_before=10000;
    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==140&&!pose.frame_scope.expired(),
        "native landing target cannot erase a positive owned698 boundary");
    accept(offer);const auto retained=pose;GamePadState jump;jump.b=true;
    check(!prepare(jump,&offer)&&world.tick()==141&&source.ground_loss_timer698()==10&&retained.frame_scope.expired()&&
        !world.step({},false,&view),"source positive698 landing input refuses before cancellation/refill/fallback");
    bind(r,NativeGRedGroundDashDemo::Mode::GroundLossSurface);
    for(unsigned t=0;t<151;++t){check(prepare(pad_at(t,r),&offer),"natural timer expiry fixture");accept(offer);}
    check(source.ground_loss_timer698()==0&&prepare(jump,&offer)&&offer.landing_target==4,"expired timer restores earlier accepted-landing jump permission");
    accept(offer);check(source.movement_programs()==std::array<u16,2>{2,2},"reviewed jump entry after actual expiry, not a forced timer reset");
    BattleMvpConfig edge;edge.arena_half=4500;edge.spawn[0]={1012.360657f,1403.599731f};edge.spawn_query_y[0]=482.747589f;
    check(world.start(edge,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        source.bind(files,art,pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLossSurface),"fresh dash-ledge timer fixture");
    for(unsigned t=0;t<7;++t){GamePadState pad;if(t==5)pad.stick_y=-127;check(prepare(pad,&offer),"dash departure");accept(offer);}
    const auto position=world.actors()[0].position;const auto edge_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!source.prepare(world,view.source_binding(),edge_frame,60,0,1u<<16,&offer,&error)&&world.tick()==7&&world.actors()[0].position==position&&
        !source.air_dash_starts()&&pose.frame_scope.expired()&&!world.step({},false,&view),"positive698 contact support does not enable air-dash overlap");

    //The positive timer classifies this genuine ledge-fall approach as steep,
    //but the native pre-contact travel still crosses a raised face. A plausible
    //post-projection endpoint must not be used to skip either check.
    const Route raised{-1502.66699f,-97.3330078f,94.339f,10,138,0,28,9172,true};
    bind(raised,NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush);
    NativeBorgMotionOffer last;
    while(world.tick()<140){check(prepare(pad_at(world.tick(),raised),&last),"original raised-face approach");accept(last);}
    check(source.ground_departures()==1&&source.ground_loss_entries()==1&&source.ground_loss_timer698()==raised.timer+1&&
        last.kind==NativeBorgMotionKind::GroundLossAir&&last.walk_ledge&&last.walk_ledge->timer698==raised.timer+1&&
        !last.walk_ledge->drift_heading&&source.movement_programs()==std::array<u16,2>{13,13},
        "accepted countdown and suppressed drift come from a genuine ledge departure");
    const auto falling=world.actors()[0];const auto current=pose;const auto resource=source.resource();
    f32 x=0,z=0;source_heading_xz(last.walk_ledge->travel5ae,&x,&z);
    const auto speed=f32(last.walk_ledge->speed44*bits_to_f32(0x3f7d70a4u));
    auto integrated=falling.position;
    integrated[0]=std::fma(1.f,f32(speed*x),integrated[0]);
    integrated[1]=std::fma(falling.vertical_speed,1.f,integrated[1]);
    integrated[2]=std::fma(1.f,f32(speed*z),integrated[2]);
    NativeBorgStaticSurface surface;BattleMvpVec projected;s16 angle=0;
    check(view.static_surface(integrated[0],integrated[2],falling.position[1],&surface)&&surface.triangle==2747&&
        source_surface_angle(surface.normal,&angle)&&angle==raised.angle&&angle>6144&&angle<=10240&&
        surface.height>falling.position[1]&&source_surface_push_position(integrated,surface.normal,surface.height,angle,&projected),
        "actual plane crosses the positive timer angle gate and lies above the current actor");
    f32 projected_floor=0;
    check(view.height(projected[0],projected[2],falling.position[1],&projected_floor)&&projected[1]>=projected_floor-.01f,
        "post-projection height alone would not reject the candidate");
    const auto& other=world.actors()[1];const auto& bounds=world.config();
    NativeBorgContactRequest travel{falling.position,other.position,
        {integrated[0]-falling.position[0],integrated[2]-falling.position[2]},bounds.arena_half,bounds.actor_radius,
        bounds.actor_height,bounds.step_height,false,other.health!=0};
    travel.vertical_end_y=integrated[1];NativeBorgContactResult contact;
    check(native_borg_resolve_contact(travel,&view,&contact)&&contact.terrain_blocked&&!contact.body_hit&&!contact.bounds_hit,
        "native pre-contact travel refuses the raised face independently of body or bounds");
    check(!prepare(pad_at(140,raised),&offer)&&error.find("steep push")!=std::string::npos&&world.tick()==140&&
        world.actors()[0].position==falling.position&&source.ground_loss_timer698()==raised.timer+1&&source.resource()==resource&&
        !source.landings()&&!source.surface_pushes()&&source.movement_programs()==std::array<u16,2>{13,13}&&
        current.frame_scope.expired()&&!world.motion_receipt_current(current.receipt)&&!world.step({},false,&view),
        "inter-gate ledge-fall contact cannot commit a push, landing, timer step or fallback");

    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLossSurface),"combined ground-loss surface requires GFMM9");
    std::printf("ground-loss/surface/timer-angle/history/receipt: %u checks PASS; six600-update genuine routes plus old-policy/positive-input/air-overlap refusals; native proxies only\n",checks);
}
