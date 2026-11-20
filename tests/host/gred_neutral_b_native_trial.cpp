#include <cstdio>
#include <filesystem>
#include <fstream>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_class3_trial_bridge.hpp"
#include "engine/gameplay/native_gred_neutral_b_trial.hpp"
#include "tests/unit/scene_physical_pools_fixture.hpp"

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
enum class Capacity {NoAttempt,Available,InstallHeader,StaleHeader,NoDescriptor,NoCarrier,BothFull};
constexpr GRedBeamFactoryParentFields trial_parent{0x0615,2,4,0,5,.75f,1.f,1.125f,.875f,true};
struct AllocationFixture {
    SceneDescriptorBootstrap models;
    ScenePhysicalPools physical;
    NativeGRedClass3TrialBridge bridge;
    std::vector<SceneDescriptorLease> held_descriptors;
    std::vector<SceneClass3Lease> held_carriers;

    bool bind(NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,Capacity capacity){
        auto recipe=physical_pools_fixture::bytes();
        recipe[160+9]=16; // exact group 9 kind for the scoped slot-0 request
        ScenePhysicalPoolsProfile profile;
        if(!check(profile.load({recipe.data(),recipe.size()},&error)&&models.begin_fresh(physical_pools_fixture::descriptor_profile(),&error)&&
                  physical.begin_fresh(profile,models,&error)&&bridge.bind(source,world,physical),
                  "actual class3/class4/model allocation domain joined to the fresh trial"))return false;
        auto pool=physical.class2_pool();
        const auto descriptors=models.model_pool();
        if(capacity==Capacity::NoCarrier||capacity==Capacity::BothFull)
            for(unsigned i=0;i<512;++i){
                SceneClass3Lease lease;
                if(!check(pool->acquire_class3(-1,false,0,&lease,&error)==SceneObjectAcquire::Allocated,
                          "occupy real carrier capacity"))return false;
                held_carriers.push_back(lease);
            }
        if(capacity==Capacity::NoDescriptor||capacity==Capacity::BothFull)
            while(descriptors->snapshot()->free_count){
                SceneDescriptorLease lease;
                if(!check(descriptors->acquire(&lease,&error)==SceneDescriptorAcquire::Acquired,
                          "occupy real descriptor capacity"))return false;
                held_descriptors.push_back(lease);
            }
        return true;
    }
    bool finish(NativeGRedNeutralBTrial& source,NativeBattleMvp& world,NativeBattleMvpView& view,Capacity capacity,
                const WeaponStockModeZeroState& after){
        using S=GRedClass3TrialStatus;
        const bool allocated=capacity==Capacity::Available||capacity==Capacity::InstallHeader||capacity==Capacity::StaleHeader;
        const auto expected=allocated?S::Allocated:capacity==Capacity::NoCarrier?S::NoCarrier:S::NoDescriptors;
        const auto pool=physical.class2_pool();
        const auto descriptors=models.model_pool();
        const auto carrier_before=*pool->class3_snapshot();
        const auto shape_before=*pool->collision_snapshot();
        const auto descriptor_before=*descriptors->snapshot();
        const auto lists_before=*pool->snapshot();
        NativeGRedNeutralBTrialReadout state;
        NativeBattleMvp foreign;
        NativeGRedNeutralBTrial other_producer;
        NativeGRedTrialBeamHandoff duplicate;
        if(!check(foreign.start({},&view)&&!bridge.try_once(source,foreign,view,&error)&&
                  !bridge.try_once(other_producer,world,view,&error)&&
                  bridge.try_once(source,world,view,&error)&&bridge.status()==expected&&
                  !source.take_beam_handoff(world,view,&duplicate)&&source.observe(world,&state)&&state.stock==after&&
                  *pool->collision_snapshot()==shape_before&&!world.shots_spawned()&&!world.hits(),
                  "same paid owner and view consume one attempt with no class4 or stock refund"))return false;
        if(expected==S::Allocated){
            const auto& lease=bridge.lease();
            const auto* record=pool->inspect(lease);
            const auto* models_owned=pool->class3_models(lease);
            if(!check(lease.current()&&record&&models_owned&&record->group01==9&&record->kind83==16&&
                      !record->active00&&!models_owned->header_installed&&pool->snapshot()->group_heads[9]==ScenePhysicalLink{3,lease.index()}&&
                      models_owned->descriptor_count99==1&&models_owned->descriptor_indices[0]==descriptor_before.free_head&&
                      pool->class3_snapshot()->free_count+1==carrier_before.free_count&&
                      descriptors->snapshot()->free_count+1==descriptor_before.free_count&&
                      bridge.factory().propose_header&&bridge.factory().argument11==43,
                      "one rootless lease and descriptor; factory header is a proposal"))return false;
            if(capacity==Capacity::InstallHeader){
                if(!check(!bridge.install_trial_header(other_producer,world,view,&error)&&
                          !pool->inspect(lease)->active00&&bridge.install_trial_header(source,world,view,&error)&&
                          bridge.status()==S::HeaderInstalled,"only the exact current caster can install the trial header"))return false;
                const auto* birth=bridge.installed_trial_birth(source,world,view);
                if(!check(birth&&birth->parent8c_slot==0&&birth->credit90_slot==0&&birth->targets_c8_cc_null&&
                          birth->copied.model94==0x0615&&birth->copied.render_index96==2&&birth->copied.resource_index97==4&&
                          birth->copied.flags_d0==5&&birth->copied.scale_b8==.75f&&birth->copied.scale_b4==1.f&&
                          birth->copied.physical_c0==1.125f&&birth->copied.relative_c4==.875f&&birth->scale58==std::array<f32,3>{1,1,1}&&
                          pool->inspect(lease)->active00==1&&pool->inspect(lease)->registration85==255&&
                          pool->class3_models(lease)->header_installed&&pool->class3_models(lease)->header==SceneObjectHeader{2,43,0,0x800c3cdcu,0x800c4468u},
                          "real header and distinct cloned fields without a callback acknowledgement"))return false;
                if(!check(!bridge.install_trial_header(source,world,view,&error)&&
                          !bridge.release_uninitialized(&error)&&!pool->release(lease,&error)&&
                          *pool->collision_snapshot()==shape_before&&descriptors->snapshot()->free_count+1==descriptor_before.free_count,
                          "installed header refuses replay and rootless cleanup; class4 untouched"))return false;
                physical.retire();
                return check(!lease.current()&&!bridge.installed_trial_birth(source,world,view),
                             "pool retirement revokes the header inspection without claiming source cleanup");
            }
            if(capacity==Capacity::StaleHeader){
                NativeBorgSourcePose republished;
                if(!check(source.pose(world,&republished)&&view.update_source(world,republished,&error)&&
                          !bridge.install_trial_header(source,world,view,&error)&&!pool->inspect(lease)->active00,
                          "same-tick republish invalidates the allocation's constructor gate"))return false;
            }else if(!check(!bridge.install_trial_header(source,world,view,&error)&&!pool->inspect(lease)->active00,
                            "lease cannot stand in for absent parent fields"))return false;
            return check(!bridge.try_once(source,world,view,&error)&&bridge.release_uninitialized(&error)&&
                         bridge.status()==S::Released&&!lease.current()&&!bridge.release_uninitialized(&error)&&
                         pool->class3_snapshot()->free_count==carrier_before.free_count&&
                         descriptors->snapshot()->free_count==descriptor_before.free_count,
                         "rootless release revokes the lease and cannot run twice");
        }
        if(!check(!bridge.lease().current()&&!bridge.factory().propose_header&&
                  *pool->class3_snapshot()==carrier_before&&*descriptors->snapshot()==descriptor_before&&
                  *pool->snapshot()==lists_before,"capacity refusal leaves pools and lists unchanged"))return false;
        if(!held_descriptors.empty()&&!check(descriptors->release(held_descriptors.back(),&error),"return descriptor capacity"))return false;
        if(!held_carriers.empty()&&!check(pool->release(held_carriers.back(),&error),"return carrier capacity"))return false;
        return check(!bridge.try_once(source,world,view,&error)&&!bridge.lease().current()&&!bridge.release_uninitialized(&error),
                     "later capacity cannot repeat the paid attempt");
    }
};
bool trial(Files& files,const std::string& art,const std::string& ground,NativeBattleMvpView& view,
           const GRedNeutralBLocalLibrary& library,s16 rounds,bool edit_final,Capacity capacity=Capacity::NoAttempt){
    NativeBattleMvp world;
    NativeBorgAnimationSeed seed;
    NativeGRedNeutralBTrial source;
    auto declared=trial_parent;
    const bool supply=capacity==Capacity::InstallHeader||capacity==Capacity::StaleHeader;
    if(!check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
              source.begin(files,art,ground,world,seed,library,rounds,&error,supply?&declared:nullptr),
              "fresh same-renderer native trial"))return false;
    declared={}; // only the trial's owned source-field sample may later be copied
    AllocationFixture allocation;
    if(capacity!=Capacity::NoAttempt&&(!allocation.bind(source,world,capacity)||
       !check(!allocation.bridge.try_once(source,world,view,&error),"unpaid frame cannot touch class3")))return false;
    NativeGRedNeutralBTrialReadout state;
    NativeGRedPracticeWorldMatrices keys;
    NativeGRedTrialBeamHandoff handoff;
    GRedBeamPoolCall call;
    if(!check(!source.observe(world,&state)&&!view.observe_gred_source_world(world,&keys)&&
              !source.take_beam_handoff(world,view,&handoff)&&!handoff.inspect(world,view,&call),
              "no accepted source frame or handoff at bind"))return false;
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
                         !source.commit(world,view.source_binding(),&error)&&!source.take_beam_handoff(world,view,&handoff)&&
                         (capacity==Capacity::NoAttempt||(!allocation.bridge.try_once(source,world,view,&error)&&
                          allocation.physical.bank(3)->free_count==512))&&
                         !handoff.inspect(world,view,&call)&&!world.shots_spawned()&&!world.hits(),
                         "foreign serial cannot publish a due debit or be retried");
        }
        NativeBorgSourcePose pose;
        if(!check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&
                  !source.take_beam_handoff(world,view,&handoff)&&(capacity==Capacity::NoAttempt||
                  !allocation.bridge.try_once(source,world,view,&error)),
                  "matching movement receipt alone does not hand off a factory call"))return false;
        if(!check(view.update_source(world,pose,&error)&&view.observe_gred_source_world(world,&keys)&&keys.tick==frame&&
                  source.observe(world,&state)&&state.tick==frame&&bool(state.paid)==(frame==17),
                  "exact native receipt owns both the staged pose and source stock result"))return false;
        if(!check(world.actors()[0].position==position&&world.actors()[0].facing==facing&&
                  world.actors()[0].health==world.config().health&&world.actors()[1].health==world.config().health&&
                  !world.shots_spawned()&&!world.hits(),"trial does not move the body, shoot or write HP"))return false;
        if(frame<17&&!check(state.stock==initial&&!source.take_beam_handoff(world,view,&handoff),
                            "no debit or handoff on opening or before the release cue"))return false;
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
    if(capacity!=Capacity::NoAttempt)return allocation.finish(source,world,view,capacity,after);
    NativeBattleMvpView unpublished;
    if(!check(world.set_paused(true)&&!source.take_beam_handoff(world,view,&handoff)&&world.set_paused(false)&&
              !source.take_beam_handoff(world,unpublished,&handoff)&&!source.take_beam_handoff(world,view,nullptr)&&
              source.take_beam_handoff(world,view,&handoff)&&handoff.inspect(world,view,&call)&&
              call==GRedBeamPoolCall{3,8,9,16,1,false},
              "exact paid frame hands off argument-43 allocator inputs once; no allocation result"))return false;
    NativeGRedTrialBeamHandoff duplicate;
    if(!check(!source.take_beam_handoff(world,view,&duplicate)&&!duplicate.inspect(world,view,&call)&&
              handoff.inspect(world,view,&call)&&!world.shots_spawned(),
              "checked-out request cannot be issued again"))return false;
    if(rounds==5){
        NativeBorgSourcePose accepted;
        if(!check(source.pose(world,&accepted)&&view.update_source(world,accepted,&error)&&
                  view.observe_gred_source_world(world,&keys)&&keys.tick==17&&
                  !handoff.inspect(world,view,&call)&&call==GRedBeamPoolCall{3,8,9,16,1,false}&&
                  !source.take_beam_handoff(world,view,&duplicate),
                  "a same-tick republish does not revalidate the old handoff or issue another"))return false;
    }
    source.retire();
    return check(!source.observe(world,&state)&&!view.observe_gred_source_world(world,&keys)&&!handoff.inspect(world,view,&call),
                 "retire revokes the accepted request and renderer observation");
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
       !trial(files,art,ground,view,library,5,true,Capacity::Available))return 1;
    for(auto capacity:{Capacity::Available,Capacity::InstallHeader,Capacity::StaleHeader,
                       Capacity::NoDescriptor,Capacity::NoCarrier,Capacity::BothFull})
        if(!trial(files,art,ground,view,library,5,false,capacity))return 1;
    std::printf("native neutral-B trial: %u checks PASS; paid class3 header, no initializer or child\n",checks);
}
