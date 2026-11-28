#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"

namespace {
using namespace gf;
constexpr auto art="local/content/native-battle-mvp-v1";
constexpr auto pack="local/content/source-gred-ground-v9";
std::string error;
unsigned visit;

void check(bool ok,const char* what){
    if(!ok){std::fprintf(stderr,"G-Red meter visit %u: %s (%s)\n",visit,what,error.c_str());std::abort();}
}

class Files final:public IFileSystem {
public:
    bool file_size(const std::string& name,u64* out)override{
        std::error_code e;const auto size=std::filesystem::file_size(name,e);
        if(e||!out)return false;
        *out=size;return true;
    }
    bool read(const std::string& name,std::vector<u8>& out)override{
        u64 size=0;if(!file_size(name,&size)||size>(32u<<20))return false;
        std::ifstream f(name,std::ios::binary);
        out.assign(std::istreambuf_iterator<char>(f),{});
        return out.size()==size;
    }
    bool exists(const std::string& name)override{return std::filesystem::exists(name);}
    bool write(const std::string&,const u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};

struct Replay {
    NativeBattleMvp world;
    NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;
    NativeBorgSourcePose pose;

    void start(Files& files,RenderEngine& render){
        NativeBorgAnimationSeed seed;
        check(view.load(files,art,render,&error)&&world.start({},&view)&&view.update(world,&error)&&
            view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,
                NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush),
            "normal source mode used by Battle Test, on the native stage and body proxy");
        check(world.actors()[0].grounded&&source.resource()==10000&&!source.cached_airborne87(),"full grounded seed");
    }

