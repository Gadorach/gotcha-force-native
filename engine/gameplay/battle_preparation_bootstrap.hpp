#pragma once
#include "engine/assets/battle_initial_child_group_order.hpp"
#include "engine/gameplay/challenge_initial_party.hpp"
#include "engine/gameplay/scene_descriptor_bootstrap.hpp"
#include "engine/gameplay/scene_physical_pools.hpp"
#include "engine/assets/battle_preparation_update.hpp"
#include "engine/gameplay/battle_views.hpp"
#include "engine/gameplay/battle_scene_services.hpp"
#include "engine/gameplay/battle_common_resources.hpp"
#include "engine/gameplay/battle_stage_resources.hpp"
#include "engine/gameplay/battle_side_resources.hpp"
#include "engine/gameplay/battle_preparation_models.hpp"
#include "engine/gameplay/battle_initial_briefing_input.hpp"
#include "engine/gameplay/sound_commands.hpp"
#include "engine/gameplay/challenge_character_resources.hpp"
#include "engine/gameplay/battle_cockpit_resources.hpp"
#include "engine/gameplay/battle_common_ui_resources.hpp"
#include "engine/gameplay/music_request_owner.hpp"
#include "engine/gameplay/battle_actor_construction.hpp"
#include "engine/assets/battle_model_publication.hpp"
#include "engine/gameplay/battle_stage_construction.hpp"
#include "engine/assets/battle_stage_publication.hpp"
#include "engine/assets/battle_contact_prefix.hpp"
#include "engine/gameplay/battle_contact_tail.hpp"
#include "engine/gameplay/collider_spawn_step.hpp"
#include "engine/gameplay/collider_visibility_segment.hpp"
#include "engine/assets/battle_prop_publication.hpp"
#include "engine/gameplay/battle_initial_presentation.hpp"
#include "engine/gameplay/battle_initial_post_world.hpp"
#include "engine/gameplay/battle_initial_ui_world.hpp"
namespace gf {
enum class BattlePreparationBootstrapStatus {Closed,AtDescriptors,AtPhysicalPools,Failed,Retired,AtActorSeed,AtStageSeed,AtSceneServices,AtViewServices,AtStageResources,AtCommonResources,AtStageBound,AtSideBound,AtStageBindings,AtBriefingResources,AtWidgetSound,AtCharacterResources,AtCharacterQueued,AtCockpitResources,AtEffectQueued,AtFaceSlots,AtCommonUi,AtResourceDrain,AtResourcesReady};
struct BattlePreparationBootstrapState {u8 scene45=0,root00=0,substate01=0;u32 countera8=0,counterac=0,counterb0=0;u64 native_revision=0;std::optional<u8> dispatch3e;};
enum class BattleModelPublicationPhase {Queued,Running,Complete,Failed,StageQueued,AwaitObjectCallbacks};
struct BattleModelPublicationState {BattleModelPublicationPhase phase=BattleModelPublicationPhase::Queued;u64 generation=1;u8 collected_mask=0,dispatched=0,callback_cursor=0,geometry_mask=0;u16 collected_objects=0,object_callback_cursor=0;};
struct BattleInitialChildPrefixState {
    ScenePhysicalLink first,next;u16 updates=0;u8 group_base=0,slot=0;
    bool all_child_updates_complete=false,render_collection_complete=false;
};
struct BattleInitialGraphicsArchivesState {
    u16 cockpit=65535,effects=65535;u32 cockpit_hash=0,effect_hash=0;
    u64 generation=0;bool cockpit_light_inputs_retained=false;
    bool point_light_rendering_supported=false,preparation_transition_complete=false;
};
struct BattleFirstColliderObservation {
    SceneCollisionLease lease;ScenePhysicalLink parent;CharacterCollisionDescriptor descriptor;
    Mat4 source_world;f32 source_c0=0;ColliderInitialPlan initialized;
};
enum class BattleSpawnColliderAction:u8 {Initialize,Update,Hold,RetireInactiveView};
struct BattleSpawnColliderObservation {
    SceneCollisionLease lease;ScenePhysicalLink object_parent;std::optional<u8> view_parent;
    BattleSpawnColliderAction action=BattleSpawnColliderAction::Update;
    CharacterCollisionDescriptor descriptor;Mat4 source_world;f32 source_c0=0;
    std::optional<ColliderInitialPlan> before,after;
};
struct BattleSpawnColliderSweepState {
    std::vector<BattleSpawnColliderObservation> order;u16 initialized=0,updated=0,held=0,retired=0;
    std::array<std::vector<SceneCollisionLease>,10> registries;
};
struct BattleContactPairObservation {u16 first=0,second=0;CollisionBroadphaseResult broadphase;};
enum class BattleContactResetOwnerKind:u8 {PhysicalObject,View};
struct BattleContactResetOwner {BattleContactResetOwnerKind kind=BattleContactResetOwnerKind::PhysicalObject;ScenePhysicalLink object;u8 view_index=255;};
struct BattleContactScratch {
    std::array<s32,4> team_remaining{};std::array<std::optional<s16>,6> health_before{};std::array<std::optional<f32>,6> sphere_bottom{};
    //Source actor-indexed scratch globals cleared by the initial2BB14 loop.
    std::array<std::optional<std::array<u8,4>>,6> cleared_bytes;
    std::array<std::optional<u32>,6> cleared_words;std::array<std::optional<u16>,6> cleared_halves;
    std::array<std::optional<f32>,6> zero_scalars{};
    std::array<s16,4> team_delta{};bool any436=false;
};
struct BattleContactPrefixState:BattleContactScratch {
    std::vector<BattleContactResetOwner> reset_order;std::vector<BattleContactPairObservation> sphere_pairs;u32 next_source=0x8002d35c;
    bool contact_pass_complete=false,pre_camera_visibility_complete=false;
    u16 flagged_static_candidates=0,frozen_static_candidates=0;u8 actor_tail_returns=0,cleared_views=0;
};
struct BattleSpawnContactState:BattleContactScratch {
    std::vector<BattleContactResetOwner> reset_order;std::vector<BattleContactPairObservation> sphere_pairs;
    u16 flagged_static_candidates=0,frozen_static_candidates=0;
    u8 actor_tail_returns=0;bool complete=false;
};
struct BattleVisibilityCast {
    u8 view=255;u16 collider=65535;ScenePhysicalLink parent;bool selected=false,broadphase_passed=false;
    f32 distance=0,radius_sum=0;std::optional<ColliderVisibilityHit> query;
};
struct BattleSpawnVisibilityState {
    std::vector<BattleVisibilityCast> casts;std::array<u8,13> visible26f{};
    std::array<bool,13> selected{};u16 hits=0;bool complete=false;
};
struct BattlePreparationBootstrapPending {bool descriptor197854=true,physical88398=true,remaining_bootstrap=true,widgets=true,update29708=true,late29770=true,current_scene=true;bool actor_seed536b0=true,stage87718=true,view_initialization=true,scene_services=true,common_resources40f74=true,stage_resources=true,cleanup_registration=true,side_resources=true,stage_bindings=true,briefing_resources=true,widget_allocations=true,widget_sound=true,character_queue=true,character_publications=true,cockpit_resources=true,effect_queue=true,effect_textures=true,face_slots=true,common_ui=true;};
//Ordered source prefix starts BEFORE descriptor/object/widget allocation. It
//never imports/resets the populated component-context pools. No actor/world/
//render/widget dispatch receipt is produced by fresh descriptor initialization.
class BattlePreparationBootstrap {
public:
    BattlePreparationBootstrap()=default;BattlePreparationBootstrap(const BattlePreparationBootstrap&)=delete;BattlePreparationBootstrap& operator=(const BattlePreparationBootstrap&)=delete;
    BattlePreparationBootstrap(BattlePreparationBootstrap&&)=default;BattlePreparationBootstrap& operator=(BattlePreparationBootstrap&&)=default;~BattlePreparationBootstrap(){retire();}
    bool begin_normal_entry(const ChallengeInitialPartyCandidate& party,const BattlePreparationUpdateProfile& update,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("ordered preparation: ")+why;return false;};
        if(attempted_||!party.current(owners)||!update.ready())return reject("fresh/current party and source profile required");
        const auto entry=party.preparation_entry_.lock();if(!entry||!entry->live||!entry->handoff_ready||entry->prefix_claimed||entry->branch_claimed||entry->state.common50!=1||entry->state.common64||entry->state.root00||entry->state.substate01||entry->state.native_revision!=1)return reject("same unclaimed fresh normal final handoff required");
        if(entry->state.common00&&(entry->state.common00!=u8(1)||entry->state.caller4!=s16(0)))return reject("fresh normal enclosing dispatch required");
        attempted_=true;status_=BattlePreparationBootstrapStatus::Failed;entry->prefix_claimed=entry->branch_claimed=true;
        try{auto next=std::make_shared<Storage>();next->party=party;next->entry=entry;next->state.scene45=update.initial_scene45();entry->scene45=next->state.scene45;next->state.native_revision=1;
            //801968CC's completed resource wait advances caller4 from0 to1 and
            //clears the fresh scene prefix before dispatching2844C.
            if(entry->state.caller4)entry->state.caller4=1;
            ++entry->state.root00;entry->state.substate01=0;++entry->state.native_revision;next->state.root00=entry->state.root00;next->state.substate01=entry->state.substate01;next->state.countera8=entry->state.countera8;next->state.counterac=entry->state.counterac;next->state.counterb0=entry->state.counterb0;++next->state.native_revision;
            storage_=std::move(next);status_=BattlePreparationBootstrapStatus::AtDescriptors;if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native entry allocation; source claim retained");}
    }
    bool initialize_descriptors(const SceneDescriptorBootstrapProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtDescriptors||!current(owners)||!profile.ready()){if(error)*error="ordered preparation: current descriptor call boundary/profile required";return false;}
        status_=BattlePreparationBootstrapStatus::Failed;if(!storage_->descriptors.begin_fresh(profile,error))return false;const auto entry=storage_->entry.lock();if(!entry||!entry->live){if(error)*error="ordered descriptors: origin retired";return false;}entry->archives=storage_->descriptors.archive_pool();++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtPhysicalPools;if(error)error->clear();return true;
    }
    bool initialize_physical_pools(const ScenePhysicalPoolsProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtPhysicalPools||!current(owners)||!profile.ready()||!storage_->descriptors.pristine_for_physical_join()){if(error)*error="ordered preparation: current untouched physical-call boundary/profile required";return false;}
        status_=BattlePreparationBootstrapStatus::Failed;if(!storage_->physical.begin_fresh(profile,storage_->descriptors,error))return false;const auto entry=storage_->entry.lock();if(!entry||!entry->live){if(error)*error="ordered preparation: entry retired during physical initialization";return false;}entry->physical_controls=storage_->physical.control_binding();++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtActorSeed;if(error)error->clear();return true;
    }
    //Fresh962C4→0685C(0) resets selected84/next15C8, then68424 selects member0
    //for every active physical slot. Do not substitute a separately prepared
    //resource allocation's caller-selected member (which may intentionally be1).
    bool initialize_actor_slots(const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("ordered actor seed: ")+why;return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtActorSeed||!current(owners))return reject("same current physical seed boundary required");
        std::array<BattleFixedSlotInput,6> input{};std::array<u8,6> next_member{};
        for(unsigned i=0;i<6;++i){const bool active=(storage_->party.active()&(1u<<i))!=0;const auto members=storage_->party.members(i);if(!active){if(!members.empty()||storage_->party.team(i)!=-1)return reject("inconsistent inactive physical slot");continue;}if(members.empty()||members.size()>30||storage_->party.team(i)<0||storage_->party.team(i)>1)return reject("nonempty source member list required");const auto& first=members.front().fields;if(first.model==0xffff||first.variant>=6)return reject("invalid first source member");input[i]={first.model,first.variant,u8(0)};next_member[i]=u8(1%members.size());}
        status_=BattlePreparationBootstrapStatus::Failed;if(!storage_->physical.seed_fixed_actors(input,error))return false;storage_->next_member=next_member;++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtStageSeed;if(error)error->clear();return true;
    }
    bool initialize_stage(const BattleStageSeedProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtStageSeed||!current(owners)){if(error)*error="ordered stage seed: current boundary after six-slot factory required";return false;}const auto* row=profile.find(storage_->party.stage().stage);if(!row){if(error)*error="ordered stage seed: exact party stage source row required";return false;}status_=BattlePreparationBootstrapStatus::Failed;if(!storage_->physical.seed_stage(*row,error))return false;++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtSceneServices;if(error)error->clear();return true;
    }
    bool initialize_views(const BattleViewsProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtSceneServices||!current(owners)||!profile.ready()){if(error)*error="ordered views: current post-stage boundary/profile required";return false;}
        const auto* display=storage_->party.display_setup();if(!display)return false;
        BattleViewSceneInputs inputs;inputs.humans=storage_->party.humans();inputs.active=storage_->party.active();inputs.players=storage_->party.players();inputs.layout29=display->layout29;inputs.split3d=display->split3d;inputs.controller_c0_c3=storage_->party.controller_order();
        status_=BattlePreparationBootstrapStatus::Failed;
        if(!storage_->views.initialize(profile,error)||!storage_->views.bind_scene_inputs(inputs,storage_->physical.class2_pool(),storage_->physical.control_binding(),error))return false;
        ++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtViewServices;if(error)error->clear();return true;
    }
    bool initialize_scene_services(SoundControlOwner& sound,SourceByteRngOwner& rng,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("ordered scene services: ")+why;return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtViewServices||!current(owners)||sound.initial_program_gain(0,1)!=127||!rng.same_owner(storage_->party.rng_source_))return reject("current view boundary/initialized bank0/SAME party RNG stream required");
        if(rng.revision()==std::numeric_limits<u64>::max()||rng.draws()>std::numeric_limits<u64>::max()-4)return reject("native RNG bookkeeping overflow");
        const auto snapshot=rng.snapshot();BattleSceneServices next;
        if(!challenge_battle_team_plan(storage_->party,owners,snapshot.state(),&next.team_fields)||next.team_fields.draws!=4)return reject("current source team/party domain");
        next.pause_context.fill(0);next.pause_context[9]=storage_->party.humans();
        //All candidate allocation/branch guards precede source mutations. This is
        //a serialized native operation, not an interrupt-level atomicity claim.
        status_=BattlePreparationBootstrapStatus::Failed;
        try{next.pause_context[13]=1; //529DC follows the verified equal-gain branch
            if(!sound.initialize_battle_controls(error)){return false;}next.sound=sound.snapshot();
            if(!rng.commit(snapshot,next.team_fields.after,next.team_fields.draws))return reject("RNG changed during source operation; no replay");
            storage_->services=std::move(next);++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtStageResources;if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain scene-service allocation; no replay");}
    }
    bool initialize_common_resources(const BattleCommonResourceProfile& profile,IFileSystem& files,const std::string& path,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtStageResources||!current(owners)||!profile.ready()||path.empty()){if(error)*error="ordered common resources: current source boundary/profile/path required";return false;}
        const auto* identity=storage_.get();status_=BattlePreparationBootstrapStatus::Failed;BattleCommonResources next;
        if(!next.load(profile,files,path,error))return false;
        //The resource candidate is local so a file callback can retire the scene
        //without destroying an object whose load method is still executing.
        if(status_!=BattlePreparationBootstrapStatus::Failed||storage_.get()!=identity||!storage_->party.current(owners)||!storage_->services.current()||!storage_->descriptors.current()||!storage_->physical.current()){if(error)*error="ordered common resources: scene retired during read";return false;}
        const auto entry=storage_->entry.lock();if(!entry||!entry->live){if(error)*error="ordered common resources: origin retired during read";return false;}
        storage_->common=std::move(next);entry->common=storage_->common.binding();++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtCommonResources;if(error)error->clear();return true;
    }
    bool initialize_stage_resources(const ChallengeStageInputsProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtCommonResources||!current(owners)||!profile.ready()){if(error)*error="ordered stage resources: current common-resource boundary/profile required";return false;}
        const auto data=storage_->party.stage_data_;const auto& selected=storage_->party.stage();
        if(!data||data->selection().stage!=selected.stage||data->selection().field_1d!=selected.field_1d||data->selection().field_1e!=selected.field_1e){if(error)*error="ordered stage resources: SAME final native data handoff required";return false;}
        const auto* row=profile.find(data->selection().payloads[0].id);const auto* seed=storage_->physical.stage_seed();if(!row||!seed||row->stage!=selected.stage||f32_to_bits(row->ground.high34)!=f32_to_bits(seed->high34)||f32_to_bits(row->ground.low68)!=f32_to_bits(seed->low68)){if(error)*error="ordered stage resources: seed/source ground-profile mismatch";return false;}
        status_=BattlePreparationBootstrapStatus::Failed;
        if(!storage_->stage_resources.initialize(data,profile,*storage_->descriptors.archive_pool(),error))return false;
        ++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtStageBound;if(error)error->clear();return true;
    }
    bool initialize_side_resources(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtStageBound||!current(owners)){if(error)*error="ordered side resources: current stage boundary required";return false;}
        const auto entry=storage_->entry.lock();
        if(!entry||!entry->deferred.initialized()||entry->deferred.read_cursor()||entry->deferred.write_cursor()){if(error)*error="ordered side resources: genuine final callback empty ring required";return false;}
        status_=BattlePreparationBootstrapStatus::Failed;
        if(!entry->deferred.register_battle_cleanup()){if(error)*error="ordered side resources: cleanup registration refused";return false;}
        if(!storage_->side_resources.initialize(storage_->party.stage_data_,*storage_->descriptors.archive_pool(),error))return false;
        ++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtSideBound;if(error)error->clear();return true;
    }
    bool initialize_stage_bindings(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtSideBound||!current(owners)){if(error)*error="ordered stage bindings: current side-resource boundary required";return false;}
        status_=BattlePreparationBootstrapStatus::Failed;
        if(!storage_->stage_resources.bind_initial_state(error))return false;
        ++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtStageBindings;if(error)error->clear();return true;
    }
    bool initialize_briefing_resources(const BattlePreparationModelProfile& profile,IFileSystem& files,const std::string& path,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtStageBindings||!current(owners)||!profile.ready()||path.empty()){if(error)*error="ordered briefing: current stage-binding boundary/profile/path required";return false;}
        const auto* identity=storage_.get();const auto archives=storage_->descriptors.archive_pool();
        status_=BattlePreparationBootstrapStatus::Failed;BattlePreparationModels next;
        //Retain the pool independently while external file callbacks may retire
        //or move the scene. The in-flight resource owner is a local candidate.
        if(!next.begin_resources(profile,files,path,*archives,error))return false;
        if(status_!=BattlePreparationBootstrapStatus::Failed||!storage_||storage_.get()!=identity){if(error)*error="ordered briefing: scene retired/moved during read";return false;}
        storage_->briefing=std::move(next);status_=BattlePreparationBootstrapStatus::AtBriefingResources;
        if(!current(owners)){status_=BattlePreparationBootstrapStatus::Failed;if(error)*error="ordered briefing: source origin/pool retired during read";return false;}
        ++storage_->state.native_revision;if(error)error->clear();return true;
    }
    bool initialize_widget_allocations(const BattlePreparationWidgetProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtBriefingResources||!current(owners)||!profile.ready()){if(error)*error="ordered widgets: current briefing-resource boundary/profile required";return false;}
        status_=BattlePreparationBootstrapStatus::Failed;
        if(!storage_->widgets.begin(profile,storage_->physical.class2_pool(),error))return false;
        ++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtWidgetSound;if(error)error->clear();return true;
    }
    //Mutates the caller's EXISTING shared source staging queue, preserving its
    //packets/count. This is EFC04 command admission, not ring drain/voice/PCM ACK.
    bool enqueue_widget_sound(const SoundCommandBanks& banks,const SoundControlOwner& sound,StartupSoundCommandQueue& staging,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtWidgetSound||!current(owners)||!sound.snapshot().same_owner(storage_->services.sound)||sound.initial_program_gain(2,116)!=127){if(error)*error="ordered widget sound: current source boundary/same initialized sound bank2 owner required";return false;}
        StartupSoundCommandObservation observation;if(!sound.snapshot().commands(&observation,error))return false;
        StartupSoundCommandPlan plan;if(!sound_null_owner_command_plan(banks,staging,observation,372,&plan,error))return false;
        //All work above is pure and can refuse unsupported tone/RNG metadata
        //without changing the caller's queue or the reached source boundary.
        staging=std::move(plan.candidate);storage_->widget_sound=plan.outcomes[0];storage_->widgets.storage_->cue372_required=false;
        ++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtCharacterResources;if(error)error->clear();return true;
    }
    bool queue_character_resources(const BattleCharacterResourceProfile& profile,const std::string& directory,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtCharacterResources||!current(owners)||directory.empty()){if(error)*error="ordered characters: current post-widget boundary/path required";return false;}
        const auto* seeds=storage_->physical.actor_seed();std::array<u8,6> selected;selected.fill(255);
        if(!seeds){if(error)*error="ordered characters: actual fixed slots required";return false;}
        for(unsigned slot=0;slot<6;++slot){
            const auto& seed=seeds->slots[slot];const auto members=storage_->party.members(slot);const bool active=(storage_->party.active()&(1u<<slot))!=0;
            if(!active){if(seed.active00||!members.empty()){if(error)*error="ordered characters: inactive physical slot mismatch";return false;}continue;}
            if(members.empty()||seed.active00!=1||seed.member492||seed.model3e8!=members[0].fields.model||seed.variant3ec!=members[0].fields.variant||seed.resource3e7!=slot){if(error)*error="ordered characters: same first-member physical seed required";return false;}
            selected[slot]=0;
        }
        status_=BattlePreparationBootstrapStatus::Failed;
        if(!storage_->characters.begin_queued(storage_->party,owners,selected,profile,storage_->descriptors.archive_pool(),directory,error))return false;
        ++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtCharacterQueued;if(error)error->clear();return true;
    }
    bool initialize_cockpit_resources(const BattleCockpitResourceProfile& profile,IFileSystem& files,const std::string& directory,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtCharacterQueued||!current(owners)||!profile.ready()||directory.empty()){if(error)*error="ordered cockpit: current character-queue boundary/profile/path required";return false;}
        const auto* display=storage_->party.display_setup();if(!display||display->layout29>1){if(error)*error="ordered cockpit: reached Challenge display setup required";return false;}
        const auto* identity=storage_.get();const u8 mode=display->layout29;status_=BattlePreparationBootstrapStatus::Failed;BattleCockpitResources next;
        if(!next.load(profile,mode,files,directory,error))return false;
        if(!storage_||storage_.get()!=identity||status_!=BattlePreparationBootstrapStatus::Failed){if(error)*error="ordered cockpit: scene retired/moved during read";return false;}
        storage_->cockpit=std::move(next);status_=BattlePreparationBootstrapStatus::AtCockpitResources;
        if(!current(owners)){status_=BattlePreparationBootstrapStatus::Failed;if(error)*error="ordered cockpit: source origin/pools retired during read";return false;}
        ++storage_->state.native_revision;if(error)error->clear();return true;
    }
    bool queue_common_effects(const BattleEffectResourceProfile& profile,const std::string& path,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtCockpitResources||!current(owners)||!profile.ready()||path.empty()){if(error)*error="ordered effects: current cockpit boundary/profile/path required";return false;}
        //42874 is an actual empty return.42878 then appends the package load
        //and four-argument texture callback to the existing deferred queue.
        status_=BattlePreparationBootstrapStatus::Failed;
        if(!storage_->characters.append_common_effects(profile,path,owners,error))return false;
        ++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtEffectQueued;if(error)error->clear();return true;
    }
    bool initialize_face_slots(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtEffectQueued||!current(owners)){if(error)*error="ordered face slots: current effect-queue boundary required";return false;}
        const auto entry=storage_->entry.lock();if(!entry||entry->state.common50!=1){if(error)*error="ordered face slots: same Challenge common+32=1 branch required";return false;}
        //42FE4 clears six header/data pairs and writes all six selectors FF.
        //Challenge common+32!=0 skips the entire role-table/file-read branch.
        storage_->faces=std::array<BattlePlayerFaceSlot,6>{};++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtFaceSlots;if(error)error->clear();return true;
    }
    bool initialize_common_ui(const BattleCommonUiProfile& profile,IFileSystem& files,const std::string& path,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtFaceSlots||!current(owners)||!profile.ready()||path.empty()){if(error)*error="ordered common UI: current face-slot boundary/profile/path required";return false;}
        const auto* identity=storage_.get();const auto pool=storage_->descriptors.archive_pool();status_=BattlePreparationBootstrapStatus::Failed;BattleCommonUiResources next;
        if(!next.load(profile,files,path,pool,error))return false;
        if(!storage_||storage_.get()!=identity||status_!=BattlePreparationBootstrapStatus::Failed){if(error)*error="ordered common UI: scene retired/moved during read";return false;}
        storage_->common_ui=std::move(next);status_=BattlePreparationBootstrapStatus::AtCommonUi;
        if(!current(owners)){status_=BattlePreparationBootstrapStatus::Failed;if(error)*error="ordered common UI: origin/pool retired";return false;}
        ++storage_->state.native_revision;if(error)error->clear();return true;
    }
    bool finish_normal_bootstrap(MusicRequestOwner& music,const SoundControlOwner& sound,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtCommonUi||!current(owners)||!music.ready()||!sound.ready()||!sound.snapshot().same_owner(storage_->services.sound)||!music.snapshot().same_sound_owner(sound.snapshot())){if(error)*error="ordered bootstrap tail: current source boundary/shared music and sound owners required";return false;}
        const auto entry=storage_->entry.lock();if(!entry||!entry->live||entry->state.route15dd){if(error)*error="ordered bootstrap tail: privately retained fresh normal route required";return false;}
        if(entry->state.common00&&(entry->state.common00!=u8(1)||entry->state.caller4!=s16(1)||entry->state.normal_dispatch_returned||entry->state.native_revision!=2)){if(error)*error="ordered bootstrap tail: current first enclosing dispatch required";return false;}
        status_=BattlePreparationBootstrapStatus::Failed;storage_->state.dispatch3e=5; //8C3A0
        MusicRequestOutcome outcome;
        if(!music.request_start(music.battle_member(),sound,&outcome,error))return false;
        storage_->music=music.snapshot();storage_->music_outcome=outcome;
        //80196948 after this first28264/2844C call returns. Only the fresh V2
        //entry owns mode1; legacy entry paths keep the common mode unknown.
        if(entry->state.common00==u8(1)){entry->state.caller4=2;entry->state.common00=3;entry->state.common01=1;entry->state.normal_dispatch_returned=true;++entry->state.native_revision;}
        ++storage_->state.native_revision;status_=BattlePreparationBootstrapStatus::AtResourceDrain;if(error)error->clear();return true;
    }
    //Loader work is enabled only after2844C has actually returned. The retained
    //storage keeps in-flight method objects alive through filesystem callbacks;
    //retirement still invalidates their scopes immediately.
    bool advance_resource_work(IFileSystem& files,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtResourceDrain||!current(owners)){if(error)*error="ordered resource drain: completed bootstrap/current resource owners required";return false;}
        const auto keep=storage_;
        const bool completed=keep->characters.advance_queued(files,owners,error);
        if(storage_!=keep||!keep->live){keep->retire();if(error)*error="ordered resource drain: scene retired/replaced during work";return false;}
        if(!completed||!current(owners)){status_=BattlePreparationBootstrapStatus::Failed;if(error&&error->empty())*error="ordered resource drain: failed/retired operation";return false;}
        ++keep->state.native_revision;
        if(keep->characters.successful_drain(owners))status_=BattlePreparationBootstrapStatus::AtResourcesReady;
        if(error){error->clear();}return true;
    }
    bool current(const std::array<const ForceInventory*,2>& owners)const{
        if(!storage_||!storage_->live||status_==BattlePreparationBootstrapStatus::Closed||status_==BattlePreparationBootstrapStatus::Failed||status_==BattlePreparationBootstrapStatus::Retired||!storage_->party.current(owners))return false;
        const auto entry=storage_->entry.lock();if(!entry||!entry->live||!entry->prefix_claimed||!entry->branch_claimed||entry->scene45!=storage_->state.scene45||entry->state.root00!=storage_->state.root00||entry->state.substate01!=storage_->state.substate01)return false;
        if(storage_->initial_graphics_archives){const auto* effects=storage_->characters.common_effects(owners);if(!storage_->cockpit.bound_bank()||!storage_->cockpit.initial_light_inputs()||!effects||!effects->bound_bank())return false;}
        if(status_!=BattlePreparationBootstrapStatus::AtDescriptors&&!storage_->descriptors.current())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtActorSeed&&(!storage_->physical.current()||entry->physical_controls.lock().get()!=storage_->physical.controls()))return false;
        if(status_>=BattlePreparationBootstrapStatus::AtViewServices&&!storage_->views.ready())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtStageResources&&!storage_->services.current())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtCommonResources&&!storage_->common.ready())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtStageBound&&!storage_->stage_resources.current())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtSideBound&&(!storage_->side_resources.current()||!entry->deferred.battle_cleanup_pending()))return false;
        if(status_>=BattlePreparationBootstrapStatus::AtStageBindings&&!storage_->stage_resources.static_bindings())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtBriefingResources&&!storage_->briefing.resources_current())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtWidgetSound&&!storage_->widgets.current())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtCharacterQueued&&!storage_->characters.current(owners))return false;
        if(status_>=BattlePreparationBootstrapStatus::AtCockpitResources&&!storage_->cockpit.ready())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtEffectQueued&&!storage_->characters.common_effects(owners))return false;
        if(status_>=BattlePreparationBootstrapStatus::AtFaceSlots&&!storage_->faces)return false;
        if(status_>=BattlePreparationBootstrapStatus::AtCommonUi&&!storage_->common_ui.current())return false;
        if(status_>=BattlePreparationBootstrapStatus::AtResourceDrain&&(!storage_->music.live()||storage_->state.dispatch3e!=5))return false;
        return status_!=BattlePreparationBootstrapStatus::AtResourcesReady||storage_->characters.successful_drain(owners);
    }
    const BattlePreparationBootstrapState* state(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?&storage_->state:nullptr;}
    const SceneDescriptorBootstrap* descriptors(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_!=BattlePreparationBootstrapStatus::AtDescriptors?&storage_->descriptors:nullptr;}
    const ScenePhysicalPools* physical_pools(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_!=BattlePreparationBootstrapStatus::AtDescriptors&&status_!=BattlePreparationBootstrapStatus::AtPhysicalPools?&storage_->physical:nullptr;}
    const std::array<u8,6>* next_member_indices(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_!=BattlePreparationBootstrapStatus::AtDescriptors&&status_!=BattlePreparationBootstrapStatus::AtPhysicalPools&&status_!=BattlePreparationBootstrapStatus::AtActorSeed?&storage_->next_member:nullptr;}
    const BattleViews* views(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtViewServices?&storage_->views:nullptr;}
    const BattleSceneServices* services(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtStageResources?&storage_->services:nullptr;}
    const BattleCommonResources* common_resources(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtCommonResources?&storage_->common:nullptr;}
    const BattleStageResources* stage_resources(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtStageBound?&storage_->stage_resources:nullptr;}
    const BattleSideResources* side_resources(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtSideBound?&storage_->side_resources:nullptr;}
    const SceneDeferredCallbacks* deferred_callbacks(const std::array<const ForceInventory*,2>& owners)const{const auto entry=storage_?storage_->entry.lock():nullptr;return current(owners)&&entry?&entry->deferred:nullptr;}
    const BattlePreparationModels* briefing_resources(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtBriefingResources?&storage_->briefing:nullptr;}
    const BattlePreparationWidgets* widgets(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtWidgetSound?&storage_->widgets:nullptr;}
    const StartupSoundCommandOutcome* widget_sound_outcome(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtCharacterResources?&storage_->widget_sound:nullptr;}
    const ChallengeCharacterResources* character_resources(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtCharacterQueued?&storage_->characters:nullptr;}
    const BattleCockpitResources* cockpit_resources(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtCockpitResources?&storage_->cockpit:nullptr;}
    const BattleEffectResources* common_effect_resources(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtEffectQueued?storage_->characters.common_effects(owners):nullptr;}
    const std::array<BattlePlayerFaceSlot,6>* face_slots(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->faces?&*storage_->faces:nullptr;}
    const BattleCommonUiResources* common_ui_resources(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtCommonUi?&storage_->common_ui:nullptr;}
    const MusicRequestOutcome* initial_music_request(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&status_>=BattlePreparationBootstrapStatus::AtResourceDrain?&storage_->music_outcome:nullptr;}
    BattlePreparationBootstrapPending pending()const{BattlePreparationBootstrapPending p;switch(status_){case BattlePreparationBootstrapStatus::AtResourcesReady:p.character_publications=p.effect_textures=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtResourceDrain:p.remaining_bootstrap=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtCommonUi:p.common_ui=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtFaceSlots:p.face_slots=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtEffectQueued:p.effect_queue=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtCockpitResources:p.cockpit_resources=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtCharacterQueued:p.character_queue=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtCharacterResources:p.widget_sound=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtWidgetSound:p.widget_allocations=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtBriefingResources:p.briefing_resources=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtStageBindings:p.stage_bindings=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtSideBound:p.cleanup_registration=p.side_resources=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtStageBound:p.stage_resources=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtCommonResources:p.common_resources40f74=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtStageResources:p.scene_services=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtViewServices:p.view_initialization=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtSceneServices:p.stage87718=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtStageSeed:p.actor_seed536b0=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtActorSeed:p.physical88398=false;[[fallthrough]];case BattlePreparationBootstrapStatus::AtPhysicalPools:p.descriptor197854=false;break;default:break;}return p;}
    u32 next_source_address(const std::array<const ForceInventory*,2>& owners)const{
        if(!current(owners))return 0;
        switch(status_){
        case BattlePreparationBootstrapStatus::AtDescriptors:return 0x800284f4;
        case BattlePreparationBootstrapStatus::AtPhysicalPools:return 0x800284f8;
        case BattlePreparationBootstrapStatus::AtActorSeed:return 0x80028510;
        case BattlePreparationBootstrapStatus::AtStageSeed:return 0x80028520;
        case BattlePreparationBootstrapStatus::AtSceneServices:return 0x80028530;
        case BattlePreparationBootstrapStatus::AtViewServices:return 0x8002859c;
        case BattlePreparationBootstrapStatus::AtStageResources:return 0x8002860c;
        case BattlePreparationBootstrapStatus::AtCommonResources:return 0x80028610;
        case BattlePreparationBootstrapStatus::AtStageBound:return 0x8002861c;
        case BattlePreparationBootstrapStatus::AtSideBound:return 0x80028638;
        case BattlePreparationBootstrapStatus::AtStageBindings:return 0x80028640;
        case BattlePreparationBootstrapStatus::AtBriefingResources:return 0x80028644;
        case BattlePreparationBootstrapStatus::AtWidgetSound:return 0x80199b34;
        case BattlePreparationBootstrapStatus::AtCharacterResources:return 0x8002864c;
        case BattlePreparationBootstrapStatus::AtCharacterQueued:return 0x80028650;
        case BattlePreparationBootstrapStatus::AtCockpitResources:return 0x80028654;
        case BattlePreparationBootstrapStatus::AtEffectQueued:return 0x8002865c;
        case BattlePreparationBootstrapStatus::AtFaceSlots:return 0x80028660;
        case BattlePreparationBootstrapStatus::AtCommonUi:return 0x80028668;
        case BattlePreparationBootstrapStatus::AtResourceDrain:
        case BattlePreparationBootstrapStatus::AtResourcesReady:return 0x800286d8;
        default:return 0;
        }
    }
    //Begin and register at revision0 as one gateway. An already advanced or
    //unregistered consumed allocation cannot be attached retroactively.
    bool begin_actor_construction(BattleActorConstruction& actor,BattleCharacterBinding&& prepared,const ChallengeCharacterDescriptorReceipt& receipt,const BattleActorBirthProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        return begin_registered_actor(actor,std::move(prepared),receipt,profile,nullptr,owners,error);
    }
    //First53B30 only. Captures the actual untouched factory cells, performs the
    //two draws against the party's current stream, then owns dispatch order.
    //This is not a claim that intervening input/view/group callbacks ran.
    bool begin_initial_actor_order(const BattleInitialActorOrderProfile& profile,SourceByteRngOwner& rng,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene initial actor order: ")+why;return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||storage_->initial_order||storage_->publication||storage_->stage_construction.phase()!=BattleStageConstructionPhase::Closed||!profile.ready()||!rng.same_owner(storage_->party.rng_source_))return reject("fresh drained scene/source profile/same current party RNG required");
        auto& physical=*storage_->physical.storage_;const auto batch=storage_->characters.batch_;
        if(!physical.actor_seeded||physical.scheduler.initial_actor_order||physical.banks[0].records.size()!=6||physical.stage.phase00||storage_->state.native_revision==std::numeric_limits<u64>::max())return reject("untouched fixed factory and stage boundary required");
        std::array<u8,6> active{};
        for(unsigned i=0;i<6;++i){
            if(physical.constructor_claimed[i]||batch->construction_claimed[i])return reject("consumed actor cannot be imported or replaced by a factory observation");
            const auto& seed=physical.actors.slots[i];if(seed.slot3e4!=i||seed.kind83||seed.active00>1||physical.banks[0].records[i].active00!=seed.active00)return reject("same native fixed-slot factory required");
            active[i]=seed.active00;
        }
        if(rng.revision()==std::numeric_limits<u64>::max()||rng.draws()>std::numeric_limits<u64>::max()-2)return reject("native RNG bookkeeping overflow");
        const auto snapshot=rng.snapshot();BattleInitialActorOrderPlan plan;if(!battle_initial_actor_order(profile,active,snapshot.state(),&plan,error))return false;
        try{
            auto control=std::make_unique<InitialActorOrderControl>();control->phase=plan.state.count?InitialActorOrderPhase::Ordered:InitialActorOrderPhase::ActorsComplete;
            //No callbacks/re-entry occur in the admitted factory branch. All
            //native storage is prepared before this serialized source commit.
            if(!rng.commit(snapshot,plan.after,2))return reject("RNG changed before source commit");
            control->rng=rng.snapshot();physical.scheduler.initial_actor_order=plan.state;
            storage_->initial_order=std::move(control);++storage_->state.native_revision;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain first ordering operation; no replay");}
    }
    const BattleInitialActorOrderState* initial_actor_order(const std::array<const ForceInventory*,2>& owners)const{return initial_order_current(owners)?&*storage_->physical.storage_->scheduler.initial_actor_order:nullptr;}
    std::optional<u8> next_initial_actor_slot(const std::array<const ForceInventory*,2>& owners)const{
        if(!initial_order_current(owners)||storage_->initial_order->phase!=InitialActorOrderPhase::Ordered||storage_->initial_order->inflight)return std::nullopt;
        const auto& p=*storage_->physical.storage_->scheduler.initial_actor_order;const auto n=storage_->initial_order->cursor;
        return n<p.count&&p.updates[n]?std::optional<u8>{u8(p.updates[n]->index)}:std::nullopt;
    }
    //0BEF4 runs before the first54070 calls. Participant inputs were produced by
    //1910B4/962C4/0685C, not reconstructed from already-created actors here.
    bool bind_initial_view_aliases(const BattleCameraEntryProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="scene initial view aliases: current unconsumed factory/order boundary required";return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!initial_order_current(owners)||storage_->initial_order->cursor||storage_->initial_order->claimed||storage_->initial_order->inflight||storage_->stage_construction.phase()!=BattleStageConstructionPhase::Closed)return reject();
        const auto entry=storage_->entry.lock();if(!entry||entry->state.common64)return reject();
        std::array<u8,6> active{};for(unsigned i=0;i<6;++i)active[i]=storage_->physical.storage_->actors.slots[i].active00;
        return storage_->views.bind_initial_aliases(profile,active,error);
    }
    bool initialize_first_graphics_presentation(const BattleInitialPresentationProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="battle initial graphics fragment: unconsumed fresh resource handoff before actor dispatch required";return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||storage_->initial_order||storage_->publication||storage_->stage_construction.phase()!=BattleStageConstructionPhase::Closed)return reject();
        const auto* input=storage_->views.scene_inputs();const auto* display=storage_->party.display_setup();
        if(!input||!input->split3d||!display)return reject();
        return storage_->presentation.initialize(profile,input->layout29,*input->split3d,display->enabled33,error);
    }
    bool initialize_initial_graphics_archives(const BattleInitialGraphicsArchivesProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="initial graphics archives: fresh drained source before actors/presentation required";return false;};
        if(!profile.ready()||status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||storage_->initial_order||storage_->publication||storage_->initial_graphics_archives||storage_->presentation.state()||storage_->stage_construction.phase()!=BattleStageConstructionPhase::Closed)return reject();
        const auto archives=storage_->descriptors.archive_pool();const auto effects=storage_->characters.batch_->effects;
        if(!archives||!archives->ready()||!effects||effects->status()!=BattleEffectResourceStatus::TexturesBound)return reject();
        status_=BattlePreparationBootstrapStatus::Failed;
        try{
            auto state=std::make_unique<BattleInitialGraphicsArchivesState>();
            if(!storage_->cockpit.bind_initial_graphics(profile,*archives,error))return false;
            if(!effects->bind_initial_graphics(profile,*archives,error))return false;
            const auto* ui=storage_->cockpit.archive_record();const auto* fx=effects->archive_record();
            if(!ui||!fx||ui->models1a!=118||fx->models1a!=157||!storage_->cockpit.initial_light_inputs())return reject();
            state->cockpit=storage_->cockpit.storage_->header.index();state->effects=effects->storage_->archive.index();
            state->cockpit_hash=ui->bank_hash;state->effect_hash=fx->bank_hash;state->generation=1;state->cockpit_light_inputs_retained=true;
            storage_->initial_graphics_archives=std::move(state);status_=BattlePreparationBootstrapStatus::AtResourcesReady;
            if(error){error->clear();}return true;
        }catch(const std::exception&){if(error)*error="initial graphics archives: uncertain prefix retained; no replay";return false;}
    }
    const BattleInitialGraphicsArchivesState* initial_graphics_archives(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?storage_->initial_graphics_archives.get():nullptr;}
    bool begin_first_presented_world_view(const BattleInitialPresentationProfile& profile,const BattleSplitProjectionProfile& projection,RenderEngine& render,u32 history_texture,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="battle first presented view: current source generation2 cameras and initial graphics owner required";return false;};
        const auto* views=current_camera_views(owners);
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!views||views->pose_generation()!=2||!storage_->spawn_visibility||!storage_->spawn_visibility->complete||!storage_->spawn_contacts||!storage_->spawn_contacts->complete)return reject();
        const auto alias=views->viewer_index(0);if(!alias||*alias<0||*alias>=6)return reject();
        const auto* record=views->view(unsigned(*alias));if(!record||!record->active00||record->pose_generation!=2)return reject();
        return storage_->presentation.begin_first_view(profile,projection,render,history_texture,u8(*alias),record->eye2e8,record->target300,record->up330,error);
    }
    const BattleInitialPresentationState* first_graphics_presentation(const std::array<const ForceInventory*,2>& owners)const{
        if(!current(owners))return nullptr;
        const auto* state=storage_->presentation.state();if(!state)return nullptr;
        if(state->phase==BattleInitialPresentationPhase::WorldViewBegun){const auto* views=current_camera_views(owners);if(!views||views->pose_generation()!=2||state->view>=6||!views->view(state->view)->active00)return nullptr;}
        return state;
    }
    //8A28C→89AEC/8A65C first initialization. The real groups48,49..54,55
    //supply the enumeration; callers cannot substitute a collider subset.
    bool initialize_first_scene_colliders(const ColliderInitialShapeProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene first colliders: ")+why;return false;};
        if(!profile.ready()||status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!initial_order_current(owners)||storage_->publication||storage_->collisions||storage_->stage_construction.object_world_generation()!=1||storage_->stage_construction.first_object_updates_complete()||storage_->views.pose_generation())return reject("first actual actor/stage worlds before camera allocation required");
        if(storage_->initial_world_dispatch){const auto* caller=initial_post_world(owners);if(!caller||caller->phase!=BattleInitialPostWorldPhase::CounterAdvanced)return reject("typed89554 path requires its actual29434 counter/common66 caller before collider work");}
        auto& physical=*storage_->physical.storage_;auto pool=physical.class2;auto& arena=*pool->storage_;
        if(physical.scheduler.registry_counts!=std::array<u16,10>{})return reject("unconsumed source registry prefix required");
        try{
            auto next=std::make_unique<CollisionInitialization>();next->objects=pool;next->order.reserve(384);
            for(unsigned slot=0;slot<6;++slot)if(physical.actors.slots[slot].active00){
                const auto scope=physical.constructors[slot].scope_.lock();
                if(!scope||!scope->live||!scope->owner||!scope->owner->current(owners)||scope->owner->status()!=BattleActorConstructionStatus::FirstWorldReady)return reject("exact active registered first-world actors required");
                const auto& s=scope->owner->storage_->state;if(s.world_generation!=1||s.native_revision!=19||!s.source_clear_seed)return reject("first actor world generation required");
                next->actors[slot]=physical.constructors[slot];
            }
            std::array<bool,384> seen{};std::array<unsigned,10> counts{};
            for(unsigned visit=0;visit<8;++visit){
                const unsigned group=48+visit;
                if(visit>0&&visit<7&&!physical.actors.slots[visit-1].active00){
                    if(arena.state.group_heads[group].present()||arena.state.group_tails[group].present())return reject("occupied inactive-actor collision group unsupported");
                    continue;
                }
                auto link=arena.state.group_heads[group];ScenePhysicalLink previous;
                next->heads[visit]=link;next->tails[visit]=arena.state.group_tails[group];
                while(link.present()){
                    if(link.physical!=4||link.index<0||unsigned(link.index)>=384||seen[unsigned(link.index)])return reject("source collision group topology/cycle");
                    seen[unsigned(link.index)]=true;const auto& r=arena.collision.records[unsigned(link.index)];
                    if(!r.allocated||r.active00!=1||r.group01!=s8(group)||r.update0c_key!=0x8008a65c||r.free_next4!=previous||arena.collision_initial[unsigned(link.index)])return reject("fresh allocated source collider/header required");
                    BattleFirstColliderObservation observation;ColliderPlaneLifecycleOwnerFields owner;owner.lifecycle_declared=true;owner.registration.declared=owner.registration.present=true;owner.registration.active00=1;
                    CollisionRegistrationPeerFields peer;peer.declared=peer.present=true;ByteView bytes;
                    if(const auto& f=arena.collision_fields[unsigned(link.index)]){
                        const auto scope=f->parent20.scope_.lock();const auto* actor=scope?scope->owner:nullptr;
                        if(!scope||!scope->live||!actor||!actor->current(owners)||!actor->storage_->first_update)return reject("current collider actor relationship");
                        const auto& s=actor->storage_->state;const auto slot=s.slot3e4;
                        if(slot>=6||visit!=unsigned(slot)+1||next->actors[slot]!=f->parent20||f->parent24!=f->parent20||f->matrix_owner60!=f->parent20)return reject("source actor-group/matrix correspondence");
                        for(const auto& a:actor->storage_->collision_attempts)if(a.result==SceneObjectAcquire::Allocated&&a.object.index()==link.index)observation.lease=a.object;
                        if(!observation.lease.current())return reject("same actual actor collider lease");
                        bytes=actor->collision_descriptor(observation.lease,owners);observation.parent={0,slot};
                        const auto& world=actor->storage_->first_update->world.geometry;
                        if(f->matrix_kind==SceneActorMatrixSlotKind::Body1aa4)observation.source_world=world.collider1aa4;
                        else{if(f->bone>=world.world.matrices.size())return reject("logical actor collider matrix");observation.source_world=world.world.matrices[f->bone];}
                        observation.source_c0=s.scalar_defaults.fieldc0;owner.retire1d8=s.field1d8;owner.freeze281=s.collider_freeze281;owner.registration.active00=s.active00;
                        owner.registration.kind83=s.kind83;owner.registration.active76c=s.visible76c;owner.registration.timer720=s.invulnerability720;owner.registration.suppressed272=s.program_seed.flags272;owner.registration.disabled1eb=s.program_seed.flags[11];owner.registration.disabled276=s.program_seed.flags[12];
                        peer.kind83=s.kind83;peer.timer720=s.invulnerability720;
                    }else if(const auto& f=arena.stage_collision_fields[unsigned(link.index)]){
                        if(visit||!f->parent20.live()||f->parent20!=f->parent24||f->parent20!=f->matrix_owner60)return reject("same current stage parent/group required");
                        const auto ref=storage_->stage_construction.object_ref(f->parent20.identity());if(!ref)return reject("registered stage object identity");
                        for(const auto& a:storage_->stage_construction.storage_->collisions)if(a.result==SceneObjectAcquire::Allocated&&a.lease.index()==link.index)observation.lease=a.lease;
                        if(!observation.lease.current())return reject("same actual stage collider lease");
                        bytes=f->descriptor();observation.parent=f->parent20.identity();
                        auto observe=[&](const auto& s){
                            if(!s.world114||!s.constructor_returned||!s.self8c.live()||s.self8c!=f->parent20)return false;
                            observation.source_world=*s.world114;observation.source_c0=s.scale_c0;owner.retire1d8=s.field1d8;owner.freeze281=s.enabled281;
                            owner.registration.suppressed272=s.suppressed272;owner.registration.disabled1eb=s.disabled1eb;owner.registration.disabled276=s.disabled276;return true;
                        };
                        if(!(ref->side?observe(*ref->side):observe(*ref->prop)))return reject("current evaluated stage collider matrix");
                        const auto& parent=arena.class3.records[unsigned(observation.parent.index)];
                        owner.registration.active00=parent.active00;owner.registration.kind83=parent.kind83;peer.kind83=owner.registration.kind83;
                        if(owner.registration.kind83!=0x16)return reject("source stage parent kind changed");
                    }else return reject("unknown/view collider consumer before first camera");
                    if(!character_collision_descriptor(bytes,&observation.descriptor,error)||!collider_initial_plan(profile,observation.descriptor,owner,peer,observation.source_world,observation.source_c0,&observation.initialized,error))return false;
                    for(unsigned list=0;list<10;++list)if(observation.initialized.membership.lists&(1u<<list))if(++counts[list]>384)return reject("source registry capacity");
                    next->order.push_back(std::move(observation));previous=link;link=r.group_next8;
                }
                if(previous!=arena.state.group_tails[group])return reject("source collision group tail");
            }
            unsigned expected=0;for(unsigned i=0;i<384;++i)if(arena.collision_fields[i]||arena.stage_collision_fields[i]||arena.view_collision_fields[i]){++expected;if(!seen[i])return reject("bound collision body omitted from source sweep");}
            if(!expected||next->order.size()!=expected)return reject("complete bound collider enumeration required");
            for(unsigned list=0;list<10;++list)next->registries[list].reserve(counts[list]);
            storage_->collisions=std::move(next);auto& committed=*storage_->collisions;
            physical.scheduler.registry_counts.fill(0); //8A28C; no reference-array tail clearing
            for(const auto& e:committed.order){
                arena.collision_initial[e.lease.index()]=e.initialized;
                for(unsigned list=0;list<10;++list)if(e.initialized.membership.lists&(1u<<list)){
                    committed.registries[list].push_back(e.lease);++physical.scheduler.registry_counts[9-list];
                }
            }
            committed.complete=true;if(storage_->initial_post_world)storage_->initial_post_world->phase=BattleInitialPostWorldPhase::CollidersInitialized;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain initialization; consumed prefix retained");}
    }
    std::span<const BattleFirstColliderObservation> first_scene_colliders(const std::array<const ForceInventory*,2>& owners)const{
        return collision_initialization_current(owners)?std::span<const BattleFirstColliderObservation>{storage_->collisions->order}:std::span<const BattleFirstColliderObservation>{};
    }
    std::optional<std::span<const SceneCollisionLease>> first_collision_registry(CollisionRegistrationList list,const std::array<const ForceInventory*,2>& owners)const{
        if(unsigned(list)>=10||!collision_initialization_current(owners))return std::nullopt;
        return std::span<const SceneCollisionLease>{storage_->collisions->registries[unsigned(list)]};
    }
    //2BB14 actor prelude,2CDA4 resets, then2CF80's first sphere-v-plane pass.
    //An overlapping broadphase refuses: no unimplemented narrow phase is
    //silently treated as a miss. Completion stops BEFORE2D35C, not at camera.
    bool advance_first_contact_prefix(const BattleContactPrefixProfile& profile,const std::array<BattleActorConstruction*,6>& mutable_actors,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("first contact prefix: ")+why;return false;};
        if(!profile.ready()||!collision_initialization_current(owners)||storage_->contact_prefix||storage_->publication||storage_->views.pose_generation())return reject("fresh current first-collider sweep before camera required");
        if(storage_->initial_world_dispatch){const auto* caller=initial_post_world(owners);if(!caller||caller->phase!=BattleInitialPostWorldPhase::CollidersInitialized)return reject("typed post-world caller must own the actual collider sweep");}
        try{
            auto next=std::make_unique<BattleContactPrefixState>();next->team_remaining=storage_->services.team_remaining118;
            struct ActorReset {BattleActorConstruction* actor;CollisionContactState state;BattleActorContactPrelude prelude;};
            std::vector<ActorReset> actors;actors.reserve(6);
            for(unsigned slot=0;slot<6;++slot){
                const auto& r=storage_->physical.storage_->actors.slots[slot];if(!r.active00){if(mutable_actors[slot])return reject("inactive actor correspondence");continue;}
                const auto scope=storage_->physical.storage_->constructors[slot].scope_.lock();if(!scope||!scope->owner)return reject("registered mutable actor required");
                auto* actor=mutable_actors[slot];if(!actor||scope->owner!=actor)return reject("exact mutable actor correspondence required");const auto& s=actor->storage_->state;
                if(s.contact_generation||s.slot3e4!=slot||s.world_generation!=1||s.native_revision!=19)return reject("unconsumed first actor contact state");
                CollisionContactState reset;if(!collision_contact_reset(actor_contact_state(s),&reset,error))return false;
                const auto prelude=battle_actor_contact_prelude_step(s.contact_prelude);
                next->any436|=bool(prelude.flag436);next->health_before[slot]=s.shorts1c4_1c6_1c8[1];
                next->cleared_bytes[slot]=std::array<u8,4>{};next->cleared_words[slot]=0;next->cleared_halves[slot]=0;
                next->zero_scalars[slot]=0;next->sphere_bottom[slot]=-10000;
                actors.push_back({actor,reset,prelude});next->reset_order.push_back({BattleContactResetOwnerKind::PhysicalObject,{0,s32(slot)},255});
            }
            auto& stage=storage_->stage_construction;auto& arena=*storage_->physical.class2_pool()->storage_;
            struct ObjectReset {BattleStageSideObject* side=nullptr;BattleStagePropObject* prop=nullptr;CollisionContactState state;};
            std::vector<ObjectReset> objects;objects.reserve(56);
            const std::array<unsigned,6> groups{40,32,0,16,8,24};
            for(unsigned group:groups){
                //2CC78 runs base, six active actor lists, then base+7.
                for(unsigned offset=0;offset<8;++offset){
                    if(offset>=1&&offset<=6&&!storage_->physical.storage_->actors.slots[offset-1].active00){
                        if(arena.state.group_heads[group+offset].present()||arena.state.group_tails[group+offset].present())return reject("occupied inactive actor contact group");
                        continue;
                    }
                    auto link=arena.state.group_heads[group+offset];ScenePhysicalLink previous;unsigned visits=0;
                    while(link.present()){
                        if(++visits>1536)return reject("contact group cycle/budget");
                        const auto ref=stage.object_ref(link);
                        if(!ref){if(group==40)return reject("unknown occupied stage contact object");break;}
                        if(group!=40||offset)return reject("stage object moved outside source group40");
                        ScenePhysicalLink before,following;bool active=false;
                        if(link.physical==2){const auto& r=arena.state.records[unsigned(link.index)];active=r.allocated&&r.active00==1;before=r.link4;following=r.link8;}
                        else{const auto& r=arena.class3.records[unsigned(link.index)];active=r.allocated&&r.active00==1;before=r.free_next4;following=r.group_next8;}
                        if(!active||before!=previous)return reject("contact group current header/links");
                        const auto collision=ref->side?ref->side->collision86:ref->prop->collision86;
                        if(collision){
                            const auto generation=ref->side?ref->side->contact_generation:ref->prop->contact_generation;if(generation)return reject("object contact reset already consumed");
                            const auto& before_state=ref->side?ref->side->contact:ref->prop->contact;CollisionContactState reset;
                            if(!collision_contact_reset(before_state,&reset,error))return false;
                            objects.push_back({ref->side,ref->prop,reset});next->reset_order.push_back({BattleContactResetOwnerKind::PhysicalObject,link,255});
                        }
                        previous=link;link=following;
                    }
                    //Non-stage groups contain actual UI/child objects. Their
                    //collision86 is presently unrepresented; require a proven
                    //constructor-owned no-collision field, not an empty list.
                    if(group!=40){
                        auto cursor=arena.state.group_heads[group+offset];unsigned budget=0;
                        while(cursor.present()){
                            if(++budget>512||cursor.physical!=2||unsigned(cursor.index)>=arena.state.records.size())return reject("unsupported occupied contact-group class/topology");
                            const auto& r=arena.state.records[unsigned(cursor.index)];
                            if(!r.allocated||r.group01!=s16(group+offset))return reject("non-stage contact group header");
                            if(r.active00&&(!r.header_installed||!r.actor_child))return reject("unjoined collision flag for occupied non-stage object");
                            //Actor-child constructors admitted here never call
                            //8AB30/8AE10; no stage/collider parent may alias them.
                            for(const auto& body:storage_->collisions->order)if(body.parent==cursor)return reject("unexpected collider parent in actor-child group");
                            cursor=r.link8;
                        }
                    }else if(previous!=arena.state.group_tails[group+offset])return reject("contact group tail");
                }
            }
            std::array<CollisionContactState,12> view_reset;
            for(unsigned i=0;i<12;++i){auto& v=storage_->views.storage_->views[i];if(v.contact_generation||v.active00||v.initialized2e4)return reject("source twelve inactive pre-camera views required");auto before=v.contact;before.field1da=v.contact1da;if(!collision_contact_reset(before,&view_reset[i],error))return false;next->reset_order.push_back({BattleContactResetOwnerKind::View,{},u8(i)});}
            auto find=[&](const SceneCollisionLease& lease)->const BattleFirstColliderObservation*{for(const auto& body:storage_->collisions->order)if(body.lease.index()==lease.index()&&body.lease.generation()==lease.generation())return &body;return nullptr;};
            const auto& spheres=storage_->collisions->registries[unsigned(CollisionRegistrationList::L297C)];
            const auto& planes=storage_->collisions->registries[unsigned(CollisionRegistrationList::L237C)];
            next->sphere_pairs.reserve(spheres.size()*planes.size());
            for(const auto& lease:spheres){
                const auto* a=find(lease);if(!a||a->descriptor.shape!=0)return reject("source first297C sphere shape required");
                if(a->parent.physical==0)next->sphere_bottom[unsigned(a->parent.index)]=f32(a->initialized.fields.center64[1]-a->initialized.fields.sphere.radius88);
                for(const auto& other:planes){
                    const auto* b=find(other);if(!b)return reject("current source237C body");
                    if(a->parent==b->parent)continue;
                    BattleContactPairObservation pair;pair.first=lease.index();pair.second=other.index();
                    //2CDA4 reset1DD, so3A05C cannot offset either center.
                    if(!collision_contact_broadphase(a->initialized.fields.center64,a->initialized.fields.broad_radius7c,b->initialized.fields.center64,b->initialized.fields.broad_radius7c,bits_to_f32(0x3dcccccdu),&pair.broadphase,error))return false;
                    if(pair.broadphase.overlap)return reject("3A05C/30348/303B0 narrow-phase response required");
                    next->sphere_pairs.push_back(pair);
                }
            }
            //Preallocate journals before the first source field store.
            for(auto& p:actors)p.actor->storage_->journal.reserve(p.actor->storage_->journal.size()+64);
            for(auto& p:actors){
                auto& s=p.actor->storage_->state;const auto old_contact=actor_contact_state(s);const auto old_prelude=s.contact_prelude;
                commit_actor_contact(s,p.state);s.contact_prelude=p.prelude;++s.contact_generation;
                auto& journal=p.actor->storage_->journal;using K=BattleActorBirthWriteKind;
                journal.push_back({K::Byte,0x7d1,1,p.prelude.timer7d1});if(std::bit_cast<s8>(old_prelude.timer7d1)<1)journal.push_back({K::Byte,0x7d0,1,std::bit_cast<u8>(p.prelude.target7d0)});
                journal.push_back({K::SourceBindingStage,0,0,0x8002cda4});
                for(u16 at:{u16(0x1cc),u16(0x1d0),u16(0x1d4)})journal.push_back({K::FloatBits,at,4,0});
                for(auto [at,value]:std::array<std::pair<u16,u8>,10>{{{0x1d9,0},{0x1da,0},{0x1de,p.state.previous1de},{0x1db,0},{0x1dc,0},{0x1dd,0},{0x280,0},{0x275,p.state.retained.previous275},{0x274,0},{0x1df,p.state.retained.timer1df}}}){
                    if(at!=0x1df||old_contact.retained.timer1df)journal.push_back({K::Byte,at,1,value});
                }
                journal.push_back({K::Half,0x270,2,0});journal.push_back({K::Half,0x286,2,0});
                for(auto [at,value]:std::array<std::pair<u16,u8>,3>{{{0x2ae,p.state.retained.timer2ae},{0x2af,p.state.retained.timer2af},{0x1e8,p.state.retained.timer1e8}}}){
                    const auto old=at==0x2ae?old_contact.retained.timer2ae:at==0x2af?old_contact.retained.timer2af:old_contact.retained.timer1e8;if(old)journal.push_back({K::Byte,at,1,value});
                }
                for(unsigned i=0;i<2;++i){
                    if(old_prelude.limits4fc_4fe[i]<=old_prelude.values4f4_4f6[i])journal.push_back({K::Half,u16(0x4fc+2*i),2,p.prelude.limits4fc_4fe[i]});
                    if(old_prelude.timers4f8_4fa[i])journal.push_back({K::Half,u16(0x4f8+2*i),2,std::bit_cast<u16>(p.prelude.timers4f8_4fa[i])});
                    else journal.push_back({K::Half,u16(0x4f4+2*i),2,p.prelude.values4f4_4f6[i]});
                }
                for(u16 at:{u16(0x6fd),u16(0x70c),u16(0x70d),u16(0x7d4),u16(0x599)})journal.push_back({K::Byte,at,1,0});
            }
            for(auto& p:objects){if(p.side){p.side->contact=p.state;++p.side->contact_generation;}else{p.prop->contact=p.state;++p.prop->contact_generation;}}
            for(unsigned i=0;i<12;++i){auto& v=storage_->views.storage_->views[i];v.contact=view_reset[i];v.contact1da=v.contact.field1da;++v.contact_generation;}
            storage_->contact_groups=arena.state.group_heads;storage_->contact_group_tails=arena.state.group_tails;
            storage_->contact_prefix=std::move(next);if(storage_->initial_post_world)storage_->initial_post_world->phase=BattleInitialPostWorldPhase::ContactPrefix;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain contact prefix; no replay");}
    }
    const BattleContactPrefixState* first_contact_prefix(const std::array<const ForceInventory*,2>& owners)const{
        if(!collision_initialization_current(owners)||!storage_->contact_prefix)return nullptr;
        const auto& arena=*storage_->physical.class2_pool()->storage_;
        if(arena.state.group_heads!=storage_->contact_groups||arena.state.group_tails!=storage_->contact_group_tails)return nullptr;
        return storage_->contact_prefix.get();
    }
    bool finish_first_contact_pass(const BattleContactTailProfile& profile,const std::array<BattleActorConstruction*,6>& mutable_actors,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("first contact tail: ")+why;return false;};
        const auto* prefix=first_contact_prefix(owners);
        if(!profile.ready()||!prefix||prefix->contact_pass_complete||prefix->next_source!=0x8002d35c||storage_->views.pose_generation())return reject("current unconsumed source reset/sphere-pass prefix required");
        if(storage_->initial_world_dispatch){const auto* caller=initial_post_world(owners);if(!caller||caller->phase!=BattleInitialPostWorldPhase::ContactPrefix)return reject("same typed post-world contact prefix required");}
        const auto& collisions=*storage_->collisions;auto& stage=storage_->stage_construction;
        auto find=[&](const SceneCollisionLease& lease)->const BattleFirstColliderObservation*{for(const auto& body:collisions.order)if(body.lease.index()==lease.index()&&body.lease.generation()==lease.generation())return &body;return nullptr;};
        //2D58C/2E1C8: empty357C.2D7C4/2DB58: empty477C.2DF08: empty177C.
        for(auto list:{CollisionRegistrationList::L357C,CollisionRegistrationList::L477C,CollisionRegistrationList::L177C})if(!collisions.registries[unsigned(list)].empty())return reject("nonempty response/attack registry requires its source consumers");
        u16 flagged=0,frozen=0;
        const auto& solids=collisions.registries[unsigned(CollisionRegistrationList::L3B7C)];
        //2D35C never uses the last body as an outer-loop candidate.
        for(unsigned i=0;i+1<solids.size();++i){
            const auto* body=find(solids[i]);if(!body)return reject("current source3B7C body required");
            if(body->descriptor.flags2&0x800){++flagged;continue;}
            if(body->parent.physical!=3)return reject("unjoined movable solid pair");
            const auto ref=stage.object_ref(body->parent);if(!ref)return reject("current static parent required");
            const u8 freeze=ref->side?ref->side->enabled281:ref->prop->enabled281;
            const auto& state=ref->side?ref->side->contact:ref->prop->contact;
            const auto generation=ref->side?ref->side->contact_generation:ref->prop->contact_generation;
            const auto* bank=storage_->physical.bank(3);const auto kind=bank->records[unsigned(body->parent.index)].kind83;
            bool skip=false;if(!battle_contact_static_pair_gate(body->descriptor.flags2,freeze,kind,&skip)||!skip||generation!=1||state.moved1dd||state.attached1db)return reject("solid contact narrow-phase response required");
            ++frozen;
        }
        const auto* seed=storage_->physical.stage_seed();if(!seed)return reject("same current mutable stage bounds required");
        unsigned actor_count=0;
        for(unsigned slot=0;slot<6;++slot){
            const bool active=storage_->physical.storage_->actors.slots[slot].active00;
            if(!active){if(mutable_actors[slot])return reject("inactive actor correspondence");continue;}
            const auto scope=collisions.actors[slot].scope_.lock();auto* actor=mutable_actors[slot];
            if(!scope||!actor||scope->owner!=actor||!actor->current(owners))return reject("exact mutable current actor required");
            const auto& s=actor->storage_->state;if(s.contact_generation!=1||s.native_revision!=19||s.world_generation!=1)return reject("initial contact generation required");
            BattleContactTailActorInput in;in.contact=actor_contact_state(s);in.position=s.position20;in.health=s.shorts1c4_1c6_1c8[1];
            const auto& p=s.contact_prelude;in.forced663=p.forced663;in.peer_mask71e=p.peer_mask71e;in.peer_mask71f=p.peer_mask71f;in.disabled4a1=p.disabled4a1;
            if(!battle_contact_tail_actor_gates(profile,in,seed->bounds38_44,error))return false;
            if(prefix->health_before[slot]!=s.shorts1c4_1c6_1c8[1])return reject("health-delta/team accounting response required");
            ++actor_count;
        }
        //2D198 sees1DD=0 for297C owners.2CE44 must also prove every collider's
        //owner1E8/1DD and actor ground-correction scratch remain unchanged.
        for(const auto& body:collisions.order){
            CollisionContactState state;
            if(body.parent.physical==0){const auto* actor=mutable_actors[unsigned(body.parent.index)];if(!actor)return reject("actor collider owner");state=actor_contact_state(actor->storage_->state);}
            else{const auto ref=stage.object_ref(body.parent);if(!ref)return reject("stage collider owner");state=ref->side?ref->side->contact:ref->prop->contact;}
            if(state.moved1dd||state.attached1db||state.current274||state.retained.timer1e8||state.retained.displacement1cc!=std::array<f32,3>{})return reject("collider center/ground correction continuation required");
        }
        for(s16 delta:prefix->team_delta)if(delta)return reject("nonzero team accounting");
        try{
            for(auto* actor:mutable_actors)if(actor)actor->storage_->journal.reserve(actor->storage_->journal.size()+1);
            for(auto* actor:mutable_actors)if(actor){actor->storage_->state.contact_generation=2;actor->storage_->journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,0x8005a580});}
            auto& completed=*storage_->contact_prefix;completed.flagged_static_candidates=flagged;completed.frozen_static_candidates=frozen;completed.actor_tail_returns=u8(actor_count);completed.contact_pass_complete=true;completed.next_source=0x80036e24;
            if(storage_->initial_post_world)storage_->initial_post_world->phase=BattleInitialPostWorldPhase::AwaitVisibility;
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain tail continuation; no replay");}
    }
    //36E24 has no active views before the first0BDA4. Still perform its13
    //26F clears and prove the two following callback dispatches are empty.
    bool finish_first_pre_camera_visibility(const BattleContactTailProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("first pre-camera visibility: ")+why;return false;};
        const auto* prefix=first_contact_prefix(owners);
        if(!profile.ready()||!prefix||!prefix->contact_pass_complete||prefix->pre_camera_visibility_complete||prefix->next_source!=0x80036e24||storage_->views.pose_generation())return reject("completed initial contact pass/current uninitialized views required");
        if(storage_->initial_world_dispatch){const auto* caller=initial_post_world(owners);if(!caller||caller->phase!=BattleInitialPostWorldPhase::AwaitVisibility)return reject("same typed post-world caller must precede visibility");}
        for(unsigned i=0;i<13;++i){const auto& v=storage_->views.storage_->views[i];if(v.active00||v.initialized2e4||v.pose_generation)return reject("active camera needs actual segment/shape visibility consumers");}
        const auto& scheduler=*storage_->physical.scheduler();
        for(unsigned i=2;i<6;++i)if(scheduler.callback_heads[i].present()||scheduler.callback_tails[i].present())return reject("occupied8972C/89658 callback lists unsupported");
        for(auto& v:storage_->views.storage_->views)v.visible26f=0;
        auto& completed=*storage_->contact_prefix;completed.pre_camera_visibility_complete=true;completed.cleared_views=13;completed.next_source=0x8000bda4;
        if(error){error->clear();}return true;
    }
    const BattleInitialUiWorldState* initial_ui_worlds(const std::array<const ForceInventory*,2>& owners)const{
        const auto* caller=initial_post_world(owners);
        return caller&&(caller->phase==BattleInitialPostWorldPhase::UiWorlds||caller->phase==BattleInitialPostWorldPhase::AwaitActorLate)&&first_contact_prefix(owners)&&storage_->initial_ui_worlds?storage_->initial_ui_worlds.get():nullptr;
    }
    bool begin_initial_ui_worlds(const BattleInitialUiWorldProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="initial UI worlds: actual post-contact/inactive-view/empty8972C boundary required";return false;};
        const auto* caller=initial_post_world(owners);const auto* contact=first_contact_prefix(owners);
        if(!profile.ready()||!caller||caller->phase!=BattleInitialPostWorldPhase::AwaitVisibility||!contact||!contact->contact_pass_complete||contact->pre_camera_visibility_complete||contact->next_source!=0x80036e24||storage_->initial_ui_worlds||storage_->publication||storage_->views.pose_generation())return reject();
        const auto& scheduler=*storage_->physical.scheduler();
        for(unsigned list:{2u,3u})if(scheduler.callback_heads[list].present()||scheduler.callback_tails[list].present())return reject();
        for(unsigned list:{4u,5u})if(!scheduler.callback_heads[list].present()||!scheduler.callback_tails[list].present())return reject();
        for(const auto& view:storage_->views.storage_->views)if(view.active00||view.initialized2e4||view.pose_generation)return reject();
        try{
            auto next=std::make_unique<BattleInitialUiWorldState>();next->visits.reserve(64);next->head=next->next=scheduler.callback_heads[4];next->cleared_views=13;next->repeated_lists_empty=true;
            for(auto& view:storage_->views.storage_->views)view.visible26f=0;
            storage_->contact_prefix->cleared_views=13;storage_->contact_prefix->next_source=0x80089658;
            storage_->initial_post_world->phase=BattleInitialPostWorldPhase::UiWorlds;storage_->initial_ui_worlds=std::move(next);
            if(error){error->clear();}return true;
        }catch(const std::exception&){return reject();}
    }
    bool advance_initial_ui_world_callback(const BattleInitialUiWorldProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="initial UI callback: actual retained source list/owner/typed callback required";return false;};
        const auto* cursor=initial_ui_worlds(owners);
        if(!profile.ready()||!cursor||cursor->traversal_exhausted||cursor->complete||storage_->views.pose_generation()||(cursor->source_list!=4&&cursor->source_list!=5))return reject();
        auto objects=storage_->physical.class2_pool();auto& arena=*objects->storage_;const auto& scheduler=*storage_->physical.scheduler();const auto link=cursor->next;
        if(scheduler.callback_heads[cursor->source_list]!=cursor->head||link.physical!=2||link.index<0||unsigned(link.index)>=arena.state.records.size())return reject();
        const auto& r=arena.state.records[unsigned(link.index)];
        if(!r.allocated||r.active00!=1||r.callback_previous104!=cursor->previous||r.world_callback||r.registration85!=0x20+cursor->source_list-4||(!r.callback_next108.present()&&scheduler.callback_tails[cursor->source_list]!=link))return reject();
        for(const auto& visit:cursor->visits)if(visit.object==link)return reject();
        AttachmentBoneTransform transform;SceneObjectLease lease;
        if(!prepare_initial_ui_callback(profile,link,cursor->source_list,owners,&transform,&lease,error))return false;
        const auto next_link=r.callback_next108;const u32 key=r.callback100_key;const bool matrix=cursor->source_list==4;
        try{
            auto next=*cursor;next.previous=link;next.next=next_link;++next.callbacks;if(matrix)++next.matrices;next.visits.push_back({link,cursor->source_list,key,matrix});
            if(!next.next.present()){
                if(next.source_list==4){
                    next.source_list=5;next.previous={};next.head=next.next=scheduler.callback_heads[5];
                    if(!next.next.present()||!scheduler.callback_tails[5].present())return reject();
                }else next.traversal_exhausted=true;
            }
            if(matrix)arena.state.records[lease.index()].world_callback=SceneObjectWorldCallbackState{transform.world,transform.position,1,0};
            *storage_->initial_ui_worlds=std::move(next);
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject();}
    }
    const SceneObjectWorldCallbackState* initial_ui_object_world(const SceneObjectLease& lease,const std::array<const ForceInventory*,2>& owners)const{
        const auto* cursor=initial_ui_worlds(owners);if(!cursor)return nullptr;
        const auto* r=storage_->physical.class2_pool()->inspect(lease);if(!r||!r->world_callback)return nullptr;
        for(const auto& visit:cursor->visits)if(visit.matrix_written&&visit.object==ScenePhysicalLink{2,s32(lease.index())})return &*r->world_callback;
        return nullptr;
    }
    bool finish_initial_ui_worlds(const BattleInitialUiWorldProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="initial UI end: unchanged complete source4/5 traversal and typed object worlds required";return false;};
        const auto* cursor=initial_ui_worlds(owners);
        if(!profile.ready()||!cursor||!cursor->traversal_exhausted||cursor->complete||cursor->next.present()||cursor->source_list!=5||cursor->callbacks!=46||cursor->matrices!=38||storage_->views.pose_generation())return reject();
        const auto& scheduler=*storage_->physical.scheduler();auto objects=storage_->physical.class2_pool();std::size_t ordinal=0;unsigned matrices=0;
        for(unsigned list:{2u,3u})if(scheduler.callback_heads[list].present()||scheduler.callback_tails[list].present())return reject();
        for(const auto& view:storage_->views.storage_->views)if(view.active00||view.initialized2e4||view.pose_generation)return reject();
        for(unsigned list:{4u,5u}){
            ScenePhysicalLink previous;auto link=scheduler.callback_heads[list];
            while(link.present()){
                if(ordinal>=cursor->visits.size()||link.physical!=2||link.index<0||unsigned(link.index)>=objects->snapshot()->records.size())return reject();
                const auto& visit=cursor->visits[ordinal++];const auto& r=objects->snapshot()->records[unsigned(link.index)];
                if(!r.allocated||r.active00!=1||r.callback_previous104!=previous||r.registration85!=0x20+list-4||visit.source_list!=list||visit.object!=link||visit.callback!=r.callback100_key||visit.matrix_written!=(list==4))return reject();
                AttachmentBoneTransform expected;SceneObjectLease lease;
                if(!prepare_initial_ui_callback(profile,link,u8(list),owners,&expected,&lease,error))return false;
                if(list==4){if(!r.world_callback||r.world_callback->generation!=1||r.world_callback->parent_world_generation||r.world_callback->world114!=expected.world||r.world_callback->position20!=expected.position)return reject();++matrices;}
                else if(r.world_callback)return reject();
                previous=link;link=r.callback_next108;
            }
            if(previous!=scheduler.callback_tails[list])return reject();
        }
        if(ordinal!=cursor->visits.size()||ordinal!=46||matrices!=38)return reject();
        storage_->initial_ui_worlds->complete=true;storage_->contact_prefix->pre_camera_visibility_complete=true;storage_->contact_prefix->next_source=0x80053a94;
        storage_->initial_post_world->phase=BattleInitialPostWorldPhase::AwaitActorLate;
        //UI worlds complete, but53A94 and group3B still precede the first camera.
        if(error){error->clear();}return true;
    }
