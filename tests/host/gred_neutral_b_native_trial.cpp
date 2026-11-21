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
enum class Capacity {NoAttempt,Available,InstallHeader,StaleHeader,NoDescriptor,NoCarrier,BothFull,
                     Shape,ShapeFull,StaleShape,StaleMuzzle,RegistryOccupied,StaleRegistry,PairOverlap};
struct BeamDrawFixture {
    const NormalGRedBeamArtBank& art;
    RenderEngine& render;
};
constexpr GRedBeamFactoryParentFields trial_parent{0x0615,2,4,0,5,.75f,1.f,1.125f,.875f,true};
constexpr ColliderGeometryProfile geometry{{0,0x3f800000u,0x42480000u,0x3c8efa35u,0x42652ee1u,0x3fc90fdbu,
                                             0x3f060a92u,0x3c23d70au},true};
bool shape_case(Capacity c){return c==Capacity::Shape||c==Capacity::ShapeFull||c==Capacity::StaleShape||
    c==Capacity::StaleMuzzle||c==Capacity::RegistryOccupied||c==Capacity::StaleRegistry||c==Capacity::PairOverlap;}
struct AllocationFixture {
    SceneDescriptorBootstrap models;
    ScenePhysicalPools physical;
    NativeGRedClass3TrialBridge bridge;
    std::vector<SceneDescriptorLease> held_descriptors;
    std::vector<SceneClass3Lease> held_carriers;
    std::vector<SceneCollisionLease> held_shapes;
    const BeamDrawFixture* draw_fixture=nullptr;
    const NormalSasukeBeamReceiverProfile* receiver_profile=nullptr;

