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
}

int main(){
    using namespace gf;
    using Mode=NativeGRedGroundDashDemo::Mode;
    Files files;const std::string pack="local/content/source-gred-ground-v9",art="local/content/native-battle-mvp-v1";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("slope dash: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"slope dash"},nullptr,&error)&&view.load(files,art,render,&error),"original stage and models");
    struct Route {f32 x,z,reference;int stick;};
    const Route down{-1443.46997f,-703.674011f,94.339f,127},up{-2377.44727f,528.269836f,591.9127f,-127};
    auto bind=[&](const Route& r,Mode mode){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={r.x,r.z};config.spawn_query_y[0]=r.reference;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"grounded original-stage placement");
    };
    auto prepare=[&](const Route& r,NativeBorgMotionOffer* offer){
        GamePadState pad;if(world.tick()==5)pad.stick_y=r.stick;
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
    std::printf("slope dash: %u checks PASS, two complete original-stage routes and older/seam refusals\n",checks);
}