private:
    bool prepare_initial_ui_callback(const BattleInitialUiWorldProfile& profile,ScenePhysicalLink link,u8 list,const std::array<const ForceInventory*,2>& owners,AttachmentBoneTransform* out,SceneObjectLease* lease,std::string* error)const{
        auto reject=[&](){if(error)*error="initial UI callback input: actual initialized source widget/HUD owner required";return false;};
        if(!profile.ready()||!out||!lease||link.physical!=2||link.index<0)return reject();
        auto objects=storage_->physical.class2_pool();if(unsigned(link.index)>=objects->snapshot()->records.size())return reject();
        const auto& r=objects->snapshot()->records[unsigned(link.index)];
        if(list==5){
            for(unsigned i=0;i<8;++i)if(storage_->initial_hud_objects[i].index()==unsigned(link.index)){
                const auto scope=storage_->physical.storage_->constructors[3-i/2].scope_.lock();if(!scope||!scope->owner)return reject();
                *lease=storage_->initial_hud_objects[i];*out={};
                return scope->owner->qualify_initial_hud_world(profile,*lease,!(i&1u),owners,error);
            }
            return reject();
        }
        const auto widgets=storage_->widgets.storage_;if(list!=4||!widgets||!widgets->current()||widgets->leases.size()!=58||r.actor_child||r.group01!=57||r.kind83!=0x15||r.registration85!=0x20||r.callback100_key!=0x80047aa4)return reject();
        unsigned ordinal=58;for(unsigned i=0;i<58;++i)if(widgets->leases[i].index()==unsigned(link.index)){ordinal=i;*lease=widgets->leases[i];break;}
        if(ordinal==58||!r.header_installed||r.header.kind10||r.header.update0c_key!=0x80199c0c||r.header.draw10c_key!=widgets->attempts[ordinal].header.draw10c_key)return reject();
        AttachmentUiLocalTransformInput input;u8 type=0;
        if(ordinal==2){
            const auto* s=storage_->widgets.type1_widget_state();if(!s||!s->initialized||s->generation!=1||s->phase18!=1||s->flags84!=1)return reject();
            type=1;input.flags84=s->flags84;input.local64=s->target64_6c;
        }else if(ordinal>=3&&ordinal<21){
            const auto* s=storage_->widgets.type2_widget_state(ordinal-3);if(!s||!s->initialized||!s->active||s->generation!=1||s->phase18!=1||s->flags84!=1)return reject();
            type=2;input.flags84=s->flags84;input.local64=s->target64_6c;input.scale58=s->values58_60;
        }else if(ordinal>=21&&ordinal<57){
            const auto* s=storage_->widgets.type5_widget_state(ordinal-21);if(!s||!s->initialized||!s->active||s->generation!=1||s->phase18!=1||s->flags84!=0x11||r.header.argument12!=s->page12)return reject();
            type=5;input.flags84=s->flags84;input.local64=s->target64_6c;input.scale58=s->values58_60;
        }else if(ordinal==57){
            const auto* s=storage_->widgets.type6_widget_state();if(!s||!s->initialized||s->generation!=1||s->phase18!=1||s->flags84!=0x19||r.header.argument12)return reject();
            type=6;input.flags84=s->flags84;input.local64=s->target64_6c;input.scale58=s->values58_60;input.angle74=s->angle74; //actual angle74, never rate80
        }else return reject();
        if(r.header.type11!=type)return reject();
        return attachment_ui_local_transform(input,out,error);
    }