    bool bind(NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,Capacity capacity){
        auto recipe=physical_pools_fixture::bytes();
        recipe[160+9]=16;recipe[160+49]=recipe[160+50]=26; // exact trial carrier, attack and receiver kinds
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
        if(capacity==Capacity::ShapeFull)
            for(unsigned i=0;i<384;++i){
                SceneCollisionLease lease;
                if(!check(pool->acquire_collision(-1,false,{},&lease,&error)==SceneObjectAcquire::Allocated,
                          "occupy real class4 capacity"))return false;
                held_shapes.push_back(lease);
            }
        return true;
    }
    bool finish(NativeGRedNeutralBTrial& source,NativeBattleMvp& world,NativeBattleMvpView& view,Capacity capacity,
                const WeaponStockModeZeroState& after){
        using S=GRedClass3TrialStatus;
        const bool allocated=capacity==Capacity::Available||capacity==Capacity::InstallHeader||capacity==Capacity::StaleHeader||
            shape_case(capacity);
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
            if(capacity==Capacity::InstallHeader||shape_case(capacity)){
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
                if(!finish_shape(source,world,view,capacity))return false;
                physical.retire();
                if(draw_fixture){
                    auto& render=draw_fixture->render;
                    render.begin_frame({});
                    if(!check(!bridge.draw_trial_beam(source,world,view,render,Camera{})&&render.end_frame(),
                              "retired carrier cannot submit prepared beam geometry"))return false;
                }
                return check(!lease.current()&&!bridge.shape_lease().current()&&!bridge.installed_trial_birth(source,world,view)&&
                             !bridge.trial_shape_hit_row(source,world,view).data&&!bridge.trial_launch(source,world,view)&&
                             !bridge.trial_sphere(source,world,view)&&!bridge.trial_shape_activation(source,world,view)&&
                             !bridge.trial_attack_entry(source,world,view,physical)&&!bridge.trial_receiver(source,world,view,physical)&&
                             !bridge.trial_receiver_entry(source,world,view,physical),
                             "pool retirement revokes both leases without claiming source cleanup");
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
    bool finish_shape(NativeGRedNeutralBTrial& source,NativeBattleMvp& world,NativeBattleMvpView& view,Capacity capacity){
        const auto pool=physical.class2_pool();
        const auto descriptors=models.model_pool();
        const auto desc_before=*descriptors->snapshot();
        const auto shape_before=*pool->collision_snapshot();
        const auto scheduler=*physical.scheduler();
        if(receiver_profile&&!check(!bridge.seed_trial_receiver_once(source,world,view,physical,*receiver_profile,1,&error)&&
                                    !bridge.trial_receiver(source,world,view,physical),
                                    "no receiver seed without an accepted attack entry"))return false;
        if(!check(!bridge.finish_trial_shape_activation_once(source,world,view,geometry,&error)&&
                  !pool->class3_models(bridge.lease())->trial_token26c&&
                  !bridge.publish_trial_attack_registry_once(source,world,view,physical,&error)&&
                  !bridge.trial_attack_entry(source,world,view,physical),
                  "activation cannot precede construction, matrix and sphere publication"))return false;
        if(!check(!bridge.seed_trial_muzzle_once(source,world,view,&error)&&
                  !bridge.trial_launch(source,world,view)&&!pool->class3_models(bridge.lease())->trial_frame114,
                  "a published actor pose and header cannot bypass class4 construction"))return false;
        if(capacity==Capacity::StaleShape){
            NativeBorgSourcePose republished;
            if(!check(source.pose(world,&republished)&&view.update_source(world,republished,&error),
                      "new publication of the same accepted tick"))return false;
        }
        if(capacity==Capacity::StaleShape||capacity==Capacity::InstallHeader)
            return check(!bridge.initialize_trial_shape_once(source,world,view,&error)&&
                         *pool->collision_snapshot()==shape_before&&bridge.shape_status()==GRedTrialShapeStatus::NotAttempted,
                         "stale header or absent credited bank refuses before allocation");
        const bool success=capacity!=Capacity::ShapeFull;
        if(!check(bridge.initialize_trial_shape_once(source,world,view,&error)&&
                  bridge.shape_status()==(success?GRedTrialShapeStatus::Constructed:GRedTrialShapeStatus::NoSlot)&&
                  *descriptors->snapshot()==desc_before&&*physical.scheduler()==scheduler&&!world.shots_spawned(),
                  "known class4 outcome, no descriptor charge, matrix or scheduler change"))return false;
        const auto* init=bridge.trial_shape_init(source,world,view);
        if(!check(init&&init->lifetime1c==300&&init->phase18==(success?1:2)&&
                  init->route==(success?GRedBeamInitRoute::MuzzleBranch:GRedBeamInitRoute::RetireRegistration),
                  "phase/life seed retained in both class4 outcomes"))return false;
        const auto& lease=bridge.shape_lease();
        if(!success){
            if(!check(!lease.current()&&*pool->collision_snapshot()==shape_before&&
                      !bridge.trial_shape_hit_row(source,world,view).data&&
                      pool->release(held_shapes.back(),&error),"full bank retires this attempt without a child or row"))return false;
            return check(!bridge.initialize_trial_shape_once(source,world,view,&error)&&!lease.current(),
                         "returning shape capacity does not retry the retired request")&&
                check(!bridge.seed_trial_muzzle_once(source,world,view,&error)&&!pool->class3_models(bridge.lease())->trial_frame114,
                      "registration refusal cannot publish a muzzle or carrier matrix");
        }
        const auto* body=pool->inspect(lease);
        const auto* shape=pool->trial_beam_shape(lease);
        const auto hit=bridge.trial_shape_hit_row(source,world,view);
        if(!check(!bridge.initialize_trial_sphere_once(source,world,view,&error)&&!bridge.trial_sphere(source,world,view),
                  "constructed child cannot sample a missing carrier transform"))return false;
        if(!check(lease.current()&&body&&shape&&body->group01==49&&body->kind83==26&&body->active00==1&&
                     body->update0c_key==0x8008a65c&&shape->carrier20==ScenePhysicalLink{3,bridge.lease().index()}&&
                     shape->carrier_generation==bridge.lease().generation()&&shape->self5c==ScenePhysicalLink{4,lease.index()}&&
                     shape->credit_slot24==0&&shape->descriptor_offset28==0x0f50&&shape->carrier_matrix_offset60==0x114&&
                     !shape->parameter11&&!shape->start48&&shape->end4a==-2&&hit.size==24&&hit.be16(0)==20&&
                     pool->collision_snapshot()->free_count+1==shape_before.free_count&&
                     !bridge.initialize_trial_shape_once(source,world,view,&error)&&!pool->release(lease,&error),
                     "typed carrier generation, credited row and constructed body; no repeat or rootless free"))return false;
        if(capacity==Capacity::StaleMuzzle){
            NativeBorgSourcePose republished;
            return check(source.pose(world,&republished)&&view.update_source(world,republished,&error)&&
                         !bridge.seed_trial_muzzle_once(source,world,view,&error)&&lease.current()&&
                         !pool->class3_models(bridge.lease())->trial_frame114,
                         "same-tick republish cannot launch through an existing class4 child");
        }
        NativeGRedPracticeWorldMatrices keys;
        NativeGRedNeutralBTrial unrelated;
        auto class4=*pool->collision_snapshot();
        if(!check(view.observe_gred_source_world(world,&keys)&&!bridge.seed_trial_muzzle_once(unrelated,world,view,&error)&&
                  bridge.seed_trial_muzzle_once(source,world,view,&error),"same-frame launch reads the accepted parent/credit matrices"))
            return false;
        const auto* launch=bridge.trial_launch(source,world,view);
        const auto& frame=pool->class3_models(bridge.lease())->trial_frame114;
        constexpr f32 offset[]{bits_to_f32(0xc2d5999au),bits_to_f32(0xc0666666u),30.f};
        std::array<f32,3> origin,wrong;
        mat4_transform_point(keys.key7,offset,origin.data());mat4_transform_point(keys.key0,offset,wrong.data());
        auto bearing=source_basis_detail::column(keys.key0,2);
        if(!check(source_basis_detail::normalize(&bearing)&&launch&&frame&&
                  launch->muzzle.origin174==origin&&origin!=wrong&&launch->muzzle.unit_bearing==bearing&&
                  launch->flight.position20==gred_beam_flight_detail::advance(origin,bearing)&&launch->flight.speed44==80&&
                  std::abs(launch->flight.distance180-1.f)<.0001f&&launch->flight.origin174==origin&&
                  launch->scale.relative_c4==1&&std::abs(launch->scale.physical_c0-1.082251f)<.0001f&&
                  std::array<f32,3>{(*frame)[3],(*frame)[7],(*frame)[11]}==launch->flight.position20,
                  "key 7 muzzle, key 0 bearing, unit advance, and carrier-owned affine translation"))return false;
        if(!check(!bridge.seed_trial_muzzle_once(source,world,view,&error)&&
                  !bridge.initialize_trial_sphere_once(unrelated,world,view,&error)&&
                  bridge.initialize_trial_sphere_once(source,world,view,&error),
                  "only the current trial can seed the child from the carrier's frame"))return false;
        const auto* sphere=bridge.trial_sphere(source,world,view);
        const f32 radius=20.f*launch->scale.physical_c0;
        if(!check(sphere&&sphere==&*shape->sphere_seed&&sphere->radius88==radius&&sphere->broad_radius7c==radius&&
                  sphere->center64==launch->flight.position20&&sphere->previous_center70==sphere->center64&&
                  !sphere->previous_radius8c&&sphere->sweep==ColliderSphereSweepSeed{
                      sphere->center64,sphere->center64,sphere->center64,sphere->center64}&&
                  !bridge.initialize_trial_sphere_once(source,world,view,&error),
                  "physical C0 sets radius; four sweep endpoints replace the previous radius"))return false;
        const auto* carrier=pool->class3_models(bridge.lease());
        if(!check(!bridge.finish_trial_shape_activation_once(unrelated,world,view,geometry,&error)&&
                  !bridge.finish_trial_shape_activation_once(source,world,view,{},&error)&&
                  !carrier->trial_token26c&&!shape->trial_phase10&&
                  bridge.finish_trial_shape_activation_once(source,world,view,geometry,&error),
                  "qualified phase-0 activation runs only for the current trial's seeded sphere"))return false;
        const auto* active=bridge.trial_shape_activation(source,world,view);
        if(!check(active&&active->phase10==1&&shape->trial_phase10==1&&active->activation_write&&
                  active->activation.scene_token==1&&active->activation.credit_count404==1&&
                  active->activation.beam_token26c==carrier->trial_token26c&&carrier->trial_token26c==1&&
                  carrier->trial_once1ca==1&&carrier->trial_byte2ad==0&&
                  active->registration_sampled&&!active->degenerate&&
                  active->membership.lists==(1u<<unsigned(CollisionRegistrationList::L477C))&&
                  !bridge.finish_trial_shape_activation_once(source,world,view,geometry,&error),
                  "one scene token, carrier latch and credit increment; 477C is a proposal only"))return false;
        if(!check(*pool->collision_snapshot()==class4&&*descriptors->snapshot()==desc_before&&*physical.scheduler()==scheduler,
                  "sphere seed and activation do not change allocation or registry"))return false;
        SceneCollisionLease neighbor;
        if(capacity==Capacity::RegistryOccupied){
            return check(pool->acquire_collision(-1,false,{},&neighbor,&error)==SceneObjectAcquire::Allocated&&
                         !bridge.publish_trial_attack_registry_once(source,world,view,physical,&error)&&
                         *physical.scheduler()==scheduler&&pool->release(neighbor,&error)&&
                         !bridge.publish_trial_attack_registry_once(source,world,view,physical,&error)&&
                         !bridge.trial_attack_entry(source,world,view,physical),
                         "extra collider refuses singleton registry without writes or later retry");
        }
        ScenePhysicalPools foreign_arena;
        GRedTrialFlightPrefix flight;
        flight.flight.speed44=-1;
        GRedBeamPhaseOnePlan phase_one;
        phase_one.active.lifetime1c=-1;
        if(!check(!bridge.publish_trial_attack_registry_once(source,world,view,foreign_arena,&error)&&
                  !bridge.publish_trial_attack_registry_once(unrelated,world,view,physical,&error)&&
                  !bridge.trial_attack_entry(source,world,view,physical)&&
                  !bridge.probe_trial_first_flight(source,world,view,physical,&flight)&&flight.flight.speed44==-1&&
                  !bridge.probe_trial_first_phase_one(source,world,view,physical,&phase_one,&error)&&phase_one.active.lifetime1c==-1&&
                  bridge.publish_trial_attack_registry_once(source,world,view,physical,&error),
                  "only the matching paid trial and its bound arena publish 477C"))return false;
        const auto entry=bridge.trial_attack_entry(source,world,view,physical);
        auto registered=scheduler;registered.registry_counts[0]=1;
        if(!check(entry&&entry->current()&&entry->index()==lease.index()&&entry->generation()==lease.generation()&&
                  *physical.scheduler()==registered&&
                  !bridge.publish_trial_attack_registry_once(source,world,view,physical,&error),
                  "one exact child lease and source-ordered count; no duplicate admission"))return false;
        const auto launch_frame=*carrier->trial_frame114;
        const auto launched=launch->flight;
        const auto seed_sphere=*shape->sphere_seed;
        if(!check(!bridge.probe_trial_first_flight(unrelated,world,view,physical,&flight)&&
                  !bridge.probe_trial_first_flight(source,world,view,foreign_arena,&flight)&&flight.flight.speed44==-1&&
                  bridge.probe_trial_first_flight(source,world,view,physical,&flight),
                  "only the current accepted carrier and attack registry can supply a flight prefix"))return false;
        const auto traveled=source_basis_detail::length(source_intersection_detail::sub(flight.flight.position20,launched.position20));
        if(!check(std::abs(traveled-80.f)<.001f&&std::abs(flight.flight.distance180-81.f)<.001f&&
                  flight.flight.origin174==launched.origin174&&flight.flight.speed44==80.5f&&
                  flight.active.route==GRedBeamActiveRoute::AwaitProximity&&flight.active.lifetime1c==300&&
                  flight.active.phase18==1&&!flight.active.request_remove_shapes&&!flight.active.refresh_stage_distance,
                  "first motion uses old speed and stops before an unobserved proximity visit"))return false;
        if(!check(!bridge.probe_trial_first_phase_one(unrelated,world,view,physical,&phase_one,&error)&&
                  !bridge.probe_trial_first_phase_one(source,world,view,foreign_arena,&phase_one,&error)&&
                  world.set_paused(true)&&!bridge.probe_trial_first_phase_one(source,world,view,physical,&phase_one,&error)&&
                  phase_one.active.lifetime1c==-1&&world.set_paused(false)&&
                  bridge.probe_trial_first_phase_one(source,world,view,physical,&phase_one,&error),
                  "first active probe reads the current trial scene, not a foreign arena or paused frame"))return false;
        std::array<f32,3> nudge{};
        for(const auto& actor:world.actors()){
            auto toward=source_intersection_detail::sub(actor.position,flight.flight.position20);
            if(!check(source_basis_detail::length(toward)>150&&source_basis_detail::normalize(&toward),
                      "proximity uses actor roots, not the receiving sphere"))return false;
            for(unsigned i=0;i<3;++i)nudge[i]+=toward[i]*3;
        }
        if(!check(phase_one.active.route==GRedBeamActiveRoute::Continue&&phase_one.proximity.stop_slot==255&&
                  phase_one.proximity.examined_mask==3&&phase_one.proximity.weight29c==3&&
                  phase_one.flight.position20==gred_beam_flight_detail::advance(flight.flight.position20,nudge)&&
                  phase_one.flight.distance180==flight.flight.distance180&&
                  phase_one.stage_visited&&phase_one.stage.queried&&!phase_one.stage.surface.grid_code&&
                  phase_one.stage.clipped_previous==launched.position20&&phase_one.stage.clipped_current==phase_one.flight.position20&&
                  phase_one.bounds_visited&&phase_one.active.lifetime1c==299&&!phase_one.active.request_remove_shapes,
                  "two far slots nudge the proposal before the real stage-grid miss and bounds check"))return false;
        if(!check(launch->flight.position20==launched.position20&&launch->flight.speed44==80&&
                  carrier->trial_frame114==launch_frame&&shape->sphere_seed==seed_sphere&&
                  *pool->collision_snapshot()==class4&&*physical.scheduler()==registered,
                  "prefix writes no launch, carrier matrix, collider or registry fields"))return false;
        if(receiver_profile){
            const f32 scale=capacity==Capacity::PairOverlap?16.f:.75f;
            if(!check(!bridge.seed_trial_receiver_once(unrelated,world,view,physical,*receiver_profile,1,&error)&&
                      !bridge.publish_trial_receiver_registry_once(source,world,view,physical,&error)&&
                      !bridge.seed_trial_receiver_once(source,world,view,physical,*receiver_profile,0,&error)&&
                      bridge.seed_trial_receiver_once(source,world,view,physical,*receiver_profile,scale,&error),
                      "only exact current trial and positive declared scale can sample Sasuke's body"))return false;
            const auto* receiver=bridge.trial_receiver(source,world,view,physical);
            if(!check(receiver&&receiver->slot==1&&receiver->sphere.radius88==22.f*scale&&
                      receiver->sphere.previous_radius8c==22.f*scale&&!receiver->sphere.sweep&&
                      receiver->sphere.center64==receiver->sphere.previous_center70&&
                      receiver->sphere.center64!=world.actors()[1].position&&
                      receiver->hit_row[1]==5&&*physical.scheduler()==registered&&*pool->collision_snapshot()==class4&&
                      !bridge.seed_trial_receiver_once(source,world,view,physical,*receiver_profile,1,&error),
                      "body 9 uses a distinct pose-derived center without registration or an extra collider"))return false;
        }
        if(capacity==Capacity::StaleRegistry){
            flight.flight.speed44=-1;
            phase_one.active.lifetime1c=-1;
            return check(pool->acquire_collision(48,false,{},&neighbor,&error)==SceneObjectAcquire::Allocated&&
                         !bridge.trial_attack_entry(source,world,view,physical)&&pool->release(neighbor,&error)&&
                         !bridge.trial_attack_entry(source,world,view,physical)&&!bridge.trial_receiver(source,world,view,physical)&&
                         !bridge.publish_trial_receiver_registry_once(source,world,view,physical,&error)&&
                         !bridge.probe_trial_first_flight(source,world,view,physical,&flight)&&flight.flight.speed44==-1&&
                         !bridge.probe_trial_first_phase_one(source,world,view,physical,&phase_one,&error)&&phase_one.active.lifetime1c==-1&&
                         *physical.scheduler()==registered,
                         "changed class4 topology invalidates the entry without replay or count rollback");
        }
        if(receiver_profile&&(capacity==Capacity::Shape||capacity==Capacity::PairOverlap)){
            const auto* seed=bridge.trial_receiver(source,world,view,physical);
            GRedSasukeFreshPairPlan pair;
            if(!check(seed&&!bridge.publish_trial_receiver_registry_once(unrelated,world,view,physical,&error)&&
                      !bridge.publish_trial_receiver_registry_once(source,world,view,foreign_arena,&error)&&
                      !bridge.probe_trial_fresh_pair(source,world,view,physical,&pair)&&
                      bridge.publish_trial_receiver_registry_once(source,world,view,physical,&error),
                      "the current pose-derived body alone can join the bound two-body arena"))return false;
            const auto body_entry=bridge.trial_receiver_entry(source,world,view,physical);
            const auto attack_entry=bridge.trial_attack_entry(source,world,view,physical);
            registered.registry_counts[1]=registered.registry_counts[9]=1;
            if(!check(body_entry&&attack_entry&&body_entry->current()&&body_entry->index()!=attack_entry->index()&&
                      attack_entry->index()==lease.index()&&attack_entry->generation()==lease.generation()&&
                      pool->inspect(*body_entry)->group01==50&&pool->inspect(*body_entry)->active00==1&&
                      pool->trial_sasuke_body(*body_entry)&&*pool->trial_sasuke_body(*body_entry)==seed->sphere&&
                      *physical.scheduler()==registered&&pool->collision_snapshot()->free_count+1==class4.free_count&&
                      *descriptors->snapshot()==desc_before&&!pool->release(*body_entry,&error)&&
                      !bridge.publish_trial_receiver_registry_once(source,world,view,physical,&error),
                      "distinct receiver lease fills 417C/117C and preserves the 477C attacker"))return false;
            class4=*pool->collision_snapshot();
            if(!check(!bridge.probe_trial_fresh_pair(unrelated,world,view,physical,&pair)&&
                       !bridge.probe_trial_fresh_pair(source,world,view,foreign_arena,&pair)&&
                       bridge.probe_trial_fresh_pair(source,world,view,physical,&pair),
                       "the fresh query consumes only the exact current registry pair"))return false;
            GRedBeamPhaseOnePlan paired;
            if(!check(bridge.probe_trial_first_phase_one(source,world,view,physical,&paired,&error)&&
                      paired.active.route==phase_one.active.route&&paired.active.lifetime1c==phase_one.active.lifetime1c&&
                      paired.proximity.examined_mask==phase_one.proximity.examined_mask&&
                      paired.flight.position20==phase_one.flight.position20,
                      "publishing a receiver collider does not change the separate proximity roster"))return false;
            using R=GRedSasukeFreshPairRoute;
            if(capacity==Capacity::PairOverlap){
                if(!check(pair.route==R::Overlap&&pair.narrow_sampled&&pair.prefix.beam_receiver290==1&&
                          pair.prefix.credit_receiver290==1&&pair.prefix.beam_vector2a0==pair.point&&
                          pair.prefix.receiver_tokens20c[0]==carrier->trial_token26c&&pair.prefix.beam_latch1d9==0x10,
                          "close spawn and deliberately large declared scale produce a scoped overlap proposal"))return false;
            }else if(!check(pair.route==R::BroadMiss&&!pair.narrow_sampled&&pair.prefix.beam_receiver290==255,
                             "separated spawn misses without proposing contact"))return false;
            if(!check(world.actors()[0].health==world.config().health&&world.actors()[1].health==world.config().health&&
                      !world.hits()&&*physical.scheduler()==registered&&*pool->collision_snapshot()==class4,
                      "query cannot commit HP, hits or registry changes"))return false;
        }
        if(draw_fixture&&capacity==Capacity::Shape){
            auto& render=draw_fixture->render;
            if(!check(!bridge.prepare_trial_beam_art(unrelated,world,view,draw_fixture->art,&error)&&
                      !bridge.prepare_trial_beam_art(source,world,view,NormalGRedBeamArtBank{},&error)&&
                      bridge.prepare_trial_beam_art(source,world,view,draw_fixture->art,&error),
                      "only a current launch and exact effect bank can prepare the shared art"))return false;
            // Frame the recorded launch point from the side, without
            // changing the matrix, pose, or simulation camera.
            Camera camera;
            for(unsigned i=0;i<3;++i)camera.target[i]=launch->flight.position20[i];
            camera.eye[0]=camera.target[0]+140;camera.eye[1]=camera.target[1]+95;camera.eye[2]=camera.target[2]+222;
            camera.viewport_width=640;camera.viewport_height=480;
            render.begin_frame({});
            if(!check(!bridge.draw_trial_beam(unrelated,world,view,render,camera)&&
                      bridge.draw_trial_beam(source,world,view,render,camera)==2&&render.end_frame(),
                      "same accepted carrier submits two authored meshes through RenderEngine"))return false;
            NativeBorgSourcePose republished;
            if(!check(source.pose(world,&republished)&&view.update_source(world,republished,&error)&&
                      !bridge.trial_attack_entry(source,world,view,physical)&&!bridge.trial_receiver(source,world,view,physical)&&
                      !bridge.trial_receiver_entry(source,world,view,physical),
                      "republish invalidates the paid-frame draw gate"))return false;
            GRedSasukeFreshPairPlan stale;
            stale.route=GRedSasukeFreshPairRoute::Suppressed;
            flight.flight.speed44=-1;
            phase_one.active.lifetime1c=-1;
            if(!check(!bridge.probe_trial_fresh_pair(source,world,view,physical,&stale)&&
                      !bridge.probe_trial_first_flight(source,world,view,physical,&flight)&&flight.flight.speed44==-1&&
                      !bridge.probe_trial_first_phase_one(source,world,view,physical,&phase_one,&error)&&phase_one.active.lifetime1c==-1&&
                       stale.route==GRedSasukeFreshPairRoute::Suppressed,
                       "a newer publication produces no replacement query result"))return false;
            render.begin_frame({});
            if(!check(!bridge.draw_trial_beam(source,world,view,render,camera)&&render.end_frame(),
                      "stale same-tick publication submits no beam"))return false;
        }
        return check(
                     *pool->collision_snapshot()==class4&&*descriptors->snapshot()==desc_before&&
                     *physical.scheduler()==registered&&!world.shots_spawned()&&!world.hits(),
                     "isolated registry writes neither geometry, descriptors nor HP");
    }
};
bool trial(Files& files,const std::string& art,const std::string& ground,NativeBattleMvpView& view,
           const GRedNeutralBLocalLibrary& library,s16 rounds,bool edit_final,Capacity capacity=Capacity::NoAttempt,
           const GRedBeamAttackProfile* attack=nullptr,const BeamDrawFixture* draw_fixture=nullptr,
           const NormalSasukeBeamReceiverProfile* receiver_profile=nullptr){
    NativeBattleMvp world;
    NativeBorgAnimationSeed seed;
    NativeGRedNeutralBTrial source;
    BattleMvpConfig config;
    if(capacity==Capacity::PairOverlap)config.spawn={BattlePoint{-110,0},BattlePoint{110,0}};
    auto declared=trial_parent;
    const bool supply=capacity==Capacity::InstallHeader||capacity==Capacity::StaleHeader||shape_case(capacity);
    if(!check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
              source.begin(files,art,ground,world,seed,library,rounds,&error,supply?&declared:nullptr,attack),
              "fresh same-renderer native trial"))return false;
    declared={}; // only the trial's owned source-field sample may later be copied
    AllocationFixture allocation;
    allocation.draw_fixture=draw_fixture;
    allocation.receiver_profile=receiver_profile;
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
    if(argc<4||argc>7){std::fprintf(stderr,
        "usage: %s MVP_ASSET_DIR GROUND_INPUTS_DIR NEUTRAL_B_GFMP [BEAM_SHAPE [BEAM_ART [SASUKE_RECEIVER]]]\n",argv[0]);return 2;}
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
    if(argc>=5){
        std::vector<u8> data;
        GRedBeamAttackProfile attack,altered;
        if(!check(files.read(argv[4],data)&&attack.load({data.data(),data.size()}),"exact credited slot-2 bank and hit row"))return 1;
        data.back()^=1;
        if(!check(!altered.load({data.data(),data.size()})&&!altered.ready()&&attack.ready(),
                  "changed hit row cannot replace the owned attack profile"))return 1;
        NormalGRedBeamArtBank beam_bank;
        if(argc>=6&&(!check(files.read(argv[5],data)&&beam_bank.load({data.data(),data.size()},&error),
                            "exact common beam art bank")))return 1;
        NormalSasukeBeamReceiverProfile receiver;
        if(argc==7){
            if(!check(files.read(argv[6],data)&&receiver.load({data.data(),data.size()},&error),"exact Sasuke receiving bank"))return 1;
            Mat4 key2;
            ColliderSphereSeed sphere;
            data.back()^=1;
            NormalSasukeBeamReceiverProfile altered;
            if(!check(!altered.load({data.data(),data.size()},&error)&&!altered.ready()&&
                      receiver.seed(key2,.5f,&sphere)&&sphere.radius88==11&&sphere.center64==std::array<f32,3>{0,-14,0},
                      "altered row refused; owned profile applies the authored local point"))return 1;
        }
        const BeamDrawFixture draw_fixture{beam_bank,render};
        for(auto capacity:{Capacity::Shape,Capacity::ShapeFull,Capacity::StaleShape,Capacity::StaleMuzzle,
                           Capacity::RegistryOccupied,Capacity::StaleRegistry})
            if(!trial(files,art,ground,view,library,5,false,capacity,&attack,argc>=6?&draw_fixture:nullptr,
                      argc==7?&receiver:nullptr))return 1;
        if(argc==7&&!trial(files,art,ground,view,library,5,false,Capacity::PairOverlap,&attack,nullptr,&receiver))return 1;
    }
    std::printf("native neutral-B trial: %u checks PASS; class3 header%s%s%s, no hit or HP write\n",
                checks,argc>=5?", class4 child, isolated attack registry and first active flight probe":"",argc>=6?", authored beam draw":"",
                argc==7?", isolated Sasuke fresh query":"");
}
