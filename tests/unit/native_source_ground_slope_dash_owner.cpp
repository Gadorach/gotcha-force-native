#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"

namespace {
unsigned checks=0;
std::string error;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"slope dash: %s (%s)\n",why,error.c_str());std::abort();}
}
class Files final:public gf::IFileSystem {
public:
    bool file_size(const std::string& n,gf::u64* o)override{
        std::error_code e;const auto s=std::filesystem::file_size(n,e);if(e||!o)return false;*o=s;return true;
    }
    bool read(const std::string& n,std::vector<gf::u8>& o)override{
        gf::u64 s=0;if(!file_size(n,&s)||s>(32u<<20))return false;
        std::ifstream in(n,std::ios::binary);o.assign(std::istreambuf_iterator<char>(in),{});return o.size()==s;
    }
    bool exists(const std::string& n)override{return std::filesystem::exists(n);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
class WithoutCreaseProof final:public gf::IBattleMvpTerrain {
    const gf::NativeBattleMvpView& view_;
public:
    explicit WithoutCreaseProof(const gf::NativeBattleMvpView& view):view_(view){}
    bool height(gf::f32 x,gf::f32 z,gf::f32 y,gf::f32* out)const override{return view_.height(x,z,y,out);}
    bool static_surface(gf::f32 x,gf::f32 z,gf::f32 y,gf::NativeBorgStaticSurface* out)const override{
        return view_.static_surface(x,z,y,out);
    }
};
}

int main(){
    using namespace gf;
    using Mode=NativeGRedGroundDashDemo::Mode;
    Files files;const std::string pack="local/content/source-gred-ground-v9",art="local/content/native-battle-mvp-v1";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("slope dash: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"slope dash"},nullptr,&error)&&view.load(files,art,render,&error),"original stage and models");
    struct Route {f32 x,z,reference;int stick;bool lateral=false;};
    const Route down{-1443.46997f,-703.674011f,94.339f,127},up{-2377.44727f,528.269836f,591.9127f,-127};
    auto bind=[&](const Route& r,Mode mode){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={r.x,r.z};config.spawn_query_y[0]=r.reference;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"grounded original-stage placement");
    };
    auto prepare=[&](const Route& r,NativeBorgMotionOffer* offer){
        GamePadState pad;if(world.tick()==5){if(r.lateral)pad.stick_x=r.stick;else pad.stick_y=r.stick;}
        NativeGRedGroundInput input;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&input,&error)&&source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"native acceptance before root/pose publication");
    };
    bind(down,Mode::GroundWindupEntryPush);
    while(world.tick()<5){NativeBorgMotionOffer offer;check(prepare(down,&offer),"older mode accepted prefix");accept(offer);}
    NativeBorgMotionOffer refused;
    check(!prepare(down,&refused)&&world.tick()==5&&!source.ordered_root().updates&&
        pose.frame_scope.expired()&&!world.step({},false,&view),"older mode still refuses slope dash before root publication");
    for(const auto route:{down,up}){
        bind(route,Mode::GroundSlopeDash);const f32 start_y=world.actors()[0].position[1];
        unsigned entries=0,moving=0,recovery=0,done=0;
        while(world.tick()<100){
            NativeBorgMotionOffer offer;const auto before=world.actors()[0];const auto retained=pose;
            check(prepare(route,&offer),"prepare source dash and idle");
            if(offer.ordered_ground){
                const auto& e=*offer.ordered_ground;NativeBorgStaticSurface a,b;
                check(offer.kind==NativeBorgMotionKind::Ground&&offer.walk_ledge&&
                    view.static_surface(before.position[0],before.position[2],before.position[1],&a)&&
                    view.static_surface(offer.proposed_position[0],offer.proposed_position[2],before.position[1],&b)&&
                    a.triangle==b.triangle&&a.normal==b.normal&&!native_borg_flat_surface(a)&&
                    offer.proposed_position[1]==b.height&&offer.walk_ledge->surface==b,"same inclined pair and exact endpoint height");
                entries+=e.phase==12;recovery+=e.phase==14;done+=e.phase==14&&!e.next_selector;
                moving+=offer.proposed_position[1]!=before.position[1];
                if(e.phase==12){
                    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    const auto tick=world.tick();
                    auto wrong=offer;wrong.proposed_position[1]=before.position[1];
                    check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&
                        world.motion_receipt_current(retained.receipt)&&!retained.frame_scope.expired(),"flat-height substitute refuses");
                    wrong=offer;wrong.ordered_ground->travel5ae=source_heading_wrap(s32(e.travel5ae)+8192);
                    check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&
                        world.actors()[0].position==before.position,"off-tangent root motion refuses");
                }
            }
            accept(offer);
            check(world.actors()[0].grounded&&world.actors()[0].vertical_speed==0&&source.resource()==10000,
                "slope dash is grounded movement, not departure/flight");
        }
        check(entries==1&&moving==17&&recovery>0&&done==1&&source.selector()==0&&
            (route.stick>0?world.actors()[0].position[1]<start_y:world.actors()[0].position[1]>start_y),
            "real uphill and downhill dashes complete ordered recovery and return to idle");
    }
    auto seam=down;seam.stick=-127;bind(seam,Mode::GroundSlopeDash);
    while(world.tick()<6){NativeBorgMotionOffer offer;check(prepare(seam,&offer),"seam approach prefix");accept(offer);}
    const auto before=world.actors()[0].position;
    check(!prepare(seam,&refused)&&world.tick()==6&&world.actors()[0].position==before&&
        pose.frame_scope.expired()&&!world.step({},false,&view),"different slope triangle is not authorized");
    const Route coplanar{1406.01709f,2922.08545f,485.585205f,127};
    bind(coplanar,Mode::GroundSlopeDash);
    while(world.tick()<32){
        NativeBorgMotionOffer offer;if(!prepare(coplanar,&offer))break;accept(offer);
    }
    const auto blocked_tick=world.tick();
    check(blocked_tick>=5&&blocked_tick<31&&pose.frame_scope.expired()&&!world.step({},false,&view),
        "the single-triangle policy still refuses this coplanar seam");
    bind(coplanar,Mode::GroundSlopeSeamDash);unsigned crossed=0;
    while(world.tick()<100){
        NativeBorgMotionOffer offer;
        check(prepare(coplanar,&offer),"prepare coplanar dash and recovery");
        if(offer.ordered_ground&&offer.walk_ledge){
            NativeBorgStaticSurface start;const auto& p=offer.observed.position;
            check(view.static_surface(p[0],p[2],p[1],&start),"selected starting plane");
            const auto& end=offer.walk_ledge->surface;
            if(start.triangle!=end.triangle){
                ++crossed;
                check(world.tick()==blocked_tick&&start.triangle==254&&end.triangle==255&&
                    view.coplanar_crossing(start.triangle,end.triangle,p,offer.proposed_position)&&start.normal==end.normal,
                    "exact native coplanar witness on the first previously refused tick");
            }
        }
        accept(offer);
        check(world.actors()[0].grounded&&world.actors()[0].vertical_speed==0&&source.resource()==10000,
            "coplanar slope dash remains grounded");
    }
    check(crossed==1&&source.selector()==0&&source.ordered_root().updates>0&&
        std::abs(world.actors()[0].position[1]-479.957f)<.01f,"complete dash/recovery across the original edge");
    bind(seam,Mode::GroundSlopeSeamDash);
    while(world.tick()<6){NativeBorgMotionOffer offer;check(prepare(seam,&offer),"noncoplanar seam prefix");accept(offer);}
    check(!prepare(seam,&refused)&&world.tick()==6&&pose.frame_scope.expired()&&!world.step({},false,&view),
        "a noncoplanar seam still refuses with the new policy");

    const Route crease{3589.76465f,-2425.93408f,192.512665f,127,true};
    bind(crease,Mode::GroundSlopeSeamDash);
    while(world.tick()<11){NativeBorgMotionOffer offer;check(prepare(crease,&offer),"crease approach");accept(offer);}
    check(!prepare(crease,&refused)&&world.tick()==11&&pose.frame_scope.expired()&&!world.step({},false,&view),
        "the exact-plane policy still refuses a shallow crease");
    bind(crease,Mode::GroundSlopeCreaseDash);crossed=0;
    while(world.tick()<100){
        NativeBorgMotionOffer offer;check(prepare(crease,&offer),"prepare shallow crease dash and recovery");
        if(offer.ordered_ground&&offer.walk_ledge){
            const auto& p=offer.observed.position;const auto& end=offer.walk_ledge->surface;NativeBorgStaticSurface start;
            check(view.static_surface(p[0],p[2],p[1],&start),"select the starting face");
            if(start.normal!=end.normal){
                ++crossed;
                check(world.tick()==11&&start.triangle==295&&end.triangle==508&&
                    !view.coplanar_crossing(start.triangle,end.triangle,p,offer.proposed_position)&&
                    view.gentle_slope_crossing(start.triangle,end.triangle,p,offer.proposed_position),
                    "the immutable stage certifies one actual shallow crease");
                const auto& e=*offer.ordered_ground;BattleMvpVec tangent;
                check(native_gred_ground_position(p,e.move_speed,e.travel5ae,start.normal,&tangent)&&
                    tangent[0]==offer.proposed_position[0]&&tangent[2]==offer.proposed_position[2]&&end.height==offer.proposed_position[1],
                    "crossing uses start-face tangent and the selected destination height");
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                const auto retained=pose;WithoutCreaseProof unverified(view);
                check(!world.step({},false,&unverified,nullptr,&frame,&offer)&&world.tick()==11&&
                    world.actors()[0].position==p&&world.motion_receipt_current(retained.receipt)&&!retained.frame_scope.expired(),
                    "native terrain must independently certify the edge before accepting the source offer");
            }
        }
        accept(offer);
        check(world.actors()[0].grounded&&world.actors()[0].vertical_speed==0&&source.resource()==10000,
            "crease traversal does not depart or spend flight meter");
    }
    check(crossed==1&&source.selector()==0&&std::abs(world.actors()[0].position[1]-150.908f)<.01f,
        "dash completes across the original crease and returns to idle");

    const struct Transition {Route route;u32 from,to,accepted_tick;f32 final_height;bool starts_flat;} transitions[]{
        {{830.733398f,2191.48145f,487.747528f,127},249,1052,11,479.49008f,true},
        {{-1692.49365f,-135.725586f,99.3391876f,-127,true},245,890,6,94.33919f,false},
    };
    for(const auto& t:transitions){
        bind(t.route,Mode::GroundSlopeCreaseDash);
        while(world.tick()+1<t.accepted_tick){
            NativeBorgMotionOffer offer;check(prepare(t.route,&offer),"approach mixed join in the earlier mode");accept(offer);
        }
        check(!prepare(t.route,&refused)&&world.tick()+1==t.accepted_tick&&pose.frame_scope.expired()&&
            !world.step({},false,&view),"the earlier crease mode still refuses the mixed join");
        bind(t.route,Mode::GroundSlopeTransitionDash);crossed=0;unsigned resets=0;
        while(world.tick()<100){
            const auto tick=world.tick();NativeBorgMotionOffer offer;
            check(prepare(t.route,&offer),"prepare mixed transition and ordered recovery");
            if(offer.ordered_ground&&offer.walk_ledge){
                const auto& e=*offer.ordered_ground;const auto& p=offer.observed.position;
                const auto& end=offer.walk_ledge->surface;NativeBorgStaticSurface start;
                check(view.static_surface(p[0],p[2],p[1],&start),"verify the starting floor independently");
                const bool start_flat=native_borg_flat_surface(start);
                if(start_flat!=native_borg_flat_surface(end)){
                    ++crossed;BattleMvpVec tangent;
                    check(tick+1==t.accepted_tick&&start.triangle==t.from&&end.triangle==t.to&&start_flat==t.starts_flat&&
                        view.gentle_slope_crossing(t.from,t.to,p,offer.proposed_position)&&
                        native_gred_ground_position(p,e.move_speed,e.travel5ae,start.normal,&tangent)&&
                        tangent[0]==offer.proposed_position[0]&&tangent[2]==offer.proposed_position[2]&&
                        end.height==offer.proposed_position[1]&&p[1]!=end.height,
                        "mixed crossing uses the real shared edge, start tangent and destination height");
                    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    const auto retained=pose;WithoutCreaseProof unverified(view);
                    auto wrong=offer;wrong.proposed_position[1]=p[1];
                    check(!world.step({},false,&unverified,nullptr,&frame,&offer)&&
                        !world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==p&&
                        world.motion_receipt_current(retained.receipt)&&!retained.frame_scope.expired(),
                        "missing native edge proof or wrong endpoint height preserves the previously accepted pose");
                }
                resets+=e.phase==14&&!e.next_selector;
            }
            accept(offer);
            check(world.actors()[0].grounded&&world.actors()[0].vertical_speed==0&&source.resource()==10000,
                "mixed dash is not a ledge departure and does not use flight meter");
        }
        check(crossed==1&&resets==1&&source.selector()==0&&std::abs(world.actors()[0].position[1]-t.final_height)<.01f,
            "one mixed join and one ordered recovery reach the original-stage endpoint");
    }
    const Route cliff{2582.89014f,-2846.36206f,116.592773f,127,true};
    bind(cliff,Mode::GroundSlopeTransitionDash);
    while(world.tick()<5){
        NativeBorgMotionOffer offer;check(prepare(cliff,&offer),"cliff approach in previous mode");accept(offer);
    }
    check(!prepare(cliff,&refused)&&world.tick()==5&&pose.frame_scope.expired()&&!world.step({},false,&view),
        "the flat/incline seam mode still refuses the cliff without fallback");
    bind(cliff,Mode::GroundInclinedDashLedge);
    unsigned departure=0,fall=0,touchdown=0,landing=0;
    while(world.tick()<100){
        const auto tick=world.tick();NativeBorgMotionOffer offer;
        check(prepare(cliff,&offer),"prepare inclined cliff dash, fall and ordinary recovery");
        using Kind=NativeBorgMotionKind;
        if(offer.kind==Kind::GroundDeparture){
            departure=tick+1;
            const auto& p=offer.observed.position;const auto& end=offer.proposed_position;
            check(departure==6&&offer.ordered_ground&&offer.walk_ledge&&offer.walk_ledge->surface.triangle==2060&&
                p[1]==end[1]&&!offer.vertical_speed&&view.inclined_ledge_crossing(827,2060,p,end,60),
                "depart from the incline through the real cliff strip, at unchanged height");
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            const auto retained=pose;WithoutCreaseProof unverified(view);
            const auto root=source.ordered_root().updates;
            auto wrong=offer;wrong.proposed_position[1]=offer.walk_ledge->surface.height;
            check(!world.step({},false,&unverified,nullptr,&frame,&offer)&&
                !world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==p&&
                world.motion_receipt_current(retained.receipt)&&!retained.frame_scope.expired()&&
                source.ordered_root().updates==root&&source.ground_loss_timer698()==0&&source.resource()==10000,
                "missing proof or snap cannot advance root, timer, meter, motion, or pose");
        }
        if(offer.kind==Kind::GroundLossEntry){fall=tick+1;check(offer.walk_ledge->timer698==30,"timer begins on fall entry");}
        if(offer.kind==Kind::GroundLossTouchdown){touchdown=tick+1;check(offer.proposed_position[1]==10,"accepted flat floor touchdown");}
        if(touchdown&&!landing&&offer.kind==Kind::JumpLanding)landing=tick+1;
        accept(offer);
        check(source.resource()==10000&&!source.surface_pushes(),"no meter spend or steep push in the cliff sequence");
    }
    check(departure==6&&fall==7&&touchdown==21&&landing==22&&source.ground_departures()==1&&
        world.actors()[0].grounded&&source.selector()==0&&source.ground_loss_timer698()==0,
        "full authored route goes from ordered departure through distinct touchdown and landing to idle");

    const Route approach{2599.00806f,-2758.12109f,125.757187f,0};
    // Both policies already had the ordinary walk handler. Certify a walking
    // route through the cliff without borrowing the ordered-dash permission.
    for(const auto mode:{Mode::GroundSlopeTransitionDash,Mode::GroundInclinedDashLedge}){
        bind(approach,mode);
        departure=fall=touchdown=landing=0;unsigned grounded_walk_steps=0;
        while(world.tick()<100){
            const auto tick=world.tick();
            GamePadState pad;pad.dpad_up=tick>=5&&tick<20&&!departure;
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            NativeGRedGroundInput input;NativeBorgMotionOffer offer;
            check(source.map_input(pad,&input,&error)&&
                source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,&offer,&error),"prepare ordinary authored walking route");
            const auto& p=offer.observed.position;const auto& end=offer.proposed_position;
            using Kind=NativeBorgMotionKind;
            check(!offer.ordered_ground,"walking never borrows ordered dash root evidence");
            if(offer.kind==Kind::Ground&&offer.action==BattleMvpAction::Move&&p!=end)++grounded_walk_steps;
            if(offer.kind==Kind::GroundDeparture){
                departure=tick+1;
                NativeBorgStaticSurface start;
                check(departure==8&&grounded_walk_steps==1&&offer.walk_ledge&&offer.walk_ledge->speed44==10&&
                    view.static_surface(p[0],p[2],p[1],&start)&&start.triangle==827&&offer.walk_ledge->surface.triangle==2060&&
                    view.inclined_ledge_crossing(827,2060,p,end,60)&&p[1]==end[1]&&!offer.vertical_speed&&
                    source.movement_programs()==std::array<u16,2>{1,1},
                    "accepted walk handler approaches the authored strip and stages the departure without a vertical snap");
            }
            if(offer.kind==Kind::GroundLossEntry)fall=tick+1;
            if(offer.kind==Kind::GroundLossTouchdown){touchdown=tick+1;check(end[1]==10,"walking route touches down on the lower flat floor");}
            if(touchdown&&!landing&&offer.kind==Kind::JumpLanding)landing=tick+1;
            accept(offer);
            if(tick+1==fall)check(source.ground_loss_timer698()==30&&
                source.movement_programs()==std::array<u16,2>{13,13},"ordinary momentum fall owns its own timer and both program lanes");
            if(tick+1==landing)check(source.movement_programs()==std::array<u16,2>{8,8},"separate walking-route landing program");
            check(source.resource()==10000&&!source.ordered_root().updates&&!source.surface_pushes(),
                "walking, falling, and landing don't charge meter or dispatch dash/push");
        }
        check(departure==8&&fall==9&&touchdown==23&&landing==24&&source.walk_departures()==1&&!source.dash_departures()&&
            world.actors()[0].grounded&&source.selector()==0&&source.ground_loss_timer698()==0,
            "walk and dash permission are independent and the older walking path recovers to idle");
    }
    std::printf("slope dash: %u checks PASS, original-stage seams, separate walk/dash cliff routes, older refusals\n",checks);
}