public:
    bool advance_spawn_collider_sweep(const ColliderSpawnSweepProfile& recipe,const ColliderInitialShapeProfile& geometry,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("spawn collider sweep: ")+why;return false;};
        if(!recipe.ready()||!geometry.ready()||!initial_order_current(owners)||storage_->spawn_collisions||!storage_->collisions||!storage_->contact_prefix||!storage_->contact_prefix->pre_camera_visibility_complete||storage_->publication||storage_->views.pose_generation()!=1||storage_->stage_construction.object_world_generation()!=2||!storage_->stage_construction.first_object_updates_complete())return reject("complete first contacts/cameras and current spawn object worlds required");
        auto& physical=*storage_->physical.storage_;auto pool=physical.class2;auto& arena=*pool->storage_;auto& views=*storage_->views.storage_;auto& stage=storage_->stage_construction;
        for(unsigned slot=0;slot<6;++slot)if(physical.actors.slots[slot].active00){
            const auto scope=physical.constructors[slot].scope_.lock();
            if(!scope||!scope->owner||!scope->live||!scope->owner->current(owners)||scope->owner->status()!=BattleActorConstructionStatus::SpawnWorldReady)return reject("current registered spawn actor required");
            const auto& s=scope->owner->storage_->state;if(s.native_revision!=20||s.world_generation!=2||s.camera_generation!=1||s.contact_generation!=2)return reject("same first-spawn actor/camera/contact boundary");
        }
        try{
            auto next=std::make_unique<BattleSpawnColliderSweepState>();next->order.reserve(384);std::array<bool,384> seen{};std::array<unsigned,10> counts{};
            for(unsigned visit=0;visit<8;++visit){
                const unsigned group=48+visit;
                if(visit>0&&visit<7&&!physical.actors.slots[visit-1].active00){if(arena.state.group_heads[group].present()||arena.state.group_tails[group].present())return reject("unjoined inactive actor collider group");continue;}
                auto link=arena.state.group_heads[group];ScenePhysicalLink previous;
                while(link.present()){
                    if(link.physical!=4||unsigned(link.index)>=384||seen[unsigned(link.index)])return reject("current class4 group topology/cycle");
                    seen[unsigned(link.index)]=true;
                    const auto& record=arena.collision.records[unsigned(link.index)];
                    if(!record.allocated||record.active00!=1||record.group01!=s8(group)||record.free_next4!=previous||record.update0c_key!=0x8008a65c)return reject("source class4 header/previous-link correspondence");
                    BattleSpawnColliderObservation observation;ColliderPlaneLifecycleOwnerFields owner;owner.lifecycle_declared=true;owner.registration.declared=owner.registration.present=true;owner.registration.active00=1;
                    CollisionRegistrationPeerFields peer;peer.declared=peer.present=true;ByteView bytes;
                    if(const auto& f=arena.view_collision_fields[unsigned(link.index)]){
                        if(visit||!f->parent20.live()||f->parent20!=f->parent24||f->parent20!=f->matrix_owner60)return reject("same live view/body relationships required");
                        const auto id=f->parent20.index();if(id>=13)return reject("view record index");
                        auto& view=views.views[id];observation.view_parent=id;
                        for(const auto& attempt:views.collisions)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.lease.index()==link.index)observation.lease=attempt.lease;
                        if(!observation.lease.current()||view.field1d8)return reject("current original camera allocation/body required");
                        bytes=f->descriptor();observation.source_world=view.transform114;observation.source_c0=view.scale_c0;
                        owner.registration.active00=view.active00;owner.registration.kind83=view.kind83;peer.kind83=view.kind83;
                        if(!view.active00){
                            if(!view.group11||id<6||id>9||view.pose_generation||arena.collision_initial[unsigned(link.index)])return reject("only initial inactive alternate view retirement supported");
                            for(const auto& registry:storage_->collisions->registries)for(const auto& lease:registry)if(lease.index()==link.index)return reject("registered view body needs registry cleanup");
                            observation.action=BattleSpawnColliderAction::RetireInactiveView;++next->retired;
                        }else{
                            if(view.group11||id>=4||view.pose_generation!=1||arena.collision_initial[unsigned(link.index)])return reject("first normal camera collider initialization required");
                            if(!character_collision_descriptor(bytes,&observation.descriptor,error))return false;
                            ColliderInitialPlan plan;if(!collider_initial_plan(geometry,observation.descriptor,owner,peer,observation.source_world,observation.source_c0,&plan,error))return false;
                            observation.after=plan;observation.action=BattleSpawnColliderAction::Initialize;++next->initialized;
                        }
                    }else if(const auto& f=arena.collision_fields[unsigned(link.index)]){
                        const auto scope=f->parent20.scope_.lock();const auto* actor=scope?scope->owner:nullptr;
                        if(!scope||!scope->live||!actor||!actor->current(owners)||!actor->storage_->first_update)return reject("actor collider parent lifetime");
                        const auto& s=actor->storage_->state;const auto slot=s.slot3e4;
                        if(slot>=6||visit!=unsigned(slot)+1||physical.constructors[slot]!=f->parent20||f->parent24!=f->parent20||f->matrix_owner60!=f->parent20)return reject("actor collision group/matrix identity");
                        for(const auto& attempt:actor->storage_->collision_attempts)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.object.index()==link.index)observation.lease=attempt.object;
                        bytes=actor->collision_descriptor(observation.lease,owners);observation.object_parent={0,slot};
                        const auto& world=actor->storage_->first_update->world.geometry;
                        if(f->matrix_kind==SceneActorMatrixSlotKind::Body1aa4)observation.source_world=world.collider1aa4;
                        else{if(f->bone>=world.world.matrices.size())return reject("current logical actor matrix");observation.source_world=world.world.matrices[f->bone];}
                        observation.source_c0=s.scalar_defaults.fieldc0;owner.retire1d8=s.field1d8;owner.freeze281=s.collider_freeze281;
                        owner.registration.kind83=s.kind83;owner.registration.active00=s.active00;owner.registration.active76c=s.visible76c;owner.registration.timer720=s.invulnerability720;owner.registration.suppressed272=s.program_seed.flags272;owner.registration.disabled1eb=s.program_seed.flags[11];owner.registration.disabled276=s.program_seed.flags[12];peer.kind83=s.kind83;peer.timer720=s.invulnerability720;
                    }else if(const auto& f=arena.stage_collision_fields[unsigned(link.index)]){
                        if(visit||!f->parent20.live()||f->parent20!=f->parent24||f->parent20!=f->matrix_owner60)return reject("stage collider parent lifetime");
                        const auto ref=stage.object_ref(f->parent20.identity());if(!ref)return reject("same live stage collider parent");
                        for(const auto& attempt:stage.storage_->collisions)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.lease.index()==link.index)observation.lease=attempt.lease;
                        bytes=f->descriptor();observation.object_parent=f->parent20.identity();
                        auto fields=[&](const auto& s){if(!s.world114||s.self8c!=f->parent20||!s.self8c.live())return false;observation.source_world=*s.world114;observation.source_c0=s.scale_c0;owner.retire1d8=s.field1d8;owner.freeze281=s.enabled281;owner.registration.suppressed272=s.suppressed272;owner.registration.disabled1eb=s.disabled1eb;owner.registration.disabled276=s.disabled276;return true;};
                        if(!(ref->side?fields(*ref->side):fields(*ref->prop)))return reject("current stage world114");
                        const auto& parent=arena.class3.records[unsigned(observation.object_parent.index)];owner.registration.active00=parent.active00;owner.registration.kind83=parent.kind83;peer.kind83=parent.kind83;
                    }else return reject("unknown occupied collider in source sweep");
                    if(!observation.lease.current())return reject("same source allocation lease required");
                    if(!observation.view_parent){
                        if(!arena.collision_initial[unsigned(link.index)]||!character_collision_descriptor(bytes,&observation.descriptor,error))return reject("actual retained previous collider fields");
                        observation.before=*arena.collision_initial[unsigned(link.index)];ColliderInitialPlan plan;
                        if(!collider_spawn_step(recipe,geometry,observation.descriptor,*observation.before,owner,peer,observation.source_world,observation.source_c0,&plan,error))return false;
                        observation.after=plan;
                        if(observation.before->fields.phase10==2){observation.action=BattleSpawnColliderAction::Hold;++next->held;}else{observation.action=BattleSpawnColliderAction::Update;++next->updated;}
                    }
                    if(observation.after)for(unsigned list=0;list<10;++list)if(observation.after->membership.lists&(1u<<list))if(++counts[list]>384)return reject("source registry capacity");
                    next->order.push_back(std::move(observation));previous=link;link=record.group_next8;
                }
                if(previous!=arena.state.group_tails[group])return reject("source collider group tail");
            }
            unsigned expected=0;for(unsigned i=0;i<384;++i)if(arena.collision_fields[i]||arena.stage_collision_fields[i]||arena.view_collision_fields[i]){++expected;if(!seen[i])return reject("bound body omitted from actual source group sweep");}
            if(next->order.size()!=expected||!expected)return reject("complete occupied collider correspondence");
            for(unsigned list=0;list<10;++list)next->registries[list].reserve(counts[list]);
            storage_->spawn_collisions=std::move(next);auto& committed=*storage_->spawn_collisions;
            physical.scheduler.registry_counts.fill(0); //8A28C; preserve all non-registry scheduler state
            for(const auto& body:committed.order){
                if(body.action==BattleSpawnColliderAction::RetireInactiveView){
                    const auto parent=arena.view_collision_fields[body.lease.index()]->parent20;
                    if(!pool->retire_initial_view_collision(body.lease,parent,error)){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain source class4 retirement; no replay");}
                }else{
                    arena.collision_initial[body.lease.index()]=*body.after;
                    for(unsigned list=0;list<10;++list)if(body.after->membership.lists&(1u<<list)){committed.registries[list].push_back(body.lease);++physical.scheduler.registry_counts[9-list];}
                }
            }
            storage_->spawn_collision_heads=arena.state.group_heads;storage_->spawn_collision_tails=arena.state.group_tails;
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain spawn collider prefix; no replay");}
    }
    const BattleSpawnColliderSweepState* spawn_collider_sweep(const std::array<const ForceInventory*,2>& owners)const{
        if(!current(owners)||!storage_->spawn_collisions||storage_->stage_construction.object_world_generation()!=2||storage_->views.pose_generation()!=1)return nullptr;
        const auto& arena=*storage_->physical.class2_pool()->storage_;const auto& sweep=*storage_->spawn_collisions;
        if(arena.state.group_heads!=storage_->spawn_collision_heads||arena.state.group_tails!=storage_->spawn_collision_tails)return nullptr;
        for(const auto& body:sweep.order)if(body.action==BattleSpawnColliderAction::RetireInactiveView){if(body.lease.current())return nullptr;}else if(!body.lease.current()||!arena.collision_initial[body.lease.index()])return nullptr;
        for(unsigned slot=0;slot<6;++slot)if(storage_->physical.storage_->actors.slots[slot].active00){const auto scope=storage_->physical.storage_->constructors[slot].scope_.lock();if(!scope||!scope->live||!scope->owner||!scope->owner->current(owners)||scope->owner->storage_->state.world_generation!=2||scope->owner->storage_->state.native_revision!=20)return nullptr;}
        for(unsigned list=0;list<10;++list)if(storage_->physical.storage_->scheduler.registry_counts[9-list]!=sweep.registries[list].size())return nullptr;
        return &sweep;
    }
    bool finish_spawn_contact_pass(const BattleContactPrefixProfile& prefix_profile,const BattleContactTailProfile& tail_profile,const std::array<BattleActorConstruction*,6>& mutable_actors,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("spawn contact pass: ")+why;return false;};
        const auto* sweep=spawn_collider_sweep(owners);
        if(!prefix_profile.ready()||!tail_profile.ready()||!sweep||storage_->spawn_contacts||storage_->views.pose_generation()!=1)return reject("unconsumed current spawn collider sweep and source contact recipes required");
        try{
            auto next=std::make_unique<BattleSpawnContactState>();next->team_remaining=storage_->services.team_remaining118;
            struct ActorPending {BattleActorConstruction* actor;CollisionContactState reset;BattleActorContactPrelude prelude;};
            std::vector<ActorPending> actors;actors.reserve(6);const auto* seed=storage_->physical.stage_seed();
            if(!seed||!contact_non_stage_groups_current())return reject("unjoined occupied child contact group");
            for(unsigned slot=0;slot<6;++slot){
                const bool active=storage_->physical.storage_->actors.slots[slot].active00;
                if(!active){if(mutable_actors[slot])return reject("inactive mutable actor correspondence");continue;}
                const auto scope=storage_->physical.storage_->constructors[slot].scope_.lock();auto* actor=mutable_actors[slot];
                if(!scope||!actor||scope->owner!=actor||!actor->current(owners))return reject("exact current mutable spawn actor");
                const auto& s=actor->storage_->state;
                if(s.contact_generation!=2||s.native_revision!=20||s.world_generation!=2||s.camera_generation!=1)return reject("same unconsumed spawn actor boundary");
                CollisionContactState reset;if(!collision_contact_reset(actor_contact_state(s),&reset,error))return false;
                const auto prelude=battle_actor_contact_prelude_step(s.contact_prelude);
                next->any436|=bool(prelude.flag436);next->health_before[slot]=s.shorts1c4_1c6_1c8[1];
                next->cleared_bytes[slot]=std::array<u8,4>{};next->cleared_words[slot]=0;next->cleared_halves[slot]=0;next->zero_scalars[slot]=0;
                BattleContactTailActorInput input;input.contact=reset;input.position=s.position20;input.health=s.shorts1c4_1c6_1c8[1];input.forced663=prelude.forced663;input.peer_mask71e=prelude.peer_mask71e;input.peer_mask71f=prelude.peer_mask71f;input.disabled4a1=prelude.disabled4a1;
                if(!battle_contact_tail_actor_gates(tail_profile,input,seed->bounds38_44,error))return false;
                actors.push_back({actor,reset,prelude});next->reset_order.push_back({BattleContactResetOwnerKind::PhysicalObject,{0,s32(slot)},255});next->sphere_bottom[slot]=-10000;
            }
            auto& stage=storage_->stage_construction;std::vector<BattleStageConstruction::ObjectRef> ordered;
            if(!stage.group40(&ordered,error))return false;
            struct ObjectPending {BattleStageSideObject* side;BattleStagePropObject* prop;CollisionContactState reset;};
            std::vector<ObjectPending> objects;objects.reserve(56);
            for(const auto& ref:ordered)if(ref.side?ref.side->collision86:ref.prop->collision86){
                const auto& state=ref.side?ref.side->contact:ref.prop->contact;const auto generation=ref.side?ref.side->contact_generation:ref.prop->contact_generation;
                if(generation!=1)return reject("source object first-contact generation required");
                CollisionContactState reset;if(!collision_contact_reset(state,&reset,error))return false;
                objects.push_back({ref.side,ref.prop,reset});next->reset_order.push_back({BattleContactResetOwnerKind::PhysicalObject,ref.link,255});
            }
            std::array<CollisionContactState,12> view_reset;
            for(unsigned i=0;i<12;++i){auto& view=storage_->views.storage_->views[i];if(view.contact_generation!=1)return reject("source view contact generation");auto state=view.contact;state.field1da=view.contact1da;if(!collision_contact_reset(state,&view_reset[i],error))return false;next->reset_order.push_back({BattleContactResetOwnerKind::View,{},u8(i)});}
            auto find=[&](const SceneCollisionLease& lease)->const BattleSpawnColliderObservation*{for(const auto& body:sweep->order)if(body.lease.index()==lease.index()&&body.lease.generation()==lease.generation()&&body.after)return &body;return nullptr;};
            const auto& spheres=sweep->registries[unsigned(CollisionRegistrationList::L297C)];const auto& planes=sweep->registries[unsigned(CollisionRegistrationList::L237C)];
            next->sphere_pairs.reserve(spheres.size()*planes.size());
            for(const auto& lease:spheres){
                const auto* a=find(lease);if(!a||a->view_parent||a->object_parent.physical!=0||a->descriptor.shape!=0)return reject("source actor297C sphere required");
                const auto& shape=a->after->fields;next->sphere_bottom[unsigned(a->object_parent.index)]=f32(shape.center64[1]-shape.sphere.radius88);
                for(const auto& other:planes){
                    const auto* b=find(other);if(!b||b->view_parent)return reject("source237C non-view plane required");
                    if(a->object_parent==b->object_parent)continue;
                    BattleContactPairObservation pair;pair.first=lease.index();pair.second=other.index();
                    if(!collision_contact_broadphase(shape.center64,shape.broad_radius7c,b->after->fields.center64,b->after->fields.broad_radius7c,bits_to_f32(0x3dcccccdu),&pair.broadphase,error))return false;
                    if(pair.broadphase.overlap)return reject("overlapping source sphere/plane response unsupported");
                    next->sphere_pairs.push_back(pair);
                }
            }
            for(auto list:{CollisionRegistrationList::L357C,CollisionRegistrationList::L477C,CollisionRegistrationList::L177C})if(!sweep->registries[unsigned(list)].empty())return reject("attack/ground response registry requires source consumers");
            const auto& solids=sweep->registries[unsigned(CollisionRegistrationList::L3B7C)];
            for(unsigned i=0;i+1<solids.size();++i){
                const auto* body=find(solids[i]);if(!body)return reject("source solid collider");
                if(body->descriptor.flags2&0x800){++next->flagged_static_candidates;continue;}
                if(body->view_parent||body->object_parent.physical!=3)return reject("movable solid response unsupported");
                const auto ref=stage.object_ref(body->object_parent);if(!ref)return reject("source static parent");
                const auto freeze=ref->side?ref->side->enabled281:ref->prop->enabled281;
                bool skip=false;if(!battle_contact_static_pair_gate(body->descriptor.flags2,freeze,storage_->physical.bank(3)->records[unsigned(body->object_parent.index)].kind83,&skip)||!skip)return reject("source movable solid pair continuation required");
                ++next->frozen_static_candidates;
            }
            //All body owners are reset above; no1DD displacement, ground move,
            //forced health change or actor response is admitted by this entry.
            for(const auto& body:sweep->order)if(body.after&&!body.view_parent&&body.object_parent.physical==3){
                const auto ref=stage.object_ref(body.object_parent);if(!ref)return reject("current collider parent");
                const auto& reset=ref->side?ref->side->contact:ref->prop->contact;
                //A timer1E8 still alive AFTER the reset needs a shape transform.
                if(reset.retained.timer1e8>1)return reject("retained stage shape-update timer continuation required");
            }
            for(auto& actor:actors)actor.actor->storage_->journal.reserve(actor.actor->storage_->journal.size()+2);
            for(auto& pending:actors){auto& body=*pending.actor->storage_;commit_actor_contact(body.state,pending.reset);body.state.contact_prelude=pending.prelude;body.state.contact_generation=4;body.journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,0x8002cda4});body.journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,0x8005a580});}
            for(auto& pending:objects){if(pending.side){pending.side->contact=pending.reset;++pending.side->contact_generation;}else{pending.prop->contact=pending.reset;++pending.prop->contact_generation;}}
            for(unsigned i=0;i<12;++i){auto& view=storage_->views.storage_->views[i];view.contact=view_reset[i];view.contact1da=view.contact.field1da;++view.contact_generation;}
            next->actor_tail_returns=u8(actors.size());next->complete=true;storage_->spawn_contacts=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain source continuation; no replay");}
    }
    const BattleSpawnContactState* spawn_contacts(const std::array<const ForceInventory*,2>& owners)const{
        return spawn_collider_sweep(owners)&&storage_->spawn_contacts&&storage_->spawn_contacts->complete?storage_->spawn_contacts.get():nullptr;
    }
    bool evaluate_spawn_visibility(const ColliderVisibilityProfile& profile,const ColliderInitialShapeProfile& geometry,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("spawn visibility: ")+why;return false;};
        const auto* contacts=spawn_contacts(owners);const auto* sweep=spawn_collider_sweep(owners);
        if(!profile.ready()||!geometry.ready()||!contacts||!sweep||storage_->spawn_visibility||storage_->views.pose_generation()!=1)return reject("current unconsumed spawn contacts and first-camera view poses required");
        auto& views=*storage_->views.storage_;const auto* input=storage_->views.scene_inputs();
        if(!input||input->layout29>2)return reject("source alias/display domain");
        const unsigned selected_count=1u<<input->layout29;
        try{
            auto next=std::make_unique<BattleSpawnVisibilityState>();next->casts.reserve(13*384);
            std::array<std::array<u16,2>,512> masks{};std::array<bool,512> parents{};
            const auto& solids=sweep->registries[unsigned(CollisionRegistrationList::L3B7C)];
            auto find=[&](const SceneCollisionLease& lease)->const BattleSpawnColliderObservation*{for(const auto& body:sweep->order)if(body.after&&body.lease.index()==lease.index()&&body.lease.generation()==lease.generation())return &body;return nullptr;};
            auto visit=[&](unsigned view_index,bool selected)->bool{
                const auto& view=views.views[view_index];
                if(!view.active00)return true;
                if(view.pose_generation!=1||view.group11||view_index>=6)return reject("ordinary first-camera record required");
                const auto delta=source_intersection_detail::sub(view.eye2e8,view.target300);
                const f32 squared=source_intersection_detail::dot(delta,delta);
                if(!std::isfinite(squared))return reject("source view segment distance");
                if(!(squared>0))return true;
                std::array<f32,3> center;for(unsigned k=0;k<3;++k)center[k]=f32(f32(view.eye2e8[k]+view.target300[k])*.5f);
                //36E24 retains BC88's SQUARED distance before multiplying by.5.
                //Do not replace this reached broad gate with half the length.
                const f32 view_radius=f32(squared*.5f);
                for(const auto& lease:solids){
                    const auto* body=find(lease);if(!body)return reject("same source3B7C body");
                    if(!selected&&!(body->descriptor.flags2&0x800))continue;
                    if(body->view_parent||body->object_parent.physical!=3)return reject("actor/view-specific visibility owner filters unsupported");
                    const auto ref=storage_->stage_construction.object_ref(body->object_parent);if(!ref)return reject("current stage collider parent");
                    const auto enabled=ref->side?ref->side->enabled26f:ref->prop->enabled26f;
                    if(!enabled)continue;
                    const auto generation=ref->side?ref->side->contact_generation:ref->prop->contact_generation;
                    const auto& contact=ref->side?ref->side->contact:ref->prop->contact;
                    if(generation!=2||contact.moved1dd||contact.retained.displacement1cc!=std::array<f32,3>{})return reject("post-reset stage visibility fields required");
                    BattleVisibilityCast cast;cast.view=u8(view_index);cast.collider=lease.index();cast.parent=body->object_parent;cast.selected=selected;
                    cast.distance=collider_plane_detail::length(source_intersection_detail::sub(body->after->fields.center64,center));
                    cast.radius_sum=f32(body->after->fields.broad_radius7c+view_radius);
                    if(!std::isfinite(cast.distance)||!std::isfinite(cast.radius_sum))return reject("source view broadphase numeric domain");
                    cast.broadphase_passed=cast.distance<=cast.radius_sum;
                    if(cast.broadphase_passed){
                        ColliderVisibilityHit hit;
                        if(!collider_visibility_segment(profile,geometry,body->after->fields,view.eye2e8,view.target300,&hit,error))return false;
                        cast.query=hit;
                        if(hit.hit){
                            ++next->hits;
                            if(selected){const unsigned index=unsigned(body->object_parent.index);parents[index]=true;masks[index][0]|=u16(1u<<view_index);if(hit.start_inside)masks[index][1]|=u16(1u<<view_index);}
                            if(body->descriptor.flags2&0x800)next->visible26f[view_index]=1;
                        }
                    }
                    next->casts.push_back(std::move(cast));
                }
                return true;
            };
            for(unsigned alias=0;alias<selected_count;++alias){
                const auto index=views.viewer_indices[alias];
                if(index<0||index>=13)return reject("source selected viewer alias");
                next->selected[unsigned(index)]=true;
                if(!visit(unsigned(index),true))return false;
            }
            for(unsigned index=0;index<13;++index)if(!next->selected[index]&&!visit(index,false))return false;
            //8972C and89658 immediately follow the visibility call. A nonempty
            //callback queue may mutate worlds before the camera; refuse it.
            const auto& scheduler=*storage_->physical.scheduler();for(unsigned i=2;i<6;++i)if(scheduler.callback_heads[i].present()||scheduler.callback_tails[i].present())return reject("unjoined post-visibility callback list");
            for(unsigned i=0;i<512;++i)if(parents[i]){
                const auto ref=storage_->stage_construction.object_ref({3,s32(i)});if(!ref)return reject("parent changed before visibility commit");
                auto& contact=ref->side?ref->side->contact:ref->prop->contact;contact.hidden270|=masks[i][0];contact.shadow286|=masks[i][1];
            }
            for(unsigned i=0;i<13;++i)views.views[i].visible26f=next->visible26f[i];
            next->complete=true;storage_->spawn_visibility=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain visibility prefix; no replay");}
    }
    const BattleSpawnVisibilityState* spawn_visibility(const std::array<const ForceInventory*,2>& owners)const{
        return spawn_contacts(owners)&&storage_->spawn_visibility&&storage_->spawn_visibility->complete?storage_->spawn_visibility.get():nullptr;
    }
    bool advance_stage_object_worlds(const BattleStageObjectUpdateProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="scene side worlds: exact first/post-spawn actor and callback boundary required";return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!initial_order_current(owners)||storage_->publication||!profile.ready()||storage_->stage_construction.phase()!=BattleStageConstructionPhase::InitializedAwaitFrame)return reject();
        const auto generation=storage_->stage_construction.object_world_generation();
        if(generation>1||storage_->views.pose_generation()!=generation)return reject();
        const auto& physical=*storage_->physical.storage_;
        for(unsigned i=0;i<6;++i)if(physical.actors.slots[i].active00){
            const auto scope=physical.constructors[i].scope_.lock();if(!scope||!scope->live||!scope->owner||!scope->owner->current(owners))return reject();
            const auto& actor=*scope->owner;const auto& s=actor.storage_->state;
            if(s.world_generation!=generation+1||s.native_revision!=19+generation||actor.status()!=(generation?BattleActorConstructionStatus::SpawnWorldReady:BattleActorConstructionStatus::FirstWorldReady))return reject();
        }
        try{return storage_->stage_construction.advance_object_worlds(profile,storage_->physical,error);}
        catch(const std::exception&){return storage_->stage_construction.fail(error,"uncertain source side-world continuation");}
    }
    bool advance_first_point_child(const BattleInitialPointChildProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("first child update prefix: ")+why;return false;};
        if(!profile.ready()||!stage_actor_prefix_current(owners)||storage_->initial_child_prefix||storage_->stage_construction.phase()!=BattleStageConstructionPhase::InitializedAwaitFrame||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d2)return reject("fresh unmasked first-world group0 child boundary required");
        auto& physical=*storage_->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;auto& scheduler=physical.scheduler;
        const auto head=arena.state.group_heads[1];
        if(head.physical!=2||head.index<0||unsigned(head.index)>=arena.state.records.size()||scheduler.callback_heads[0].present()||scheduler.callback_tails[0].present())return reject("actual slot0 group head and unconsumed callback0 required");
        const auto scope=physical.constructors[0].scope_.lock();if(!scope||!scope->owner)return reject("current actual slot0 actor");
        const auto* actor=scope->owner;SceneObjectLease lease;
        for(const auto& attempt:actor->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::AiPointFollower&&attempt.source_index==1&&attempt.object.index()==head.index)lease=attempt.object;
        const auto* record=objects->inspect(lease);if(!record||record->link4.present()||record->link8.physical!=2||record->point_child_update)return reject("actual first point follower before its field-follower sibling required");
        const auto next=record->link8;if(unsigned(next.index)>=arena.state.records.size()||arena.state.records[unsigned(next.index)].header.update0c_key!=0x800c6394)return reject("next source sibling correspondence required");
        HsdJointInstanceBank roots;BattleInitialPointChildNumbers numbers;
        if(!actor->prepare_initial_point_child(profile,lease,owners,&roots,&numbers,error))return false;
        try{
            //Preallocate the receipt before any source callback/model effects.
            auto prefix=std::make_unique<BattleInitialChildPrefixState>();prefix->first=head;prefix->next=next;
            auto& r=arena.state.records[unsigned(head.index)];r.point_child_update.emplace();r.point_child_update->phase18=1;
            r.callback100_key=0x80047aa4;r.callback_previous104=r.callback_next108={};r.registration85=0;
            scheduler.callback_heads[0]=scheduler.callback_tails[0]=head; //89100(0,append)
            //The source actor reference owns identity; only this scene dispatches
            //the mutable child update. Parent actor state itself is not advanced.
            if(!actor->commit_initial_point_child(lease,roots,numbers,error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            prefix->updates=1;storage_->initial_child_object=lease;storage_->initial_child_prefix=std::move(prefix);
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain child update; consumed prefix retained, no replay");}
    }
    bool advance_first_field_child(const BattleInitialFieldChildProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("first field-child update: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=1||prefix->slot||prefix->group_base||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d2)return reject("current unmasked point61 prefix before89554 required");
        auto& physical=*storage_->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;auto& scheduler=physical.scheduler;
        const auto link=prefix->next,first=prefix->first;
        if(link.physical!=2||link.index<0||unsigned(link.index)>=arena.state.records.size()||first.physical!=2||first.index<0||unsigned(first.index)>=arena.state.records.size()||scheduler.callback_heads[0]!=first||scheduler.callback_tails[0]!=first)return reject("actual pending sibling and callback0 head/tail correspondence required");
        const auto& prior=arena.state.records[unsigned(first.index)];const auto& record=arena.state.records[unsigned(link.index)];
        if(prior.link8!=link||prior.callback100_key!=0x80047aa4||prior.registration85||prior.callback_next108.present()||record.link4!=first||record.link8.present()||arena.state.group_tails[1]!=link)return reject("actual slot0 source tail after initialized point child required");
        const auto scope=physical.constructors[0].scope_.lock();if(!scope||!scope->owner)return reject("current exact slot0 parent");
        const auto* actor=scope->owner;SceneObjectLease lease;
        for(const auto& attempt:actor->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::AiFieldFollower&&attempt.source_index==0&&attempt.object.index()==link.index)lease=attempt.object;
        HsdJointInstanceBank roots;BattleInitialFieldChildNumbers numbers;
        if(!actor->prepare_initial_field_child(profile,lease,owners,&roots,&numbers,error))return false;
        auto next=*prefix;next.updates=2;next.slot=6;next.next=arena.state.group_heads[0];
        for(unsigned slot=1;slot<6;++slot)if(physical.banks[0].records[slot].active00&&arena.state.group_heads[1+slot].present()){next.slot=u8(slot);next.next=arena.state.group_heads[1+slot];break;}
        try{
            auto& r=arena.state.records[unsigned(link.index)];r.field_child_update.emplace();r.field_child_update->phase18=1;
            r.callback100_key=0x80047aa4;r.callback_previous104=first;r.callback_next108={};r.registration85=0;
            arena.state.records[unsigned(first.index)].callback_next108=link;scheduler.callback_tails[0]=link; //89100(0,append), after61
            if(!actor->commit_initial_field_child(lease,roots,numbers,error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            storage_->initial_field_child_object=lease;*storage_->initial_child_prefix=next;
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain source sibling prefix; no replay");}
    }
    bool advance_first_sasuke_attachment(const BattleInitialSasukeChildrenProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("first Sasuke attachment: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->group_base||prefix->slot!=1||(prefix->updates!=2&&prefix->updates!=3)||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d2)return reject("unmasked current group0/slot1 cursor after both070B children required");
        const unsigned ordinal=prefix->updates-2;auto& physical=*storage_->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;auto& scheduler=physical.scheduler;
        const auto link=prefix->next;if(link.physical!=2||link.index<0||unsigned(link.index)>=arena.state.records.size())return reject("actual current class2 group2 node required");
        const auto& r=arena.state.records[unsigned(link.index)];const u8 type=u8(7-ordinal);
        if(r.header.type11!=type||r.group01!=2)return reject("source type7 then type6, not caller-selected children");
        ScenePhysicalLink prior;
        if(!ordinal){
            if(arena.state.group_heads[2]!=link||r.link4.present()||!r.link8.present()||scheduler.callback_heads[7].present()||scheduler.callback_tails[7].present())return reject("actual first slot1 child and fresh selector11 list required");
        }else{
            const auto* previous=objects->inspect(storage_->initial_sasuke_child_objects[0]);if(!previous||!previous->bank_child_update||!previous->bank_child_update->initialized)return reject("owned preceding type7 update required");
            prior={2,s32(storage_->initial_sasuke_child_objects[0].index())};
            if(previous->link8!=link||r.link4!=prior||r.link8.present()||arena.state.group_tails[2]!=link||scheduler.callback_heads[7]!=prior||scheduler.callback_tails[7]!=prior||previous->callback_next108.present()||previous->registration85!=0x11)return reject("exact type7->6 source group/callback append correspondence required");
        }
        const auto scope=physical.constructors[1].scope_.lock();if(!scope||!scope->owner)return reject("current registered Sasuke parent");
        const auto* actor=scope->owner;SceneObjectLease lease;
        for(const auto& attempt:actor->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::BankAttachment&&attempt.source_index==type&&attempt.object.index()==link.index)lease=attempt.object;
        HsdJointInstanceBank roots;StartupAttachmentUpdate step;
        if(!actor->prepare_initial_sasuke_child(profile,lease,owners,&roots,&step,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        if(!next.next.present()){next.slot=6;next.next=arena.state.group_heads[0];for(unsigned slot=2;slot<6;++slot)if(physical.banks[0].records[slot].active00&&arena.state.group_heads[1+slot].present()){next.slot=u8(slot);next.next=arena.state.group_heads[1+slot];break;}}
        const auto* p=profile.attachments();const u16 record=p->entries[type-6].record;
        try{
            auto& current=arena.state.records[unsigned(link.index)];current.bank_child_update.emplace();current.bank_child_update->state.phase=1;
            current.callback100_key=0x80047aa4;current.registration85=0x11;current.callback_previous104=prior;current.callback_next108={};
            if(prior.present())arena.state.records[unsigned(prior.index)].callback_next108=link;else scheduler.callback_heads[7]=link;
            scheduler.callback_tails[7]=link; //89100 selector11, NOT scheduler index7 in byte85
            if(!actor->commit_initial_bank_child(lease,roots,step,record,error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            storage_->initial_sasuke_child_objects[ordinal]=lease;*storage_->initial_child_prefix=next;
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain source child prefix; no replay");}
    }
    bool advance_first_gred_attachment(const BattleInitialGRedChildrenProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("first G-Red attachment: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->group_base||prefix->slot!=3||(prefix->updates!=4&&prefix->updates!=5)||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d2)return reject("actual slot3/group4 cursor after both Sasuke children required");
        const unsigned ordinal=prefix->updates-4;auto& physical=*storage_->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;auto& scheduler=physical.scheduler;
        const auto link=prefix->next;if(link.physical!=2||link.index<0||unsigned(link.index)>=arena.state.records.size())return reject("actual pending class2 child required");
        const auto& r=arena.state.records[unsigned(link.index)];const u8 type=u8(25-ordinal);
        if(r.header.type11!=type||r.group01!=4)return reject("source type25 then24, not a remapped preview child");
        ScenePhysicalLink prior;
        if(!ordinal){
            if(arena.state.group_heads[4]!=link||r.link4.present()||!r.link8.present()||scheduler.callback_heads[9].present()||scheduler.callback_tails[9].present())return reject("first actual slot3 child and fresh selector13 list required");
        }else{
            const auto* previous=objects->inspect(storage_->initial_gred_child_objects[0]);if(!previous||!previous->bank_child_update||!previous->bank_child_update->initialized)return reject("owned preceding type25 required");
            prior={2,s32(storage_->initial_gred_child_objects[0].index())};
            if(previous->link8!=link||r.link4!=prior||r.link8.present()||arena.state.group_tails[4]!=link||scheduler.callback_heads[9]!=prior||scheduler.callback_tails[9]!=prior||previous->callback_next108.present()||previous->registration85!=0x13)return reject("actual25->24 group/callback append correspondence");
        }
        const auto scope=physical.constructors[3].scope_.lock();if(!scope||!scope->owner)return reject("current actual G-Red parent at slot3");
        const auto* actor=scope->owner;SceneObjectLease lease;
        for(const auto& attempt:actor->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::BankAttachment&&attempt.source_index==type&&attempt.object.index()==link.index)lease=attempt.object;
        HsdJointInstanceBank roots;StartupAttachmentUpdate step;
        if(!actor->prepare_initial_gred_child(profile,lease,owners,&roots,&step,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        if(!next.next.present()){next.slot=6;next.next=arena.state.group_heads[0];for(unsigned slot=4;slot<6;++slot)if(physical.banks[0].records[slot].active00&&arena.state.group_heads[1+slot].present()){next.slot=u8(slot);next.next=arena.state.group_heads[1+slot];break;}}
        const u16 record=profile.attachments()->entries[type-24].record;
        try{
            auto& current=arena.state.records[unsigned(link.index)];current.bank_child_update.emplace();current.bank_child_update->state.phase=1;
            current.callback100_key=0x80047aa4;current.registration85=0x13;current.callback_previous104=prior;current.callback_next108={};
            if(prior.present())arena.state.records[unsigned(prior.index)].callback_next108=link;else scheduler.callback_heads[9]=link;
            scheduler.callback_tails[9]=link;
            if(!actor->commit_initial_bank_child(lease,roots,step,record,error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            storage_->initial_gred_child_objects[ordinal]=lease;*storage_->initial_child_prefix=next;
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain source G-Red prefix; no replay");}
    }
    bool advance_first_empty_child_groups(const BattleInitialChildGroupOrderProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("first child group continuation: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=6||prefix->group_base||prefix->slot!=6||prefix->next.present()||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d1||storage_->physical.controls()->d2)return reject("unmasked consumed active group0 children before89554 required");
        const auto objects=storage_->physical.class2_pool();const auto& state=*objects->snapshot();
        auto empty=[&](unsigned group){return !state.group_heads[group].present()&&!state.group_tails[group].present();};
        if(!empty(0)||!empty(7))return reject("occupied unparented group0/7 remains unsupported");
        for(unsigned group=8;group<24;++group)if(!empty(group))return reject("occupied group10/8 continuation cannot be skipped");
        //All lists in these source calls are genuinely empty, including active
        //actor buckets and unparented tails. No callbacks or pool rewrites occur.
        auto next=*prefix;next.group_base=24;next.slot=6;next.next=state.group_heads[24];
        const auto* actors=storage_->physical.bank(0);
        for(unsigned slot=0;slot<6;++slot)if(actors->records[slot].active00&&state.group_heads[25+slot].present()){next.slot=u8(slot);next.next=state.group_heads[25+slot];break;}
        *storage_->initial_child_prefix=next;if(error){error->clear();}return true;
    }
    bool advance_first_shape_child(const BattleInitialShapeChildProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("first shape child: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);const auto* stage=stage_resources(owners);
        if(!profile.ready()||!prefix||prefix->updates!=6||prefix->group_base!=24||prefix->slot||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d1||!stage||!stage->current()||!stage->inputs()->native_data_bound()||stage->inputs()->stage()!=2||!stage->bank()||stage->bank()->hash()!=0xbdf8e542||!stage->initial_light_constructor())return reject("current source group18 cursor and owned stage2/main2 light constructors required");
        auto& physical=*storage_->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;auto& scheduler=physical.scheduler;const auto link=prefix->next;
        if(link.physical!=2||link.index<0||unsigned(link.index)>=arena.state.records.size()||arena.state.group_heads[25]!=link||scheduler.callback_heads[6].present()||scheduler.callback_tails[6].present())return reject("actual first slot0 group25 shape and fresh selector10 list required");
        const auto& r=arena.state.records[unsigned(link.index)];
        if(r.link4.present()||r.link8.physical!=2||r.link8.index<0||unsigned(r.link8.index)>=arena.state.records.size()||arena.state.records[unsigned(r.link8.index)].header.update0c_key!=0x8001c42c)return reject("actual first shape before its source sibling required");
        const auto scope=physical.constructors[0].scope_.lock();if(!scope||!scope->owner)return reject("current exact070B parent");
        SceneObjectLease lease;for(const auto& attempt:scope->owner->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::Shape&&attempt.source_index==1&&attempt.object.index()==link.index)lease=attempt.object;
        BattleInitialShapeNumbers numbers;
        if(!scope->owner->prepare_initial_shape_child(profile,lease,*stage->initial_light_constructor(),owners,&numbers,error))return false;
        //All allocation-free source effects follow preflight. No model/world
        //publication, callback execution or parent-clock advancement occurs.
        auto next=*prefix;++next.updates;next.next=r.link8;
        auto& current=arena.state.records[unsigned(link.index)];current.shape_child_update.emplace();auto& update=*current.shape_child_update;
        update.phase18=1;current.callback100_key=0x8001c5ac;current.registration85=0x10;current.callback_previous104=current.callback_next108={};
        scheduler.callback_heads[6]=scheduler.callback_tails[6]=link;
        update.numbers=numbers;update.render82=1;update.generation=1;update.light_constructor_generation=stage->initial_light_constructor()->generation;update.initialized=true;
        storage_->initial_shape_child_object=lease;*storage_->initial_child_prefix=next;
        if(error){error->clear();}return true;
    }
    bool advance_initial_shape_continuation(const BattleInitialShapeContinuationProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        return advance_initial_shape_cursor(profile,false,owners,error);
    }
    bool advance_initial_gred_shapes(const BattleInitialShapeContinuationProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        return advance_initial_shape_cursor(profile,true,owners,error);
    }
private:
    bool advance_initial_shape_cursor(const BattleInitialShapeContinuationProfile& profile,bool after_auxiliary,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("initial shape continuation: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);const auto* stage=stage_resources(owners);
        const bool before=prefix&&prefix->updates>=7&&prefix->updates<=11&&prefix->slot<3;
        const bool after=prefix&&(prefix->updates==14||prefix->updates==15)&&prefix->slot==3;
        if(!profile.ready()||!prefix||(after_auxiliary?!after:!before)||prefix->group_base!=24||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d1||!stage||!stage->current()||!stage->inputs()->native_data_bound()||stage->inputs()->stage()!=2||!stage->bank()||stage->bank()->hash()!=0xbdf8e542||!stage->initial_light_constructor())return reject("owned exact pre/post-auxiliary source shape cursor required");
        auto& physical=*storage_->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;auto& scheduler=physical.scheduler;
        const auto link=prefix->next;const unsigned slot=prefix->slot,group=25+slot,list=6+slot;const u8 selector=u8(0x10+slot);
        if(link.physical!=2||link.index<0||unsigned(link.index)>=arena.state.records.size())return reject("actual pending class2 source node required");
        const auto& r=arena.state.records[unsigned(link.index)];
        if(r.group01!=int(group)||r.header.update0c_key!=0x8001c42c)return reject("non-shape source callbacks cannot be skipped");
        const auto scope=physical.constructors[slot].scope_.lock();if(!scope||!scope->owner)return reject("current exact source parent");
        SceneObjectLease lease;unsigned source_index=2;
        for(const auto& attempt:scope->owner->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::Shape&&attempt.object.index()==link.index){lease=attempt.object;source_index=attempt.source_index;}
        if(source_index>1)return reject("owned source constructor shape index required");
        if(source_index==1){
            if(!r.link8.present())return reject("actual first LIFO source shape");
            if(after_auxiliary){
                const auto* previous=objects->inspect(storage_->initial_common_aux_objects[1]);
                if(!previous||!previous->common_auxiliary_update||!previous->common_auxiliary_update->initialized||previous->link8!=link||r.link4!=ScenePhysicalLink{2,s32(storage_->initial_common_aux_objects[1].index())})return reject("actual completed auxiliary40 before G-Red shape required");
            }else if(arena.state.group_heads[group]!=link||r.link4.present())return reject("actual source shape group head");
        }
        else{
            if(r.link4.physical!=2||r.link4.index<0||unsigned(r.link4.index)>=arena.state.records.size())return reject("actual previous source shape required");
            if(after_auxiliary){
                if(r.link8.physical!=2||r.link8.index<0||unsigned(r.link8.index)>=arena.state.records.size()||arena.state.records[unsigned(r.link8.index)].header.update0c_key!=0x8013be88)return reject("actual following bar child must remain pending");
            }else if(r.link8.present()||arena.state.group_tails[group]!=link)return reject("actual source shape tail");
            const auto& previous=arena.state.records[unsigned(r.link4.index)];
            if(!previous.shape_child_update||!previous.shape_child_update->initialized||previous.link8!=link)return reject("preceding source shape must actually be initialized");
        }
        const auto tail=scheduler.callback_tails[list];
        if(scheduler.callback_heads[list].present()!=tail.present())return reject("source callback head/tail correspondence");
        if(tail.present()){
            if(tail.physical!=2||tail.index<0||unsigned(tail.index)>=arena.state.records.size())return reject("unjoined callback-tail physical class");
            const auto& last=arena.state.records[unsigned(tail.index)];
            if(!last.allocated||last.active00!=1||last.registration85!=selector||last.callback_next108.present())return reject("actual retained callback append tail required");
        }
        BattleInitialShapeNumbers numbers;
        if(!scope->owner->prepare_shape_continuation(profile,lease,source_index,*stage->initial_light_constructor(),owners,&numbers,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        if(!next.next.present()){next.slot=6;next.next=arena.state.group_heads[24];for(unsigned s=slot+1;s<6;++s)if(physical.banks[0].records[s].active00&&arena.state.group_heads[25+s].present()){next.slot=u8(s);next.next=arena.state.group_heads[25+s];break;}}
        auto& current=arena.state.records[unsigned(link.index)];current.shape_child_update.emplace();auto& update=*current.shape_child_update;
        update.phase18=1;current.callback100_key=0x8001c5ac;current.registration85=selector;current.callback_previous104=tail;current.callback_next108={};
        if(tail.present())arena.state.records[unsigned(tail.index)].callback_next108=link;else scheduler.callback_heads[list]=link;
        scheduler.callback_tails[list]=link;
        update.numbers=numbers;update.render82=1;update.generation=1;update.light_constructor_generation=stage->initial_light_constructor()->generation;update.initialized=true;
        const unsigned receipt=after_auxiliary?5+prefix->updates-14:prefix->updates-7;
        storage_->continuation_shape_objects[receipt]=lease;*storage_->initial_child_prefix=next;
        if(error){error->clear();}return true;
    }
public:
    bool advance_initial_common_auxiliary(const BattleInitialCommonAuxiliaryProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("initial common auxiliary: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);const auto* graphics=initial_graphics_archives(owners);
        if(!profile.ready()||!prefix||prefix->group_base!=24||prefix->slot!=3||(prefix->updates!=12&&prefix->updates!=13)||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d1||!graphics||graphics->generation!=1)return reject("ordered pre-actor archive fragment and exact group28 source cursor required");
        auto& physical=*storage_->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;auto& scheduler=physical.scheduler;
        const auto effects=storage_->characters.batch_->effects;const auto& archive=effects->storage_->archive;
        if(!effects->bound_bank()||effects->bound_bank()->hash()!=0x6dd5d3b2)return reject("current selected split-effect archive required");
        const unsigned ordinal=prefix->updates-12;const u8 type=u8(41-ordinal);const auto link=prefix->next;
        if(link.physical!=2||link.index<0||unsigned(link.index)>=arena.state.records.size())return reject("actual pending class2 source node required");
        const auto& r=arena.state.records[unsigned(link.index)];
        if(r.group01!=28||r.header.type11!=type||r.header.update0c_key!=0x800c7664)return reject("source41 then40 callback order required");
        if(!ordinal){if(arena.state.group_heads[28]!=link||r.link4.present()||!r.link8.present())return reject("actual source group28 head required");}
        else{
            const auto* prior=objects->inspect(storage_->initial_common_aux_objects[0]);
            if(!prior||!prior->common_auxiliary_update||!prior->common_auxiliary_update->initialized||prior->link8!=link||r.link4!=ScenePhysicalLink{2,s32(storage_->initial_common_aux_objects[0].index())})return reject("owned preceding41 required");
        }
        const auto tail=scheduler.callback_tails[9];
        if(!scheduler.callback_heads[9].present()||tail.physical!=2||tail.index<0||unsigned(tail.index)>=arena.state.records.size())return reject("actual retained selector13 append tail required");
        const auto& previous=arena.state.records[unsigned(tail.index)];
        if(!previous.allocated||previous.active00!=1||previous.registration85!=0x13||previous.callback_next108.present())return reject("source callback tail correspondence");
        const auto scope=physical.constructors[3].scope_.lock();if(!scope||!scope->owner)return reject("current actual G-Red parent");
        SceneObjectLease lease;for(const auto& attempt:scope->owner->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::CommonAuxiliary&&attempt.source_index==type&&attempt.object.index()==link.index)lease=attempt.object;
        HsdJointInstanceBank roots;StartupAuxiliaryState state;
        if(!scope->owner->prepare_initial_common_auxiliary(profile,lease,archive,owners,&roots,&state,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        try{
            auto& current=arena.state.records[unsigned(link.index)];current.common_auxiliary_update.emplace();current.common_auxiliary_update->state.phase=1;
            current.callback100_key=0x800c79f8;current.registration85=0x13;current.callback_previous104=tail;current.callback_next108={};
            arena.state.records[unsigned(tail.index)].callback_next108=link;scheduler.callback_tails[9]=link;
            if(!scope->owner->commit_initial_common_auxiliary(lease,archive,roots,state,error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            storage_->initial_common_aux_objects[ordinal]=lease;*storage_->initial_child_prefix=next;
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain source prefix retained; no replay");}
    }
    bool advance_initial_bar_child(const BattleInitialBarChildProfile& profile,ISceneScratchHeap& heap,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("initial bar child: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=16||prefix->group_base!=24||prefix->slot!=3||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d1)return reject("actual source bar cursor after G-Red shapes required");
        const auto keep=storage_;auto& physical=*keep->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;auto& scheduler=physical.scheduler;const auto link=prefix->next;
        if(link.physical!=2||link.index<0||unsigned(link.index)>=arena.state.records.size())return reject("actual pending class2 source node");
        const auto& r=arena.state.records[unsigned(link.index)];
        if(r.group01!=28||r.link8.present()||arena.state.group_tails[28]!=link||r.link4.physical!=2||r.link4.index<0||unsigned(r.link4.index)>=arena.state.records.size()||!arena.state.records[unsigned(r.link4.index)].shape_child_update||!arena.state.records[unsigned(r.link4.index)].shape_child_update->initialized)return reject("actual group28 tail after initialized shape75");
        const auto scope=physical.constructors[3].scope_.lock();if(!scope||!scope->owner)return reject("actual current G-Red parent");
        SceneObjectLease lease;for(const auto& attempt:scope->owner->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::Bar&&attempt.source_index==0&&attempt.object.index()==link.index)lease=attempt.object;
        if(!scope->owner->initial_bar_child_current(profile,lease,owners,error))return false;
        const auto tail=scheduler.callback_tails[1];
        if(!scheduler.callback_heads[1].present()||!tail.present())return reject("actual retained stage-side callback1 list required");
        if(tail.physical==3){
            if(unsigned(tail.index)>=arena.class3.records.size())return reject("stage-side tail range");
            const auto& record=arena.class3.records[unsigned(tail.index)];if(!record.allocated||record.active00!=1||record.registration85!=1||record.callback_next108.present())return reject("current stage-side append tail");
        }else if(tail.physical==2){
            if(unsigned(tail.index)>=arena.state.records.size())return reject("class2 tail range");
            const auto& record=arena.state.records[unsigned(tail.index)];if(!record.allocated||record.active00!=1||record.registration85!=1||record.callback_next108.present())return reject("current class2 append tail");
        }else return reject("unjoined callback1 physical class");
        SceneScratchLease scratch;const auto allocation=keep->descriptors.acquire_scratch(180,heap,&scratch,error);
        if(storage_!=keep||!keep->live){keep->retire();return reject("scene retired/moved during scratch callback; no late bar publication");}
        if(allocation==SceneScratchAcquire::Refused){status_=BattlePreparationBootstrapStatus::Failed;return false;}
        auto next=*prefix;++next.updates;next.slot=6;next.next=arena.state.group_heads[24];
        for(unsigned slot=4;slot<6;++slot)if(physical.banks[0].records[slot].active00&&arena.state.group_heads[25+slot].present()){next.slot=u8(slot);next.next=arena.state.group_heads[25+slot];break;}
        auto& current=arena.state.records[unsigned(link.index)];current.bar_child_update.emplace();auto& update=*current.bar_child_update;update.allocation=allocation;update.generation=1;
        if(allocation==SceneScratchAcquire::Allocated){
            if(!keep->descriptors.claim_scratch_consumer(scratch)){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain source scratch consumer binding");}
            update.scratch_dc=update.buffer54=SceneScratchReference{3,scratch.index(),scratch.generation(),scratch.requested_bytes()};
            update.phase18=1;update.render82=1;update.bone89=3; //high model byte06 -> source default selector3
            current.callback100_key=0x8013c144;current.registration85=1;current.callback_previous104=tail;current.callback_next108={};
            if(tail.physical==3)arena.class3.records[unsigned(tail.index)].callback_next108=link;else arena.state.records[unsigned(tail.index)].callback_next108=link;
            scheduler.callback_tails[1]=link;keep->initial_bar_scratch=scratch;
        }else update.phase18=2; //genuine pool/heap failure; original source continues, no callback registration
        update.initialized=true;keep->initial_bar_object=lease;*keep->initial_child_prefix=next;
        if(error){error->clear();}return true;
    }
    bool advance_initial_bar_child(const BattleInitialBarChildProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){SceneScratchHeap heap;return advance_initial_bar_child(profile,heap,owners,error);}
    //Explicit unbound memory-allocation seam. A lease is not a source actor,
    //callback or world capability; consumer cleanup remains separately gated.
    SceneScratchAcquire acquire_scene_scratch(u32 bytes,ISceneScratchHeap& heap,const std::array<const ForceInventory*,2>& owners,SceneScratchLease* out,std::string* error){
        if(!current(owners)||status_!=BattlePreparationBootstrapStatus::AtResourcesReady){if(out)*out={};if(error)*error="scene scratch: current resource-ready domain required";return SceneScratchAcquire::Refused;}
        const auto keep=storage_;return keep->descriptors.acquire_scratch(bytes,heap,out,error);
    }
    bool advance_initial_post_bar_groups(const BattleInitialBarChildProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=17||prefix->group_base!=24||prefix->slot!=6||prefix->next.present()||storage_->physical.controls()->d1){if(error)*error="post-bar: exact consumed active group18 cursor required";return false;}
        const auto* state=storage_->physical.class2_pool()->snapshot();
        for(unsigned group:{24u,31u})if(state->group_heads[group].present()||state->group_tails[group].present()){if(error)*error="post-bar: occupied unparented source group cannot be skipped";return false;}
        auto next=*prefix;next.group_base=56;next.slot=6;next.next=state->group_heads[56];*storage_->initial_child_prefix=next;
        if(error){error->clear();}return true;
    }
    bool advance_initial_hud_child(const BattleInitialHudChildrenProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("initial HUD child: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);const auto* graphics=initial_graphics_archives(owners);
        if(!profile.ready()||!prefix||prefix->group_base!=56||prefix->slot!=6||prefix->updates<17||prefix->updates>=25||storage_->stage_construction.object_world_generation()||storage_->physical.controls()->d1||!graphics||graphics->generation!=1)return reject("actual post-bar group38/source cursor and ordered cockpit archive required");
        auto& physical=*storage_->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;auto& scheduler=physical.scheduler;
        const auto& cockpit=storage_->cockpit;const auto* bank=cockpit.bound_bank();
        if(!bank||bank->hash()!=0x2fbc80c9||bank->records()!=118)return reject("same actual split cockpit archive required");
        const auto& archive=cockpit.storage_->header;const unsigned ordinal=prefix->updates-17;const bool one=!(ordinal&1u);const unsigned slot=3-ordinal/2;const auto link=prefix->next;
        if(link.physical!=2||link.index<0||unsigned(link.index)>=arena.state.records.size())return reject("actual pending source class2 node required");
        const auto& r=arena.state.records[unsigned(link.index)];
        if(!ordinal){if(arena.state.group_heads[56]!=link||r.link4.present())return reject("actual group38 head required");}
        else{
            const auto& prior=storage_->initial_hud_objects[ordinal-1];const auto* previous=objects->inspect(prior);
            if(!previous||!previous->hud_child_update||!previous->hud_child_update->initialized||previous->link8!=link||r.link4!=ScenePhysicalLink{2,s32(prior.index())})return reject("actual preceding initialized HUD node required");
        }
        //Retain occupied suffixes rather than treating eight expected nodes as
        //proof that all source group38 updates have completed.
        const auto scope=physical.constructors[slot].scope_.lock();if(!scope||!scope->owner)return reject("same actual source parent required");
        SceneObjectLease lease;for(const auto& attempt:scope->owner->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==(one?BattleActorChildKind::PostClassOne:BattleActorChildKind::PostClassSix)&&attempt.object.index()==link.index)lease=attempt.object;
        if(!scope->owner->initial_hud_child_current(profile,lease,one,owners,error))return false;
        const auto tail=scheduler.callback_tails[5];
        if(scheduler.callback_heads[5].present()!=tail.present())return reject("callback5 head/tail correspondence");
        if(tail.present()){
            if(tail.physical!=2||tail.index<0||unsigned(tail.index)>=arena.state.records.size())return reject("unjoined callback5 physical class");
            const auto& last=arena.state.records[unsigned(tail.index)];
            if(!last.allocated||last.active00!=1||last.registration85!=0x21||last.callback_next108.present())return reject("actual callback5 append tail");
        }
        HsdRecordAnimationTables tables;
        if(!bank->animation_tables(one?56:81,&tables)||tables.joint||tables.material||tables.shape)return reject("exact NULL animation tables required, not empty parsed vectors");
        HsdJointInstanceBank roots;const std::array<u16,3> records{81,81,50};const std::array<u16,1> single{56};
        if(!roots.construct_normal_records(*bank,one?std::span<const u16>{single}:std::span<const u16>{records},error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        try{
            auto& current=arena.state.records[unsigned(link.index)];current.hud_child_update.emplace();auto& update=*current.hud_child_update;
            update.phase18=1; //81478/808B4 never store render82; fresh clear0 stays.
            current.callback100_key=one?0x800818cc:0x80080f88;current.registration85=0x21;current.callback_previous104=tail;current.callback_next108={};
            if(tail.present())arena.state.records[unsigned(tail.index)].callback_next108=link;else scheduler.callback_heads[5]=link;
            scheduler.callback_tails[5]=link;
            if(!one){
                for(unsigned offset:{0u,2u,8u,10u})store_be16(update.controls144.data()+offset,0xffff);
                update.controls144[4]=update.controls144[5]=0xff;
            }
            for(unsigned i=0;i<roots.instances();++i){
                const unsigned descriptor_slot=one?0:i+2;const u16 record=one?56:records[i];HsdJointInstanceOwner root;
                if(!roots.take_instance(i,&root,error)||!SceneModelBindings::attach_object_descriptor(*objects,lease,descriptor_slot,archive,record,std::move(root),error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
                const auto& descriptor=arena.leases[lease.index()][descriptor_slot];
                if(one){
                    if(!SceneModelBindings::finish_fresh_empty_material_animation(*arena.descriptors,descriptor,archive,error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
                    update.empty_animation_traversed=true;
                }else if(i<2&&!SceneModelBindings::set_fresh_diffuse_rgb(*arena.descriptors,descriptor,i?std::array<u8,3>{255,0,0}:std::array<u8,3>{0,255,0},error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            }
            update.generation=1;update.initialized=true;storage_->initial_hud_objects[ordinal]=lease;*storage_->initial_child_prefix=next;
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain initializer prefix retained; no replay");}
    }
    bool advance_initial_briefing_groups(const BattleInitialBriefingWidgetProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=25||prefix->group_base!=56||prefix->slot!=6||prefix->next.present()||storage_->state.scene45!=1){if(error)*error="initial briefing groups: consumed known HUD chain/current scene1 required";return false;}
        const auto* state=storage_->physical.class2_pool()->snapshot();
        if(state->group_heads[58].present()||state->group_tails[58].present()){if(error)*error="initial briefing groups: occupied group3A cannot be skipped";return false;}
        auto next=*prefix;next.group_base=57;next.next=state->group_heads[57];*storage_->initial_child_prefix=next;
        if(error){error->clear();}return true;
    }
    bool begin_initial_briefing_widget(const BattleInitialBriefingWidgetProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("initial briefing widget: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=25||prefix->group_base!=57||prefix->slot!=6||storage_->state.scene45!=1||storage_->stage_construction.object_world_generation())return reject("actual first scene1/group39 cursor before world dispatch required");
        const auto widgets=storage_->widgets.storage_;auto& models=storage_->briefing;
        if(!widgets||!widgets->current()||widgets->first_widget_prefix_started||widgets->models_preparation_claimed||widgets->attempts.empty()||widgets->leases.empty()||models.status()!=BattlePreparationModelsStatus::ResourcesBound||!models.resources_current())return reject("same unclaimed widget/briefing resource owner required");
        auto objects=widgets->pool;auto& arena=*objects->storage_;const auto link=prefix->next;const auto& attempt=widgets->attempts.front();const auto& lease=widgets->leases.front();
        if(objects!=storage_->physical.class2_pool()||attempt.result!=SceneObjectAcquire::Allocated||attempt.type!=4||attempt.argument!=3||attempt.descriptors!=3||lease.index()!=attempt.object_index||lease.generation()!=attempt.object_generation||link!=ScenePhysicalLink{2,s32(lease.index())}||arena.state.group_heads[57]!=link)return reject("actual first type4/argument3 allocation and source group head required");
        const auto& r=arena.state.records[lease.index()];
        if(!r.allocated||r.active00!=1||r.group01!=57||r.kind83!=0x15||r.link4.present()||!r.header_installed||r.header!=SceneObjectHeader{0,4,3,0x80199c0c,0x8019b228}||r.actor_child||r.briefing_widget_prefix||r.descriptor_count99!=3||r.registration85!=255||r.callback100_key||r.callback_previous104.present()||r.callback_next108.present())return reject("unchanged first source widget header/storage required");
        const auto& archive=models.storage_->archive;const auto* bank=archive.bank();
        if(!bank||!bank->same_source(models.storage_->bank)||bank->hash()!=0x22e678b4||bank->records()!=27||!archive.belongs_to(*arena.descriptors))return reject("actual resource-first briefing archive required");
        for(unsigned slot=0;slot<3;++slot)if(SceneModelBindings::object_root(*objects,lease,slot))return reject("all three actual fresh descriptor roots must be unbound");
        BattleInitialBriefingWidgetPrefix next;if(!battle_initial_briefing_widget_prefix(profile,attempt.argument,&next,error))return false;
        HsdJointInstanceBank roots;const std::array<u16,3> records{18,16,17};
        if(!roots.construct_normal_records(*bank,records,error))return false;
        try{
            widgets->models_preparation_claimed=widgets->first_widget_prefix_started=true;
            auto& current=arena.state.records[lease.index()];current.briefing_widget_prefix.emplace();auto& partial=*current.briefing_widget_prefix;
            partial.phase18=1;partial.render82=0x12;partial.timer1c=179;partial.material_step4c=next.material_step4c;partial.rate7e=0xa0;partial.rate80=0x80;
            for(unsigned slot=0;slot<3;++slot){
                widgets->first_widget_roots[slot]=roots.receipt(slot);HsdJointInstanceOwner root;
                if(!roots.take_instance(slot,&root,error)||!SceneModelBindings::attach_object_descriptor(*objects,lease,slot,archive,records[slot],std::move(root),error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            }
            for(unsigned slot=0;slot<2;++slot)if(!SceneModelBindings::request_fresh_material_texture_zero(*arena.descriptors,arena.leases[lease.index()][slot+1],archive,&widgets->first_widget_animation[slot],error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            partial=next;
            //19B094 now reads actual selected controller words. No implicit zero
            //input, counter1653 decrement, dispatch completion or cursor advance.
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain source pre-input prefix retained; no replay");}
    }
    bool bind_initial_briefing_input(const BattleInitialBriefingWidgetProfile& profile,const SourceMainDigitalInputOwner& input,const std::array<const ForceInventory*,2>& owners,std::string* error){
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=25||prefix->group_base!=57||prefix->slot!=6||storage_->state.scene45!=1){if(error)*error="initial briefing input: actual first widget/source scene1 boundary required";return false;}
        const auto widgets=storage_->widgets.storage_;const auto receipt=input.receipt();
        if(!widgets||!widgets->current()||!widgets->first_widget_prefix_started||widgets->leases.empty()||widgets->first_widget_input_bound||!receipt.covers(storage_->party.humans(),0x1f70)){if(error)*error="initial briefing input: one current explicit MAIN digital sample covering the actual human ports required";return false;}
        const auto& first=widgets->leases.front();
        if(prefix->next!=ScenePhysicalLink{2,s32(first.index())}){if(error)*error="initial briefing input: actual first widget cursor required";return false;}
        //Pin this exact source/sample without consuming a controller word. A
        //later publish, retired owner or different stream cannot replace it.
        widgets->first_widget_input=receipt;widgets->first_widget_input_bound=true;
        if(error){error->clear();}return true;
    }
    bool finish_initial_briefing_widget(const BattleInitialBriefingWidgetProfile& profile,const SourceMainDigitalInputReceipt& input,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("initial briefing input: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=25||prefix->group_base!=57||prefix->slot!=6||storage_->state.scene45!=1||storage_->stage_construction.object_world_generation())return reject("actual first widget pre-input cursor required");
        const auto widgets=storage_->widgets.storage_;
        if(!widgets||!widgets->current()||!widgets->first_widget_prefix_started||!widgets->first_widget_input_bound||!input.same_sample(widgets->first_widget_input))return reject("the SAME privately bound, still-current MAIN sample is required");
        auto objects=widgets->pool;auto& arena=*objects->storage_;const auto& lease=widgets->leases.front();const auto link=ScenePhysicalLink{2,s32(lease.index())};
        if(prefix->next!=link||arena.state.group_heads[57]!=link)return reject("actual source group39 head required");
        auto& r=arena.state.records[lease.index()];
        if(!r.briefing_widget_prefix||!r.briefing_widget_prefix->awaiting_input||r.briefing_widget_prefix->update_complete)return reject("unconsumed controller-word branch required");
        u32 pressed=0;
        if(!input.masked_pressed(storage_->party.humans(),0x1f70,&pressed))return reject("known pressed-word subset/history for every selected port required");
        BattleInitialBriefingInputResult result;
        if(!battle_initial_briefing_input(profile,*r.briefing_widget_prefix,pressed,&result,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        if(result.decrement_counter1653)widgets->counter1653=u8(unsigned(widgets->counter1653)-1u);
        *r.briefing_widget_prefix=result.state;*storage_->initial_child_prefix=next;
        //Only widget0's actual dispatch has returned. Other occupied widgets,
        //89554, world/render callbacks and the enclosing transition stay pending.
        if(error){error->clear();}return true;
    }
    bool advance_initial_briefing_type0(const BattleInitialBriefingType0Profile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("briefing type0: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=26||prefix->group_base!=57||prefix->slot!=6||storage_->state.scene45!=1||storage_->stage_construction.object_world_generation())return reject("actual widget1 cursor after completed widget0 dispatch required");
        const auto widgets=storage_->widgets.storage_;auto& models=storage_->briefing;
        if(!widgets||!widgets->current()||widgets->type0_widget_initialized||widgets->attempts.size()<2||widgets->leases.size()<2||models.status()!=BattlePreparationModelsStatus::ResourcesBound||!models.resources_current())return reject("same current widget/briefing owners required");
        auto objects=widgets->pool;auto& arena=*objects->storage_;const auto& attempt=widgets->attempts[1];const auto& lease=widgets->leases[1];const auto link=ScenePhysicalLink{2,s32(lease.index())};const auto first=ScenePhysicalLink{2,s32(widgets->leases.front().index())};
        if(objects!=storage_->physical.class2_pool()||attempt.result!=SceneObjectAcquire::Allocated||attempt.type||attempt.argument||attempt.descriptors!=2||lease.index()!=attempt.object_index||lease.generation()!=attempt.object_generation||prefix->next!=link||arena.state.group_heads[57]!=first||arena.state.records[unsigned(first.index)].link8!=link)return reject("actual next type0 allocation/unchanged source head and predecessor required");
        const auto& r=arena.state.records[lease.index()];
        if(!r.allocated||r.active00!=1||r.group01!=57||r.kind83!=0x15||r.link4!=first||!r.header_installed||r.header!=SceneObjectHeader{0,0,0,0x80199c0c,0x8019a064}||r.actor_child||r.briefing_type0_update||r.briefing_widget_prefix||r.descriptor_count99!=2||r.registration85!=255||r.callback100_key||r.callback_previous104.present()||r.callback_next108.present())return reject("unchanged fresh source type0 header/storage required");
        const auto& archive=models.storage_->archive;const auto* bank=archive.bank();
        if(!bank||!bank->same_source(models.storage_->bank)||bank->hash()!=0x22e678b4||bank->records()!=27||!archive.belongs_to(*arena.descriptors))return reject("actual resource-first briefing archive required");
        for(unsigned slot=0;slot<2;++slot)if(SceneModelBindings::object_root(*objects,lease,slot))return reject("actual descriptor roots must be unbound");
        BattleInitialBriefingType0State next_state;if(!battle_initial_briefing_type0(profile,&next_state,error))return false;
        HsdJointInstanceBank roots;const std::array<u16,2> records{2,3};if(!roots.construct_normal_records(*bank,records,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        try{
            widgets->type0_widget_initialized=true;
            auto& current=arena.state.records[lease.index()];current.briefing_type0_update.emplace();current.briefing_type0_update->phase18=1;
            for(unsigned slot=0;slot<2;++slot){
                widgets->type0_widget_roots[slot]=roots.receipt(slot);HsdJointInstanceOwner root;
                if(!roots.take_instance(slot,&root,error)||!SceneModelBindings::attach_object_descriptor(*objects,lease,slot,archive,records[slot],std::move(root),error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            }
            current.briefing_type0_update->fields144_174=next_state.fields144_174;
            if(!SceneModelBindings::set_fresh_material_alpha(*arena.descriptors,arena.leases[lease.index()][0],0,error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            *current.briefing_type0_update=next_state;*storage_->initial_child_prefix=next;
            //199DC0 returns here. Do not run phase1/199EA8, read new input,
            //decrement counter1653, register callbacks or advance a world.
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain initializer prefix retained; no replay");}
    }
    bool advance_initial_briefing_type1(const BattleInitialBriefingType1Profile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("briefing type1: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);const auto entry=storage_?storage_->entry.lock():nullptr;
        if(!profile.ready()||!prefix||prefix->updates!=27||prefix->group_base!=57||prefix->slot!=6||storage_->state.scene45!=1||storage_->stage_construction.object_world_generation()||!entry||!entry->live||entry->state.common32!=1||!storage_->party.initial_wave46_)return reject("actual type1 cursor with fresh V3 mode lineage and committed AI wave required");
        BattleInitialBriefingType1State state;
        if(!battle_initial_briefing_type1(profile,*entry->state.common32,entry->state.common64,*storage_->party.initial_wave46_,&state,error))return false;
        const auto widgets=storage_->widgets.storage_;auto& models=storage_->briefing;
        if(!widgets||!widgets->current()||widgets->type1_widget_initialized||widgets->attempts.size()<3||widgets->leases.size()<3||models.status()!=BattlePreparationModelsStatus::ResourcesBound||!models.resources_current())return reject("same live widget/briefing owners required");
        auto objects=widgets->pool;auto& arena=*objects->storage_;auto& scheduler=storage_->physical.storage_->scheduler;
        const auto& attempt=widgets->attempts[2];const auto& lease=widgets->leases[2];const auto link=ScenePhysicalLink{2,s32(lease.index())};const auto prior=ScenePhysicalLink{2,s32(widgets->leases[1].index())};const auto first=ScenePhysicalLink{2,s32(widgets->leases[0].index())};
        if(objects!=storage_->physical.class2_pool()||attempt.result!=SceneObjectAcquire::Allocated||attempt.type!=1||attempt.argument||attempt.descriptors!=3||lease.index()!=attempt.object_index||lease.generation()!=attempt.object_generation||prefix->next!=link||arena.state.group_heads[57]!=first||arena.state.records[unsigned(prior.index)].link8!=link)return reject("actual next type1 allocation/unchanged source head and predecessor required");
        const auto& r=arena.state.records[lease.index()];
        if(!r.allocated||r.active00!=1||r.group01!=57||r.kind83!=0x15||r.link4!=prior||!r.header_installed||r.header!=SceneObjectHeader{0,1,0,0x80199c0c,0x8019a330}||r.actor_child||r.briefing_type1_update||r.descriptor_count99!=3||r.registration85!=255||r.callback100_key||r.callback_previous104.present()||r.callback_next108.present())return reject("unchanged fresh source type1 header/storage required");
        if(scheduler.callback_heads[4].present()||scheduler.callback_tails[4].present())return reject("actual first selector20 callback list must be empty; occupied unjoined tails are not skipped");
        const auto& archive=models.storage_->archive;const auto* bank=archive.bank();
        if(!bank||!bank->same_source(models.storage_->bank)||bank->hash()!=0x22e678b4||bank->records()!=27||!archive.belongs_to(*arena.descriptors))return reject("actual resource-first briefing archive required");
        for(unsigned slot=0;slot<3;++slot)if(SceneModelBindings::object_root(*objects,lease,slot))return reject("actual descriptor roots must be unbound");
        HsdJointInstanceBank roots;const std::array<u16,3> records{0,1,1};if(!roots.construct_normal_records(*bank,records,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        try{
            widgets->type1_widget_initialized=true;auto& current=arena.state.records[lease.index()];current.briefing_type1_update.emplace();current.briefing_type1_update->phase18=1;
            for(unsigned slot=0;slot<3;++slot){
                widgets->type1_widget_roots[slot]=roots.receipt(slot);HsdJointInstanceOwner root;
                if(!roots.take_instance(slot,&root,error)||!SceneModelBindings::attach_object_descriptor(*objects,lease,slot,archive,records[slot],std::move(root),error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            }
            for(unsigned digit=0;digit<2;++digit){
                const auto& descriptor=arena.leases[lease.index()][digit+1];
                const bool ok=state.digit_visible[digit]?SceneModelBindings::request_fresh_material_texture_selector(*arena.descriptors,descriptor,archive,state.selectors[digit],&widgets->type1_digit_animation[digit],error):SceneModelBindings::set_fresh_descriptor_visibility(*arena.descriptors,descriptor,0,error);
                if(!ok){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            }
            *current.briefing_type1_update=state;
            current.callback100_key=0x80047aa4;current.registration85=0x20;current.callback_previous104=current.callback_next108={};
            scheduler.callback_heads[4]=scheduler.callback_tails[4]=link;
            *storage_->initial_child_prefix=next;
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain digit initializer prefix retained; no replay");}
    }
    bool advance_initial_briefing_type2(const BattleInitialBriefingType2Profile& profile,SourceHalfwordRngOwner& rng,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("briefing type2: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);const auto entry=storage_?storage_->entry.lock():nullptr;
        if(!profile.ready()||!prefix||prefix->updates<28||prefix->updates>=46||prefix->group_base!=57||prefix->slot!=6||storage_->state.scene45!=1||storage_->stage_construction.object_world_generation()||!entry||!entry->live||entry->state.common32!=1||storage_->party.initial_wave46_!=0)return reject("actual mode1/first-wave type2 source cursor required");
        const unsigned ordinal=prefix->updates-28;const auto widgets=storage_->widgets.storage_;auto& models=storage_->briefing;
        if(!widgets||!widgets->current()||widgets->type2_initialized!=ordinal||widgets->attempts.size()<21||widgets->leases.size()<21||models.status()!=BattlePreparationModelsStatus::ResourcesBound||!models.resources_current())return reject("same live widget/briefing owners and exact attempted prefix required");
        for(unsigned i=0;i<18;++i){const auto& a=widgets->attempts[3+i];const auto& l=widgets->leases[3+i];if(a.result!=SceneObjectAcquire::Allocated||a.type!=2||a.argument!=i||a.descriptors!=3||a.object_index!=l.index()||a.object_generation!=l.generation())return reject("missing source allocations remain unjoined, never a selected occupied subset");}
        BattleInitialBriefingType2Facts facts;facts.active=storage_->party.active();facts.humans=storage_->party.humans();for(unsigned i=0;i<6;++i)facts.teams[i]=storage_->party.team(i);
        for(unsigned i=0;i<2;++i){facts.counts[i]=storage_->services.team_fields.teams[i].members;facts.slots[i]=storage_->services.team_fields.teams[i].slots;}
        BattleInitialBriefingType2State state;if(!battle_initial_briefing_type2(profile,u8(ordinal),facts,&state,error))return false;
        if(state.active&&!state.variant12&&(!entry->halfword_rng||!rng.same_owner(*entry->halfword_rng)))return reject("actual independent halfword stream must have been bound BEFORE setup callbacks; no late seed or byte-RNG substitution");
        auto objects=widgets->pool;auto& arena=*objects->storage_;auto& scheduler=storage_->physical.storage_->scheduler;
        const auto& lease=widgets->leases[3+ordinal];const auto link=ScenePhysicalLink{2,s32(lease.index())};const auto prior=ScenePhysicalLink{2,s32(widgets->leases[2+ordinal].index())};const auto first=ScenePhysicalLink{2,s32(widgets->leases[0].index())};
        if(prefix->next!=link||arena.state.group_heads[57]!=first||arena.state.records[unsigned(prior.index)].link8!=link)return reject("unchanged actual group39 head/predecessor required");
        const auto& r=arena.state.records[lease.index()];
        if(!r.allocated||r.active00!=1||r.group01!=57||r.kind83!=0x15||r.link4!=prior||r.header!=SceneObjectHeader{0,2,u8(ordinal),0x80199c0c,0x8019ac1c}||r.descriptor_count99!=3||r.actor_child||r.briefing_type2_update||r.registration85!=255||r.callback100_key)return reject("unchanged fresh source type2 object required");
        const auto expected_tail=ordinal?widgets->type2_callback_tail:ScenePhysicalLink{2,s32(widgets->leases[2].index())};const auto tail=scheduler.callback_tails[4];
        if(scheduler.callback_heads[4]!=ScenePhysicalLink{2,s32(widgets->leases[2].index())}||tail!=expected_tail||tail.physical!=2||tail.index<0||unsigned(tail.index)>=arena.state.records.size())return reject("actual retained selector20 list required");
        const auto& last=arena.state.records[unsigned(tail.index)];if(!last.allocated||last.active00!=1||last.registration85!=0x20||last.callback100_key!=0x80047aa4||last.callback_next108.present())return reject("actual callback20 append tail required");
        const auto& archive=models.storage_->archive;const auto* bank=archive.bank();
        if(!bank||!bank->same_source(models.storage_->bank)||bank->hash()!=0x22e678b4||bank->records()!=27||!archive.belongs_to(*arena.descriptors))return reject("actual resource-first briefing archive required");
        for(unsigned slot=0;slot<3;++slot)if(SceneModelBindings::object_root(*objects,lease,slot))return reject("all actual initial roots must be unbound");
        std::vector<u16> records{0};if(state.active)for(unsigned slot=0;slot<2;++slot)if(state.records[slot]>=0)records.push_back(u16(state.records[slot]));
        HsdJointInstanceBank roots;if(!roots.construct_normal_records(*bank,records,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        try{
            auto& current=arena.state.records[lease.index()];current.briefing_type2_update.emplace();current.briefing_type2_update->phase18=1;
            auto bind=[&](unsigned instance,unsigned slot,u16 record){widgets->type2_roots[ordinal][slot]=roots.receipt(instance);HsdJointInstanceOwner root;return roots.take_instance(instance,&root,error)&&SceneModelBindings::attach_object_descriptor(*objects,lease,slot,archive,record,std::move(root),error);};
            if(!bind(0,2,0)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            current.header.argument12=state.variant12;
            if(state.active){
                unsigned instance=1;
                for(unsigned slot=0;slot<2;++slot)if(state.records[slot]>=0){
                    if(!bind(instance++,slot,u16(state.records[slot]))){status_=BattlePreparationBootstrapStatus::Failed;return false;}
                    if(!state.variant12){
                        if(!rng.next_halfword(&state.random[slot])){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain actual halfword draw; consumed prefix retained");}
                        state.selectors[slot]=u8(state.random[slot]&7u);auto& animation=widgets->type2_animation[ordinal][slot];const auto& descriptor=arena.leases[lease.index()][slot];
                        if(!SceneModelBindings::request_fresh_material_texture_selector(*arena.descriptors,descriptor,archive,f32(state.selectors[slot]),&animation,error)||!SceneModelBindings::set_requested_texture_diffuse_rgb(*arena.descriptors,descriptor,animation,slot?std::array<u8,3>{0,0,0}:std::array<u8,3>{255,255,255},error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
                    }
                }
                current.callback100_key=0x80047aa4;current.registration85=0x20;current.callback_previous104=tail;current.callback_next108={};
                arena.state.records[unsigned(tail.index)].callback_next108=link;scheduler.callback_tails[4]=link;widgets->type2_callback_tail=link;
            }else{widgets->counter1653=u8(unsigned(widgets->counter1653)-1u);widgets->type2_callback_tail=tail;}
            *current.briefing_type2_update=state;++widgets->type2_initialized;*storage_->initial_child_prefix=next;
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain source prefix retained; no replay or RNG restoration");}
    }
    bool advance_initial_briefing_type5(const BattleInitialBriefingType5Profile& profile,const ForceEditorArt& icons,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("briefing type5: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);const auto entry=storage_?storage_->entry.lock():nullptr;
        if(!profile.ready()||!prefix||prefix->updates<46||prefix->updates>=82||prefix->group_base!=57||prefix->slot!=6||storage_->state.scene45!=1||storage_->stage_construction.object_world_generation()||!entry||!entry->live||entry->state.common32!=1||storage_->party.initial_wave46_!=0||!owners[0]||!profile.mapping_valid(icons,*owners[0]->catalog()))return reject("actual mode1/card cursor and complete source icon mapping required");
        const unsigned ordinal=prefix->updates-46;const auto widgets=storage_->widgets.storage_;
        if(!widgets||!widgets->current()||widgets->type5_initialized!=ordinal||widgets->attempts.size()<57||widgets->leases.size()<57||!storage_->common_ui.current())return reject("same live widgets/unitall archive required");
        for(unsigned i=0;i<36;++i){const auto& a=widgets->attempts[21+i];const auto& l=widgets->leases[21+i];if(a.result!=SceneObjectAcquire::Allocated||a.type!=5||a.argument!=i||a.descriptors!=5||a.object_index!=l.index()||a.object_generation!=l.generation())return reject("missing original allocations remain unjoined, not a selected subset");}
        BattleInitialBriefingType2Facts facts;facts.active=storage_->party.active();facts.humans=storage_->party.humans();std::array<u8,6> counts{};
        for(unsigned i=0;i<6;++i){facts.teams[i]=storage_->party.team(i);counts[i]=u8(storage_->party.members(i).size());}
        for(unsigned i=0;i<2;++i){facts.counts[i]=storage_->services.team_fields.teams[i].members;facts.slots[i]=storage_->services.team_fields.teams[i].slots;}
        BattleInitialBriefingType5State state;if(!battle_initial_briefing_type5(profile,u8(ordinal),facts,counts,&state,error))return false;
        const auto members=storage_->party.members(state.slot96);
        for(unsigned slot=0;slot<state.count1b;++slot){const auto& member=members[state.member_begin+slot];const int record=icons.icon(member.fields.model);if(record<0||record>=202||member.fields.variant>=6||member.fields.level>=10)return reject("current unedited source member/icon record required");state.records[slot]=s16(record);state.models[slot]=member.fields.model;}
        auto objects=widgets->pool;auto& arena=*objects->storage_;auto& scheduler=storage_->physical.storage_->scheduler;
        const auto& lease=widgets->leases[21+ordinal];const auto link=ScenePhysicalLink{2,s32(lease.index())};const auto prior=ScenePhysicalLink{2,s32(widgets->leases[20+ordinal].index())};const auto first=ScenePhysicalLink{2,s32(widgets->leases[0].index())};
        if(prefix->next!=link||arena.state.group_heads[57]!=first||arena.state.records[unsigned(prior.index)].link8!=link)return reject("unchanged actual group39 head/predecessor required");
        const auto& r=arena.state.records[lease.index()];
        if(!r.allocated||r.active00!=1||r.group01!=57||r.kind83!=0x15||r.link4!=prior||r.header!=SceneObjectHeader{0,5,u8(ordinal),0x80199c0c,0x8019b718}||r.descriptor_count99!=5||r.actor_child||r.briefing_type5_update||r.registration85!=255||r.callback100_key)return reject("unchanged fresh source card object required");
        const auto expected_tail=ordinal?widgets->type5_callback_tail:widgets->type2_callback_tail;const auto tail=scheduler.callback_tails[4];
        if(scheduler.callback_heads[4]!=ScenePhysicalLink{2,s32(widgets->leases[2].index())}||tail!=expected_tail||tail.physical!=2||tail.index<0||unsigned(tail.index)>=arena.state.records.size())return reject("actual retained callback list4 required");
        const auto& last=arena.state.records[unsigned(tail.index)];if(!last.allocated||last.registration85!=0x20||last.callback100_key!=0x80047aa4||last.callback_next108.present())return reject("actual append tail required");
        const auto& archive=storage_->common_ui.storage_->archive;const auto* bank=archive.bank();
        if(!bank||!bank->same_source(storage_->common_ui.storage_->bank)||bank->hash()!=0x87d2f859||bank->records()!=202||!archive.belongs_to(*arena.descriptors))return reject("same actual unitall/default-root archive required");
        for(unsigned slot=0;slot<5;++slot)if(SceneModelBindings::object_root(*objects,lease,slot))return reject("all actual source card descriptors must be unbound");
        std::vector<u16> records;for(unsigned slot=0;slot<state.count1b;++slot)records.push_back(u16(state.records[slot]));
        HsdJointInstanceBank roots;if(!roots.construct_normal_records(*bank,records,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        try{
            auto& current=arena.state.records[lease.index()];current.briefing_type5_update.emplace();current.briefing_type5_update->phase18=1;
            for(unsigned slot=0;slot<state.count1b;++slot){widgets->type5_roots[ordinal][slot]=roots.receipt(slot);HsdJointInstanceOwner root;if(!roots.take_instance(slot,&root,error)||!SceneModelBindings::attach_object_descriptor(*objects,lease,slot,archive,u16(state.records[slot]),std::move(root),error)||!SceneModelBindings::disable_fresh_default_culling(*arena.descriptors,arena.leases[lease.index()][slot],error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}}
            current.header.argument12=state.page12;
            if(state.active){current.callback100_key=0x80047aa4;current.registration85=0x20;current.callback_previous104=tail;current.callback_next108={};arena.state.records[unsigned(tail.index)].callback_next108=link;scheduler.callback_tails[4]=link;widgets->type5_callback_tail=link;}
            else{widgets->counter1653=u8(unsigned(widgets->counter1653)-1u);widgets->type5_callback_tail=tail;}
            *current.briefing_type5_update=state;++widgets->type5_initialized;*storage_->initial_child_prefix=next;if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain card prefix retained; no replay");}
    }
    bool advance_initial_briefing_type6(const BattleInitialBriefingType6Profile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("briefing type6: ")+why;return false;};
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=82||prefix->group_base!=57||prefix->slot!=6||storage_->state.scene45!=1||storage_->stage_construction.object_world_generation())return reject("actual final-widget cursor after all36 cards required");
        const auto widgets=storage_->widgets.storage_;auto& models=storage_->briefing;
        if(!widgets||!widgets->current()||widgets->type6_initialized||widgets->attempts.size()!=58||widgets->leases.size()!=58||models.status()!=BattlePreparationModelsStatus::ResourcesBound||!models.resources_current())return reject("original same widget/briefing owners required");
        const auto& attempt=widgets->attempts[57];const auto& lease=widgets->leases[57];auto objects=widgets->pool;auto& arena=*objects->storage_;auto& scheduler=storage_->physical.storage_->scheduler;const auto link=ScenePhysicalLink{2,s32(lease.index())};const auto prior=ScenePhysicalLink{2,s32(widgets->leases[56].index())};const auto first=ScenePhysicalLink{2,s32(widgets->leases[0].index())};
        if(attempt.result!=SceneObjectAcquire::Allocated||attempt.type!=6||attempt.argument||attempt.descriptors!=1||attempt.object_index!=lease.index()||attempt.object_generation!=lease.generation()||prefix->next!=link||arena.state.group_heads[57]!=first||arena.state.records[unsigned(prior.index)].link8!=link)return reject("actual original argument0 allocation and list order required");
        const auto& r=arena.state.records[lease.index()];
        if(!r.allocated||r.active00!=1||r.group01!=57||r.kind83!=0x15||r.link4!=prior||r.header!=SceneObjectHeader{0,6,0,0x80199c0c,0x8019bae8}||r.descriptor_count99!=1||r.briefing_type6_update||r.registration85!=255||r.callback100_key)return reject("unchanged original source final widget required");
        const auto tail=scheduler.callback_tails[4];if(tail!=widgets->type5_callback_tail||tail.physical!=2||tail.index<0||unsigned(tail.index)>=arena.state.records.size())return reject("actual card callback tail required");
        const auto& last=arena.state.records[unsigned(tail.index)];if(last.registration85!=0x20||last.callback100_key!=0x80047aa4||last.callback_next108.present())return reject("actual selector20 append correspondence required");
        const auto& archive=models.storage_->archive;const auto* bank=archive.bank();
        if(!bank||bank->hash()!=0x22e678b4||!bank->same_source(models.storage_->bank)||!archive.belongs_to(*arena.descriptors)||SceneModelBindings::object_root(*objects,lease,0))return reject("same default briefing archive/unbound descriptor required");
        BattleInitialBriefingType6State state;if(!battle_initial_briefing_type6(profile,attempt.argument,&state,error))return false;
        HsdJointInstanceBank roots;const std::array<u16,1> records{21};if(!roots.construct_normal_records(*bank,records,error))return false;
        auto next=*prefix;++next.updates;next.next=r.link8;
        try{
            widgets->type6_initialized=true;auto& current=arena.state.records[lease.index()];current.briefing_type6_update.emplace();current.briefing_type6_update->phase18=1;
            widgets->counter1653=u8(unsigned(widgets->counter1653)-1u);
            current.callback100_key=0x80047aa4;current.registration85=0x20;current.callback_previous104=tail;current.callback_next108={};arena.state.records[unsigned(tail.index)].callback_next108=link;scheduler.callback_tails[4]=link;
            widgets->type6_root=roots.receipt(0);HsdJointInstanceOwner root;
            if(!roots.take_instance(0,&root,error)||!SceneModelBindings::attach_object_descriptor(*objects,lease,0,archive,21,std::move(root),error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
            *current.briefing_type6_update=state;*storage_->initial_child_prefix=next;if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain final-widget prefix retained; no replay");}
    }
    bool finish_initial_child_updates(const BattleInitialBriefingType6Profile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=83||prefix->group_base!=57||prefix->next.present()||prefix->all_child_updates_complete||!storage_->widgets.type6_widget_state()){if(error)*error="child update end: actual consumed final widget/empty suffix required";return false;}
        const auto* objects=storage_->physical.class2_pool()->snapshot();const auto widgets=storage_->widgets.storage_;
        if(objects->group_tails[57]!=ScenePhysicalLink{2,s32(widgets->leases[57].index())}||objects->group_heads[58].present()||objects->group_tails[58].present()){if(error)*error="child update end: occupied/changed source tail cannot be skipped";return false;}
        storage_->initial_child_prefix->all_child_updates_complete=true;
        //This closes ONLY the initial group-update segment, not89554, graphics,
        //counter1653==0, a full29434/286D8 transition or a renderable frame.
        if(error){error->clear();}return true;
    }
    const BattleInitialPostWorldState* initial_post_world(const std::array<const ForceInventory*,2>& owners)const{
        const auto* world=initial_world_dispatch(owners);
        if(!world||!world->complete||!world->stage_aggregate_released||!storage_->initial_post_world)return nullptr;
        const auto& state=*storage_->initial_post_world;const auto entry=storage_->entry.lock();
        return entry&&entry->live&&state.generation==1&&state.common66==0&&entry->state.common66==state.common66&&entry->state.counterb0==state.after&&storage_->state.counterb0==state.after&&entry->state.native_revision==state.entry_revision&&storage_->state.native_revision==state.scene_revision?&state:nullptr;
    }
    bool advance_initial_world_counter(const BattleInitialPostWorldProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="initial29434 counter: same fresh completed89554/entry/normal66 owner required";return false;};
        const auto* world=initial_world_dispatch(owners);
        if(!profile.ready()||!world||!world->complete||!world->stage_aggregate_released||storage_->initial_post_world||storage_->stage_construction.object_world_generation()!=1||storage_->collisions||storage_->publication||storage_->views.pose_generation())return reject();
        const auto entry=storage_->entry.lock();if(!entry||!entry->live||entry->state.common66||entry->state.counterb0||storage_->state.counterb0!=entry->state.counterb0||entry->state.native_revision==std::numeric_limits<u64>::max()||storage_->state.native_revision==std::numeric_limits<u64>::max())return reject();
        BattleInitialPostWorldState next;next.before=entry->state.counterb0;
        if(!battle_initial_post_world_counter(profile,next.before,&next.after,error))return false;
        next.generation=1;next.common66=entry->state.common66;next.entry_revision=entry->state.native_revision+1;next.scene_revision=storage_->state.native_revision+1;
        entry->state.counterb0=storage_->state.counterb0=next.after;entry->state.native_revision=next.entry_revision;storage_->state.native_revision=next.scene_revision;storage_->initial_post_world=next;
        //Only the first caller increment. No collider/contact/UI/camera/draw or
        //enclosing29434/286D8 return is implied by these source counter stores.
        if(error){error->clear();}return true;
    }
    bool begin_initial_world_dispatch(const BattleInitialWorldDispatchProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        const auto* prefix=first_child_update_prefix(owners);
        if(!profile.ready()||!prefix||prefix->updates!=83||!prefix->all_child_updates_complete||prefix->next.present()||storage_->initial_world_dispatch||storage_->stage_construction.object_world_generation()||storage_->publication||storage_->state.scene45!=1){if(error)*error="initial89554: exact completed initial updates/current first actors required";return false;}
        try{auto next=std::make_unique<BattleInitialWorldDispatchState>();next->head=next->next=storage_->physical.storage_->scheduler.callback_heads[0];next->visits.reserve(32);storage_->initial_world_dispatch=std::move(next);if(error){error->clear();}return true;}
        catch(const std::exception&){if(error)*error="initial89554: cursor allocation failed";return false;}
    }
    const BattleInitialWorldDispatchState* initial_world_dispatch(const std::array<const ForceInventory*,2>& owners)const{
        return first_child_update_prefix(owners)&&storage_->initial_world_dispatch?storage_->initial_world_dispatch.get():nullptr;
    }
    const SceneObjectWorldCallbackState* first_callback_object_world(const SceneObjectLease& lease,const std::array<const ForceInventory*,2>& owners)const{
        const auto* dispatch=initial_world_dispatch(owners);if(!dispatch)return nullptr;
        const auto objects=storage_->physical.class2_pool();const auto* r=objects->inspect(lease);if(!r||!r->world_callback)return nullptr;
        for(const auto& visit:dispatch->visits)if(visit.object==ScenePhysicalLink{2,s32(lease.index())}&&visit.matrix_written)return &*r->world_callback;
        return nullptr;
    }
    bool advance_initial_world_callback(const BattleInitialWorldDispatchProfile& profile,const BattleStageObjectUpdateProfile& side_profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        return advance_initial_world_callback_impl(profile,side_profile,nullptr,nullptr,owners,error);
    }
    bool advance_initial_shape_world_callback(const BattleInitialWorldDispatchProfile& profile,const BattleStageObjectUpdateProfile& side_profile,const BattleInitialShapeWorldProfile& shape_profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(!shape_profile.ready()){if(error)*error="initial world: explicit static-shape source profile required";return false;}
        return advance_initial_world_callback_impl(profile,side_profile,&shape_profile,nullptr,owners,error);
    }
    bool advance_initial_world_tail_callback(const BattleInitialWorldDispatchProfile& profile,const BattleStageObjectUpdateProfile& side_profile,const BattleInitialShapeWorldProfile& shape_profile,const BattleInitialWorldTailProfile& tail_profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(!shape_profile.ready()||!tail_profile.ready()){if(error)*error="initial world tail: explicit shape and inactive auxiliary profiles required";return false;}
        return advance_initial_world_callback_impl(profile,side_profile,&shape_profile,&tail_profile,owners,error);
    }
    bool finish_initial_world_dispatch(const BattleInitialWorldTailProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="initial89554 end: unchanged complete occupied-list traversal and owned first worlds required";return false;};
        const auto* state=initial_world_dispatch(owners);
        if(!profile.ready()||!state||!state->traversal_exhausted||state->complete||state->stage_aggregate_released||state->next.present()||state->list_cursor!=BattleInitialWorldDispatchProfile::source_lists.size()||storage_->stage_construction.object_world_generation()||storage_->publication||storage_->views.pose_generation())return reject();
        const auto& physical=*storage_->physical.storage_;const auto objects=physical.class2;const auto& arena=*objects->storage_;const auto& scheduler=physical.scheduler;
        std::size_t ordinal=0;unsigned matrices=0,side_matrices=0;
        for(u8 list:BattleInitialWorldDispatchProfile::source_lists){
            ScenePhysicalLink previous;auto link=scheduler.callback_heads[list];
            while(link.present()){
                if(ordinal>=state->visits.size())return reject();
                const auto& visit=state->visits[ordinal++];ScenePhysicalLink before,next;u32 key=0;u8 registration=255;
                if(link.physical==2&&link.index>=0&&unsigned(link.index)<arena.state.records.size()){
                    const auto& r=arena.state.records[unsigned(link.index)];if(!r.allocated)return reject();before=r.callback_previous104;next=r.callback_next108;key=r.callback100_key;registration=r.registration85;
                    if(visit.matrix_written){if(!r.world_callback||r.world_callback->generation!=1||r.world_callback->parent_world_generation!=1)return reject();}
                    else if(r.world_callback)return reject();
                }else if(link.physical==3&&link.index>=0&&unsigned(link.index)<arena.class3.records.size()){
                    const auto& r=arena.class3.records[unsigned(link.index)];if(!r.allocated)return reject();before=r.callback_previous104;next=r.callback_next108;key=r.callback100_key;registration=r.registration85;
                    if(!visit.matrix_written||key!=0x80082060||side_matrices>=storage_->stage_construction.object_world_order().size()||storage_->stage_construction.object_world_order()[side_matrices++]!=link)return reject();
                }else return reject();
                if(before!=previous||visit.object!=link||visit.source_list!=list||visit.callback!=key)return reject();
                const u8 expected=list<2?list:u8(0x10+list-6);if(registration!=expected)return reject();
                matrices+=visit.matrix_written;previous=link;link=next;
            }
            if(previous!=scheduler.callback_tails[list])return reject();
        }
        if(ordinal!=state->visits.size()||ordinal!=state->callbacks||matrices!=state->matrices||side_matrices!=storage_->stage_construction.object_world_order().size())return reject();
        if(!storage_->stage_construction.release_initial_callback_worlds())return reject();
        storage_->initial_world_dispatch->complete=storage_->initial_world_dispatch->stage_aggregate_released=true;
        //Only89554 is complete.89658/UI, collision/visibility, model publication,
        //presentation history and the enclosing preparation transition are not.
        if(error){error->clear();}return true;
    }
private:
    bool advance_initial_world_callback_impl(const BattleInitialWorldDispatchProfile& profile,const BattleStageObjectUpdateProfile& side_profile,const BattleInitialShapeWorldProfile* shape_profile,const BattleInitialWorldTailProfile* tail_profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("initial89554 callback: ")+why;return false;};
        const auto* cursor=initial_world_dispatch(owners);
        if(!profile.ready()||!cursor||cursor->traversal_exhausted||cursor->complete||storage_->stage_construction.object_world_generation()||storage_->publication||storage_->views.pose_generation())return reject("current incomplete first source dispatcher required");
        auto& physical=*storage_->physical.storage_;auto objects=physical.class2;auto& arena=*objects->storage_;const auto& scheduler=physical.scheduler;const auto link=cursor->next;
        if(cursor->source_list!=BattleInitialWorldDispatchProfile::source_lists[cursor->list_cursor]||scheduler.callback_heads[cursor->source_list]!=cursor->head||!link.present())return reject("actual retained source list/head/cursor required");
        ScenePhysicalLink before,next;u32 key=0;u8 registration=255;
        if(link.physical==2&&link.index>=0&&unsigned(link.index)<arena.state.records.size()){const auto& r=arena.state.records[unsigned(link.index)];if(!r.allocated)return reject("retired class2 callback");before=r.callback_previous104;next=r.callback_next108;key=r.callback100_key;registration=r.registration85;}
        else if(link.physical==3&&link.index>=0&&unsigned(link.index)<arena.class3.records.size()){const auto& r=arena.class3.records[unsigned(link.index)];if(!r.allocated)return reject("retired class3 callback");before=r.callback_previous104;next=r.callback_next108;key=r.callback100_key;registration=r.registration85;}
        else return reject("unjoined callback physical class/range");
        if(before!=cursor->previous||(!next.present()&&scheduler.callback_tails[cursor->source_list]!=link))return reject("actual callback predecessor/tail correspondence");
        for(const auto& visit:cursor->visits)if(visit.object==link)return reject("callback cycle/replay");
        AttachmentBoneTransform transform;bool matrix=false,side=false,shape=false;SceneObjectLease follower;BattleInitialShapeWorld shape_world;
        if(cursor->source_list==0&&link.physical==2&&key==0x80047aa4&&registration==0){
            const bool point=link==ScenePhysicalLink{2,s32(storage_->initial_child_object.index())};
            if(!point&&link!=ScenePhysicalLink{2,s32(storage_->initial_field_child_object.index())})return reject("unknown occupied list0 callback");
            follower=point?storage_->initial_child_object:storage_->initial_field_child_object;
            const auto parent=physical.constructors[0].scope_.lock();
            if(!parent||!parent->owner||!parent->owner->prepare_initial_follower_world(profile,follower,point,owners,&transform,error))return false;
            matrix=true;
        }else if(cursor->source_list==1&&link.physical==3&&key==0x80082060&&registration==1){
            if(!storage_->stage_construction.prepare_side_world_callback(side_profile,storage_->physical,link,&transform,error))return false;
            matrix=side=true;
        }else if(cursor->source_list==1&&link==ScenePhysicalLink{2,s32(storage_->initial_bar_object.index())}&&key==0x8013c144&&registration==1){
            const auto* r=objects->inspect(storage_->initial_bar_object);
            if(!r||!r->bar_child_update||!r->bar_child_update->initialized||r->bar_child_update->mask1e||r->bar_child_update->allocation!=SceneScratchAcquire::Allocated||!storage_->initial_bar_scratch.current())return reject("only genuinely registered zero-mask bar callback is joined");
            //13C144 reads the parent but writes NO position/matrix/scratch data
            //on mask1E==0. Do not manufacture an identity or initialize scratch.
        }else if(shape_profile&&link.physical==2&&cursor->source_list>=6&&key==0x8001c5ac){
            const auto& r=arena.state.records[unsigned(link.index)];const auto* c=r.actor_child?&*r.actor_child:nullptr;
            if(!c||c->index96>=4||registration!=0x10+c->index96)return reject("actual initialized shape source owner required");
            const auto parent=physical.constructors[c->index96].scope_.lock();if(!parent||!parent->owner)return reject("retired shape parent");
            for(const auto& attempt:parent->owner->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::Shape&&attempt.object.index()==unsigned(link.index))follower=attempt.object;
            const auto* stage=stage_resources(owners);if(!stage||!stage->initial_light_constructor())return reject("current owned stage grids/light alias required");
            if(!parent->owner->prepare_initial_shape_world(*shape_profile,follower,*stage->inputs(),*stage->initial_light_constructor(),owners,&shape_world,error))return false;
            transform.world=shape_world.world114;transform.position=shape_world.position20;transform.valid=true;matrix=shape=true;
        }else if(shape_profile&&link.physical==2&&(cursor->source_list==7||cursor->source_list==9)&&key==0x80047aa4){
            const auto& r=arena.state.records[unsigned(link.index)];const auto* c=r.actor_child?&*r.actor_child:nullptr;
            if(!c||(c->index96!=1&&c->index96!=3)||registration!=0x10+c->index96||!r.bank_child_update)return reject("actual initialized bank attachment required");
            const auto parent=physical.constructors[c->index96].scope_.lock();if(!parent||!parent->owner)return reject("retired bank parent");
            for(const auto& attempt:parent->owner->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::BankAttachment&&attempt.object.index()==unsigned(link.index))follower=attempt.object;
            if(!parent->owner->prepare_initial_bank_world(profile,follower,owners,&transform,error))return false;
            matrix=true;
        }else if(tail_profile&&cursor->source_list==9&&link.physical==2&&key==0x800c79f8&&registration==0x13){
            const auto parent=physical.constructors[3].scope_.lock();if(!parent||!parent->owner)return reject("retired actual auxiliary parent");
            for(const auto& attempt:parent->owner->storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&attempt.kind==BattleActorChildKind::CommonAuxiliary&&attempt.object.index()==unsigned(link.index))follower=attempt.object;
            if(!parent->owner->qualify_initial_inactive_auxiliary_world(*tail_profile,follower,owners,error))return false;
            //The source zero-channel branch returns before all inline writes.
        }else return reject("occupied source callback requires its typed consumer; no suffix skipping");
        try{
            auto advance=*cursor;advance.next=next;advance.previous=link;++advance.callbacks;if(matrix)++advance.matrices;
            advance.visits.push_back({link,cursor->source_list,key,matrix});
            while(!advance.next.present()){
                if(scheduler.callback_tails[advance.source_list]!=advance.previous)return reject("changed source tail at list end");
                if(++advance.list_cursor>=BattleInitialWorldDispatchProfile::source_lists.size()){
                    if(!tail_profile)return reject("full89554 completion requires explicit tail qualification");
                    advance.traversal_exhausted=true;break;
                }
                advance.source_list=BattleInitialWorldDispatchProfile::source_lists[advance.list_cursor];advance.previous={};advance.head=advance.next=scheduler.callback_heads[advance.source_list];
                if(!advance.next.present()&&scheduler.callback_tails[advance.source_list].present())return reject("nonempty tail of empty source list");
            }
            if(matrix){
                if(side){if(!storage_->stage_construction.commit_side_world_callback(link,transform)){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain side matrix commit");}storage_->stage_construction.storage_->world_order.push_back(link);}
                else{auto& r=arena.state.records[follower.index()];r.world_callback=SceneObjectWorldCallbackState{transform.world,transform.position,1,1};if(shape){r.shape_child_update->first_world=shape_world;r.shape_child_update->numbers.visibility144=shape_world.visibility144;}}
            }
            *storage_->initial_world_dispatch=std::move(advance);
            //Keep aggregate stage world generation0 until ALL89554 callbacks
            //complete. Partial matrices cannot authorize collisions/cameras.
            if(error){error->clear();}return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain callback prefix retained; no replay");}
    }
public:
    const BattleInitialChildPrefixState* first_child_update_prefix(const std::array<const ForceInventory*,2>& owners)const{
        if(!stage_actor_prefix_current(owners)||!storage_->initial_child_prefix||!storage_->initial_child_object.current())return nullptr;
        const auto* record=storage_->physical.class2_pool()->inspect(storage_->initial_child_object);
        if(!record||!record->point_child_update||!record->point_child_update->initialized)return nullptr;
        if(storage_->initial_child_prefix->updates>=2){const auto* field=storage_->physical.class2_pool()->inspect(storage_->initial_field_child_object);if(!field||!field->field_child_update||!field->field_child_update->initialized)return nullptr;}
        for(unsigned i=0;i<2&&storage_->initial_child_prefix->updates>2+i;++i){
            const auto objects=storage_->physical.class2_pool();const auto& lease=storage_->initial_sasuke_child_objects[i];const auto* child=objects->inspect(lease);
            if(!child||!child->bank_child_update||!child->bank_child_update->initialized)return nullptr;
            for(unsigned style=0;style<2;++style){const auto* root=SceneModelBindings::object_root(*objects,lease,style);if(!root||!root->current()||root->state()->record!=child->bank_child_update->record)return nullptr;}
        }
        for(unsigned i=0;i<2&&storage_->initial_child_prefix->updates>4+i;++i){
            const auto objects=storage_->physical.class2_pool();const auto& lease=storage_->initial_gred_child_objects[i];const auto* child=objects->inspect(lease);
            if(!child||!child->bank_child_update||!child->bank_child_update->initialized)return nullptr;
            for(unsigned style=0;style<2;++style){const auto* root=SceneModelBindings::object_root(*objects,lease,style);if(!root||!root->current()||root->state()->record!=child->bank_child_update->record)return nullptr;}
        }
        if(storage_->initial_child_prefix->updates>=7){
            const auto* shape=storage_->physical.class2_pool()->inspect(storage_->initial_shape_child_object);const auto* lights=storage_->stage_resources.initial_light_constructor();
            if(!shape||!shape->shape_child_update||!shape->shape_child_update->initialized||!lights||lights->generation!=shape->shape_child_update->light_constructor_generation)return nullptr;
        }
        for(unsigned i=0;i<7;++i){
            const unsigned completed_at=i<5?8+i:15+(i-5);
            if(storage_->initial_child_prefix->updates<completed_at)continue;
            const auto* shape=storage_->physical.class2_pool()->inspect(storage_->continuation_shape_objects[i]);const auto* lights=storage_->stage_resources.initial_light_constructor();
            if(!shape||!shape->shape_child_update||!shape->shape_child_update->initialized||!lights||lights->generation!=shape->shape_child_update->light_constructor_generation)return nullptr;
        }
        for(unsigned i=0;i<2&&storage_->initial_child_prefix->updates>12+i;++i){
            const auto objects=storage_->physical.class2_pool();const auto& lease=storage_->initial_common_aux_objects[i];const auto* aux=objects->inspect(lease);
            if(!aux||!aux->common_auxiliary_update||!aux->common_auxiliary_update->initialized||aux->common_auxiliary_update->matrix54.owner!=ScenePhysicalLink{2,s32(lease.index())}||aux->common_auxiliary_update->matrix54.generation!=lease.generation())return nullptr;
            for(unsigned slot=0;slot<2;++slot){const auto* root=SceneModelBindings::object_root(*objects,lease,slot);if(!root||!root->current()||root->state()->record!=51)return nullptr;}
        }
        if(storage_->initial_child_prefix->updates>=17){
            const auto* bar=storage_->physical.class2_pool()->inspect(storage_->initial_bar_object);
            if(!bar||!bar->bar_child_update||!bar->bar_child_update->initialized)return nullptr;
            if(bar->bar_child_update->allocation==SceneScratchAcquire::Allocated&&!storage_->initial_bar_scratch.current())return nullptr;
        }
        for(unsigned i=0;i<8&&storage_->initial_child_prefix->updates>17+i;++i){
            const auto objects=storage_->physical.class2_pool();const auto& lease=storage_->initial_hud_objects[i];const auto* hud=objects->inspect(lease);
            if(!hud||!hud->hud_child_update||!hud->hud_child_update->initialized)return nullptr;
            for(unsigned slot=0;slot<hud->descriptor_count99;++slot){
                const auto* root=SceneModelBindings::object_root(*objects,lease,slot);const u16 record=(i&1u)?(slot==2||slot==3?81:slot==4?50:0):56;
                if(record){if(!root||!root->current()||root->state()->record!=record)return nullptr;}
                else if(root)return nullptr;
            }
        }
        if(storage_->initial_child_prefix->updates>=26){
            const auto* widget=storage_->widgets.first_widget_prefix();if(!widget||!widget->update_complete||widget->awaiting_input)return nullptr;
        }
        if(storage_->initial_child_prefix->updates>=27&&!storage_->widgets.type0_widget_state())return nullptr;
        if(storage_->initial_child_prefix->updates>=28&&!storage_->widgets.type1_widget_state())return nullptr;
        if(storage_->initial_child_prefix->updates>28&&storage_->widgets.initialized_type2_widgets()!=std::min<unsigned>(18,storage_->initial_child_prefix->updates-28))return nullptr;
        if(storage_->initial_child_prefix->updates>46&&storage_->widgets.initialized_type5_widgets()!=std::min<unsigned>(36,storage_->initial_child_prefix->updates-46))return nullptr;
        if(storage_->initial_child_prefix->updates>=83&&!storage_->widgets.type6_widget_state())return nullptr;
        return storage_->initial_child_prefix.get();
    }
    bool advance_stage_first_object_updates(const BattleStageObjectUpdateProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!initial_order_current(owners)||storage_->publication||!first_camera_views(owners)){if(error)*error="scene stage update: actual first-camera/pre-spawn actor boundary required";return false;}
        try{return storage_->stage_construction.advance_first_object_updates(profile,storage_->physical,error);}
        catch(const std::exception&){return storage_->stage_construction.fail(error,"uncertain source group40 continuation");}
    }
    bool activate_first_camera_views(const BattleCameraEntryProfile& profile,const std::array<BattleActorConstruction*,6>& actors,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(storage_&&storage_->collisions){const auto* prefix=first_contact_prefix(owners);if(!prefix||!prefix->pre_camera_visibility_complete){if(error)*error="first camera: collider contact/visibility continuation remains pending";return false;}}
        if(storage_&&storage_->initial_post_world){if(error)*error="first camera: typed29434 actor-late/group3B continuation remains pending";return false;}
        auto reject=[&](const char* why){if(error)*error=std::string("scene first camera views: ")+why;return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!initial_order_current(owners)||storage_->initial_order->phase!=InitialActorOrderPhase::StageEntered||!storage_->views.initial_aliases_bound()||storage_->stage_construction.phase()!=BattleStageConstructionPhase::InitializedAwaitFrame||!storage_->stage_construction.current()||!profile.ready())return reject("same completed actor/stage prefix and initial view aliases required");
        const auto entry=storage_->entry.lock();if(!entry||entry->state.common64)return reject("normal camera branch required");
        const auto& physical=*storage_->physical.storage_;const auto batch=storage_->characters.batch_;const auto* input=storage_->views.scene_inputs();
        std::array<BattleViews::ActorObservation,6> observed;
        for(unsigned i=0;i<6;++i){
            if(!(input->active&(1u<<i))){
                if(actors[i]||physical.constructor_claimed[i]||batch->construction_claimed[i]||physical.actors.slots[i].active00)return reject("inactive cell must retain its actual factory state");
                continue;
            }
            const auto scope=physical.constructors[i].scope_.lock();
            if(!physical.constructor_claimed[i]||!batch->construction_claimed[i]||!scope||!scope->live||!actors[i]||scope->owner!=actors[i]||!scope->owner->current(owners)||scope->owner->status()!=BattleActorConstructionStatus::FirstWorldReady)return reject("exact mutable registered first-world actors required");
            const auto& body=*scope->owner->storage_;const auto& state=body.state;
            if(body.allocation.batch_.lock()!=batch||state.slot3e4!=i||state.native_revision!=19||state.world_generation!=1||state.camera_generation||state.view_behaviour43d||state.field3f1||state.view_slot487!=storage_->views.storage_->actor487[i])return reject("fresh matching actor/view fields required");
            CharacterProgramClassFields fields;if(!body.initial_program||!body.initial_program->vm.observe_class_fields(&fields))return reject("current shared VM camera group required");
            auto& p=observed[i];p.phase18=state.phase18;p.behaviour43d=state.view_behaviour43d;p.field3f1=state.field3f1;p.reference=physical.constructors[i];p.world_generation=state.world_generation;
            p.input.actor_position=state.position20;p.input.values88c=state.scalar_effects.values88c;p.input.offset6d0=state.offset6d0;p.input.scale_b4=state.scalar_defaults.scale_b4;p.input.heading72=state.angles70[1];p.input.group582=fields.default_group;p.input.layout29=input->layout29;p.input.split3d=input->split3d.value_or(0);
        }
        try{
            for(auto* actor:actors)if(actor)actor->storage_->journal.reserve(actor->storage_->journal.size()+2);
            auto commit=[&](unsigned slot,s16 heading){
                auto* actor=actors[slot];if(!actor||!actor->current(owners)||actor->storage_->state.camera_generation)return false;
                auto& body=*actor->storage_;body.state.field3f1=1;body.state.camera_heading5b0=heading;body.state.camera_generation=1;
                body.journal.push_back({BattleActorBirthWriteKind::Byte,0x3f1,1,1});body.journal.push_back({BattleActorBirthWriteKind::Half,0x5b0,2,std::bit_cast<u16>(heading)});return true;
            };
            return storage_->views.activate_first_poses(profile,observed,storage_->common.binding(),commit,error);
        }catch(const std::exception&){if(storage_->views.storage_)storage_->views.storage_->scope->live=false;return reject("uncertain camera prefix; no replay");}
    }
    const BattleViews* current_camera_views(const std::array<const ForceInventory*,2>& owners)const{
        if(!current(owners)||!storage_->views.first_poses_ready())return nullptr;
        const auto& views=*storage_->views.storage_;
        for(unsigned i=0;i<6;++i)if(views.inputs->active&(1u<<i)){
            const auto scope=views.actors[i].scope_.lock();if(!scope||!scope->live||!scope->owner||!scope->owner->current(owners)||views.actors[i]!=storage_->physical.storage_->constructors[i])return nullptr;
            const auto& state=scope->owner->storage_->state;
            if(state.world_generation!=views.actor_worlds[i]||state.camera_generation!=views.pose_generation||state.field3f1!=1||state.camera_heading5b0!=views.actor_headings[i])return nullptr;
        }
        return &storage_->views;
    }
    const BattleViews* first_camera_views(const std::array<const ForceInventory*,2>& owners)const{const auto* views=current_camera_views(owners);return views&&views->pose_generation()==1?views:nullptr;}
    bool advance_spawn_camera_views(const BattleCameraFollowProfile& profile,const std::array<BattleActorConstruction*,6>& actors,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene spawn camera: ")+why;return false;};
        if(storage_&&storage_->collisions&&!spawn_visibility(owners))return reject("post-spawn collider/contact/active-view visibility continuation remains pending");
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!initial_order_current(owners)||storage_->initial_order->phase!=InitialActorOrderPhase::StageEntered||storage_->views.pose_generation()!=1||storage_->stage_construction.phase()!=BattleStageConstructionPhase::InitializedAwaitFrame||!storage_->stage_construction.current()||!profile.ready())return reject("same initialized scene/first views/current source recipe required");
        if(storage_->stage_construction.object_world_generation()&&storage_->stage_construction.object_world_generation()!=2)return reject("opted-in stage objects require their post-spawn callback worlds");
        const auto entry=storage_->entry.lock();if(!entry||entry->state.common64)return reject("normal source camera branch required");
        const auto& physical=*storage_->physical.storage_;const auto* inputs=storage_->views.scene_inputs();const auto batch=storage_->characters.batch_;
        std::array<BattleViews::ActorObservation,6> observed;
        for(unsigned i=0;i<6;++i){
            if(!(inputs->active&(1u<<i))){if(actors[i]||physical.constructor_claimed[i]||batch->construction_claimed[i]||physical.actors.slots[i].active00)return reject("inactive factory correspondence");continue;}
            const auto scope=physical.constructors[i].scope_.lock();
            if(!scope||!scope->live||!actors[i]||scope->owner!=actors[i]||!actors[i]->current(owners)||actors[i]->status()!=BattleActorConstructionStatus::SpawnWorldReady)return reject("exact mutable registered spawn worlds required");
            const auto& body=*actors[i]->storage_;const auto& s=body.state;
            if(body.allocation.batch_.lock()!=batch||s.slot3e4!=i||s.native_revision!=20||s.world_generation!=2||s.camera_generation!=1||s.field3f1!=1||s.view_behaviour43d||s.camera_target508_bound||s.camera_lock7c9||s.view_slot487!=storage_->views.storage_->actor487[i])return reject("first-follow actor branch has changed");
            CharacterProgramClassFields fields;if(!body.initial_program||!body.initial_program->vm.observe_class_fields(&fields))return reject("current shared VM group required");
            auto& p=observed[i];p.phase18=s.phase18;p.behaviour43d=s.view_behaviour43d;p.field3f1=s.field3f1;p.reference=physical.constructors[i];p.world_generation=2;p.target50c=s.camera_target50c;p.width668=s.dimensions.field668;p.target508_bound=s.camera_target508_bound;p.lock7c9=s.camera_lock7c9;
            p.input.actor_position=s.position20;p.input.values88c=s.scalar_effects.values88c;p.input.offset6d0=s.offset6d0;p.input.scale_b4=s.scalar_defaults.scale_b4;p.input.heading72=s.angles70[1];p.input.group582=fields.default_group;p.input.layout29=inputs->layout29;p.input.split3d=inputs->split3d.value_or(0);
        }
        const auto* stage=storage_->stage_resources.inputs();const auto* seed=storage_->physical.stage_seed();if(!stage||!seed||stage->stage()!=seed->stage)return reject("same owned stage/grid required");
        try{
            for(auto* actor:actors)if(actor)actor->storage_->journal.reserve(actor->storage_->journal.size()+1);
            auto commit=[&](unsigned slot,s16 heading){
                auto* actor=actors[slot];if(!actor||!actor->current(owners)||actor->storage_->state.camera_generation!=1)return false;
                actor->storage_->state.camera_heading5b0=heading;actor->storage_->state.camera_generation=2;
                actor->storage_->journal.push_back({BattleActorBirthWriteKind::Half,0x5b0,2,std::bit_cast<u16>(heading)});return true;
            };
            return storage_->views.follow_first_spawn(profile,observed,*stage,seed->bounds38_44,seed->high34,seed->low68,commit,error);
        }catch(const std::exception&){storage_->views.storage_->scope->live=false;return reject("uncertain source continuation; no replay");}
    }
    bool begin_stage_side_construction(const BattleStageSideProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)){if(error)*error="scene stage construction: same drained scene required";return false;}
        if(storage_->initial_order&&(!initial_order_current(owners)||storage_->initial_order->phase!=InitialActorOrderPhase::ActorsComplete)){if(error)*error="scene stage construction: actual ordered first worlds must finish first";return false;}
        if(!stage_actor_prefix_current(owners)){if(error)*error="scene stage construction: current registered first-world actor prefix required";return false;}
        const bool result=storage_->stage_construction.begin_side(profile,storage_->physical,storage_->side_resources,storage_->common,error);
        if(storage_->initial_order){if(result)storage_->initial_order->phase=InitialActorOrderPhase::StageEntered;else if(storage_->stage_construction.phase()==BattleStageConstructionPhase::Failed)storage_->initial_order->phase=InitialActorOrderPhase::Failed;}return result;
    }
    const BattleStageConstruction* stage_construction(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->stage_construction.current()?&storage_->stage_construction:nullptr;}
    bool finish_stage_construction(const BattleStagePropsProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)){if(error)*error="scene stage construction: same drained scene required";return false;}
        if(storage_->initial_order&&(!initial_order_current(owners)||storage_->initial_order->phase!=InitialActorOrderPhase::StageEntered)){if(error)*error="scene stage construction: ordered actor prefix no longer current";return false;}
        if(!stage_actor_prefix_current(owners)){if(error)*error="scene stage construction: actor prefix changed during stage initialization";return false;}
        return storage_->stage_construction.finish_props_and_roots(profile,storage_->physical,storage_->stage_resources,error);
    }
    bool begin_actor_first_update(BattleActorConstruction& actor,BattleCharacterBinding&& prepared,const ChallengeCharacterDescriptorReceipt& receipt,const BattleActorBirthProfile& birth,const BattleActorFirstUpdateProfile& caller,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(!caller.ready()||status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||receipt.slot_>=6||receipt.batch_.lock()!=storage_->characters.batch_||(storage_->physical.controls()->d2&(1u<<receipt.slot_))){if(error)*error="scene first actor update: current unmasked fresh caller required";return false;}
        return begin_registered_actor(actor,std::move(prepared),receipt,birth,&caller,owners,error);
    }
    bool finish_actor_first_update(BattleActorConstruction& actor,const BattleGroundWorldProfile& world,const BattleGroundIkProfile& ik,const BattleLookTailProfile& look,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="scene first actor world: same current registered caller/stage required";return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||!actor.current(owners))return reject();
        if(storage_->initial_order&&(!initial_order_current(owners)||!storage_->initial_order->inflight||*storage_->initial_order->inflight!=actor.storage_->state.slot3e4))return reject();
        const auto& body=*actor.storage_;const auto slot=body.state.slot3e4;const auto& physical=*storage_->physical.storage_;
        if(slot>=6||body.allocation.batch_.lock()!=storage_->characters.batch_||!physical.constructor_claimed[slot])return reject();
        const auto scope=physical.constructors[slot].scope_.lock();const auto* stage=storage_->stage_resources.inputs();const auto* seed=storage_->physical.stage_seed();const auto* controls=storage_->physical.controls();
        if(!scope||!scope->live||scope->owner!=&actor||!stage||!seed||!controls||stage->stage()!=storage_->party.stage().stage)return reject();
        if(!actor.finish_first_update(world,ik,look,*stage,seed->stage,seed->bounds38_44,*controls,owners,error))return false;
        if(storage_->initial_order){auto& order=*storage_->initial_order;order.inflight.reset();++order.cursor;order.phase=order.cursor==physical.scheduler.initial_actor_order->count?InitialActorOrderPhase::ActorsComplete:InitialActorOrderPhase::Ordered;}
        return true;
    }
    bool advance_actor_spawn_entry(BattleActorConstruction& actor,const BattleActorSpawnEntryProfile& profile,const BattleGroundWorldProfile& world,const BattleGroundIkProfile& ik,const BattleLookTailProfile& look,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="scene spawn entry: same current registered actor/stage required";return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||!actor.current(owners))return reject();
        if(storage_->initial_order&&(!initial_order_current(owners)||storage_->initial_order->phase!=InitialActorOrderPhase::StageEntered))return reject();
        if(storage_->views.initial_aliases_bound()&&!storage_->views.first_poses_ready())return reject();
        if(storage_->views.storage_&&storage_->views.storage_->pose_attempted&&!storage_->views.first_poses_ready())return reject();
        if(storage_->stage_construction.phase()!=BattleStageConstructionPhase::Closed&&(storage_->stage_construction.phase()!=BattleStageConstructionPhase::InitializedAwaitFrame||!storage_->stage_construction.current()))return reject();
        if(storage_->stage_construction.object_world_generation()&&!storage_->stage_construction.first_object_updates_complete())return reject();
        const auto& body=*actor.storage_;const auto slot=body.state.slot3e4;const auto& physical=*storage_->physical.storage_;
        if(slot>=6||body.allocation.batch_.lock()!=storage_->characters.batch_||!physical.constructor_claimed[slot])return reject();
        const auto scope=physical.constructors[slot].scope_.lock();const auto* stage=storage_->stage_resources.inputs();const auto* controls=storage_->physical.controls();
        if(!scope||!scope->live||scope->owner!=&actor||!stage||!controls||stage->stage()!=storage_->party.stage().stage)return reject();
        return actor.advance_spawn_entry(profile,world,ik,look,*stage,*controls,owners,error);
    }
    bool advance_actor_initial_target(BattleActorConstruction& actor,const BattleActorInitialTargetProfile& profile,SourceByteRngOwner& rng,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene initial target: ")+why;return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||!actor.current(owners)||actor.status()!=BattleActorConstructionStatus::AwaitTargetSelection)return reject("current drained scene/target boundary required");
        const auto& physical=*storage_->physical.storage_;const auto batch=storage_->characters.batch_;const auto slot=actor.storage_->state.slot3e4;
        if(slot>=6||actor.storage_->allocation.batch_.lock()!=batch||!physical.constructor_claimed[slot])return reject("same registered scene body required");
        const auto self=physical.constructors[slot].scope_.lock();if(!self||!self->live||self->owner!=&actor)return reject("current registered self required");
        DeclaredBattleTargetSlots observed;observed.declared=true;
        for(unsigned i=0;i<6;++i){
            auto& p=observed.slots[i];
            if(physical.constructor_claimed[i]){
                const auto scope=physical.constructors[i].scope_.lock();
                if(!scope||!scope->live||!scope->owner||!scope->owner->target_observation(owners,&p))return reject("registered body retired/failed/unrepresented");
                const auto& body=*scope->owner->storage_;if(body.allocation.batch_.lock()!=batch||body.state.slot3e4!=i)return reject("registered body lost scene/slot correspondence");
            }else{
                if(batch->construction_claimed[i])return reject("consumed slot lacks constructor registration");
                const auto& seed=physical.actors.slots[i];
                //537A8 actually cleared these physical fields. Party team
                //metadata is NOT team88 until541AC writes the constructing body.
                p.existence_declared=p.exists=p.activity_declared=p.kind_declared=p.normal_fields_declared=p.team_declared=p.position_declared=true;
                p.active00=seed.active00;p.kind83=seed.kind83;p.model3e8=seed.model3e8;
            }
        }
        return actor.advance_initial_target(profile,observed,rng,owners,error);
    }
    bool advance_actor_initial_placement(BattleActorConstruction& actor,const ChallengeBattleSpawnProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene first placement: ")+why;return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||!actor.current(owners)||actor.status()!=BattleActorConstructionStatus::AwaitPlacementHistory)return reject("current drained scene/placement boundary required");
        const auto& body=*actor.storage_;const auto& s=body.state;const auto slot=s.slot3e4;const auto batch=storage_->characters.batch_;const auto entry=storage_->entry.lock();
        if(slot>=6||s.team88>=2||body.allocation.batch_.lock()!=batch||!entry||entry->state.common50!=1||s.role490||s.mode4a0||s.first43b||s.placement_complete||s.native_revision!=15||s.team88!=storage_->party.team(slot)||storage_->state.native_revision==std::numeric_limits<u64>::max())return reject("same fresh normal scene body/team required");
        const auto& physical=*storage_->physical.storage_;const auto scope=physical.constructors[slot].scope_.lock();
        if(!physical.constructor_claimed[slot]||!scope||!scope->live||scope->owner!=&actor)return reject("current registered physical body required");
        //No collision update/registration is inferred from allocated class4
        //bodies.37B98's two actual source counts determine whether arrays are read.
        if(!storage_->physical.initial_dynamic_projection_empty())return reject("nonempty dynamic registries require their current shape consumers");
        const auto* stage=storage_->stage_resources.inputs();const auto* seed=storage_->physical.stage_seed();auto& team=storage_->services.team_fields.teams[s.team88];
        if(!stage||!stage->native_data_bound()||!seed||seed->stage!=stage->stage()||stage->stage()!=storage_->party.stage().stage||team.id!=s.team88||!(team.mask&s.mask3e5))return reject("same stage/grid owner and mutable team cursor required");
        const StageRayParameters ray{seed->bounds38_44[1],seed->bounds38_44[0],seed->bounds38_44[3],seed->bounds38_44[2],stage->parameters().cell_size,true};
        ChallengeSpawnActorFields observed;observed.declared=true;observed.mode490=s.role490;observed.first43b=s.first43b;observed.movement661=s.scalar_effects.field661;
        observed.ground={true,s.program_seed.flags[2],s.position20[1],s.previous2c[1],s.offset6d0,s.local_setup.field6d4};
        ChallengeFirstPlacementPlan placement;ChallengeSpawnStaticTail tail;const auto cursor=team.cursor_fe;
        if(!challenge_first_placement_plan(profile,*stage,observed,s.team88,cursor,&placement,error)||!challenge_spawn_static_tail(placement,*stage,ray,&tail,error))return false;
        if(!current(owners)||team.cursor_fe!=cursor||!storage_->physical.initial_dynamic_projection_empty())return reject("scene/team/registry changed during planning");
        if(!actor.commit_initial_placement(profile,placement,tail,owners,error))return false;
        team.cursor_fe=placement.next_cursor;++storage_->state.native_revision;if(error)error->clear();return true;
    }
    //Bounded first world-group0 callback entry.8C098 owns the three count
    //resets;8A16C clears render lists;53928 observes all six actual slots.
    //This does not grant camera, stage/child callback or complete scene authority.
    bool begin_actor_model_publication(const BattleModelPublicationProfile& profile,const ModelPassQueueProfile& queues,const std::array<const ForceInventory*,2>& owners,std::string* error){
        return begin_model_collection(profile,queues,false,owners,error);
    }
    //Bounded87814→87F38 prefix. All live normal views must select the same
    //normal root, so this does not invent common28/31/33 or choose a viewer.
    bool begin_stage_model_publication(const BattleStagePublicationProfile& stage_profile,const BattleModelPublicationProfile& profile,const ModelPassQueueProfile& queues,const std::array<const ForceInventory*,2>& owners,std::string* error){
        const auto* views=current_camera_views(owners);
        if(!stage_profile.ready()||!views||views->pose_generation()!=2||storage_->state.scene45!=1||storage_->stage_construction.phase()!=BattleStageConstructionPhase::InitializedAwaitFrame||!storage_->stage_construction.current()||storage_->stage_construction.object_world_generation()!=2||!storage_->stage_construction.first_object_updates_complete()){
            if(error)*error="scene stage publication: current initial stage/ordinary spawn cameras/source recipe required";
            return false;
        }
        const auto* controls=storage_->physical.controls();
        if(!controls||controls->d9){if(error)*error="scene stage publication: source sceneD9 branch unsupported";return false;}
        for(unsigned i=0;i<6;++i)if(views->scene_inputs()->active&(1u<<i)){
            const auto* v=views->view(i);
            if(!v||!v->active00||v->pose_generation!=2||v->alternate2e7){if(error)*error="scene stage publication: viewer-dependent shadow branch requires presentation selection";return false;}
        }
        return begin_model_collection(profile,queues,true,owners,error);
    }
    bool begin_stage_prop_publication(const BattleStagePublicationProfile& stage_profile,const BattlePropPublicationProfile& props,const BattleModelPublicationProfile& actors,const ModelPassQueueProfile& queues,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(!props.ready()||!storage_||!storage_->spawn_visibility||!storage_->spawn_visibility->complete||!storage_->spawn_contacts||!storage_->spawn_contacts->complete){if(error)*error="stage/prop publication: actual completed spawn contacts/visibility and source prop recipe required";return false;}
        if(!begin_stage_model_publication(stage_profile,actors,queues,owners,error))return false;
        auto& p=*storage_->publication;p.prop_callback_queued=true;
        //87814 queues the stage first.288FC then queues89EB4/89F28/89F9C
        //into groups0/1/2 respectively, before any8C1B0 execution.
        storage_->physical.storage_->scheduler.late_queue_counts={2,1,1};
        if(error){error->clear();}return true;
    }
private:
    bool contact_non_stage_groups_current()const{
        const auto& arena=*storage_->physical.class2_pool()->storage_;
        for(unsigned base:{32u,0u,16u,8u,24u})for(unsigned offset=0;offset<8;++offset){
            const unsigned group=base+offset;
            if(offset>0&&offset<7&&!storage_->physical.storage_->actors.slots[offset-1].active00){if(arena.state.group_heads[group].present()||arena.state.group_tails[group].present())return false;continue;}
            auto link=arena.state.group_heads[group];ScenePhysicalLink previous;unsigned count=0;
            while(link.present()){
                if(++count>512||link.physical!=2||unsigned(link.index)>=arena.state.records.size())return false;
                const auto& r=arena.state.records[unsigned(link.index)];
                if(!r.allocated||r.group01!=s16(group)||r.link4!=previous||(r.active00&&(!r.actor_child||!r.header_installed)))return false;
                previous=link;link=r.link8;
            }
            if(previous!=arena.state.group_tails[group])return false;
        }
        return true;
    }
    static CollisionContactState actor_contact_state(const BattleActorBirthState& s){
        CollisionContactState p;p.retained=s.contact_retained;
        const auto& flags=s.program_seed.flags;p.field1d9=flags[0];p.field1da=flags[1];p.attached1db=flags[2];p.previous1de=flags[3];p.field1dc=flags[4];p.moved1dd=flags[5];p.field280=flags[6];p.current274=flags[10];p.hidden270=s.hidden_views270;p.shadow286=s.shadow_views286;return p;
    }
    static void commit_actor_contact(BattleActorBirthState& s,const CollisionContactState& p){
        s.contact_retained=p.retained;auto& flags=s.program_seed.flags;flags[0]=p.field1d9;flags[1]=p.field1da;flags[2]=p.attached1db;flags[3]=p.previous1de;flags[4]=p.field1dc;flags[5]=p.moved1dd;flags[6]=p.field280;flags[10]=p.current274;s.hidden_views270=p.hidden270;s.shadow_views286=p.shadow286;
    }
    bool begin_model_collection(const BattleModelPublicationProfile& profile,const ModelPassQueueProfile& queues,bool stage_prefix,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene actor model collection: ")+why;return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||storage_->publication||!profile.ready()||!model_pass_queue_profile_valid(queues))return reject("fresh current scene/source callback and queue recipes required");
        if(storage_->views.initial_aliases_bound()&&(!current_camera_views(owners)||storage_->views.pose_generation()!=2))return reject("current first-spawn camera poses required before model collection");
        try{
            auto next=std::make_unique<ActorPublication>();if(!next->queue.load_global(queues))return reject("fresh shared model queue required");
            auto& physical=*storage_->physical.storage_;const auto batch=storage_->characters.batch_;
            for(unsigned i=0;i<6;++i){
                if(physical.constructor_claimed[i]){
                    const auto scope=physical.constructors[i].scope_.lock();
                    if(!scope||!scope->live||!scope->owner||!scope->owner->current(owners))return reject("stale registered physical body");
                    const auto& body=*scope->owner->storage_;const auto& s=body.state;
                    if(body.allocation.batch_.lock()!=batch||s.slot3e4!=i)return reject("same source body/slot correspondence required");
                    next->actors[i]=physical.constructors[i];next->registered[i]=true;next->worlds[i]=s.world_generation;next->revisions[i]=s.native_revision;
                    if(s.render_enabled82){
                        if(!s.active00||s.render_enabled82!=i+65||scope->owner->status()!=BattleActorConstructionStatus::SpawnWorldReady||s.world_generation!=2)return reject("visible body needs its current normal spawn world");
                        next->state.collected_mask|=u8(1u<<i);
                    }
                }else if(batch->construction_claimed[i])return reject("consumed slot cannot fall back to factory visibility");
                //Untouched factory82 is genuinely zero, including inactive cells.
            }
            if(!next->state.collected_mask)return reject("visible source actor required for this bounded callback entry");
            auto& scheduler=physical.scheduler;
            scheduler.late_queue_counts={0,0,0}; //8C098, not8853C
            scheduler.render_heads.fill({});scheduler.render_tails.fill({}); //8A16C, not a pool reset
            for(unsigned i=0;i<6;++i)if(next->state.collected_mask&(1u<<i)){
                const unsigned list=ScenePhysicalSchedulerSnapshot::actor_render_base+i;
                physical.banks[0].records[i].render_next110={};
                scheduler.render_heads[list]=scheduler.render_tails[list]={0,s32(i)};
            }
            if(stage_prefix){
                if(!storage_->stage_construction.collect_first_objects(storage_->physical,error)){status_=BattlePreparationBootstrapStatus::Failed;return false;}
                next->state.collected_objects=u16(storage_->stage_construction.object_update_order().size());
            }
            next->stage_prefix=stage_prefix;
            next->actor_callback_key=stage_prefix?0:0x8008a0a8;scheduler.late_queue_counts[0]=1; //8C0DC
            if(stage_prefix){next->stage_callback_key=0x80087f38;next->state.phase=BattleModelPublicationPhase::StageQueued;}
            storage_->publication=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattlePreparationBootstrapStatus::Failed;return reject("uncertain collection; no replay");}
    }
public:
    bool advance_stage_model_publication(const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene stage callback: ")+why;return false;};
        if(!publication_current(owners)||!storage_->publication->stage_prefix)return reject("same collected source stage prefix required");
        auto& p=*storage_->publication;
        if(p.state.phase!=BattleModelPublicationPhase::StageQueued||p.stage_callback_key!=0x80087f38||storage_->physical.storage_->scheduler.late_queue_counts[0]!=(p.prop_callback_queued?2u:1u))return reject("first queued87814 callback required");
        if(!p.queue.begin(1)||!storage_->stage_construction.publish_initial_normal_models(p.queue,p.scope,p.state.generation,error)){invalidate_publication();return false;}
        p.state.callback_cursor=1;p.state.phase=BattleModelPublicationPhase::AwaitObjectCallbacks;
        if(error){error->clear();}return true;
    }
    bool advance_prop_model_publication(const BattlePropPublicationProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene prop callback: ")+why;return false;};
        if(!profile.ready()||!publication_current(owners)||!storage_->publication->prop_callback_queued||storage_->publication->state.phase!=BattleModelPublicationPhase::AwaitObjectCallbacks||storage_->publication->state.callback_cursor!=1||storage_->publication->state.object_callback_cursor)return reject("unconsumed queued89EB4 after the actual stage callback required");
        if(storage_->physical.storage_->scheduler.late_queue_counts!=std::array<std::optional<u32>,3>{2,1,1})return reject("same source callback-prefix counts required");
        u8 active=0;const auto* views=current_camera_views(owners);if(!views)return reject("current camera poses required");
        for(unsigned i=0;i<6;++i)if(views->view(i)->active00)active|=u8(1u<<i);
        if(!storage_->stage_construction.publish_prop_draw_prefix(profile,storage_->physical,storage_->publication->queue,storage_->publication->scope,storage_->publication->state.generation,active,error)){invalidate_publication();return false;}
        storage_->publication->state.object_callback_cursor=48;if(error)error->clear();return true;
    }
    std::span<const BattlePropPublishedModel> published_prop_models(const std::array<const ForceInventory*,2>& owners)const{
        return publication_current(owners)&&storage_->stage_construction.prop_publication_current()?std::span<const BattlePropPublishedModel>{storage_->stage_construction.storage_->published_props}:std::span<const BattlePropPublishedModel>{};
    }
    std::optional<ScenePhysicalLink> next_object_draw(const std::array<const ForceInventory*,2>& owners)const{
        if(published_prop_models(owners).empty())return {};
        const auto next=storage_->stage_construction.storage_->next_object_draw;return next.present()?std::optional<ScenePhysicalLink>{next}:std::nullopt;
    }
    bool prepare_prop_published_geometry(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(published_prop_models(owners).size()!=48||storage_->publication->state.object_callback_cursor<48){if(error)*error="scene prop geometry: same current complete prop publication prefix required";return false;}
        return storage_->stage_construction.prepare_prop_published_geometry(error);
    }
    const AttachmentModelScene* published_prop_geometry(unsigned index,const std::array<const ForceInventory*,2>& owners)const{
        if(index>=48||published_prop_models(owners).size()!=48||!storage_->stage_construction.storage_->prop_geometry_prepared||storage_->stage_construction.storage_->prop_geometry.size()!=48)return nullptr;
        const auto& geometry=storage_->stage_construction.storage_->prop_geometry[index];return geometry.ready()?&geometry:nullptr;
    }
    bool advance_side_identity_publication(const BattleSidePublicationProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(!profile.ready()||published_prop_models(owners).size()!=48||storage_->publication->state.object_callback_cursor!=48||storage_->publication->state.callback_cursor!=1){if(error)*error="scene side prefix: current prop-complete89EB4 cursor required";return false;}
        const auto* views=current_camera_views(owners);if(!views)return false;u8 active=0;
        for(unsigned i=0;i<6;++i)if(views->view(i)->active00)active|=u8(1u<<i);
        auto& p=*storage_->publication;
        if(!storage_->stage_construction.publish_side_identity_prefix(profile,storage_->physical,p.queue,p.scope,p.state.generation,active,error)){invalidate_publication();return false;}
        p.state.object_callback_cursor=55;if(error)error->clear();return true;
    }
    std::span<const BattleSidePublishedModel> published_side_identity_models(const std::array<const ForceInventory*,2>& owners)const{
        return publication_current(owners)&&storage_->stage_construction.side_publication_current()?std::span<const BattleSidePublishedModel>{storage_->stage_construction.storage_->published_sides}:std::span<const BattleSidePublishedModel>{};
    }
    bool prepare_side_identity_geometry(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(published_side_identity_models(owners).size()!=7||storage_->publication->state.object_callback_cursor<55){if(error)*error="scene side geometry: current seven-object identity prefix required";return false;}
        return storage_->stage_construction.prepare_side_identity_geometry(error);
    }
    const AttachmentModelScene* published_side_identity_geometry(unsigned index,const std::array<const ForceInventory*,2>& owners)const{
        if(index>=7||published_side_identity_models(owners).size()!=7||!storage_->stage_construction.storage_->side_geometry_prepared)return nullptr;
        const auto& geometry=storage_->stage_construction.storage_->side_geometry[index];return geometry.ready()?&geometry:nullptr;
    }
    bool advance_first_presented_side(const BattleSidePublicationProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        const auto* presentation=first_graphics_presentation(owners);
        if(!profile.ready()||!presentation||presentation->phase!=BattleInitialPresentationPhase::WorldViewBegun||presentation->common31||presentation->common33||presentation->common34||presentation->alias||presentation->half||!presentation->history_restored||!publication_current(owners)||storage_->publication->state.object_callback_cursor!=55){if(error)*error="presented side callback: owned first source graphics selection/active render frame and side cursor55 required";return false;}
        auto& p=*storage_->publication;
        if(!storage_->stage_construction.publish_presented_last_side(profile,storage_->physical,p.queue,p.scope,p.state.generation,presentation->view,error)){invalidate_publication();return false;}
        p.presented_frame=presentation->render_frame;p.presented_view_bound=true;p.state.object_callback_cursor=56;
        //The known stage-object prefix ends here. Non-stage render collection,
        //8A010 and actor dispatch remain unjoined; do NOT acknowledge89EB4 done.
        if(error){error->clear();}return true;
    }
    const BattleSidePublishedModel* presented_side_model(const std::array<const ForceInventory*,2>& owners)const{
        return publication_current(owners)&&storage_->publication->presented_view_bound&&storage_->stage_construction.storage_->presented_side?&*storage_->stage_construction.storage_->presented_side:nullptr;
    }
    bool prepare_first_presented_side_geometry(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(!presented_side_model(owners)){if(error)*error="presented side geometry: current owned-view publication required";return false;}
        return storage_->stage_construction.prepare_presented_side_geometry(error);
    }
    const AttachmentModelScene* first_presented_side_geometry(const std::array<const ForceInventory*,2>& owners)const{
        if(!presented_side_model(owners)||!storage_->stage_construction.storage_->presented_side_geometry_prepared)return nullptr;
        const auto& geometry=storage_->stage_construction.storage_->presented_side_geometry;return geometry.ready()?&geometry:nullptr;
    }
    bool prepare_stage_published_geometry(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(!publication_current(owners)||storage_->publication->state.phase!=BattleModelPublicationPhase::AwaitObjectCallbacks){if(error)*error="scene stage geometry: current completed stage callback required";return false;}
        if(!storage_->stage_construction.prepare_published_geometry(error))return false;
        if(error){error->clear();}return true;
    }
    std::span<const BattleStagePublishedModel> published_stage_models(const std::array<const ForceInventory*,2>& owners)const{
        return publication_current(owners)&&storage_->publication->stage_prefix&&storage_->stage_construction.publication_current()?std::span<const BattleStagePublishedModel>{storage_->stage_construction.storage_->published}:std::span<const BattleStagePublishedModel>{};
    }
    const AttachmentModelScene* published_stage_geometry(unsigned index,const std::array<const ForceInventory*,2>& owners)const{
        if(index>=2||published_stage_models(owners).size()!=2||!storage_->stage_construction.storage_->geometry_prepared)return nullptr;
        return &storage_->stage_construction.storage_->geometry[index];
    }
    bool stage_model_passes(const std::array<const ForceInventory*,2>& owners,std::vector<ModelPassVisit>* out)const{
        if(out)out->clear();
        return out&&published_stage_models(owners).size()==2&&storage_->publication->state.phase==BattleModelPublicationPhase::AwaitObjectCallbacks&&storage_->publication->queue.plan(out);
    }
    std::optional<u8> next_actor_model_slot(const std::array<const ForceInventory*,2>& owners)const{
        if(!publication_current(owners))return std::nullopt;
        const auto& p=*storage_->publication;if(p.stage_prefix||p.state.phase==BattleModelPublicationPhase::Complete)return std::nullopt;
        for(unsigned i=p.next_slot;i<6;++i)if(p.state.collected_mask&(1u<<i))return u8(i);
        return std::nullopt;
    }
    bool advance_actor_model_publication(BattleActorConstruction& actor,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene actor model callback: ")+why;return false;};
        if(!publication_current(owners)){if(storage_&&storage_->publication)invalidate_publication();return reject("current collected actors/worlds required");}
        auto& p=*storage_->publication;if(p.stage_prefix)return reject("intervening source object callbacks remain pending");
        if(p.state.phase==BattleModelPublicationPhase::Complete)return reject("callback already consumed");
        const auto slot=next_actor_model_slot(owners);if(!slot)return reject("no actor callback pending");
        const auto source=p.actors[*slot].scope_.lock();
        if(!source||source->owner!=&actor)return reject("source list order and exact mutable body required");
        auto& physical=*storage_->physical.storage_;const unsigned list=ScenePhysicalSchedulerSnapshot::actor_render_base+*slot;
        if(physical.scheduler.render_heads[list]!=ScenePhysicalLink{0,*slot}||physical.scheduler.render_tails[list]!=ScenePhysicalLink{0,*slot}||physical.banks[0].records[*slot].render_next110.present()){invalidate_publication();return reject("unjoined render-list consumer");}
        if(p.state.phase==BattleModelPublicationPhase::Queued){if(p.actor_callback_key!=0x8008a0a8||physical.scheduler.late_queue_counts[0]!=u32(1)||!p.queue.begin(1)){invalidate_publication();return reject("actual queued8A0A8/09790 boundary required");}p.state.phase=BattleModelPublicationPhase::Running;}
        if(!actor.publish_models(p.queue,p.scope,p.state.generation,owners,error)){invalidate_publication();return false;}
        p.revisions[*slot]=actor.storage_->state.native_revision;p.next_slot=u8(*slot+1);++p.state.dispatched;
        bool more=false;for(unsigned i=p.next_slot;i<6;++i)more|=(p.state.collected_mask&(1u<<i))!=0;
        if(!more){p.state.phase=BattleModelPublicationPhase::Complete;p.state.callback_cursor=1;}
        if(error){error->clear();}return true;
    }
    const BattleModelPublicationState* actor_model_publication(const std::array<const ForceInventory*,2>& owners)const{return publication_current(owners)?&storage_->publication->state:nullptr;}
    bool prepare_actor_published_geometry(BattleActorConstruction& actor,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="scene actor geometry: completed current callback and exact unprepared actor required";return false;};
        if(!publication_current(owners)||storage_->publication->state.phase!=BattleModelPublicationPhase::Complete||!actor.current(owners))return reject();
        auto& p=*storage_->publication;const auto slot=actor.storage_->state.slot3e4;
        if(slot>=6||!(p.state.collected_mask&(1u<<slot))||(p.state.geometry_mask&(1u<<slot)))return reject();
        const auto scope=p.actors[slot].scope_.lock();if(!scope||scope->owner!=&actor)return reject();
        if(!actor.prepare_published_geometry(owners,error)){invalidate_publication();return false;}
        p.revisions[slot]=actor.storage_->state.native_revision;p.state.geometry_mask|=u8(1u<<slot);if(error)error->clear();return true;
    }
    bool actor_model_passes(const std::array<const ForceInventory*,2>& owners,std::vector<ModelPassVisit>* out)const{
        if(out)out->clear();
        return out&&publication_current(owners)&&storage_->publication->state.phase==BattleModelPublicationPhase::Complete&&storage_->publication->queue.plan(out);
    }
    bool widget_dispatch_authority_ready()const{return false;}BattlePreparationBootstrapStatus status()const{return status_;}
    void retire(){if(storage_)storage_->retire();storage_.reset();attempted_=true;status_=BattlePreparationBootstrapStatus::Retired;} //does not clear entry fields or claims
 private:
    enum class InitialActorOrderPhase {Ordered,Updating,ActorsComplete,StageEntered,Failed};
    struct InitialActorOrderControl {
        InitialActorOrderPhase phase=InitialActorOrderPhase::Ordered;u8 cursor=0,claimed=0;std::optional<u8> inflight;
        SourceByteRngSnapshot rng;std::array<SceneActorReference,6> actors;
    };
    bool initial_order_current(const std::array<const ForceInventory*,2>& owners)const{
        if(!current(owners)||!storage_->initial_order)return false;
        const auto& order=*storage_->initial_order;const auto& physical=*storage_->physical.storage_;const auto batch=storage_->characters.batch_;
        if(order.phase==InitialActorOrderPhase::Failed||!order.rng.live()||!physical.scheduler.initial_actor_order||order.cursor>physical.scheduler.initial_actor_order->count)return false;
        for(unsigned i=0;i<6;++i){
            if(!(order.claimed&(1u<<i))){if(physical.constructor_claimed[i]||batch->construction_claimed[i])return false;continue;}
            if(!physical.constructor_claimed[i]||!batch->construction_claimed[i]||order.actors[i]!=physical.constructors[i])return false;
            const auto scope=order.actors[i].scope_.lock();if(!scope||!scope->live||!scope->owner||!scope->owner->current(owners))return false;
            const auto& a=*scope->owner->storage_;if(a.allocation.batch_.lock()!=batch||a.state.slot3e4!=i||!a.state.first_update_started)return false;
            if(order.phase!=InitialActorOrderPhase::StageEntered&&order.inflight!=std::optional<u8>{u8(i)}&&(scope->owner->status()!=BattleActorConstructionStatus::FirstWorldReady||a.state.world_generation!=1||a.state.native_revision!=19))return false;
        }
        return true;
    }
    bool begin_registered_actor(BattleActorConstruction& actor,BattleCharacterBinding&& prepared,const ChallengeCharacterDescriptorReceipt& receipt,const BattleActorBirthProfile& profile,const BattleActorFirstUpdateProfile* caller,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene actor begin: ")+why;return false;};
        if(status_!=BattlePreparationBootstrapStatus::AtResourcesReady||!current(owners)||receipt.batch_.lock()!=storage_->characters.batch_||storage_->stage_construction.phase()!=BattleStageConstructionPhase::Closed)return reject("same drained scene before stage initialization required");
        if(storage_->initial_order){
            const auto next=next_initial_actor_slot(owners);
            if(!caller||!caller->ready()||!next||receipt.slot_!=*next)return reject("source order and exact first54070 caller required");
        }
        if(!actor.begin(std::move(prepared),receipt,owners,profile,error)){if(storage_->initial_order)storage_->initial_order->phase=InitialActorOrderPhase::Failed;return false;}
        if(!register_constructor(actor,owners,error)){actor.retire();if(storage_->initial_order)storage_->initial_order->phase=InitialActorOrderPhase::Failed;return false;}
        if(storage_->views.initial_aliases_bound())actor.storage_->state.view_slot487=storage_->views.storage_->actor487[actor.storage_->state.slot3e4];
        if(storage_->initial_order){
            auto& order=*storage_->initial_order;const auto slot=actor.storage_->state.slot3e4;const auto& fields=storage_->physical.storage_->scheduler.initial_actor_order->factory_writes[slot];
            actor.storage_->state.scalar_effects.field204=fields.field204;actor.storage_->state.scalar_effects.field208=fields.field208;actor.storage_->state.scalar_effects.field664=fields.field664;
            order.claimed|=u8(1u<<slot);order.actors[slot]=storage_->physical.storage_->constructors[slot];order.inflight=slot;order.phase=InitialActorOrderPhase::Updating;
        }
        if(caller&&!actor.begin_first_update(*caller,owners,error)){if(storage_->initial_order)storage_->initial_order->phase=InitialActorOrderPhase::Failed;return false;}
        return true;
    }
    //29434 calls53A2C/54070 before87CF0. Standalone stage diagnostics may
    //observe untouched factory actors; consumed slots require actual registered
    //first worlds, never retroactive ordering or a caller-declared empty list.
    bool stage_actor_prefix_current(const std::array<const ForceInventory*,2>& owners)const{
        if(!current(owners))return false;
        const auto& physical=*storage_->physical.storage_;const auto batch=storage_->characters.batch_;
        for(unsigned i=0;i<6;++i){
            if(bool(batch->construction_claimed[i])!=physical.constructor_claimed[i])return false;
            if(!physical.constructor_claimed[i])continue;
            const auto scope=physical.constructors[i].scope_.lock();
            if(!scope||!scope->live||!scope->owner||!scope->owner->current(owners)||scope->owner->status()!=BattleActorConstructionStatus::FirstWorldReady)return false;
            const auto& actor=*scope->owner->storage_;
            if(actor.allocation.batch_.lock()!=batch||actor.state.slot3e4!=i||actor.state.world_generation!=1||actor.state.native_revision!=19)return false;
        }
        return true;
    }
    struct ActorPublication {
        BattleModelPublicationState state;std::shared_ptr<const HsdJointWorldScope> scope=std::make_shared<const HsdJointWorldScope>();
        ModelPassQueue queue;std::array<SceneActorReference,6> actors;std::array<bool,6> registered{};std::array<u64,6> worlds{},revisions{};u8 next_slot=0;u32 actor_callback_key=0;
        bool stage_prefix=false;u32 stage_callback_key=0;
        bool prop_callback_queued=false;
        bool presented_view_bound=false;RenderFrameReceipt presented_frame;
        ~ActorPublication(){scope->live_=false;}
    };
    struct CollisionInitialization {
        std::weak_ptr<SceneObjectPool> objects;std::array<SceneActorReference,6> actors;
        std::array<ScenePhysicalLink,8> heads{},tails{};
        std::vector<BattleFirstColliderObservation> order;
        //Only source-written prefixes are represented. Unread reference-array
        //tails remain unknown; zero counts never imply zero-filled guest arrays.
        std::array<std::vector<SceneCollisionLease>,10> registries;bool complete=false;
    };
    bool collision_initialization_current(const std::array<const ForceInventory*,2>& owners)const{
        if(!current(owners)||!storage_->collisions||!storage_->collisions->complete||storage_->stage_construction.object_world_generation()!=1||storage_->stage_construction.first_object_updates_complete())return false;
        const auto& p=*storage_->collisions;const auto pool=p.objects.lock();if(!pool||!pool->ready()||pool!=storage_->physical.class2_pool())return false;
        const auto& arena=*pool->storage_;
        for(unsigned i=0;i<6;++i)if(p.actors[i].live()){
            const auto scope=p.actors[i].scope_.lock();if(!scope||!scope->owner||!scope->owner->current(owners)||scope->owner->storage_->state.world_generation!=1||scope->owner->storage_->state.native_revision!=19)return false;
        }else if(storage_->physical.storage_->actors.slots[i].active00)return false;
        for(unsigned i=0;i<8;++i)if(arena.state.group_heads[48+i]!=p.heads[i]||arena.state.group_tails[48+i]!=p.tails[i])return false;
        for(const auto& e:p.order)if(!e.lease.current()||!arena.collision_initial[e.lease.index()]||arena.collision.records[e.lease.index()].active00!=1)return false;
        for(unsigned list=0;list<10;++list)if(storage_->physical.storage_->scheduler.registry_counts[9-list]!=p.registries[list].size())return false;
        return true;
    }
    void invalidate_publication(){auto& p=*storage_->publication;p.scope->live_=false;p.state.phase=BattleModelPublicationPhase::Failed;}
    bool publication_current(const std::array<const ForceInventory*,2>& owners)const{
        if(!current(owners)||!storage_->publication||!storage_->publication->scope->live()||storage_->publication->state.phase==BattleModelPublicationPhase::Failed)return false;
        const auto& p=*storage_->publication;const auto& physical=*storage_->physical.storage_;const auto batch=storage_->characters.batch_;
        if(p.presented_view_bound){const auto* presentation=first_graphics_presentation(owners);if(!presentation||!presentation->render_frame.same_frame(p.presented_frame))return false;}
        if(p.stage_prefix&&(!current_camera_views(owners)||storage_->state.scene45!=1||physical.controls->d9||!storage_->stage_construction.object_collection_current(storage_->physical)))return false;
        for(unsigned i=0;i<6;++i){
            if(p.registered[i]){const auto scope=p.actors[i].scope_.lock();if(!scope||!scope->live||!scope->owner||!scope->owner->current(owners))return false;const auto& s=scope->owner->storage_->state;if(s.world_generation!=p.worlds[i]||s.native_revision!=p.revisions[i]||p.actors[i]!=physical.constructors[i])return false;}
            else if(physical.constructor_claimed[i]||batch->construction_claimed[i])return false;
        }
        return true;
    }
    bool register_constructor(BattleActorConstruction& actor,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](){if(error)*error="scene actor registration: fresh matching fixed slot/receipt required";return false;};
        if(!current(owners)||!actor.current(owners)||actor.status()!=BattleActorConstructionStatus::Cleared)return reject();
        const auto& a=*actor.storage_;const auto& s=a.state;const auto batch=a.allocation.batch_.lock();const auto physical=storage_->physical.storage_.get();
        if(!batch||batch!=storage_->characters.batch_||batch->origin.lock()!=storage_->entry.lock()||!physical||!physical->actor_seeded||s.slot3e4>=6||s.native_revision||!s.source_clear_seed||a.allocation.slot_!=s.slot3e4||!batch->construction_claimed[s.slot3e4]||batch->selected_member_indices[s.slot3e4]!=0)return reject();
        const auto& seed=physical->actors.slots[s.slot3e4];
        if(!seed.active00||s.active00!=seed.active00||s.kind83!=seed.kind83||s.resource3e7!=seed.resource3e7||s.mask3e5!=seed.mask3e5||s.model3e8!=seed.model3e8||s.model3ea!=seed.model3ea||s.variant3ec!=seed.variant3ec||s.variant3ed!=seed.variant3ed||s.selected492!=seed.member492||s.selected492||s.phase18||s.team88||s.position20!=std::array<f32,3>{})return reject();
        SceneActorReference reference;reference.scope_=a.parent_scope;if(!storage_->physical.bind_constructor(s.slot3e4,reference))return reject();
        if(error){error->clear();}return true;
    }
    struct Storage {
    bool live=true;ChallengeInitialPartyCandidate party;std::weak_ptr<challenge_preparation_entry_detail::Storage> entry;BattlePreparationBootstrapState state;SceneDescriptorBootstrap descriptors;ScenePhysicalPools physical;std::array<u8,6> next_member{};BattleViews views;BattleSceneServices services;BattleCommonResources common;BattleStageResources stage_resources;BattleSideResources side_resources;BattlePreparationModels briefing;BattlePreparationWidgets widgets;StartupSoundCommandOutcome widget_sound;ChallengeCharacterResources characters;BattleCockpitResources cockpit;std::optional<std::array<BattlePlayerFaceSlot,6>> faces;BattleCommonUiResources common_ui;MusicRequestSnapshot music;MusicRequestOutcome music_outcome;std::unique_ptr<ActorPublication> publication;
    BattleStageConstruction stage_construction;std::unique_ptr<InitialActorOrderControl> initial_order;std::unique_ptr<CollisionInitialization> collisions;std::unique_ptr<BattleContactPrefixState> contact_prefix;
    std::array<ScenePhysicalLink,64> contact_groups{},contact_group_tails{};
    std::unique_ptr<BattleSpawnColliderSweepState> spawn_collisions;std::array<ScenePhysicalLink,64> spawn_collision_heads{},spawn_collision_tails{};
    std::unique_ptr<BattleSpawnContactState> spawn_contacts;std::unique_ptr<BattleSpawnVisibilityState> spawn_visibility;
    BattleInitialPresentation presentation;
    std::unique_ptr<BattleInitialGraphicsArchivesState> initial_graphics_archives;
    std::unique_ptr<BattleInitialChildPrefixState> initial_child_prefix;SceneObjectLease initial_child_object,initial_field_child_object;
    std::unique_ptr<BattleInitialWorldDispatchState> initial_world_dispatch;
    std::optional<BattleInitialPostWorldState> initial_post_world;
    std::unique_ptr<BattleInitialUiWorldState> initial_ui_worlds;
    std::array<SceneObjectLease,2> initial_sasuke_child_objects;
    std::array<SceneObjectLease,2> initial_gred_child_objects;
    SceneObjectLease initial_shape_child_object;
    std::array<SceneObjectLease,7> continuation_shape_objects;
    std::array<SceneObjectLease,2> initial_common_aux_objects;
    SceneObjectLease initial_bar_object;SceneScratchLease initial_bar_scratch;
    std::array<SceneObjectLease,8> initial_hud_objects;
    void retire(){if(!live)return;live=false;publication.reset();initial_order.reset();stage_construction.retire();characters.retire();widgets.retire();briefing.retire();common_ui.retire();cockpit.retire();stage_resources.retire();side_resources.retire();common.retire();views=BattleViews{};physical.retire();descriptors.retire();}
    ~Storage(){retire();}
};std::shared_ptr<Storage> storage_;bool attempted_=false;BattlePreparationBootstrapStatus status_=BattlePreparationBootstrapStatus::Closed;
};
} // namespace gf
