#include <cstdio>
#include <filesystem>
#include <fstream>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_neutral_b_trial.hpp"

namespace {
using namespace gf;
class Files final:public IFileSystem {
public:
    bool file_size(const std::string& path,u64* out)override{
        std::error_code error;const auto size=std::filesystem::file_size(path,error);
        if(error||!out)return false;
        *out=size;return true;
    }
    bool read(const std::string& path,std::vector<u8>& out)override{
        u64 size=0;
        if(!file_size(path,&size)||!size||size>(32u<<20))return false;
        std::ifstream stream(path,std::ios::binary);
        out.resize(std::size_t(size));return bool(stream.read(reinterpret_cast<char*>(out.data()),std::streamsize(size)));
    }
    bool exists(const std::string& path)override{
        std::error_code error;return std::filesystem::exists(path,error)&&!error;
    }
    bool write(const std::string&,const u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
unsigned checks=0;
std::string error;
bool check(bool ok,const char* why){
    ++checks;
    if(!ok)std::fprintf(stderr,"native neutral-B trial: %s (%s)\n",why,error.c_str());
    return ok;
}
bool trial(Files& files,const std::string& art,const std::string& ground,NativeBattleMvpView& view,
           const GRedNeutralBLocalLibrary& library,s16 rounds,bool edit_final){
    NativeBattleMvp world;
    NativeBorgAnimationSeed seed;
    NativeGRedNeutralBTrial source;
    if(!check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
              source.begin(files,art,ground,world,seed,library,rounds,&error),"fresh same-renderer native trial"))return false;
    NativeGRedNeutralBTrialReadout state;
    NativeGRedPracticeWorldMatrices keys;
    if(!check(!source.observe(world,&state)&&!view.observe_gred_source_world(world,&keys),"no accepted source frame at bind"))return false;
    const WeaponStockModeZeroState initial{rounds,rounds,180,0};
    const auto position=world.actors()[0].position;
    const auto facing=world.actors()[0].facing;
    for(unsigned frame=1;frame<=17;++frame){
        const auto camera=NativeBattleMvpView::movement_frame(world,1152,648);
        if(frame==8){
            NativeBorgMotionOffer blocked;
            if(!check(world.set_paused(true)&&!source.prepare(world,view.source_binding(),camera,&blocked,&error)&&
                      source.observe(world,&state)&&state.tick==7&&state.stock==initial&&!state.paid&&
                      view.observe_gred_source_world(world,&keys)&&keys.tick==7&&world.set_paused(false),
                      "pause preserves the last accepted stock and source pose"))return false;
        }
        NativeBorgMotionOffer offer;
        if(!check(source.prepare(world,view.source_binding(),camera,&offer,&error)&&world.tick()==frame-1&&
                  offer.proposed_position==position&&offer.facing==facing&&!source.observe(world,&state),
                  "staged source pose and debit are not a published frame"))return false;
        if(frame==17&&edit_final)++offer.serial;
        if(!check(world.step({},false,&view,nullptr,&camera,&offer)&&world.tick()==frame&&
                  !view.observe_gred_source_world(world,&keys),"accepted motion precedes pose publication"))return false;
        if(frame==17&&edit_final){
            return check(!source.commit(world,view.source_binding(),&error)&&!source.observe(world,&state)&&
                         !source.commit(world,view.source_binding(),&error)&&!world.shots_spawned()&&!world.hits(),
                         "foreign serial cannot publish a due debit or be retried");
        }
        NativeBorgSourcePose pose;
        if(!check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&
                  view.update_source(world,pose,&error)&&view.observe_gred_source_world(world,&keys)&&keys.tick==frame&&
                  source.observe(world,&state)&&state.tick==frame&&bool(state.paid)==(frame==17),
                  "exact native receipt owns both the staged pose and source stock result"))return false;
        if(!check(world.actors()[0].position==position&&world.actors()[0].facing==facing&&
                  world.actors()[0].health==world.config().health&&world.actors()[1].health==world.config().health&&
                  !world.shots_spawned()&&!world.hits(),"trial does not move the body, shoot or write HP"))return false;
        if(frame<17&&!check(state.stock==initial,"no debit on opening or before the release cue"))return false;
    }
    const WeaponStockModeZeroState after{s16(rounds-1),rounds,180,rounds==1?180.f:0.f};
    const auto& paid=*state.paid;
    NativeBorgMotionOffer blocked;
    const auto camera=NativeBattleMvpView::movement_frame(world,1152,648);
    if(!check(state.stock==after&&paid.stock==after&&paid.owner_slot==0&&paid.object_type==2&&paid.argument==43&&
              !source.commit(world,view.source_binding(),&error)&&
              !source.prepare(world,view.source_binding(),camera,&blocked,&error)&&
              !world.step({},false,&view)&&source.observe(world,&state)&&state.stock==after,
              "one debit, one accepted request, and no reentry or legacy fallback"))return false;
    source.retire();
    return check(!source.observe(world,&state)&&!view.observe_gred_source_world(world,&keys),"retire revokes the accepted request and renderer observation");
}
}

int main(int argc,char** argv){
    using namespace gf;
    if(argc!=4){std::fprintf(stderr,"usage: %s MVP_ASSET_DIR GROUND_INPUTS_DIR NEUTRAL_B_GFMP\n",argv[0]);return 2;}
    const std::string art=argv[1],ground=argv[2];
    std::array<std::string,6> names{art+"/g-red.hsd",art+"/g-red.prefix",art+"/g-red.pose",
                                   art+"/g-red.motions",argv[3],ground+"/pose-height.gfhc"};
    std::array<std::vector<u8>,6> data;std::array<ByteView,6> input;
    Files files;
    for(unsigned i=0;i<names.size();++i){
        if(!check(files.read(names[i],data[i]),"local exact animation input"))return 1;
        input[i]=ByteView{data[i].data(),data[i].size()};
    }
    GRedNeutralBLocalLibrary library;
    if(!check(library.load({input[0],input[1],input[2],input[3],input[4],input[5]},&error),"qualified local animation library"))return 1;
    headless::Render backend;RenderEngine render(backend);NativeBattleMvpView view;
    if(!check(render.initialize({1152,648,false,true,"source neutral-B trial"},nullptr,&error)&&
              view.load(files,art,render,&error),"shared headless renderer and stage"))return 1;
    if(!trial(files,art,ground,view,library,5,false)||!trial(files,art,ground,view,library,1,false)||
       !trial(files,art,ground,view,library,5,true))return 1;
    std::printf("native neutral-B trial: %u checks PASS; scoped cue at tick 17, local stock only, no allocated beam\n",checks);
}