    NativeBorgMotionOffer step(GamePadState pad){
        NativeBorgMotionOffer offer;
        visit=world.tick()+1;
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        const auto meter=source.resource();
        const auto cache=source.cached_airborne87();
        const auto observed=world.actors()[0].position;
        check(source.prepare_battle_tick(world,view.source_binding(),frame,pad,&offer,&error)&&
            world.tick()+1==visit&&world.actors()[0].position==observed&&
            source.resource()==meter&&source.cached_airborne87()==cache,"prepared input/resource stays private");
        NativeBorgMotionReceipt receipt;
        check(world.step({},false,&view,nullptr,&frame,&offer)&&
            source.resource()==meter&&source.cached_airborne87()==cache&&
            world.motion_receipt(offer,&receipt)&&receipt.tick()==visit&&
            source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&
            pose.receipt.serial()==receipt.serial()&&view.update_source(world,pose,&error),
            "world accepts before source/resource/pose publication");
        return offer;
    }
};

void lift_dash_landing(Replay& r){
    unsigned lift_entries=0,lift_updates=0,windups=0,handoffs=0,actives=0,landing=0;
    unsigned active_updates=0,refills=0,dash_total=0;
    bool touched=false,ready=false;
    for(unsigned t=0;t<400&&!ready;++t){
        GamePadState pad{};pad.b=t>=5&&t<54;
        if(t>=45&&t<71)pad.stick_y=-127;
        const auto before=r.source.resource();const auto cache_before=r.source.cached_airborne87();
        const auto offer=r.step(pad);
        const auto after=r.source.resource();const auto delta=after-before;
        if(offer.kind==NativeBorgMotionKind::JumpTakeoff)check(!delta,"takeoff animation is free");
        if(offer.jump_motion&&offer.jump_motion->phase==5){
            const bool entry=offer.jump_motion->entry;
            check(offer.jump_motion->resource_before==before&&delta==(entry?-150:-100),"ascent entry and active use independent costs");
            if(entry){++lift_entries;std::printf("gred-meter: lift-entry %u %d %d\n",visit,before,after);}
            else ++lift_updates;
        }
        if(offer.air_motion){
            const auto& a=*offer.air_motion;
            const bool handoff=a.phase==15&&!a.entry&&a.next_selector==16;
            check(a.resource_before==before&&a.resource_after==after&&
                delta==(handoff?-450:a.phase==16?-50:0),"air dash charges only the handoff and active visits");
            dash_total+=unsigned(-delta);
            if(a.phase==15&&a.entry){++windups;std::printf("gred-meter: wind-up %u %d %d\n",visit,before,after);}
            if(handoff){++handoffs;std::printf("gred-meter: handoff %u %d %d\n",visit,before,after);}
            if(a.phase==16){++active_updates;if(a.entry){++actives;std::printf("gred-meter: active-entry %u %d %d\n",visit,before,after);}}
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            check(!touched&&r.world.actors()[0].grounded&&cache_before&&r.source.cached_airborne87()&&
                !delta&&r.source.battle_landing_entry_pending(),"accepted touchdown does not clear the controller cache or refill");
            touched=true;std::printf("gred-meter: touchdown %u %d %d\n",visit,before,after);
        }
        if(offer.kind==NativeBorgMotionKind::JumpLanding&&offer.jump_motion->entry){
            check(touched&&!landing&&cache_before&&!r.source.cached_airborne87()&&!delta&&r.source.landing_entry_accepted(),
                  "separate landing entry clears cache, not the meter");
            landing=visit;std::printf("gred-meter: landing-entry %u %d %d\n",visit,before,after);
        }
        if(delta>0){
            check(landing&&visit==landing+refills+1&&!cache_before&&r.world.actors()[0].grounded&&after==std::min(10000,before+2000),
                  "refill only after committed landing cache, capped at the meter limit");
            if(!refills)std::printf("gred-meter: first-refill %u %d %d\n",visit,before,after);
            ++refills;
        }
        ready=landing&&after==10000&&r.source.selector()==0;
    }
    check(ready&&lift_entries==1&&lift_updates>0&&windups==1&&handoffs==1&&actives==1&&
          active_updates==31&&dash_total==2000&&refills>0,"one lift, minimum-duration dash and grounded refill");
    std::printf("gred-meter: dash-total %u %u\n",active_updates,dash_total);
}

struct ZeroProbe {
    unsigned pulse=0,touchdown=0,landing=0;
    NativeBorgJumpMotion jump;
    BattleMvpVec position{};
    f32 vertical_speed=0;
    bool operator==(const ZeroProbe&)const=default;
};

ZeroProbe zero_control(Replay& r,bool press){
    bool depleted=false,falling=false,released=false,probed=false;
    unsigned refills=0;
    ZeroProbe result;
    for(unsigned t=0;t<400&&refills<5;++t){
        GamePadState pad{};
        const bool pulse=falling&&released&&!probed;
        pad.b=(!falling&&t>=5)||(pulse&&press);
        const auto before=r.source.resource();const auto launches=r.source.relaunches();
        const auto offer=r.step(pad);
        const auto after=r.source.resource();
        if(!after&&!depleted){
            check(before>0&&offer.jump_motion&&offer.jump_motion->phase==5,"held lift clamps the final spend to zero");
            depleted=true;
        }
        if(pulse){
            check(depleted&&before==0&&after==0&&offer.jump_motion&&offer.jump_motion->phase==6&&
                offer.kind==NativeBorgMotionKind::JumpAir&&r.source.relaunches()==launches&&r.source.selector()==6,
                "fresh airborne jump at zero never stages a relaunch or debit");
            const auto& motion=*offer.jump_motion;
            check(motion.held_jump==press,"falling proof saw the fresh button, not just the neutral control");
            result.pulse=visit;result.jump=motion;result.jump.held_jump=false; // only this input observation may differ
            result.position=r.world.actors()[0].position;result.vertical_speed=r.world.actors()[0].vertical_speed;
            probed=true;std::printf("gred-meter: zero-relaunch %u %u %d %d\n",unsigned(press),visit,before,after);
        }
        if(falling&&!pad.b)released=true;
        if(depleted&&!falling&&offer.jump_motion&&offer.jump_motion->phase==6){
            check(pad.b&&!after&&offer.jump_motion->entry&&!r.world.actors()[0].grounded,"held zero-meter lift exits to fall");
            falling=true;
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            check(!before&&!after&&r.source.cached_airborne87(),"empty touchdown retains airborne cache");
            result.touchdown=visit;
        }
        if(offer.kind==NativeBorgMotionKind::JumpLanding&&offer.jump_motion->entry){
            check(result.touchdown&&visit==result.touchdown+1&&!before&&!after&&!r.source.cached_airborne87(),"empty landing entry does not refill");
            result.landing=visit;
        }
        if(after>before){
            check(result.landing&&visit==result.landing+refills+1&&after==2000*int(refills+1),
                  "five consecutive grounded +2000 updates after empty landing entry");
            ++refills;
        }
    }
    check(probed&&result.landing&&refills==5&&r.source.resource()==10000&&!r.source.relaunches()&&!r.source.air_dash_starts(),
          "zero-meter refusal plus full refill");
    std::printf("gred-meter: empty-refill %u %u %u\n",unsigned(press),result.landing,refills);
    return result;
}
} // namespace

int main(){
    Files files;
    if(!files.exists(std::string(pack)+"/motion.gfmm")){std::puts("G-Red meter: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"G-Red meter checkpoints"},nullptr,&error),"headless renderer");
    ZeroProbe neutral;
    for(unsigned run=0;run<3;++run){
        Replay replay;
        visit=0;replay.start(files,render);
        if(!run)lift_dash_landing(replay);
        else if(run==1)neutral=zero_control(replay,false);
        else check(zero_control(replay,true)==neutral,"fresh zero-meter pulse matches neutral motion and landing order");
    }
}
