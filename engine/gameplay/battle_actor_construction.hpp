#pragma once
#include "engine/assets/battle_initial_sasuke_children.hpp"
#include "engine/gameplay/battle_initial_gred_children.hpp"
#include "engine/assets/battle_initial_shape_continuation.hpp"
#include "engine/gameplay/battle_initial_common_auxiliary.hpp"
#include "engine/assets/battle_initial_bar_child.hpp"
#include "engine/gameplay/attachment_direct_bone_transform.hpp"
#include "engine/assets/battle_initial_world_dispatch.hpp"
#include "engine/gameplay/battle_initial_shape_world.hpp"
#include "engine/gameplay/battle_initial_auxiliary_world.hpp"
#include "engine/gameplay/battle_initial_ui_world.hpp"
#include "engine/assets/battle_initial_hud_children.hpp"
#include "engine/assets/battle_actor_birth.hpp"
#include "engine/gameplay/battle_character_binding.hpp"
#include "engine/gameplay/challenge_battle_roster.hpp"
#include "engine/gameplay/battle_actor_class_selection.hpp"
#include "engine/gameplay/battle_actor_resource_maps.hpp"
#include "engine/gameplay/battle_actor_level_cost.hpp"
#include "engine/gameplay/battle_actor_scalar_defaults.hpp"
#include "engine/gameplay/battle_actor_scalar_effects.hpp"
#include "engine/gameplay/battle_actor_dimensions.hpp"
#include "engine/gameplay/battle_actor_local_setup.hpp"
#include "engine/gameplay/battle_actor_program_seed.hpp"
#include "engine/gameplay/battle_actor_timer_prefix.hpp"
#include "engine/assets/battle_actor_initial_target.hpp"
#include "engine/gameplay/battle_target_query.hpp"
#include "engine/gameplay/startup_actor_class.hpp"
#include "engine/gameplay/startup_auxiliary.hpp"
#include "engine/assets/character_collision_bank.hpp"
#include "engine/gameplay/challenge_spawn_static_tail.hpp"
#include "engine/assets/battle_actor_initial_controls.hpp"
#include "engine/assets/battle_actor_initial_program.hpp"
#include "engine/gameplay/character_program_directional_control.hpp"
#include "engine/assets/battle_actor_first_update.hpp"
#include "engine/assets/battle_actor_spawn_entry.hpp"
#include "engine/gameplay/battle_actor_look_tail.hpp"
#include "engine/gameplay/model_pass_queue.hpp"
#include "engine/gameplay/character_model_parts.hpp"
#include "engine/assets/battle_initial_ai.hpp"
#include "engine/gameplay/collision_contact_reset.hpp"
namespace gf {
enum class BattleActorConstructionStatus {Closed,Cleared,AwaitMemberInitializer,Failed,Retired,AwaitClassSelection,AwaitModelBinding,AwaitScalarSetup,AwaitScalarDefaults,AwaitScalarEffects,AwaitDimensions,AwaitLocalSetup,AwaitProgramSeed,AwaitTimers,AwaitTargetSelection,AwaitClassInitializer,AwaitPostClassChildren,AwaitCollisionInitialization,AwaitPlacementHistory,AwaitProgramStart,AwaitInitialProgramRequest,ConstructedAwaitWorld,FirstWorldReady,SpawnWorldReady,ModelsPublished};
enum class BattleActorBirthWriteKind:u8 {Byte,Half,Word,FloatBits,Clear,SourceKey,ActorDataResource,SelfReference,SourceBindingStage,ResourceByte,SpecificMotionReference,CommonMotionReference,BattleDataReference,CollisionReference,SceneTeamByte,ResourceWord};
struct BattleActorBirthWrite {BattleActorBirthWriteKind kind=BattleActorBirthWriteKind::Byte;u16 offset=0,extent=0;u32 value=0;bool operator==(const BattleActorBirthWrite&)const=default;};
struct BattleActorInitialTargetState {
    std::optional<u8> target300,previous304;u8 mode30a=0,field30f=0;
    u16 delay32a=0,table328=0,final328=0;std::array<u8,3> draws{};SourceByteRngState after;
    BattleNearestTargetResult query;bool complete=false;
};
struct BattleActorInitialControls {
    std::array<u8,2> current5da_5db{},previous5dc_5dd{},selected5e5_5e4{},previous7cf_7ce{};
    std::array<u8,8> fields540{};u8 field588=0,field1db3=0;u32 field754=0;bool complete=false;
};
struct BattleActorContactPrelude {
    s8 target7d0=0;u8 timer7d1=0,flag436=0;
    std::array<u16,2> values4f4_4f6{},limits4fc_4fe{};std::array<s16,2> timers4f8_4fa{};
    u8 field6fd=0,field70c=0,field70d=0,field7d4=0,field599=0;
    u8 forced663=0,peer_mask71e=0,peer_mask71f=0,disabled4a1=0;
};
inline BattleActorContactPrelude battle_actor_contact_prelude_step(BattleActorContactPrelude next){
    if(std::bit_cast<s8>(next.timer7d1)<1){next.timer7d1=0;next.target7d0=-1;}else --next.timer7d1;
    for(unsigned i=0;i<2;++i){
        next.limits4fc_4fe[i]=std::max(next.limits4fc_4fe[i],next.values4f4_4f6[i]);
        if(next.timers4f8_4fa[i])next.timers4f8_4fa[i]=std::bit_cast<s16>(u16(s32(next.timers4f8_4fa[i])-1));
        else next.values4f4_4f6[i]=0;
    }
    next.field6fd=next.field70c=next.field70d=next.field7d4=next.field599=0;return next;
}
//Represented fields ONLY, not a general1E00 source-image/guest-memory codec. Self
//references are native ownership relationships, never host pointers in u32 words.
//No Mat4 is defaulted to an actor world, and no local/program pose is claimed.
struct BattleActorBirthState {
    u8 active00=0,kind83=0,phase18=0,phase19=0,phase1a=0,phase1b=0,field493=0;
    u8 slot3e4=255,mask3e5=0,resource3e7=255,variant3ec=0,variant3ed=0,selected492=255,team88=0;
    u16 model3e8=0,model3ea=0,render_model94=0;u8 render_index96=255,resource_index97=255;
    u8 field3f2=0,field486=0,field501=0,role490=0,mode4a0=0,generation3f0=0;
    std::array<f32,3> position20{};f32 offset6d0=0;std::array<f32,4> fields44_48_4c_50{};
    std::array<s16,3> angles7c_7e_80{};std::array<ScenePhysicalLink,4> fields530_to_53c{};
    std::array<u8,128> cleared144{};std::array<u8,916> cleared540{};
    bool self8c=false,self90=false,source_clear_seed=false;u64 native_revision=0;
    u8 level3ee=0,level3ef=0;u32 acquired3f4=0,experience3f8=0;std::array<s16,3> shorts1c4_1c6_1c8{};
    BattleActorClassFields class_fields;u8 flags48a=0;
    u8 ai3e6=0,field488=0,field3f3=0,field26f=0,render_enabled82=0,bone_count1d90=0;
    u32 callback10c_key=0,visible76c=0,field5a0=0;
    s16 cost4aa=0;
    BattleActorScalarDefaults scalar_defaults;
    BattleActorScalarEffects scalar_effects;
    BattleActorDimensions dimensions;
    BattleActorLocalSetup local_setup;
    BattleActorProgramSeed program_seed;
    BattleActorTimerPrefixPlan timer_prefix;
    BattleActorInitialTargetState initial_target;
    u8 field4a8=0,flags708=0;u16 field6f6=0;bool class_initialized=false,post_class_children=false;
    std::array<std::optional<StartupAuxiliaryConstruction>,2> auxiliary_children;
    u8 collision_enabled86=0,field1d8=0,collider_freeze281=0;bool collision_construction=false;
    u8 field491=0,first43b=0,first43c=0,field3f1=0,field741=0,field742=0;
    std::array<f32,3> previous2c{},history5e8{},anchor354{};
    std::array<s16,3> angles70{},previous76{};
    ChallengeFirstPlacementPlan placement;ChallengeSpawnStaticTail placement_tail;bool placement_complete=false;
    BattleActorInitialControls initial_controls;
    bool initial_program_requested=false,constructor_complete=false;u8 field500=0;
    bool first_update_started=false,first_update_completed=false;u64 world_generation=0;
    std::array<s16,3> previous_look1d6a{};u8 field1ca=0,field730=0,field731=0,field732=0;std::array<f32,3> push724{};
    std::array<f32,3> scale58{};f32 spawn_time_d4=0,spawn_duration_d8=0;bool spawn_entry_completed=false;
    u16 hidden_views270=0;u8 forced_hidden59a=0,alternate_ae=0;f32 invulnerability720=0;u32 suppressed810=0;
    bool model_world_published=false,model_geometry_prepared=false;
    BattleModelDomain model_domain=BattleModelDomain::NormalPair;
    std::array<u8,2> ai_flags144_145{};std::array<u8,4> ai_flags168_16b{};std::array<std::array<f32,3>,2> ai_points150_15c{};
    u8 view_slot487=0,view_behaviour43d=0;s16 camera_heading5b0=0;u64 camera_generation=0;
    //Fresh null508/zero50C/7C9 retained through the admitted first spawn entry.
    bool camera_target508_bound=false;std::array<f32,3> camera_target50c{};u8 camera_lock7c9=0;
    CollisionContactRetained contact_retained;u16 shadow_views286=0;u64 contact_generation=0;
    BattleActorContactPrelude contact_prelude;
    //537A8 cleared all six bytes. Admitted first program/world paths do not run
    //5568C's channel decrement/reload work or write these source counters.
    std::array<u8,6> effect_counts_a8{};
    u8 retire_662=0; //fresh clear; no admitted first-program retirement write
    //537A8 clear.29434's6AE60 precedes the first constructors; no admitted
    //first program/world/contact operation re-runs its target-display producer.
    u8 target_display7d2=0;
};
enum class BattleActorChildKind {Bar,Shape,CommonAuxiliary,BankAttachment,PostClassSix,PostClassOne,AiFieldFollower,AiPointFollower};
struct BattleActorChildAttempt {BattleActorChildKind kind=BattleActorChildKind::Bar;u8 source_index=0,descriptors=0;SceneObjectAcquire result=SceneObjectAcquire::Refused;SceneObjectLease object;};
struct BattleActorCollisionAttempt {u8 ordinal=0,record=0;SceneObjectAcquire result=SceneObjectAcquire::Refused;SceneCollisionLease object;};
struct BattleActorPublishedModel {u16 record=0,descriptor=0;u64 world_generation=0;};
struct BattleActorConstructionPending {
    bool member_68344=true,class_selection_56ce0=true,actor_model_motion_binding=true,scalar_setup=true,local_pose_4bb14=true;
    bool timer_and_target_rng=true,class_initializer_4b8=true,effects=true,colliders=true,program=true,placement_history=true,full_constructor=true;
    bool model_records_419ec=true,bone_parts_5380c=true,motion_tables_41b20=true;
    bool level_cost_55f90=true;
    bool defaults_56180=true;
    bool scene_archive_binding=true;
    bool scene_model_descriptors=true;
    bool scalar_effects_562b8=true,child_callbacks=true;
    bool dimensions_55dcc=true;
    bool program_seed_3cc24=true,timer_prefix_1c80c=true,collision_construction_8ab30=true,initial_program_request_4beb8=true,first_world_frame=true,model_world_publication=true;
};
//Declared numeric adapter for owned/source checks, never native actor authority.
namespace battle_actor_birth_detail {
inline bool seed(const BattleActorBirthProfile& p,u8 slot,u8 selected,u16 model,u8 variant,BattleModelDomain domain,BattleActorBirthState* out,std::string* error){
    if(out)*out={};
    if(!out||!battle_actor_birth_profile_valid(p)||slot>=6||selected>=30||!battle_model_in_domain(domain,model)||variant){if(error)*error="battle actor birth seed: qualified fresh source domain";return false;}
    BattleActorBirthState s;s.model_domain=domain;s.active00=1;s.slot3e4=s.resource3e7=s.render_index96=s.resource_index97=slot;s.mask3e5=u8(1u<<slot);s.model3e8=s.model3ea=s.render_model94=model;s.variant3ec=s.variant3ed=variant;s.selected492=selected;s.self8c=s.self90=s.source_clear_seed=true;*out=s;if(error)error->clear();return true;
}
}
inline bool battle_actor_birth_seed(const BattleActorBirthProfile& p,u8 slot,u8 selected,u16 model,u8 variant,BattleActorBirthState* out,std::string* error){
    return battle_actor_birth_detail::seed(p,slot,selected,model,variant,BattleModelDomain::NormalPair,out,error);
}
//541AC direct writes through542A8 ONLY. Fresh role490=0 then reaches68344,
//whose roster-tail inputs/mutations must be implemented before continuing.
inline bool battle_actor_birth_prefix(const BattleActorBirthProfile& p,const BattleActorBirthState& input,u8 team,BattleActorBirthState* out,std::vector<BattleActorBirthWrite>* writes,std::string* error){
    if(out==&input){if(error)*error="battle actor birth prefix: input/output alias";return false;}
    if(writes)writes->clear();
    auto fail=[&](){if(out)*out={};if(error)*error="battle actor birth prefix: source seed/ordinary role/team required";return false;};
    if(!out||!writes||!battle_actor_birth_profile_valid(p)||!input.source_clear_seed||input.native_revision||input.phase18||input.role490||input.mode4a0||team>=2)return fail();
    BattleActorBirthState s=input;std::vector<BattleActorBirthWrite> journal;auto put=[&](BattleActorBirthWriteKind kind,unsigned at,unsigned extent,u32 value){journal.push_back({kind,u16(at),u16(extent),value});};using K=BattleActorBirthWriteKind;
    s.phase18=1;s.phase19=s.phase1a=s.phase1b=s.field493=0;put(K::Byte,0x18,1,1);for(unsigned at:{0x19u,0x1au,0x1bu,0x493u})put(K::Byte,at,1,0);
    s.position20[1]=f32(s.position20[1]+s.offset6d0);if(!std::isfinite(s.position20[1])||std::abs(s.position20[1])>1e7f)return fail();put(K::FloatBits,0x24,4,f32_to_bits(s.position20[1]));
    s.cleared144.fill(0);s.cleared540.fill(0);s.offset6d0=0;s.scalar_effects.field664=0;put(K::Clear,p.words[1],p.words[2],0);put(K::Clear,p.words[3],p.words[4],0);
    s.fields44_48_4c_50.fill(bits_to_f32(p.words[5]));for(unsigned at:{0x50u,0x48u,0x4cu,0x44u})put(K::FloatBits,at,4,p.words[5]);
    s.angles7c_7e_80.fill(0);for(unsigned at:{0x80u,0x7eu,0x7cu})put(K::Half,at,2,0);
    s.field501=0;put(K::Byte,0x501,1,0);s.fields530_to_53c.fill({});for(unsigned at:{0x530u,0x534u,0x538u,0x53cu})put(K::Word,at,4,0);
    s.resource3e7=s.slot3e4;put(K::Byte,0x3e7,1,s.resource3e7);put(K::Half,0x3e8,2,s.model3e8);put(K::Byte,0x3ec,1,s.variant3ec);s.team88=team;put(K::Byte,0x88,1,team);
    s.native_revision=1;*out=s;*writes=std::move(journal);if(error)error->clear();return true;
}
//Declared numeric continuation; not permission to mutate a native actor owner.
inline bool battle_actor_birth_member(const BattleActorBirthState& input,const BattleMemberContext& context,BattleActorBirthState* out,std::vector<BattleActorBirthWrite>* writes,std::string* error){
    if(out==&input){if(error)*error="battle actor member: input/output alias";return false;}
    if(out)*out={};
    if(writes)writes->clear();
    if(!out||!writes||!input.source_clear_seed||input.native_revision!=1||input.phase18!=1||input.role490||input.mode4a0||input.resource3e7>=6){if(error)*error="battle actor member: ordinary prefix cursor required";return false;}
    BattleMemberInitialization plan;if(!battle_actor_member_initialize(context,&plan,error))return false;
    //The mutable native owner below admits Challenge ONLY. Story mutations remain
    //represented in the standalone68344 planner, not silently dropped here.
    if(plan.roster_word8){if(error)*error="battle actor member: roster mutation requires its own owner";return false;}
    auto next=input;std::vector<BattleActorBirthWrite> journal;
    for(const auto& write:plan.writes)journal.push_back({write.extent==1?BattleActorBirthWriteKind::Byte:write.extent==2?BattleActorBirthWriteKind::Half:BattleActorBirthWriteKind::Word,write.offset,write.extent,write.value});
    next.selected492=plan.selected492;next.level3ee=plan.level3ee;next.acquired3f4=plan.acquired3f4;next.experience3f8=plan.experience3f8;if(plan.actor_shorts1c4_1c6_1c8)next.shorts1c4_1c6_1c8=*plan.actor_shorts1c4_1c6_1c8;
    next.model3ea=next.model3e8;next.variant3ed=next.variant3ec;next.level3ef=next.level3ee;next.generation3f0=u8(unsigned(next.generation3f0)+1);next.native_revision=2;
    journal.push_back({BattleActorBirthWriteKind::Half,0x3ea,2,next.model3ea});journal.push_back({BattleActorBirthWriteKind::Byte,0x3ed,1,next.variant3ed});journal.push_back({BattleActorBirthWriteKind::Byte,0x3ef,1,next.level3ef});journal.push_back({BattleActorBirthWriteKind::Byte,0x3f0,1,next.generation3f0});
    *out=next;*writes=std::move(journal);if(error)error->clear();return true;
}
//Declared source-field continuation through54364 only. No419EC/5380C/41B20
//completion is inferred from the earlier preparation of a model or motion bank.
inline bool battle_actor_birth_class(const BattleActorBirthState& input,const BattleNormalClassRow& row,const StartupActorSetupProfile& setup,BattleActorBirthState* out,std::vector<BattleActorBirthWrite>* writes,std::string* error){
    if(out==&input){if(error)*error="battle actor class: input/output alias";return false;}
    if(out)*out={};
    if(writes)writes->clear();
    if(!out||!writes||!input.source_clear_seed||input.native_revision!=2||input.phase18!=1||input.role490||input.mode4a0||input.flags48a||input.class_fields.installed||input.slot3e4>=6||input.resource3e7!=input.slot3e4||input.model3e8!=row.model||input.variant3ec!=row.variant||input.model_domain!=row.domain){if(error)*error="battle actor class: ordinary member-complete source cursor required";return false;}
    BattleActorClassSelectionPlan plan;if(!battle_actor_class_select(row,setup,input.resource3e7,&plan,error))return false;
    auto next=input;next.class_fields=plan.fields;std::vector<BattleActorBirthWrite> journal;using K=BattleActorBirthWriteKind;
    for(const auto& write:plan.writes)journal.push_back({write.kind==BattleActorClassWriteKind::SourceKey?K::SourceKey:K::ActorDataResource,write.offset,4,write.value});
    const s16 secondary=std::bit_cast<s16>(startup_actor_setup_detail::data(setup).be16(0xa6));if(secondary>=0){next.model3ea=u16(secondary);journal.push_back({K::Half,0x3ea,2,next.model3ea});}
    next.self8c=next.self90=true;journal.push_back({K::SelfReference,0x8c,4,input.slot3e4});journal.push_back({K::SelfReference,0x90,4,input.slot3e4});
    next.render_model94=next.model3e8;next.render_index96=next.slot3e4;next.resource_index97=next.resource3e7;journal.push_back({K::Half,0x94,2,next.render_model94});journal.push_back({K::Byte,0x96,1,next.render_index96});journal.push_back({K::Byte,0x97,1,next.resource_index97});
    next.native_revision=3;*out=next;*writes=std::move(journal);if(error)error->clear();return true;
}
//541AC5438C..54410 after the three source binding stages; Challenge common50=1.
//This numeric adapter cannot authorize/replace those native source consumers.
inline bool battle_actor_birth_resource_tail(const BattleActorBirthState& input,u8 humans,BattleActorBirthState* out,std::vector<BattleActorBirthWrite>* writes,std::string* error){
    if(out==&input){if(error)*error="battle actor resource tail: input/output alias";return false;}
    if(out)*out={};
    if(writes)writes->clear();
    if(!out||!writes||humans>=64||!input.source_clear_seed||input.native_revision!=3||!input.class_fields.installed||input.phase18!=1||input.role490||input.flags48a||input.mode4a0||input.slot3e4>=6||input.resource3e7!=input.slot3e4||input.mask3e5!=u8(1u<<input.slot3e4)){if(error)*error="battle actor resource tail: ordinary current class cursor/human mask required";return false;}
    auto next=input;using K=BattleActorBirthWriteKind;std::vector<BattleActorBirthWrite> journal;auto put=[&](K kind,u16 at,u16 extent,u32 value){journal.push_back({kind,at,extent,value});};
    next.ai3e6=next.field488=next.field3f3=(humans&input.mask3e5)?0:1;for(unsigned at:{0x3e6u,0x488u,0x3f3u})put(K::Byte,u16(at),1,next.ai3e6);
    next.callback10c_key=0x80056aa8;next.visible76c=0xffffffffu;next.field5a0=0;next.field26f=1;next.render_enabled82=0;put(K::SourceKey,0x10c,4,next.callback10c_key);put(K::Word,0x76c,4,next.visible76c);put(K::Word,0x5a0,4,0);put(K::Byte,0x26f,1,1);put(K::Byte,0x82,1,0);
    next.native_revision=4;*out=next;*writes=std::move(journal);if(error)error->clear();return true;
}
//Declared adapter; native authority still requires the exact current resource,
//catalog and captured member. Stops BEFORE5614C, not a completed scalar setup.
inline bool battle_actor_birth_level_cost(const BattleActorBirthState& input,const BattleActorLevelRow& row,u32 model_hash,BattleActorBirthState* out,std::vector<BattleActorBirthWrite>* writes,std::string* error){
    if(out==&input){if(error)*error="battle actor level cost: input/output alias";return false;}
    if(out)*out={};
    if(writes)writes->clear();
    if(!out||!writes||!input.source_clear_seed||input.native_revision!=4||!input.class_fields.installed||input.phase18!=1||input.role490||input.mode4a0||input.flags48a||input.model3e8!=row.model||input.variant3ec!=row.variant||input.level3ee!=row.level||row.model_hash!=model_hash||input.model_domain!=row.domain){if(error)*error="battle actor level cost: current ordinary member/resource row required";return false;}
    BattleActorLevelCostPlan plan;if(!battle_actor_level_cost(row,input.ai3e6,input.field486,&plan,error))return false;
    auto next=input;next.shorts1c4_1c6_1c8=plan.shorts1c4_1c6_1c8;next.cost4aa=plan.cost4aa;next.native_revision=5;std::vector<BattleActorBirthWrite> journal;for(const auto& write:plan.writes)journal.push_back({BattleActorBirthWriteKind::Half,write.offset,2,write.value});*out=next;*writes=std::move(journal);if(error)error->clear();return true;
}
class BattleActorConstruction {
public:
    BattleActorConstruction()=default;BattleActorConstruction(const BattleActorConstruction&)=delete;BattleActorConstruction& operator=(const BattleActorConstruction&)=delete;
    BattleActorConstruction(BattleActorConstruction&& other)noexcept{*this=std::move(other);}
    BattleActorConstruction& operator=(BattleActorConstruction&& other)noexcept{
        if(this!=&other){retire();storage_=std::move(other.storage_);status_=other.status_;attempted_=other.attempted_;if(storage_)storage_->parent_scope->owner=this;other.status_=BattleActorConstructionStatus::Retired;other.attempted_=true;}return *this;
    }
    ~BattleActorConstruction(){retire();}
    //Moves the SAME genuine prepared body/rig/bank allocation; no second decode,
    //neighbor substitution, declared binding, or public snapshot can authorize it.
    //Prepared resources may exist early natively; their source actor-field binds
    //and callbacks are NOT acknowledged ahead of the constructor cursor.
    bool begin(BattleCharacterBinding&& prepared,const ChallengeCharacterDescriptorReceipt& allocation,const std::array<const ForceInventory*,2>& owners,const BattleActorBirthProfile& profile,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle actor construction: ")+why;return false;};
        if(attempted_)return reject("fresh one-shot owner required");
        attempted_=true;status_=BattleActorConstructionStatus::Failed;
        const auto batch=allocation.batch_.lock();const auto resource=allocation.data(owners);const unsigned slot=allocation.slot_;
        if(!battle_actor_birth_profile_valid(profile)||!batch||!resource||slot>=6||!prepared.current(owners)||!prepared.storage_||prepared.storage_->resource!=resource||!prepared.bank_instances())return reject("same current genuine full-bank allocation required");
        const auto index=batch->selected_member_indices[slot];const auto members=batch->party.members(slot);if(index>=members.size()||batch->party.team(slot)<0||batch->party.team(slot)>1||members[index].fields.model!=resource->selection().model||members[index].fields.variant!=resource->variant())return reject("captured current member/resource mismatch");
        const auto domain=prepared.storage_->klass.domain;
        if(domain==BattleModelDomain::InitialAi&&((batch->party.humans()&(1u<<slot))||members[index].origin!=ChallengePartyOrigin::InitialAi||!battle_model_in_domain(domain,resource->selection().model)))return reject("V2 class entry requires the actual initial AI member");
        const auto scene_entry=batch->origin.lock();
        if(batch->queued&&(!scene_entry||!scene_entry->live||scene_entry->physical_controls.expired()))return reject("current queued-scene physical arena required");
        //First construction is one-shot per current allocation slot, even if a
        //caller prepared a second model instance for that same receipt. This is
        //serialized native ownership, NOT a global source actor-pool/respawn ACK.
        if(batch->construction_claimed[slot])return reject("allocation slot already claimed for first construction");
        batch->construction_claimed[slot]=true; //never reset/replayed after uncertain work or retire
        try{auto next=std::make_unique<Storage>();next->profile=profile;next->allocation=allocation;next->selected=members[index];next->team=u8(batch->party.team(slot));if(!battle_actor_birth_detail::seed(profile,u8(slot),index,resource->selection().model,resource->variant(),domain,&next->state,error))return false;
            next->binding=std::move(prepared);if(!next->binding.current(owners)||allocation.data(owners)!=resource)return reject("allocation retired during transfer; no replay");next->parent_scope->owner=this;if(batch->queued){next->parent_scope->arena=scene_entry->physical_controls;next->parent_scope->arena_bound=true;}storage_=std::move(next);status_=BattleActorConstructionStatus::Cleared;if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native allocation/transfer; no replay");}
    }
    bool current(const std::array<const ForceInventory*,2>& owners)const{return status_!=BattleActorConstructionStatus::Closed&&status_!=BattleActorConstructionStatus::Failed&&status_!=BattleActorConstructionStatus::Retired&&storage_&&storage_->parent_scope->live&&storage_->parent_scope->arena_live()&&storage_->parent_scope->owner==this&&storage_->binding.current(owners)&&storage_->allocation.data(owners)==storage_->binding.storage_->resource&&(!storage_->member_joined||roster_current(owners))&&(!storage_->class_joined||class_sources_current())&&(!storage_->resources_joined||resource_sources_current(owners))&&(!storage_->effects_joined||constructor_children_current())&&(!storage_->local_joined||local_sources_current())&&(!storage_->program_seeded||program_source_current())&&(!storage_->state.initial_program_requested||initial_program_current())&&(!storage_->state.model_world_published||publication_current());}
    bool advance_direct_prefix(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattleActorConstructionStatus::Cleared){if(error)*error="battle actor construction: prefix already applied or owner closed";return false;}
        if(!current(owners)){status_=BattleActorConstructionStatus::Failed;if(error)*error="battle actor construction: allocation/party no longer current";return false;}
        try{BattleActorBirthState next;std::vector<BattleActorBirthWrite> journal;if(!battle_actor_birth_prefix(storage_->profile,storage_->state,storage_->team,&next,&journal,error)){status_=BattleActorConstructionStatus::Failed;return false;}if(!current(owners)){status_=BattleActorConstructionStatus::Failed;if(error)*error="battle actor construction: owner retired before prefix commit";return false;}
            auto combined=storage_->journal;combined.insert(combined.end(),journal.begin(),journal.end());storage_->state=next;storage_->journal=std::move(combined);status_=BattleActorConstructionStatus::AwaitMemberInitializer;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattleActorConstructionStatus::Failed;if(error)*error="battle actor construction: uncertain prefix allocation; no replay";return false;}
    }
    //68344 then541AC542C0..54320, stopping BEFORE56CE0. No class callback,
    //model/program pose, final RNG, effect or live collider completion implied.
    bool advance_member_initializer(ChallengeBattleRoster& roster,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle actor construction: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitMemberInitializer)return reject("member cursor not pending");
        if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("allocation no longer current");}
        const auto batch=storage_->allocation.batch_.lock();const auto context=roster.storage_;const unsigned slot=storage_->state.resource3e7;
        if(!roster.current(owners)||!context||context->allocation.lock()!=batch||slot>=6||context->member_initialized[slot]||context->selected[slot]!=storage_->state.selected492)return reject("same current roster/allocation/member required");
        try{const auto index=context->selected[slot];const auto& rows=context->members[slot];if(index>=rows.size()||rows[index].selected.fields!=storage_->selected.fields)return reject("captured member changed");
            BattleActorBirthState next;std::vector<BattleActorBirthWrite> tail;const BattleMemberContext input{u8(1),std::nullopt,index,rows};if(!battle_actor_birth_member(storage_->state,input,&next,&tail,error)){status_=BattleActorConstructionStatus::Failed;return false;}
            auto journal=storage_->journal;journal.insert(journal.end(),tail.begin(),tail.end());
            if(!current(owners)||!context->current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("retired before member commit");}
            storage_->state=next;storage_->journal=std::move(journal);storage_->roster=context;storage_->member_joined=true;context->member_initialized[slot]=true;++context->revision;status_=BattleActorConstructionStatus::AwaitClassSelection;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattleActorConstructionStatus::Failed;return reject("uncertain member allocation; no replay");}
    }
    bool advance_class_selection(const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle actor construction: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitClassSelection)return reject("class cursor not pending");
        if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("allocation/roster no longer current");}
        try{const auto& prepared=*storage_->binding.storage_;BattleActorBirthState next;std::vector<BattleActorBirthWrite> tail;
            if(!battle_actor_birth_class(storage_->state,prepared.klass,prepared.setup,&next,&tail,error)){status_=BattleActorConstructionStatus::Failed;return false;}
            //Real4AC resource bytes and final4B0 config view; provenance keys are
            //never cast to host callbacks/pointers. Views remain heap-stable.
            const auto data=prepared.resource->actor_data(),config=startup_actor_setup_detail::config(prepared.setup);
            if(data.size!=prepared.setup.actor_data.size()||!std::equal(data.data,data.data+data.size,prepared.setup.actor_data.begin())){status_=BattleActorConstructionStatus::Failed;return reject("prepared actor-data source mismatch");}
            auto journal=storage_->journal;journal.insert(journal.end(),tail.begin(),tail.end());
            if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("retired before class commit");}
            storage_->state=next;storage_->journal=std::move(journal);storage_->actor_data4ac=data;storage_->class_config4b0=config;storage_->class_joined=true;status_=BattleActorConstructionStatus::AwaitModelBinding;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattleActorConstructionStatus::Failed;return reject("uncertain class allocation; no replay");}
    }
    bool advance_model_resources(ByteView part_fields,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle actor construction: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitModelBinding)return reject("resource cursor not pending");
        if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("allocation/roster/class no longer current");}
        auto& prepared=*storage_->binding.storage_;CharacterModelPartsProfile parts;
        //Pure metadata refusal is BEFORE source work, preserving the pending cursor.
        if(!character_model_parts_load(part_fields,prepared.klass.model_hash,&parts,error)||parts.model_id!=storage_->state.model3e8||parts.domain!=prepared.klass.domain)return reject("same-model qualified part table required");
        try{const auto& bank=prepared.instances;std::vector<const HsdJointInstanceStateSet*> instances;for(unsigned i=0;i<bank.instances();++i)instances.push_back(bank.state(i));BattleActorResourceMaps maps;
            if(!prepared.bank_material||prepared.bank_material->source_cache_words()!=std::array<u32,2>{255,0}||!battle_actor_resource_maps(prepared.model.rig(),parts,instances,prepared.motion,storage_->state.resource_index97,&maps,error)){status_=BattleActorConstructionStatus::Failed;return false;}
            std::vector<BattleBankInstanceReceipt> receipts;std::vector<std::vector<MaterialContextBinding>> contexts;
            for(unsigned style=0;style<2;++style)for(const auto& model:maps.models[style]){auto receipt=storage_->binding.bank_instance_receipt(model.instance);if(!receipt.current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("instance allocation changed during binding");}receipts.push_back(receipt);std::vector<MaterialContextBinding> rows;for(u32 dobj:model.material_dobjs)rows.push_back({dobj,prepared.bank_material.get()});contexts.push_back(std::move(rows));}
            const auto batch=storage_->allocation.batch_.lock();BattleActorBirthState next;std::vector<BattleActorBirthWrite> tail;if(!batch||!battle_actor_birth_resource_tail(storage_->state,batch->party.humans(),&next,&tail,error)){status_=BattleActorConstructionStatus::Failed;return false;}next.bone_count1d90=maps.bone_count1d90;
            auto journal=storage_->journal;for(u32 call:{0x800419ecu,0x8005380cu,0x80041b20u})journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,call});journal.insert(journal.end(),tail.begin(),tail.end());
            if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("retired before resource commit");}
            SceneArchiveLease archive;
            if(batch->queued){
                const auto pool=batch->archives.lock();const auto& source=batch->queue.rows[storage_->state.resource3e7];
                if(!pool||!source.published_complete||batch->resources[storage_->state.resource3e7]!=prepared.resource||!source.archive.current()||source.archive.bank()||batch->model_contexts[storage_->state.resource3e7])return reject("same current blank412C4 header/cache required before419EC binding");
                //Use the already prepared shared bank: no second decode and no
                //replacement/pristine descriptor pool. Bind the actual header
                //allocated by this slot's deferred publication.
                if(!pool->bind_borrowed_bank(source.archive,prepared.bank,error)){status_=BattleActorConstructionStatus::Failed;return false;}
                archive=source.archive;
                storage_->archive=archive;storage_->archive_bound=true;
                batch->model_contexts[storage_->state.resource3e7]=prepared.bank_material;
                const auto descriptors=pool->descriptor_owner();if(!descriptors){status_=BattleActorConstructionStatus::Failed;return reject("current shared model-descriptor domain required");}
                prepared.descriptor_pool=descriptors;prepared.descriptors.reserve(bank.instances());
                //419EC allocates ALL normal descriptors, then ALL alternate
                //descriptors. Transfer the exact prepared roots into those
                //owners; retain weak identities, never duplicate root graphs.
                for(unsigned i=0;i<bank.instances();++i){
                    const auto* state=bank.state(i);if(!state){status_=BattleActorConstructionStatus::Failed;return reject("prepared root already transferred");}
                    const auto record=state->record;const auto style=state->style;const auto identity=bank.receipt(i);SceneDescriptorLease descriptor;
                    if(descriptors->acquire(&descriptor,error)!=SceneDescriptorAcquire::Acquired){status_=BattleActorConstructionStatus::Failed;return reject("model-descriptor allocation failed; consumed prefix retained");}
                    prepared.descriptors.push_back({descriptor,identity});HsdJointInstanceOwner root;
                    if(!prepared.instances.take_instance(i,&root,error)||!SceneModelBindings::attach_actor(*descriptors,descriptor,archive,record,style,std::move(root),prepared.bank_material,error)){status_=BattleActorConstructionStatus::Failed;return false;}
                }
                prepared.descriptors_bound=true;
            }
            storage_->state=next;storage_->journal=std::move(journal);storage_->parts_profile=parts;storage_->resource_maps=std::move(maps);storage_->resource_receipts=std::move(receipts);storage_->material_contexts=std::move(contexts);storage_->resource_bank=&bank;storage_->resource_motion=&prepared.motion;storage_->archive=archive;storage_->archive_bound=batch->queued;storage_->resources_joined=true;status_=BattleActorConstructionStatus::AwaitScalarSetup;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattleActorConstructionStatus::Failed;return reject("uncertain resource binding allocation; no replay");}
    }
    bool advance_level_cost(const BattleActorLevelProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle actor construction: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitScalarSetup)return reject("level/cost cursor not pending");
        if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("allocation/roster/resources no longer current");}
        const auto& state=storage_->state;const auto* row=profile.find(state.model3e8,state.variant3ec,state.level3ee,storage_->binding.storage_->klass.model_hash);
        if(!profile.for_catalog(*owners[0]->catalog())||!row||row->level!=storage_->selected.fields.level)return reject("current catalog/model/captured member level required");
        try{BattleActorBirthState next;std::vector<BattleActorBirthWrite> tail;if(!battle_actor_birth_level_cost(state,*row,storage_->binding.storage_->klass.model_hash,&next,&tail,error)){status_=BattleActorConstructionStatus::Failed;return false;}auto journal=storage_->journal;journal.insert(journal.end(),tail.begin(),tail.end());
            if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("retired before level/cost commit");}
            storage_->state=next;storage_->journal=std::move(journal);storage_->level_row=*row;storage_->level_cost_joined=true;status_=BattleActorConstructionStatus::AwaitScalarDefaults;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattleActorConstructionStatus::Failed;return reject("uncertain level/cost allocation; no replay");}
    }
    //Inside5614C: complete56180, stopping BEFORE its following562B8 call.
    bool advance_scalar_defaults(const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle actor defaults: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitScalarDefaults)return reject("defaults cursor not pending");
        if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("allocation/roster no longer current");}
        const auto context=storage_->roster.lock();const auto& state=storage_->state;
        if(!context||!context->player_defaults||state.slot3e4>=6)return reject("genuine current player context required");
        //Fresh clear plus represented source writes qualify zero489/500. Existing
        //48A was installed/guarded above; no respawn/alternate scale branch grant.
        if(!state.source_clear_seed||state.native_revision!=5||state.role490||state.flags48a||state.mode4a0)return reject("fresh ordinary scalar branch required");
        try{BattleActorScalarDefaults defaults;if(!battle_actor_scalar_defaults(storage_->binding.storage_->setup,(*context->player_defaults)[state.slot3e4],&defaults,error))return false;
            auto next=state;next.scalar_defaults=std::move(defaults);next.scale58=next.scalar_defaults.actor_scale;next.native_revision=6;auto journal=storage_->journal;
            for(const auto& w:next.scalar_defaults.writes)journal.push_back({w.extent==1?BattleActorBirthWriteKind::Byte:w.extent==2?BattleActorBirthWriteKind::Half:BattleActorBirthWriteKind::FloatBits,w.offset,w.extent,w.value});
            if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("retired before defaults commit");}
            storage_->state=std::move(next);storage_->journal=std::move(journal);storage_->defaults_joined=true;status_=BattleActorConstructionStatus::AwaitScalarEffects;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattleActorConstructionStatus::Failed;return reject("uncertain defaults allocation; no replay");}
    }
    bool advance_scalar_effects(const BattleActorEffectsProfile& profile,const std::shared_ptr<SceneObjectPool>& objects,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle actor scalar effects: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitScalarEffects||!current(owners))return reject("current56180-complete constructor required");
        const auto batch=storage_->allocation.batch_.lock();const auto archives=batch?batch->archives.lock():nullptr;
        const auto& s=storage_->state;auto& prepared=*storage_->binding.storage_;
        if(!batch||!batch->queued||!archives||!objects||!objects->matches_descriptor_owner(archives->descriptor_owner())||!storage_->archive_bound||!prepared.descriptors_bound||!s.source_clear_seed||s.native_revision!=6||s.role490||s.mode4a0||s.flags48a||!s.self8c||!s.self90||!profile.for_model(s.model3e8,prepared.klass.model_hash)||batch->model_contexts[s.resource3e7]!=prepared.bank_material)return reject("same fresh scene/model/context/object-pool domain required");
        BattleActorScalarEffects effects;if(!battle_actor_scalar_effects(prepared.setup,storage_->level_row,&effects,error))return false;
        status_=BattleActorConstructionStatus::Failed;
        auto fail=[&](const char* why){storage_->parent_scope->live=false;return reject(why);};
        try{
            storage_->children_pool=objects;storage_->children.reserve(3+profile.shapes().size());auto journal=storage_->journal;
            SceneActorReference parent;parent.scope_=storage_->parent_scope;
            SceneObjectActorChildFields copied;copied.parent8c=copied.parent90=parent;copied.model94=s.render_model94;copied.index96=s.render_index96;copied.resource97=s.resource_index97;copied.team88=s.team88;
            copied.scale_b8=s.scalar_defaults.rank.scale_b8;copied.scale_b4=s.scalar_defaults.scale_b4;copied.scale.fill(copied.scale_b4);copied.fieldc0=s.scalar_defaults.fieldc0;copied.fieldc4=s.scalar_defaults.fieldc4;
            //C8/CC/D0 and54 remain the genuine fresh constructor clear here.
            auto spawn=[&](BattleActorChildKind kind,u8 index,const BattleActorShapeSeed* shape){
                BattleActorChildAttempt attempt;attempt.kind=kind;attempt.source_index=index;attempt.descriptors=kind==BattleActorChildKind::Bar?5:0;
                attempt.result=objects->acquire(24,false,attempt.descriptors,SceneObjectParentFields{s.kind83,s8(s.slot3e4)},&attempt.object,error);
                if(attempt.result==SceneObjectAcquire::Refused)return false;
                if(attempt.result==SceneObjectAcquire::Allocated){
                    auto fields=copied;SceneObjectHeader header;
                    if(kind==BattleActorChildKind::Bar){header={0x1b,index,0,0x8013be88,0x8013c264};fields.field13=5;}
                    else{header={6,shape->type11,shape->argument12,0x8001c42c,0x8001c70c};fields.field89=shape->field89;fields.scale_b4=shape->scale_b4;}
                    if(!objects->install_actor_child(attempt.object,header,fields))return false;
                }
                storage_->children.push_back(attempt);
                journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,kind==BattleActorChildKind::Bar?0x8013bd80u:0x80088aa0u});return true;
            };
            using K=BattleActorEffectOperationKind;using W=BattleActorBirthWriteKind;
            for(const auto& op:effects.operations){
                if(op.kind==K::BarChild){if(!spawn(BattleActorChildKind::Bar,u8(op.offset),nullptr))return fail("uncertain bar-child allocation/header; prefix retained");continue;}
                if(op.kind==K::ResetColour){journal.push_back({W::SourceBindingStage,0,0,0x80006238});prepared.bank_material->reset_source_colour();for(unsigned i=0;i<4;++i)journal.push_back({W::ResourceByte,u16(0xc28+i),1,i==3?255u:0u});continue;}
                const auto kind=op.kind==K::Byte?W::Byte:op.kind==K::Half?W::Half:op.kind==K::Word?W::Word:W::FloatBits;
                journal.push_back({kind,op.offset,u16(op.kind==K::Byte?1:op.kind==K::Half?2:4),op.value});
            }
            journal.push_back({W::SourceBindingStage,0,0,0x8001c2c8});
            for(unsigned i=0;i<profile.shapes().size();++i)if(!spawn(BattleActorChildKind::Shape,u8(i),&profile.shapes()[i]))return fail("uncertain shape-child allocation/header; prefix retained");
            if(!storage_->binding.current(owners)||!roster_current(owners)||!objects->ready())return fail("source parent/child owner retired during construction");
            storage_->state.scalar_effects=std::move(effects);++storage_->state.native_revision;storage_->journal=std::move(journal);storage_->effects_joined=true;status_=BattleActorConstructionStatus::AwaitDimensions;if(error)error->clear();return true;
        }catch(const std::exception&){return fail("uncertain effect construction; no replay");}
    }
    bool advance_dimensions(const std::array<const ForceInventory*,2>& owners,std::string* error){
        return advance_dimensions_impl(nullptr,owners,error);
    }
    bool advance_ai_dimensions(const BattleInitialAiProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        return advance_dimensions_impl(&profile,owners,error);
    }
private:
    bool advance_dimensions_impl(const BattleInitialAiProfile* ai,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle actor dimensions: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitDimensions||!current(owners))return reject("current562B8-complete parent/children required");
        const auto& s=storage_->state;
        //No reached operation has written5E0 since the fresh clear.4D08C thus
        //stores zero without reading unbuilt bone/world matrices.27A44's AI
        //table entry is separate from the original human-only gateway.
        if(!s.source_clear_seed||s.native_revision!=7||s.role490||s.mode4a0||s.flags48a||(ai?(!ai->ready()||!s.ai3e6||s.model_domain!=BattleModelDomain::InitialAi):bool(s.ai3e6)))return reject("fresh ordinary matching human/AI branch required");
        try{BattleActorDimensions dimensions;
            if(!battle_actor_fresh_dimensions(storage_->binding.storage_->setup,s.scalar_effects,s.scalar_defaults.scale_b4,&dimensions,error))return false;
            if(ai){if(!ai->input(s.scalar_defaults.player.field20,&dimensions.input334)||dimensions.writes.empty()||dimensions.writes.back().offset!=0x334)return reject("actual AI selector/table required");dimensions.writes.back().value=dimensions.input334;}
            auto journal=storage_->journal;using K=BattleActorBirthWriteKind;
            journal.push_back({K::SourceBindingStage,0,0,0x80055dcc});
            for(const auto& w:dimensions.writes){
                if(w.offset==0x6d0)journal.push_back({K::SourceBindingStage,0,0,0x8004d08c});
                if(w.offset==0x7fc)journal.push_back({K::SourceBindingStage,0,0,0x80068004});
                if(w.offset==0x334)journal.push_back({K::SourceBindingStage,0,0,0x80027a44});
                const bool copied=(w.offset>=0x678&&w.offset<=0x680)||(w.offset>=0x814&&w.offset<=0x84c);
                journal.push_back({w.extent==1?K::Byte:copied?K::Word:K::FloatBits,w.offset,w.extent,w.value});
            }
            storage_->state.offset6d0=dimensions.offset6d0;storage_->state.dimensions=std::move(dimensions);++storage_->state.native_revision;storage_->journal=std::move(journal);storage_->dimensions_joined=true;status_=BattleActorConstructionStatus::AwaitLocalSetup;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattleActorConstructionStatus::Failed;storage_->parent_scope->live=false;return reject("uncertain scalar continuation; no replay");}
    }
public:
    bool advance_local_setup(const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle actor local setup: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitLocalSetup||!current(owners))return reject("current dimension-complete constructor required");
        const auto batch=storage_->allocation.batch_.lock();const auto entry=batch?batch->origin.lock():nullptr;const auto& prepared=*storage_->binding.storage_;const auto& s=storage_->state;
        if(!batch||!batch->queued||!entry||!entry->live||!entry->common.live()||!entry->common.block(2).size||!prepared.resource->motion_present()||!prepared.motion.loaded||s.native_revision!=8||!s.source_clear_seed||s.mode4a0)return reject("same native specific/common motion owners and fresh local boundary required");
        const auto& row=batch->queue.rows[s.resource3e7];const auto specific=prepared.resource->motion_bank();
        if(row.published[2].data!=specific.data||row.published[2].size!=specific.size)return reject("specific motion publication mismatch");
        try{BattleActorLocalSetup local;if(!battle_actor_local_setup(prepared.setup,prepared.model.rig(),storage_->resource_maps,s.position20[1],&local,error))return false;
            auto journal=storage_->journal;using K=BattleActorBirthWriteKind;using L=BattleActorLocalWriteKind;
            journal.push_back({K::SpecificMotionReference,0x1d78,4,s.resource3e7});journal.push_back({K::CommonMotionReference,0x1d7c,4,2});
            for(const auto& w:local.writes){const auto kind=w.kind==L::Byte?K::Byte:w.kind==L::Half?K::Half:w.kind==L::Word?K::Word:w.kind==L::FloatBits?K::FloatBits:w.kind==L::Clear?K::Clear:K::SourceBindingStage;journal.push_back({kind,w.offset,w.extent,w.value});}
            storage_->common_motion=entry->common;storage_->specific_motion=&prepared.motion;storage_->specific_motion_bytes=specific;
            storage_->state.local_setup=std::move(local);storage_->state.offset6d0=0;++storage_->state.native_revision;storage_->journal=std::move(journal);storage_->local_joined=true;status_=BattleActorConstructionStatus::AwaitProgramSeed;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain local initialization; no replay");}
    }
    bool advance_program_seed(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattleActorConstructionStatus::AwaitProgramSeed||!current(owners)){if(error)*error="battle program seed: current local initialization required";return false;}
        const auto batch=storage_->allocation.batch_.lock();const auto slot=storage_->state.resource3e7;const auto data=storage_->binding.storage_->resource->battle_data();
        if(!batch||!batch->queued||!data.data||!data.size||batch->queue.rows[slot].published[1].data!=data.data||batch->queue.rows[slot].published[1].size!=data.size){if(error)*error="battle program seed: same nonempty battle-data publication required";return false;}
        try{auto seed=battle_actor_program_seed();auto journal=storage_->journal;using K=BattleActorBirthWriteKind;journal.push_back({K::BattleDataReference,0x3fc,4,slot});
            for(const auto& w:seed.writes)journal.push_back({w.bytes==1?K::Byte:w.bytes==2?K::Half:K::Word,w.offset,w.bytes,w.value});
            storage_->battle_data3fc=data;storage_->state.program_seed=std::move(seed);++storage_->state.native_revision;storage_->journal=std::move(journal);storage_->program_seeded=true;status_=BattleActorConstructionStatus::AwaitTimers;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain program reset; no replay");}
    }
    bool advance_timer_prefix(const BattleActorTimerProfile& profile,SourceByteRngOwner& rng,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattleActorConstructionStatus::AwaitTimers||!current(owners)||!profile.ready()){if(error)*error="battle actor timers: current program-seed boundary/source profile required";return false;}
        const auto batch=storage_->allocation.batch_.lock();const auto entry=batch?batch->origin.lock():nullptr;
        if(!batch||!entry||!entry->live||entry->state.route15dd||!rng.same_owner(batch->party.rng_source_)||rng.revision()==std::numeric_limits<u64>::max()||rng.draws()>std::numeric_limits<u64>::max()-6){if(error)*error="battle actor timers: same live party RNG/normal route/bounded bookkeeping required";return false;}
        const auto snapshot=rng.snapshot();const auto& s=storage_->state;BattleActorTimerPrefixInput input;input.declared=true;input.domain=s.model_domain;input.model=s.model3e8;input.selector438=s.scalar_defaults.player.field20;input.level439=s.scalar_defaults.player.field32;input.field1c6=s.shorts1c4_1c6_1c8[1];
        try{BattleActorTimerPrefixPlan plan;if(!battle_actor_timer_prefix(profile,input,snapshot.state(),&plan,error))return false;
            auto journal=storage_->journal;for(const auto& w:plan.writes)journal.push_back({w.bytes==1?BattleActorBirthWriteKind::Byte:BattleActorBirthWriteKind::Half,w.offset,w.bytes,w.value});
            if(!current(owners)||!rng.commit(snapshot,plan.after,6))return fail_continuation(error,"timer/RNG owner changed before commit");
            storage_->state.timer_prefix=std::move(plan);++storage_->state.native_revision;storage_->journal=std::move(journal);storage_->timers_joined=true;status_=BattleActorConstructionStatus::AwaitTargetSelection;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain timer continuation; no replay");}
    }
    bool advance_class_initializer(const StartupActorClassProfile& profile,const StartupAuxiliaryProfile& auxiliary,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle class initializer: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitClassInitializer||!current(owners))return reject("current target-complete constructor required");
        const auto batch=storage_->allocation.batch_.lock();const auto entry=batch?batch->origin.lock():nullptr;const auto objects=storage_->children_pool.lock();
        auto& s=storage_->state;const bool gred=s.model3e8==0x615;
        if(!entry||!entry->live||!objects||!objects->ready()||!s.source_clear_seed||s.native_revision!=12||s.class_initialized||s.kind83||!s.self8c||!s.self90||s.variant3ec||s.mode4a0||s.class_fields.keys4b4_to_4f0[1]!=(gred?0x8018ce1cu:0x8006f708u))return reject("same normal parent/pool/source callback required");
        if(gred&&(!entry->state.normal_dispatch_returned||entry->state.common00!=u8(3)||!startup_auxiliary_profile_valid(auxiliary)))return reject("actual normal common-mode return and C74EC source operands required");
        StartupActorClassState input;input.model_id=s.model3e8;input.field4a8=s.field4a8;input.field6f6=s.field6f6;
        StartupActorClassState initialized;std::vector<StartupActorClassEffect> requests;
        if(!startup_actor_class_initialize(profile,input,&initialized,&requests))return reject("qualified source class profile required");
        try{
            auto journal=storage_->journal;storage_->children.reserve(storage_->children.size()+requests.size());
            using W=BattleActorBirthWriteKind;
            //Allocate candidate bookkeeping before the first source write/pop.
            journal.reserve(journal.size()+requests.size()*2+2);
            status_=BattleActorConstructionStatus::Failed;
            if(gred){s.field4a8=initialized.field4a8;s.field6f6=initialized.field6f6;journal.push_back({W::Byte,0x4a8,1,s.field4a8});journal.push_back({W::Half,0x6f6,2,s.field6f6});}
            SceneActorReference parent;parent.scope_=storage_->parent_scope;
            SceneObjectActorChildFields copied;copied.parent8c=copied.parent90=parent;copied.model94=s.render_model94;copied.index96=s.render_index96;copied.resource97=s.resource_index97;copied.team88=s.team88;
            copied.scale_b8=s.scalar_defaults.rank.scale_b8;copied.scale_b4=s.scalar_defaults.scale_b4;copied.scale.fill(copied.scale_b4);copied.fieldc0=s.scalar_defaults.fieldc0;copied.fieldc4=s.scalar_defaults.fieldc4;
            for(const auto& request:requests){
                const bool common=request.kind==StartupActorClassEffectKind::AuxiliaryC74ECRequired;
                const bool sasuke=request.kind==StartupActorClassEffectKind::Attachment4D6DCRequired;
                if(!common&&!sasuke&&request.kind!=StartupActorClassEffectKind::Auxiliary185F6CRequired)return fail_continuation(error,"unjoined class child request");
                BattleActorChildAttempt attempt;attempt.kind=common?BattleActorChildKind::CommonAuxiliary:BattleActorChildKind::BankAttachment;attempt.source_index=request.id;attempt.descriptors=2;
                attempt.result=objects->acquire(common?24:0,false,2,SceneObjectParentFields{s.kind83,s8(s.slot3e4)},&attempt.object,error);
                if(attempt.result==SceneObjectAcquire::Refused)return fail_continuation(error,"uncertain class child acquisition; occupancy retained");
                const u32 source=common?0x800c74ecu:sasuke?0x8004d6dcu:0x80185f6cu;
                if(attempt.result==SceneObjectAcquire::Allocated){
                    auto fields=copied;SceneObjectHeader header;
                    if(common){const auto& row=auxiliary.entries[request.id-40];header={0x0e,request.id,row.channel,0x800c7664,0x800c7954};fields.field13=2;}
                    else header={0,request.id,0,sasuke?0x8004d7fcu:0x801860c4u,sasuke?0x8004dabcu:0x80186480u};
                    if(!objects->install_actor_child(attempt.object,header,fields,common?std::nullopt:std::optional<u8>{0x18}))return fail_continuation(error,"uncertain class child header; occupancy retained");
                }
                storage_->children.push_back(attempt);journal.push_back({W::SourceBindingStage,0,0,source});
                if(common){
                    StartupAuxiliaryParent observed;observed.model94=s.render_model94;observed.kind83=s.kind83;
                    StartupAuxiliaryConstruction result;
                    if(!startup_auxiliary_construct(auxiliary,request.id-40,observed,*entry->state.common00,attempt.result==SceneObjectAcquire::Allocated,s.flags708,&result,error))return fail_continuation(error,"shared auxiliary constructor refused after source allocation");
                    s.auxiliary_children[request.id-40]=result;
                    if(result.parent_write){s.flags708=result.parent708;journal.push_back({W::Byte,0x708,1,s.flags708});}
                }
            }
            if(!storage_->binding.current(owners)||!roster_current(owners)||!entry->live||!objects->ready())return fail_continuation(error,"class parent/scene retired before commit");
            s.class_initialized=true;++s.native_revision;storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::AwaitPostClassChildren;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain class child continuation; no replay");}
    }
    bool advance_ai_class_initializer(const BattleInitialAiProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle AI class initializer: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitClassInitializer||!current(owners)||!profile.ready())return reject("current target-complete AI constructor required");
        const auto objects=storage_->children_pool.lock();auto& s=storage_->state;
        if(!objects||!objects->ready()||s.model_domain!=BattleModelDomain::InitialAi||!s.ai3e6||!battle_model_in_domain(s.model_domain,s.model3e8)||s.native_revision!=12||s.class_initialized||s.kind83||!s.self8c||!s.self90||s.variant3ec||s.mode4a0||s.class_fields.keys4b4_to_4f0[1]!=(s.model3e8==0x70b?0x8019c60cu:0x801985b0u))return reject("same initial AI parent/pool/class callback required");
        try{
            auto journal=storage_->journal;storage_->children.reserve(storage_->children.size()+3);journal.reserve(journal.size()+16);status_=BattleActorConstructionStatus::Failed;
            using W=BattleActorBirthWriteKind;
            if(s.model3e8==0x70b){
                SceneActorReference parent;parent.scope_=storage_->parent_scope;SceneObjectActorChildFields copied;
                copied.parent8c=copied.parent90=parent;copied.model94=s.render_model94;copied.index96=s.render_index96;copied.resource97=s.resource_index97;copied.team88=s.team88;
                copied.scale_b8=s.scalar_defaults.rank.scale_b8;copied.scale_b4=s.scalar_defaults.scale_b4;copied.scale.fill(copied.scale_b4);copied.fieldc0=s.scalar_defaults.fieldc0;copied.fieldc4=s.scalar_defaults.fieldc4;
                const unsigned count=s.level3ee>8?3:2;
                for(unsigned i=0;i<count;++i){
                    BattleActorChildAttempt attempt;attempt.kind=i?BattleActorChildKind::AiPointFollower:BattleActorChildKind::AiFieldFollower;attempt.source_index=u8(i);attempt.descriptors=2;
                    attempt.result=objects->acquire(0,false,2,SceneObjectParentFields{s.kind83,s8(s.slot3e4)},&attempt.object,error);
                    if(attempt.result==SceneObjectAcquire::Refused)return fail_continuation(error,"AI child allocator refused; prefix retained");
                    if(attempt.result==SceneObjectAcquire::Allocated){
                        auto fields=copied;SceneObjectHeader header;
                        if(!i){header={0,0,0,0x800c6394,0x800c6724};fields.field13=3;fields.field144=SceneActorFieldReference{parent,SceneActorFieldSlot::Flag168};}
                        else{header={0,3,0,0x801fcda8,0x801fd09c};fields.field144=SceneActorFieldReference{parent,i==1?SceneActorFieldSlot::Point150:SceneActorFieldSlot::Point15c};fields.field148=SceneActorFieldReference{parent,i==1?SceneActorFieldSlot::Flag169:SceneActorFieldSlot::Flag16a};}
                        if(!objects->install_actor_child(attempt.object,header,fields))return fail_continuation(error,"AI child field references refused");
                    }
                    storage_->children.push_back(attempt);journal.push_back({W::SourceBindingStage,0,0,i?0x801fcc50u:0x800c627cu});
                }
                s.ai_flags168_16b.fill(1);for(unsigned i=0;i<4;++i)journal.push_back({W::Byte,u16(0x168+i),1,1});
                for(unsigned i=0;i<2;++i){s.ai_points150_15c[i]=s.position20;s.ai_points150_15c[i][1]=f32(profile.point_height()+s.position20[1]);for(unsigned k=0;k<3;++k)journal.push_back({k==1?W::FloatBits:W::Word,u16(0x150+12*i+4*k),4,f32_to_bits(s.ai_points150_15c[i][k])});}
            }else{ //0105 does not enter010A's two DE5A4 allocations.
                s.ai_flags144_145={};journal.push_back({W::Byte,0x145,1,0});journal.push_back({W::Byte,0x144,1,0});
            }
            if(!storage_->binding.current(owners)||!roster_current(owners)||!objects->ready())return fail_continuation(error,"AI class owner retired");
            s.class_initialized=true;++s.native_revision;storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::AwaitPostClassChildren;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain AI class initialization; consumed prefix retained");}
    }
    bool advance_post_class_children(const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle post-class children: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitPostClassChildren||!current(owners))return reject("current class-initialized constructor required");
        const auto batch=storage_->allocation.batch_.lock();const auto entry=batch?batch->origin.lock():nullptr;const auto objects=storage_->children_pool.lock();auto& s=storage_->state;
        if(!entry||!entry->live||!entry->scene45||!objects||!objects->ready()||s.native_revision!=13||!s.class_initialized||s.post_class_children)return reject("same scene common64/scene45/pool boundary required");
        try{
            auto journal=storage_->journal;journal.reserve(journal.size()+2);storage_->children.reserve(storage_->children.size()+2);
            const bool enabled=!entry->state.common64&&std::bit_cast<s8>(*entry->scene45)<5;
            SceneActorReference parent;parent.scope_=storage_->parent_scope;
            status_=BattleActorConstructionStatus::Failed;
            for(unsigned i=0;i<2;++i){
                journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,i?0x800813a8u:0x800807e4u});
                if(!enabled)continue;
                BattleActorChildAttempt attempt;attempt.kind=i?BattleActorChildKind::PostClassOne:BattleActorChildKind::PostClassSix;attempt.source_index=u8(i);attempt.descriptors=i?1:6;
                //8893C is the unparented allocator: group56 is not slot-biased.
                attempt.result=objects->acquire(56,false,attempt.descriptors,std::nullopt,&attempt.object,error);
                if(attempt.result==SceneObjectAcquire::Refused)return fail_continuation(error,"uncertain post-class acquisition; occupancy retained");
                if(attempt.result==SceneObjectAcquire::Allocated){
                    SceneObjectActorChildFields fields;fields.parent8c=fields.parent90=parent;
                    const SceneObjectHeader header{0,0,0,i?0x80081438u:0x80080874u,i?0x80081584u:0x80080a74u};
                    if(!objects->install_actor_child(attempt.object,header,fields))return fail_continuation(error,"uncertain post-class header; occupancy retained");
                }
                storage_->children.push_back(attempt);
            }
            if(!storage_->binding.current(owners)||!roster_current(owners)||!entry->live||!objects->ready())return fail_continuation(error,"post-class owner retired");
            s.post_class_children=true;++s.native_revision;storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::AwaitCollisionInitialization;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain post-class continuation; no replay");}
    }
    bool advance_collision_construction(const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle collision construction: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitCollisionInitialization||!current(owners))return reject("current post-class constructor required");
        const auto pool=storage_->children_pool.lock();auto& s=storage_->state;const auto source=storage_->battle_data3fc;
        if(!pool||!pool->collision_snapshot()||s.native_revision!=14||!s.source_clear_seed||s.mode4a0||s.flags48a||s.kind83||s.field501||s.collision_construction||!s.class_fields.parameters_key27c)return reject("same actual class4 pool/fresh normal binding required");
        try{
            std::vector<u8> ids;if(!character_collision_body_ids(source,&ids,error))return false;
            std::vector<SceneCollisionFields> planned;planned.reserve(ids.size());std::vector<u16> flags;flags.reserve(ids.size());
            SceneActorReference parent;parent.scope_=storage_->parent_scope;
            for(u8 id:ids){
                const u32 offset=0x20+80u*id;const ByteView row{source.data+offset,80};SceneCollisionFields f;
                f.parent20=f.parent24=f.descriptor_owner28=f.parameter_owner2c=f.matrix_owner60=parent;
                f.descriptor_offset28=offset;f.parameter_source_key=s.class_fields.parameters_key27c;f.record12=id;f.parameter11=u8(row.be16(4));f.start48=std::bit_cast<s16>(row.be16(6));f.end4a=std::bit_cast<s16>(row.be16(8));
                if(row.data[1]&0x80u)f.matrix_kind=SceneActorMatrixSlotKind::Body1aa4;
                else{f.bone=row.data[1];if(f.bone>=s.bone_count1d90)return reject("unrepresented logical matrix slot; no fabricated world");}
                planned.push_back(f);flags.push_back(row.be16(2));
            }
            auto journal=storage_->journal;journal.reserve(journal.size()+2+3*ids.size());storage_->collision_attempts.reserve(ids.size());
            status_=BattleActorConstructionStatus::Failed;s.collision_enabled86=1;s.field1d8=0;
            using W=BattleActorBirthWriteKind;journal.push_back({W::Byte,0x86,1,1});journal.push_back({W::Byte,0x1d8,1,0});
            for(unsigned i=0;i<ids.size();++i){
                BattleActorCollisionAttempt attempt;attempt.ordinal=u8(i);attempt.record=ids[i];
                attempt.result=pool->acquire_collision(48,false,SceneObjectParentFields{s.kind83,s8(s.slot3e4)},&attempt.object,error);
                if(attempt.result==SceneObjectAcquire::Refused)return fail_continuation(error,"uncertain class4 allocation; consumed prefix retained");
                journal.push_back({W::SourceBindingStage,0,0,0x80088aa0});
                if(attempt.result==SceneObjectAcquire::Allocated){
                    auto fields=planned[i];fields.self5c={4,attempt.object.index()};
                    if(!pool->install_collision_child(attempt.object,fields))return fail_continuation(error,"uncertain collision fields; consumed prefix retained");
                    if((flags[i]&0x200u)&&s.field501<4){const auto slot=s.field501++;storage_->collision_slots[slot]=attempt.object;s.fields530_to_53c[slot]=fields.self5c;journal.push_back({W::Byte,0x501,1,s.field501});journal.push_back({W::CollisionReference,u16(0x530+4*slot),4,attempt.object.index()});}
                }
                storage_->collision_attempts.push_back(attempt);
            }
            if(!storage_->binding.current(owners)||!roster_current(owners)||!pool->ready())return fail_continuation(error,"collision source/pool retired");
            s.collision_construction=true;++s.native_revision;storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::AwaitPlacementHistory;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain collision construction; no replay");}
    }
    std::span<const BattleActorCollisionAttempt> collision_attempts(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->state.collision_construction?std::span<const BattleActorCollisionAttempt>{storage_->collision_attempts}:std::span<const BattleActorCollisionAttempt>{};}
    //Resolve to the SAME retained source bytes, never a separately decoded bank.
    //The parameter table and world matrices are not exposed as available data.
    ByteView collision_descriptor(const SceneCollisionLease& lease,const std::array<const ForceInventory*,2>& owners)const{
        if(!current(owners)||!storage_->state.collision_construction)return {};
        const auto pool=storage_->children_pool.lock();const auto* f=pool?pool->collision_child(lease):nullptr;SceneActorReference self;self.scope_=storage_->parent_scope;
        if(!f||!(f->descriptor_owner28==self)||!storage_->battle_data3fc.contains(f->descriptor_offset28,80))return {};
        return {storage_->battle_data3fc.data+f->descriptor_offset28,80};
    }
    bool advance_initial_controls(const BattleActorInitialControlsProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattleActorConstructionStatus::AwaitProgramStart||!current(owners)){if(error)*error="battle initial controls: current placement-complete constructor required";return false;}
        const auto& s=storage_->state;
        if(s.native_revision!=16||!s.placement_complete||s.initial_controls.complete||s.mode4a0||s.field742||!profile.matches(s.model3e8,storage_->binding.storage_->setup.model_hash,s.class_fields.keys4b4_to_4f0[15])){if(error)*error="battle initial controls: same normal4F0 table/selector0 required";return false;}
        try{
            auto next=s.initial_controls;auto journal=storage_->journal;using K=BattleActorBirthWriteKind;
            auto byte=[&](u16 at,u8 value){journal.push_back({K::Byte,at,1,value});};
            //6A750 is the secondary selection;6A6FC follows for the primary.
            journal.push_back({K::SourceBindingStage,0,0,0x8006a750});
            next.previous7cf_7ce[1]=next.selected5e5_5e4[1];byte(0x7ce,next.previous7cf_7ce[1]);next.selected5e5_5e4[1]=0;byte(0x5e4,0);
            next.previous5dc_5dd[1]=next.current5da_5db[1];byte(0x5dd,next.previous5dc_5dd[1]);next.current5da_5db[1]=profile.secondary();byte(0x5db,next.current5da_5db[1]);
            for(unsigned i:{7u,6u,5u,4u}){next.fields540[i]=0;byte(u16(0x540+i),0);}
            next.field588=0;byte(0x588,0);next.field754=0;journal.push_back({K::Word,0x754,4,0});next.field1db3=0;byte(0x1db3,0);
            journal.push_back({K::SourceBindingStage,0,0,0x8006a6fc});
            next.previous7cf_7ce[0]=next.selected5e5_5e4[0];byte(0x7cf,next.previous7cf_7ce[0]);next.selected5e5_5e4[0]=0;byte(0x5e5,0);
            next.previous5dc_5dd[0]=next.current5da_5db[0];byte(0x5dc,next.previous5dc_5dd[0]);next.current5da_5db[0]=profile.primary();byte(0x5da,next.current5da_5db[0]);
            for(unsigned i:{3u,2u,1u,0u}){next.fields540[i]=0;byte(u16(0x540+i),0);}
            if(!current(owners))return fail_continuation(error,"control owner changed before commit");
            next.complete=true;storage_->state.initial_controls=next;++storage_->state.native_revision;storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::AwaitInitialProgramRequest;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain initial control selection; no replay");}
    }
    bool advance_initial_program(const BattleActorInitialProgramProfile& profile,const CharacterPoseHeightProfile& height,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle initial program: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitInitialProgramRequest||!current(owners))return reject("current initial-controls boundary required");
        const auto& s=storage_->state;const auto& prepared=*storage_->binding.storage_;
        if(s.native_revision!=17||!s.source_clear_seed||!s.placement_complete||!s.initial_controls.complete||s.initial_program_requested||s.mode4a0||s.role490||s.flags48a||s.field742||!character_pose_height_profile_valid(height)||!profile.matches(prepared.setup,s.class_fields.program_keys1d80[0],s.class_fields.program_keys1d80[2]))return reject("same normal class/program/profile and fresh source state required");
        try{
            auto program=std::make_unique<InitialProgram>();program->profile=profile;program->height=height;
            //The selected descriptors use specific bank0/entry9. Resolve from
            //the actual4BB14-bound bank; callers cannot provide substitute clips.
            const auto& descriptor=profile.asset().descriptors.front();std::vector<CharacterMotionClipBinding> clips;
            for(unsigned lane=0;lane<2;++lane){const u8 group=descriptor.bytes[4+8*lane],choice=descriptor.bytes[5+8*lane];
                if(group&0x80u)return reject("common-bank request consumer not joined");
                if(std::any_of(clips.begin(),clips.end(),[&](const auto& c){return c.group==group&&c.index==choice;}))continue;
                CharacterMotionSelection selection;CharacterMotionClipBinding clip;clip.group=group;clip.index=choice;
                if(!character_motion_select(*storage_->specific_motion,group,choice,&selection,error)||!hsd_bone_motion_load(selection.archive,prepared.model.rig().binding,&clip.clip,error))return false;
                clips.push_back(std::move(clip));
            }
            if(clips.size()!=1||!clips[0].clip.public_bone_count)return reject("qualified shared initial clip required");
            program->root_mode=clips[0].clip.bones[0].has_aobj?CharacterMotionRootObservationMode::AObjOnly:CharacterMotionRootObservationMode::AObjOrPublicNull;
            const auto& cfg=prepared.setup.class_config;const std::array<s8,2> model_bones{std::bit_cast<s8>(cfg[0x42]),std::bit_cast<s8>(cfg[0x43])};
            if(!program->driver.load_request_prefix(prepared.resource->model_hsd(),prepared.model.rig(),profile.asset(),clips,s.local_setup.poses[0],model_bones,error)||!program->vm.load_fresh_actor_request_prefix(profile.asset(),{0,s.local_setup.field6d8,s.offset6d0},s.position20,error))return false;
            CharacterPoseHeightFields fields;fields.model_group3e8=u8(s.model3e8>>8);fields.field661=s.scalar_effects.field661;fields.scale_b4=s.scalar_defaults.scale_b4;fields.actor_y=s.position20[1];fields.height66c=s.dimensions.field66c;fields.radius670=s.dimensions.field670;
            //No5AE/598/1DB2 consumer has changed their fresh clears. In
            //particular the placed heading72 is NOT a substitute for5AE.
            CharacterMotionRootFields root;root.bias1d9c=s.local_setup.weight1d9c;root.delta5f4=s.scalar_effects.rate5f4;root.scale_b4=s.scalar_defaults.scale_b4;root.multipliers1dd4=s.local_setup.multipliers1dd4;
            if(!program->controls.bind_with_root(program->vm,program->driver,height,fields,{},root,error,program->root_mode)||!program->vm.request_with_actor_controls(0,8,u8(15&s.local_setup.mask579),-1,s.local_setup.motion_delta1dc8,program->driver,program->controls,&program->operations,error,{0,0}))return false;
            CharacterLocalPose pose;if(!program->controls.compose_local_pose(program->vm.state(),&pose,error))return false;
            const auto& live=program->vm.state();
            if(live.actor.root.position!=s.position20||live.actor.pose_height.offset6d0!=s.offset6d0||live.actor.pose_height.multiplier6d8!=1)return reject("unexpected initial position/height consumer");
            for(const auto& lane:live.lanes)if(!lane.motion.selected||lane.wait!=24||lane.cursor!=28||lane.done)return reject("first source wait not reached");
            auto next=s;auto journal=storage_->journal;journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,0x8004beb8});
            next.local_setup.poses[0]=std::move(pose);next.initial_program_requested=true;
            //Remaining541AC tail: fresh null508, role490=0, mode4A0=0 and
            //unchanged489/48A bypass retained/respawn branches; only500 is stored.
            next.field500=0;journal.push_back({BattleActorBirthWriteKind::Byte,0x500,1,0});next.constructor_complete=true;++next.native_revision;
            if(!current(owners))return fail_continuation(error,"program source owner changed before commit");
            storage_->initial_program=std::move(program);storage_->state=std::move(next);storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::ConstructedAwaitWorld;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain initial program request; no replay");}
    }
    const CharacterProgramState* initial_program_state(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->initial_program?&storage_->initial_program->vm.state():nullptr;}
    //Inspection only; model publication uses the private owner/generation join.
    const BattleActorWorldAfterLook* current_world(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->state.world_generation&&storage_->first_update?&storage_->first_update->world:nullptr;}
    const BattleActorWorldAfterLook* first_world(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->state.world_generation==1?current_world(owners):nullptr;}
    std::span<const BattleActorPublishedModel> published_models(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->publication?std::span<const BattleActorPublishedModel>{storage_->publication->models}:std::span<const BattleActorPublishedModel>{};}
    const HsdSkinGeometry* body_geometry(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->state.model_geometry_prepared?&storage_->binding.storage_->model.world_geometry():nullptr;}
    const CharacterMotionSnapshot* initial_motion_snapshot(unsigned lane,const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->initial_program&&lane<2?storage_->initial_program->controls.snapshot(storage_->initial_program->vm.state().lanes[lane].motion):nullptr;}
    std::span<const CharacterProgramOperation> initial_program_operations(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->initial_program?std::span<const CharacterProgramOperation>{storage_->initial_program->operations}:std::span<const CharacterProgramOperation>{};}
    ByteView common_motion_input(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->local_joined?storage_->common_motion.block(2):ByteView{};}
    ByteView specific_motion_input(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->local_joined?storage_->specific_motion_bytes:ByteView{};}
    ByteView battle_program_input(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->program_seeded?storage_->battle_data3fc:ByteView{};}
    //Inspection only. A copied snapshot is not a step/frame/collider capability.
    const BattleActorBirthState* state(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?&storage_->state:nullptr;}
    const ChallengePartyMember* selected_member(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?&storage_->selected:nullptr;}
    std::span<const BattleActorBirthWrite> writes(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?std::span<const BattleActorBirthWrite>{storage_->journal}:std::span<const BattleActorBirthWrite>{};}
    std::weak_ptr<const CharacterModelBinding> model_binding()const{return storage_?storage_->binding.model_binding():std::weak_ptr<const CharacterModelBinding>{};}
    const BattleCharacterBinding* prepared_binding(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?&storage_->binding:nullptr;}
    const BattleActorResourceMaps* resource_maps(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->resources_joined?&storage_->resource_maps:nullptr;}
    std::size_t bound_material_contexts(const std::array<const ForceInventory*,2>& owners)const{if(!current(owners)||!storage_->resources_joined)return 0;std::size_t count=0;for(const auto& model:storage_->material_contexts)count+=model.size();return count;}
    const BattleActorLevelRow* selected_level_row(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->level_cost_joined?&storage_->level_row:nullptr;}
    const SceneArchiveRecord* scene_archive_record(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->archive_bound?storage_->archive.inspect():nullptr;}
    std::span<const BattleActorChildAttempt> child_attempts(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->effects_joined?std::span<const BattleActorChildAttempt>{storage_->children}:std::span<const BattleActorChildAttempt>{};}
    BattleActorConstructionStatus status()const{return status_;}
    BattleActorConstructionPending pending()const{
        BattleActorConstructionPending p;if(!storage_)return p;
        if(storage_->member_joined)p.member_68344=false;
        if(storage_->class_joined)p.class_selection_56ce0=false;
        if(storage_->resources_joined)p.model_records_419ec=p.bone_parts_5380c=p.motion_tables_41b20=false;
        if(storage_->level_cost_joined)p.level_cost_55f90=false;
        if(storage_->defaults_joined)p.defaults_56180=false;
        if(storage_->archive_bound)p.scene_archive_binding=false;
        if(storage_->binding.storage_&&storage_->binding.storage_->descriptors_bound)p.scene_model_descriptors=false;
        if(storage_->effects_joined)p.scalar_effects_562b8=false;
        if(storage_->dimensions_joined)p.dimensions_55dcc=p.scalar_setup=false;
        if(storage_->local_joined)p.local_pose_4bb14=false;
        if(storage_->program_seeded)p.program_seed_3cc24=false;
        if(storage_->timers_joined)p.timer_prefix_1c80c=false;
        if(storage_->state.initial_target.complete)p.timer_and_target_rng=false;
        if(storage_->state.class_initialized)p.class_initializer_4b8=false;
        if(storage_->state.collision_construction)p.collision_construction_8ab30=false;
        if(storage_->state.placement_complete)p.placement_history=false;
        if(storage_->state.initial_program_requested)p.initial_program_request_4beb8=false;
        if(storage_->state.constructor_complete)p.full_constructor=false;
        if(storage_->state.first_update_completed)p.first_world_frame=false;
        if(storage_->state.model_world_published)p.model_world_publication=false;
        return p;
    }
    u32 next_source_call()const{
        if(!storage_)return 0;
        switch(status_){
        case BattleActorConstructionStatus::AwaitMemberInitializer:return storage_->profile.words[6];
        case BattleActorConstructionStatus::AwaitClassSelection:return 0x80056ce0;
        case BattleActorConstructionStatus::AwaitModelBinding:return 0x800419ec;
        case BattleActorConstructionStatus::AwaitScalarSetup:return 0x80055f90;
        case BattleActorConstructionStatus::AwaitScalarDefaults:return 0x8005614c;
        case BattleActorConstructionStatus::AwaitScalarEffects:return 0x800562b8;
        case BattleActorConstructionStatus::AwaitDimensions:return 0x80055dcc;
        case BattleActorConstructionStatus::AwaitLocalSetup:return 0x8004bb14;
        case BattleActorConstructionStatus::AwaitProgramSeed:return 0x8003cc24;
        case BattleActorConstructionStatus::AwaitTimers:return 0x8001c80c;
        case BattleActorConstructionStatus::AwaitTargetSelection:return 0x8002532c;
        case BattleActorConstructionStatus::AwaitClassInitializer:return storage_->state.class_fields.keys4b4_to_4f0[1];
        case BattleActorConstructionStatus::AwaitPostClassChildren:return 0x800807e4;
        case BattleActorConstructionStatus::AwaitCollisionInitialization:return 0x8008ab30;
        case BattleActorConstructionStatus::AwaitPlacementHistory:return 0x80056d28;
        case BattleActorConstructionStatus::AwaitProgramStart:return 0x8006a750;
        case BattleActorConstructionStatus::AwaitInitialProgramRequest:return 0x8004beb8;
        default:return 0;
        }
    }
    void retire(){if(storage_){if(storage_->publication){const auto scope=storage_->publication->scope.lock();if(scope)scope->live_=false;}storage_->parent_scope->live=false;storage_->parent_scope->owner=nullptr;}storage_.reset();status_=BattleActorConstructionStatus::Retired;attempted_=true;}
private:
    friend class BattlePreparationBootstrap;
    bool prepare_published_geometry(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(!current(owners)||!storage_->publication||storage_->state.native_revision!=21||storage_->state.model_geometry_prepared){if(error)*error="battle model geometry: current unconsumed publication required";return false;}
        auto& prepared=*storage_->binding.storage_;auto& publication=*storage_->publication;
        try{
            CharacterWorldPose world;const auto* root=prepared.descriptors.empty()?nullptr:storage_->binding.bound_instance(0);if(!root||!root->world_current())return false;
            const auto& joints=root->state()->joints;world.joint_offsets.resize(storage_->state.bone_count1d90);world.matrices.resize(world.joint_offsets.size());
            u32 visible=0;
            for(unsigned i=0;i<world.matrices.size();++i){const auto& map=storage_->resource_maps.bones[0][i];if(map.instance_joint>=joints.size()||joints[map.instance_joint].joint.archive_offset!=map.joint_offset||!joints[map.instance_joint].world)return false;
                const auto& joint=joints[map.instance_joint];world.joint_offsets[i]=map.joint_offset;world.matrices[i]=*joint.world;if(!(joint.joint.flags&0x10u))visible|=1u<<i;}
            ActorFadeMaterial fade;if(!prepared.material.material(prepared.model,&fade))return false;
            if(!publication.parts.bind_prepared(prepared.model,prepared.bank,storage_->parts_profile,error))return false;
            CharacterModelRenderState render_state;render_state.visible_bones=visible;render_state.dynamic_material=true;
            if(!prepared.material.publish(prepared.model,world,render_state,error)||!publication.parts.prepare(prepared.model,world,1,storage_->initial_program->vm.state().actor.models,visible,storage_->state.suppressed810,fade,error)){
                prepared.model.discard_pose();publication.parts.discard_pose();return fail_continuation(error,"published-world skin/part evaluation refused");
            }
            //Verify the rigid evaluator consumed the SAME published part roots.
            const auto selected=publication.parts.selection();
            for(unsigned i=0;i<2;++i)if(selected.visible[i]){const auto* part=storage_->binding.bound_instance(selected.record[i]);if(!part||!part->world_current()||part->state()->joints.size()!=1||!part->state()->joints[0].world||!std::equal(part->state()->joints[0].world->m,part->state()->joints[0].world->m+16,world.matrices[storage_->parts_profile.groups[i].bone].m))return fail_continuation(error,"part renderer/publication source mismatch");}
            storage_->state.model_geometry_prepared=true;++storage_->state.native_revision;if(error)error->clear();return true;
        }catch(const std::exception&){prepared.model.discard_pose();publication.parts.discard_pose();return fail_continuation(error,"uncertain published geometry; no draw grant");}
    }
    bool publish_models(ModelPassQueue& queue,const std::shared_ptr<const HsdJointWorldScope>& frame,u64 frame_generation,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle model publication: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::SpawnWorldReady||!current(owners)||!frame||!frame->live()||!queue.ready()||!frame_generation)return reject("current collected spawn/world callback required");
        const auto& s=storage_->state;auto& prepared=*storage_->binding.storage_;const auto pool=prepared.descriptor_pool.lock();
        if(!pool||s.native_revision!=20||s.world_generation!=2||!s.spawn_entry_completed||s.model_world_published||!s.active00||!s.render_enabled82||s.callback10c_key!=0x80056aa8||s.mode4a0||s.flags48a||s.hidden_views270||s.forced_hidden59a||s.invulnerability720!=0||s.class_fields.keys4b4_to_4f0[8])return reject("normal initial visibility/class callback domain required");
        //10514 is read-only.270=0 makes its enclosing conjunction false for
        //every view;59A=0/720=0 then force style0. No active camera is invented.
        const auto& world=storage_->first_update->world.geometry.world;
        CharacterModelPartSelection parts;if(!character_model_parts_select(storage_->parts_profile,storage_->initial_program->vm.state().actor.models,s.visible76c,s.suppressed810,&parts))return reject("same current part-map selectors required");
        try{
            struct Pending {u16 record=0;SceneDescriptorLease lease;HsdJointInstanceReceipt identity;HsdJointInstanceStateSet state;};
            std::vector<Pending> pending;pending.reserve(3);auto publication=std::make_unique<Publication>();publication->scope=frame;publication->models.reserve(3);
            auto prepare=[&](u16 record,const std::vector<SceneModelBindings::JointWorld>& writes){
                if(record>=storage_->resource_maps.models[0].size())return false;
                const auto index=storage_->resource_maps.models[0][record].instance;if(index>=prepared.descriptors.size())return false;
                const auto& descriptor=prepared.descriptors[index];Pending p;p.record=record;p.lease=descriptor.lease;p.identity=descriptor.identity;
                if(!SceneModelBindings::prepare_actor_world(*pool,p.lease,storage_->archive,p.identity,prepared.bank_material.get(),record,s.world_generation,writes,&p.state,error))return false;
                publication->models.push_back({record,p.lease.index(),s.world_generation});pending.push_back(std::move(p));return true;
            };
            if(world.joint_offsets.size()!=s.bone_count1d90||world.matrices.size()!=s.bone_count1d90)return reject("current logical world extent");
            std::vector<SceneModelBindings::JointWorld> body;body.reserve(s.bone_count1d90);
            for(unsigned i=0;i<s.bone_count1d90;++i){if(world.joint_offsets[i]!=storage_->resource_maps.bones[0][i].joint_offset)return reject("same logical608 joint map required");body.push_back({world.joint_offsets[i],world.matrices[i],bool(s.visible76c&(1u<<i))});}
            if(!prepare(0,body))return false;
            for(unsigned i=0;i<2;++i)if(parts.visible[i]){
                const auto record=parts.record[i];const auto bone=storage_->parts_profile.groups[i].bone;
                if(record>=storage_->resource_maps.models[0].size()||bone>=s.bone_count1d90)return reject("part source row/bone extent");
                const std::vector<SceneModelBindings::JointWorld> writes{{storage_->resource_maps.models[0][record].root,world.matrices[bone],std::nullopt}};
                if(!prepare(record,writes))return false;
            }
            auto journal=storage_->journal;journal.reserve(journal.size()+8);journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,0x80056aa8});journal.push_back({BattleActorBirthWriteKind::Byte,0xae,1,0});
            if(!current(owners))return reject("owner changed before source writes");
            for(unsigned i=0;i<pending.size();++i){
                auto& p=pending[i];
                if(i==1)journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,0x8004fcfc});
                if(i)journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,0x800076d0});
                if(!SceneModelBindings::commit_world(*pool,p.lease,p.identity,frame,std::move(p.state))||!queue.register_model(u32(p.lease.index())+1,0x47,pool->inspect(p.lease)->fields.payload10_1f[0]!=0)){frame->live_=false;return fail_continuation(error,"uncertain root/queue publication; source prefix retained");}
                std::array<u32,3> links;if(!queue.source_links(u32(p.lease.index())+1,&links)||!SceneModelBindings::bind_queue_links(*pool,p.lease,frame,links,frame_generation)){frame->live_=false;return fail_continuation(error,"uncertain source model links; no replay");}
                journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,0x800097b4});
            }
            if(pending.size()==1)journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,0x8004fcfc});
            storage_->publication=std::move(publication);storage_->state.alternate_ae=0;storage_->state.model_world_published=true;++storage_->state.native_revision;storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::ModelsPublished;if(error)error->clear();return true;
        }catch(const std::exception&){frame->live_=false;return fail_continuation(error,"uncertain model callback; consumed prefix retained");}
    }
    static void copy_update_history(BattleActorBirthState& next,std::vector<BattleActorBirthWrite>& journal){
        using K=BattleActorBirthWriteKind;journal.push_back({K::SourceBindingStage,0,0,0x8005685c});
        next.history5e8=next.previous2c;next.previous2c=next.position20;next.previous76=next.angles70;next.previous_look1d6a=next.local_setup.angles1d64;next.local_setup.field6d4=next.offset6d0;
        auto copy=[&](u16 at,f32 value){journal.push_back({K::Word,at,4,f32_to_bits(value)});};
        for(unsigned i=0;i<3;++i)copy(u16(0x5e8+4*i),next.history5e8[i]);
        for(unsigned i=0;i<3;++i)copy(u16(0x2c+4*i),next.previous2c[i]);
        journal.push_back({K::Word,0x76,4,(u32(u16(next.previous76[0]))<<16)|u16(next.previous76[1])});journal.push_back({K::Half,0x7a,2,u16(next.previous76[2])});
        journal.push_back({K::Word,0x1d6a,4,(u32(u16(next.previous_look1d6a[0]))<<16)|u16(next.previous_look1d6a[1])});journal.push_back({K::Half,0x1d6e,2,u16(next.previous_look1d6a[2])});copy(0x6d4,next.local_setup.field6d4);
    }
    bool begin_first_update(const BattleActorFirstUpdateProfile& profile,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattleActorConstructionStatus::Cleared||!current(owners)||!profile.ready()||storage_->state.native_revision||storage_->first_update||!storage_->state.source_clear_seed){if(error)*error="battle first update: fresh registered body/prelude required";return false;}
        try{
            auto update=std::make_unique<FirstUpdate>();update->profile=profile;auto next=storage_->state;auto journal=storage_->journal;
            copy_update_history(next,journal);
            //CC is null in this actual untouched factory seed; no524 copies.
            next.first_update_started=true;storage_->first_update=std::move(update);storage_->state=std::move(next);storage_->journal=std::move(journal);if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain first update prelude; no replay");}
    }
    bool finish_first_update(const BattleGroundWorldProfile& world_profile,const BattleGroundIkProfile& ik,const BattleLookTailProfile& look_profile,const ChallengeStageInputs& stage,u32 stage_id,const std::array<f32,4>& bounds,const ScenePhysicalControlFields& controls,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle first update: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::ConstructedAwaitWorld||!current(owners)||!storage_->first_update)return reject("same fresh caller prelude and completed constructor required");
        const auto& s=storage_->state;const auto& vm=storage_->initial_program->vm.state();
        if(!s.first_update_started||s.first_update_completed||s.native_revision!=18||s.world_generation||s.mode4a0||s.flags48a||s.field730||s.phase18!=1||(controls.d2&s.mask3e5)||!stage.native_data_bound()||stage.stage()!=stage_id||s.scalar_effects.colours7ec!=std::array<u32,2>{})return reject("qualified first normal caller/gates/stage/empty colour streams required");
        //5A580's ordinary in-bounds return is reached here. Bounce/clamp effects
        //must not be dropped or approximated when another caller reaches them.
        const f32 margin=storage_->first_update->profile.bound_margin();
        if((vm.actor.behaviour&0x02000000u)||s.position20[0]<bounds[1]+margin||s.position20[0]>bounds[0]-margin||s.position20[2]<bounds[3]+margin||s.position20[2]>bounds[2]-margin)return reject("bound/clamp/bounce consumer required");
        try{
            BattleActorWorldAfterLook world;if(!evaluate_owned_world(s,world_profile,ik,look_profile,stage,controls,&world,error))return false;
            auto next=s;auto journal=storage_->journal;using K=BattleActorBirthWriteKind;
            next.local_setup.origin_y6dc=f32(next.position20[1]+next.offset6d0);journal.push_back({K::FloatBits,0x6dc,4,f32_to_bits(next.local_setup.origin_y6dc)});
            journal.push_back({K::SourceBindingStage,0,0,0x8008b480}); //actual sceneEB;730=0 skips push branch
            journal.push_back({K::SourceBindingStage,0,0,0x8005a580});journal.push_back({K::SourceBindingStage,0,0,0x800465fc});
            next.local_setup.adjust1d54=world.geometry.contacts.adjust1d54;next.local_setup.velocity1d60=world.geometry.contacts.velocity1d60;
            for(unsigned i=0;i<s.bone_count1d90;++i)next.local_setup.extra_angles[i]=world.next_modifiers.extra_angles[i];
            next.field1ca=next.field732=next.field730=next.field731=0;for(u16 at:{u16(0x1ca),u16(0x732),u16(0x730),u16(0x731)})journal.push_back({K::Byte,at,1,0});
            next.push724.fill(0);for(u16 at:{u16(0x724),u16(0x728),u16(0x72c)})journal.push_back({K::FloatBits,at,4,0});
            journal.push_back({K::SourceBindingStage,0,0,0x80006284});
            if(!current(owners))return fail_continuation(error,"world source owner changed before commit");
            next.first_update_completed=true;next.world_generation=1;++next.native_revision;world.geometry.actor_frame_owner_pending=false;
            storage_->first_update->world=std::move(world);storage_->state=std::move(next);storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::FirstWorldReady;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain first world computation; no replay");}
    }
    bool evaluate_owned_world(const BattleActorBirthState& s,const BattleGroundWorldProfile& world_profile,const BattleGroundIkProfile& ik,const BattleLookTailProfile& look_profile,const ChallengeStageInputs& stage,const ScenePhysicalControlFields& controls,BattleActorWorldAfterLook* out,std::string* error)const{
        const auto& prepared=*storage_->binding.storage_;const auto& vm=storage_->initial_program->vm.state();
        CharacterPoseFrame modifiers;if(!character_pose_initial_frame(prepared.model.rig(),Mat4{},&modifiers,error))return false;
        for(unsigned i=0;i<s.bone_count1d90;++i){modifiers.scales[i]=s.local_setup.scales[0][i];modifiers.extra_angles[i]=s.local_setup.extra_angles[i];}modifiers.bone1_look_angles=s.local_setup.angles1d64;
        BattleActorWorldInput actor;actor.declared=actor.retained1dbc_declared=true;actor.position20=s.position20;actor.scale58=s.scale58;actor.angles70=s.angles70;actor.offset6d0=s.offset6d0;actor.retained1dbc=s.local_setup.retained1dbc;
        BattleGroundContactInput ground;ground.declared=true;ground.flags5e0=vm.actor.behaviour;ground.control1d98=vm.actor.lane1_descriptor[0];ground.control1d99=vm.actor.lane1_descriptor[1];ground.scalar_b4=s.scalar_defaults.scale_b4;ground.prior_velocity1d60=s.local_setup.velocity1d60;
        ground.ground={true,s.program_seed.flags[2],s.position20[1],s.previous2c[1],s.offset6d0,s.local_setup.field6d4};
        BattleLookTailInput look;look.gates_declared=true;look.scene_maskd2=controls.d2;look.owner_mask3e5=s.mask3e5;look.field741=s.field741;look.field660=s.scalar_effects.field660;look.phase18=s.phase18;
        return battle_actor_world_after_look(world_profile,ik,look_profile,prepared.setup,prepared.model.rig(),s.local_setup.poses[0],modifiers,actor,ground,look,stage,out,error);
    }
    bool advance_spawn_entry(const BattleActorSpawnEntryProfile& profile,const BattleGroundWorldProfile& world_profile,const BattleGroundIkProfile& ik,const BattleLookTailProfile& look_profile,const ChallengeStageInputs& stage,const ScenePhysicalControlFields& controls,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle first spawn entry: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners)||!storage_->first_update||!profile.ready())return reject("same completed first caller/source profile required");
        const auto& s=storage_->state;auto& prepared=*storage_->binding.storage_;
        if(s.native_revision!=19||s.world_generation!=1||s.spawn_entry_completed||s.phase18!=1||s.phase19||s.phase1a||s.mode4a0||s.field741!=1||s.field742||s.field730||s.render_enabled82||s.render_index96!=s.slot3e4||(controls.d2&s.mask3e5)||!stage.native_data_bound()||s.scalar_effects.colours7ec!=std::array<u32,2>{})return reject("fresh unmasked phase1/substate0 ordinary appearance required");
        ActorFadeState colour;if(!prepared.material.prepare_spawn_tick(prepared.model,s.local_setup.motion_delta1dc8,&colour))return reject("same material owner with source curve0 prepared at bind required");
        try{
            auto next=s;auto journal=storage_->journal;copy_update_history(next,journal);using K=BattleActorBirthWriteKind;
            auto call=[&](u32 key){journal.push_back({K::SourceBindingStage,0,0,key});};auto byte=[&](u16 at,u8 value){journal.push_back({K::Byte,at,1,value});};auto real=[&](u16 at,f32 value){journal.push_back({K::FloatBits,at,4,f32_to_bits(value)});};
            call(0x8005475c);call(0x800547dc);next.phase1a=1;byte(0x1a,1);call(0x800061a8);
            byte(0x7e0,0);next.scalar_effects.colours7ec[1]=profile.colour_key();journal.push_back({K::SourceKey,0x7f0,4,profile.colour_key()});journal.push_back({K::Word,0x7e8,4,0});byte(0x7fb,255);byte(0x7f8,0);byte(0x7f9,0);byte(0x7fa,0);
            next.spawn_time_d4=profile.duration();real(0xd4,next.spawn_time_d4);
            const f32 base=s.scalar_defaults.scale_b4;real(0x58,f32(profile.narrow()*base));real(0x60,f32(profile.narrow()*base));real(0x5c,f32(profile.duration()*base));
            next.render_enabled82=u8(next.render_index96+65);byte(0x82,next.render_enabled82);next.spawn_duration_d8=next.spawn_time_d4;journal.push_back({K::Word,0xd8,4,f32_to_bits(next.spawn_duration_d8)});
            const f32 angle=std::fma(-profile.angle_scale(),f32(next.spawn_time_d4/next.spawn_duration_d8),profile.angle_scale());
            if(angle!=0)return reject("only the first exact zero-angle45204 call is joined");
            call(0x80045204);const f32 width=std::fma(profile.gain(),0.f,profile.narrow()),vertical=std::fma(-profile.vertical_factor(),width,profile.duration());
            real(0x58,width);real(0x5c,vertical);next.scale58={f32(width*base),f32(vertical*base),f32(width*base)};real(0x58,next.scale58[0]);real(0x5c,next.scale58[1]);journal.push_back({K::Word,0x60,4,f32_to_bits(next.scale58[2])});
            next.spawn_time_d4=f32(next.spawn_time_d4-1);real(0xd4,next.spawn_time_d4);if(next.spawn_time_d4<=0)return reject("appearance completion consumers required");
            //This entry does not reach55568/4CD24; the original first program
            //wait and both AObj clocks remain unchanged.
            BattleActorWorldAfterLook world;if(!evaluate_owned_world(next,world_profile,ik,look_profile,stage,controls,&world,error))return false;
            next.local_setup.origin_y6dc=f32(next.position20[1]+next.offset6d0);real(0x6dc,next.local_setup.origin_y6dc);call(0x8008b480);call(0x8005a580);call(0x800465fc);
            next.local_setup.adjust1d54=world.geometry.contacts.adjust1d54;next.local_setup.velocity1d60=world.geometry.contacts.velocity1d60;
            for(unsigned i=0;i<s.bone_count1d90;++i)next.local_setup.extra_angles[i]=world.next_modifiers.extra_angles[i];
            next.field1ca=next.field732=next.field730=next.field731=0;for(u16 at:{u16(0x1ca),u16(0x732),u16(0x730),u16(0x731)})byte(at,0);
            next.push724.fill(0);for(u16 at:{u16(0x724),u16(0x728),u16(0x72c)})real(at,0);
            call(0x80006284);real(0x7e8,colour.channels[1].time);journal.push_back({K::Word,0x7f8,4,colour.channels[1].rgba});journal.push_back({K::ResourceWord,0xc28,4,colour.material_rgba});
            if(!current(owners)||!prepared.material.commit_spawn_tick(prepared.model,colour))return fail_continuation(error,"spawn/material owner changed before commit");
            prepared.bank_material->set_source_colour(colour.material_rgba);next.spawn_entry_completed=true;next.world_generation=2;++next.native_revision;world.geometry.actor_frame_owner_pending=false;
            storage_->first_update->world=std::move(world);storage_->state=std::move(next);storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::SpawnWorldReady;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain spawn/world/colour continuation; no replay");}
    }
    //Only the same scene gateway can supply ground/static results and establish
    //the zero-count dynamic branch. No caller-declared registry-miss parameter.
    bool commit_initial_placement(const ChallengeBattleSpawnProfile& profile,const ChallengeFirstPlacementPlan& placement,const ChallengeSpawnStaticTail& tail,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattleActorConstructionStatus::AwaitPlacementHistory||!current(owners)||storage_->state.native_revision!=15||storage_->state.placement_complete||storage_->state.first43b||storage_->state.role490||storage_->state.mode4a0){if(error)*error="battle placement: fresh normal current body required";return false;}
        try{
            auto next=storage_->state;auto journal=storage_->journal;using K=BattleActorBirthWriteKind;
            auto put=[&](K kind,u16 at,u16 n,u32 value){journal.push_back({kind,at,n,value});};
            auto word=[&](u16 at,f32 value){put(K::Word,at,4,f32_to_bits(value));};
            auto real=[&](u16 at,f32 value){put(K::FloatBits,at,4,f32_to_bits(value));};
            auto call=[&](u32 key){put(K::SourceBindingStage,0,0,key);};
            next.field491=u8(next.field491+1);put(K::Byte,0x491,1,next.field491);call(0x80056d28);
            next.first43b=1;put(K::Byte,0x43b,1,1);
            const u16 cursor=u16(0xfe + 0x3c*next.team88);put(K::SceneTeamByte,cursor,1,placement.chosen_cursor);
            next.first43c=placement.proposed43c;put(K::Byte,0x43c,1,next.first43c);put(K::SceneTeamByte,cursor,1,placement.next_cursor);
            word(0x20,placement.pre_projection[0]);word(0x24,profile.initial_y());word(0x28,placement.pre_projection[2]);word(0x30,profile.initial_y());
            call(0x8003d6e4);real(0x24,placement.ground_height);
            const auto kind=next.scalar_effects.field661;if(kind==3||kind==4||kind==5||kind==10)real(0x24,placement.pre_projection[1]);
            call(0x800452a0);next.angles70[1]=tail.heading72;put(K::Half,0x72,2,u16(tail.heading72));
            call(0x8003f044);if(tail.static_hit.hit)word(0x24,tail.after_static[1]);
            call(0x80037b98); //both owned source counts are zero; output not written
            next.position20=tail.after_static;next.previous2c=next.history5e8=next.anchor354=next.position20;next.previous76=next.angles70;
            for(unsigned k=0;k<3;++k)word(u16(0x2c+4*k),next.previous2c[k]);
            for(unsigned k=0;k<3;++k)word(u16(0x5e8+4*k),next.history5e8[k]);
            put(K::Word,0x76,4,(u32(u16(next.angles70[0]))<<16)|u16(next.angles70[1]));put(K::Half,0x7a,2,u16(next.angles70[2]));
            for(unsigned k=0;k<3;++k)word(u16(0x354+4*k),next.anchor354[k]);
            next.field3f1=0;next.field741=1;next.field742=0;put(K::Byte,0x3f1,1,0);put(K::Byte,0x741,1,1);put(K::Byte,0x742,1,0);
            next.placement=placement;next.placement.heading_required=next.placement.static_projection_required=next.placement.actor_projection_required=false;
            next.placement_tail=tail;next.placement_tail.actor_projection_required=next.placement_tail.history_publication_pending=false;next.placement_complete=true;++next.native_revision;
            if(!current(owners))return fail_continuation(error,"placement owner changed before commit");
            storage_->state=std::move(next);storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::AwaitProgramStart;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain placement continuation; no replay");}
    }
    //Only the bootstrap can supply this snapshot, built from its six actual
    //physical claims. Public declared-query inputs cannot reach this commit.
    bool advance_initial_target(const BattleActorInitialTargetProfile& profile,const DeclaredBattleTargetSlots& slots,SourceByteRngOwner& rng,const std::array<const ForceInventory*,2>& owners,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle initial target: ")+why;return false;};
        if(status_!=BattleActorConstructionStatus::AwaitTargetSelection||!current(owners))return reject("current timer-complete constructor required");
        const auto batch=storage_->allocation.batch_.lock();const auto entry=batch?batch->origin.lock():nullptr;const auto& s=storage_->state;
        if(!entry||!entry->live||entry->state.route15dd||entry->state.common50!=1||s.native_revision!=11||!s.source_clear_seed||s.mode4a0||s.initial_target.complete||!profile.matches(storage_->binding.storage_->setup,s.class_fields.config_key4b0)||!rng.same_owner(batch->party.rng_source_)||rng.revision()==std::numeric_limits<u64>::max()||rng.draws()>std::numeric_limits<u64>::max()-3)return reject("fresh normal source profile/route/current party RNG required");
        BattleNearestTargetResult query;if(!battle_nearest_declared_target(profile.query(),slots,{true,s.slot3e4,0},&query,error))return false;
        //A nonnull fresh target reaches23BF4, whose consumers are not joined.
        //Refuse BEFORE committing any of2532C's writes or RNG; never fake a miss.
        if(query.found)return reject("changed-target continuation23BF4 unsupported");
        const auto snapshot=rng.snapshot();
        try{BattleActorInitialTargetState next;next.query=query;next.after=snapshot.state();
            for(auto& draw:next.draws)draw=source_byte_rng_step(next.after);
            next.delay32a=profile.delay(next.draws[0]);if(!profile.suffix(s.scalar_defaults.player.field20,s.scalar_defaults.player.field32,next.draws[1],&next.table328))return reject("suffix selector/level outside source table");
            next.final328=next.draws[2]&7;next.complete=true;
            auto journal=storage_->journal;using K=BattleActorBirthWriteKind;
            journal.push_back({K::Half,0x32a,2,next.delay32a});journal.push_back({K::Byte,0x30a,1,0});
            journal.push_back({K::Word,0x300,4,0});journal.push_back({K::Word,0x304,4,0});journal.push_back({K::Byte,0x30f,1,0});
            //25A84's null300 early return has no writes.20FD4 then consumes its
            //own draw and stores328, even though1C80C immediately overwrites it.
            journal.push_back({K::Byte,0x2e4,1,0});journal.push_back({K::Byte,0x2e5,1,0});journal.push_back({K::Half,0x328,2,next.table328});journal.push_back({K::Half,0x328,2,next.final328});
            if(!current(owners)||!rng.commit(snapshot,next.after,3))return fail_continuation(error,"target/RNG owner changed before commit");
            storage_->state.initial_target=next;storage_->state.timer_prefix.target_selection_pending=storage_->state.timer_prefix.suffix_pending=false;
            ++storage_->state.native_revision;storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::AwaitClassInitializer;if(error)error->clear();return true;
        }catch(const std::exception&){return fail_continuation(error,"uncertain target continuation; no replay");}
    }
    bool target_observation(const std::array<const ForceInventory*,2>& owners,BattleTargetActorFields* out)const{
        if(!out||!current(owners))return false;
        const auto& s=storage_->state;
        //After the initial request, the shared VM owns5E0 (opcode3 writes200).
        //720 retains its factory clear through22. Later consumers must extend
        //this observation rather than silently inheriting either field.
        if(!s.source_clear_seed||s.native_revision>22)return false;
        BattleTargetActorFields p;p.existence_declared=p.exists=p.activity_declared=p.kind_declared=p.normal_fields_declared=p.team_declared=p.position_declared=true;
        p.active00=s.active00;p.kind83=s.kind83;p.phase18=s.phase18;p.team88=s.team88;p.position20=s.position20;p.model3e8=s.model3e8;p.status272=storage_->program_seeded?s.program_seed.flags272:0;
        if(storage_->initial_program)p.flags5e0=storage_->initial_program->vm.state().actor.behaviour;
        *out=p;return true;
    }
    bool fail_continuation(std::string* error,const char* why){status_=BattleActorConstructionStatus::Failed;if(storage_)storage_->parent_scope->live=false;if(error)*error=std::string("battle actor construction: ")+why;return false;}
    bool local_sources_current()const{const auto b=storage_->allocation.batch_.lock();const auto e=b?b->origin.lock():nullptr;const auto& prepared=*storage_->binding.storage_;const auto specific=prepared.resource->motion_bank();return e&&e->live&&storage_->common_motion.same_owner(e->common)&&storage_->specific_motion==&prepared.motion&&storage_->specific_motion_bytes.data==specific.data&&storage_->specific_motion_bytes.size==specific.size;}
    bool program_source_current()const{const auto data=storage_->binding.storage_->resource->battle_data();return storage_->battle_data3fc.data==data.data&&storage_->battle_data3fc.size==data.size;}
    bool initial_program_current()const{
        const auto& p=storage_->initial_program;const auto& s=storage_->state;CharacterProgramClassFields fields;
        const auto* asset=p?p->vm.program_asset():nullptr;
        return p&&p->vm.ready()&&asset&&p->vm.observe_class_fields(&fields)&&fields.scope.lock()==p->controls.scope().lock()&&p->controls.motion_driver_matches(p->driver)&&p->profile.matches(storage_->binding.storage_->setup,s.class_fields.program_keys1d80[0],s.class_fields.program_keys1d80[2])&&p->driver.matches_program(*asset)&&p->driver.matches_program(p->profile.asset());
    }
    bool publication_current()const{
        const auto& publication=storage_->publication;if(!publication)return false;const auto scope=publication->scope.lock();if(!scope||!scope->live())return false;
        for(const auto& model:publication->models){if(model.record>=storage_->resource_maps.models[0].size())return false;const auto index=storage_->resource_maps.models[0][model.record].instance;const auto* root=storage_->binding.bound_instance(index);if(!root||!root->world_from(scope)||root->state()->world_generation!=storage_->state.world_generation||model.world_generation!=storage_->state.world_generation)return false;}
        return true;
    }
    bool constructor_children_current()const{const auto objects=storage_->children_pool.lock();if(!objects||!objects->ready())return false;for(const auto& attempt:storage_->children)if(attempt.result==SceneObjectAcquire::Allocated&&!objects->actor_child(attempt.object))return false;for(const auto& attempt:storage_->collision_attempts)if(attempt.result==SceneObjectAcquire::Allocated&&!objects->collision_child(attempt.object))return false;return true;}
    bool qualify_initial_hud_world(const BattleInitialUiWorldProfile& profile,const SceneObjectLease& lease,bool one,const std::array<const ForceInventory*,2>& owners,std::string* error)const{
        auto reject=[&](){if(error)*error="initial HUD world: exact current child/parent and inactive target gate required";return false;};
        if(!profile.ready()||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;
        if(!s.source_clear_seed||s.slot3e4>=4||s.native_revision!=19||s.world_generation!=1||s.contact_generation!=2||s.phase18!=1||!r||!c||r->world_callback||r->group01!=56||r->kind83!=0x14||r->active00!=1||!r->hud_child_update||!r->hud_child_update->initialized||r->hud_child_update->generation!=1||r->hud_child_update->phase18!=1||r->hud_child_update->render82||r->registration85!=0x21||r->callback100_key!=(one?0x800818ccu:0x80080f88u)||r->descriptor_count99!=(one?1:6)||r->header!=SceneObjectHeader{0,0,0,one?0x80081438u:0x80080874u,one?0x80081584u:0x80080a74u})return reject();
        SceneObjectActorChildFields expected;expected.parent8c.scope_=expected.parent90.scope_=storage_->parent_scope;if(*c!=expected)return reject();
        if(!one&&!battle_initial_hud_world_inactive(s.target_display7d2,s.phase18))return reject();
        //Both admitted branches return before all object/material/model writes.
        //818CC is truly empty; do not read a fabricated parent target for it.
        if(error){error->clear();}return true;
    }
    bool qualify_initial_inactive_auxiliary_world(const BattleInitialWorldTailProfile& profile,const SceneObjectLease& lease,const std::array<const ForceInventory*,2>& owners,std::string* error)const{
        auto reject=[&](){if(error)*error="initial auxiliary world: actual initialized slot3/common-effect/inline owner required";return false;};
        if(!profile.ready()||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;
        if(!s.source_clear_seed||s.slot3e4!=3||s.model3e8!=0x615||s.model_domain!=BattleModelDomain::NormalPair||s.ai3e6||s.kind83||s.variant3ec||s.role490||s.mode4a0||s.flags48a||s.native_revision!=19||s.world_generation!=1||s.phase18!=1||!r||!c||r->world_callback||r->group01!=28||r->active00!=1||!r->header_installed||r->header.kind10!=0x0e||(r->header.type11!=40&&r->header.type11!=41)||r->header.argument12!=2*(r->header.type11-40)||r->header.update0c_key!=0x800c7664||r->header.draw10c_key!=0x800c7954||r->callback100_key!=0x800c79f8||r->registration85!=0x13||r->descriptor_count99!=2||!r->common_auxiliary_update||!r->common_auxiliary_update->initialized||r->common_auxiliary_update->generation!=1||c->index96!=3||c->model94!=s.model3e8||c->field13!=2)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self)return reject();
        const auto& aux=*r->common_auxiliary_update;
        if(aux.scratch_allocated||aux.matrix54.owner!=ScenePhysicalLink{2,s32(lease.index())}||aux.matrix54.generation!=lease.generation()||aux.matrix54.source_field!=0x144||aux.matrix54.count!=2)return reject();
        for(unsigned slot=0;slot<2;++slot){const auto* root=SceneModelBindings::object_root(*objects,lease,slot);if(!root||!root->current()||root->state()->record!=51||root->world_current()||root->state()->world_evaluated)return reject();}
        return battle_initial_inactive_auxiliary_world(profile,r->header.argument12,s.effect_counts_a8,aux.state,error);
    }
    bool prepare_initial_shape_world(const BattleInitialShapeWorldProfile& profile,const SceneObjectLease& lease,const ChallengeStageInputs& stage,const HsdInitialLightConstructor& lights,const std::array<const ForceInventory*,2>& owners,BattleInitialShapeWorld* out,std::string* error)const{
        auto reject=[&](){if(error)*error="initial shape world: actual first actor/bone/static-grid/light ownership required";return false;};
        if(!profile.ready()||!out||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners)||!stage.native_data_bound()||stage.stage()!=2)return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;const auto* world=current_world(owners);
        if(!s.source_clear_seed||s.slot3e4>=4||s.native_revision!=19||s.world_generation!=1||s.variant3ec||s.role490||s.mode4a0||!world||!world->evaluated||!r||!c||r->world_callback||!r->shape_child_update||!r->shape_child_update->initialized||r->shape_child_update->generation!=1||r->header.type11||r->callback100_key!=0x8001c5ac||r->registration85!=0x10+s.slot3e4||c->field89>=world->geometry.world.matrices.size()||(c->field89!=16&&c->field89!=19)||r->shape_child_update->light_constructor_generation!=lights.generation||lights.generation!=1||lights.animation_requested)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self||c->model94!=s.model3e8||c->index96!=s.slot3e4)return reject();
        //55DCC's cached87 and3CC24's attachment flag are the actual reached
        //producer fields. A nonzero value needs the unjoined capsule/platform path.
        StageGroundActor ground{true,s.program_seed.flags[2],s.position20[1],s.previous2c[1],s.offset6d0,s.local_setup.field6d4};
        return battle_initial_shape_world(profile,{stage.grid(0),stage.grid(1),stage.grid(2)},stage.parameters(),ground,s.dimensions.cached87,world->geometry.world.matrices[c->field89],r->shape_child_update->numbers.direction9c,s.dimensions.field668,s.dimensions.size7fc,s.scalar_defaults.scale_b4,c->scale_b4,out,error);
    }
    bool prepare_initial_bank_world(const BattleInitialWorldDispatchProfile& profile,const SceneObjectLease& lease,const std::array<const ForceInventory*,2>& owners,AttachmentBoneTransform* out,std::string* error)const{
        auto reject=[&](){if(error)*error="initial bank world: same initialized normal parent/child/first bone worlds required";return false;};
        if(!profile.ready()||!out||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;const auto* world=current_world(owners);
        if(!s.source_clear_seed||s.model_domain!=BattleModelDomain::NormalPair||s.native_revision!=19||s.world_generation!=1||(s.slot3e4!=1&&s.slot3e4!=3)||s.variant3ec||s.role490||s.mode4a0||!world||!world->evaluated||!r||!c||r->world_callback||!r->bank_child_update||!r->bank_child_update->initialized||r->bank_child_update->generation!=1||r->callback100_key!=0x80047aa4||r->registration85!=0x10+s.slot3e4)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self||c->model94!=s.model3e8||c->index96!=s.slot3e4)return reject();
        return attachment_bone_transform(world->geometry.world.matrices,r->bank_child_update->state.transform,out,error);
    }
    bool prepare_initial_follower_world(const BattleInitialWorldDispatchProfile& profile,const SceneObjectLease& lease,bool point,const std::array<const ForceInventory*,2>& owners,AttachmentBoneTransform* out,std::string* error)const{
        auto reject=[&](){if(error)*error="initial follower world: same070B/current first-world owner and initialized source child required";return false;};
        if(!profile.ready()||!out||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;const auto* world=current_world(owners);
        if(!s.source_clear_seed||s.model_domain!=BattleModelDomain::InitialAi||s.model3e8!=0x70b||s.slot3e4||s.native_revision!=19||s.world_generation!=1||!world||!world->evaluated||!r||!c||r->group01!=1||r->active00!=1||r->callback100_key!=0x80047aa4||r->registration85||r->world_callback)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self)return reject();
        if(point){
            if(!r->point_child_update||!r->point_child_update->initialized||r->point_child_update->generation!=1||r->point_child_update->flags84!=0x54||r->point_child_update->bone89!=1||c->scale!=r->point_child_update->numbers.scale58)return reject();
            AttachmentDirectBoneTransformInput input;input.bone=1;input.position20=r->point_child_update->numbers.position20;input.angles=r->point_child_update->numbers.angles70;input.scale58=c->scale;
            return attachment_direct_bone_transform(world->geometry.world.matrices,input,out,error);
        }
        if(!r->field_child_update||!r->field_child_update->initialized||r->field_child_update->generation!=1||r->field_child_update->flags84!=0x1e||r->field_child_update->bone89!=12||c->scale!=r->field_child_update->numbers.scale58)return reject();
        AttachmentScaledWorldPositionInput input;input.position20=r->field_child_update->numbers.position20;input.angles=r->field_child_update->numbers.angles70;input.scale58=c->scale;
        return attachment_scaled_world_position_transform(input,out,error);
    }
    bool prepare_initial_point_child(const BattleInitialPointChildProfile& profile,const SceneObjectLease& lease,const std::array<const ForceInventory*,2>& owners,HsdJointInstanceBank* roots,BattleInitialPointChildNumbers* numbers,std::string* error)const{
        auto reject=[&](){if(error)*error="initial point child: exact fresh070B parent/Point150/Flag169 source ownership required";return false;};
        if(!roots||!numbers||!profile.ready()||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;
        if(!s.source_clear_seed||s.model_domain!=BattleModelDomain::InitialAi||s.model3e8!=0x70b||s.variant3ec||s.slot3e4||s.render_index96||s.native_revision!=19||s.world_generation!=1||!storage_->initial_program||!r||!c||r->group01!=1||r->active00!=1||!r->header_installed||r->descriptor_count99!=2||r->header.update0c_key!=0x801fcda8||r->header.draw10c_key!=0x801fd09c||r->header.type11!=3||r->point_child_update||r->registration85!=255||!c->field144||!c->field148||c->field144->slot!=SceneActorFieldSlot::Point150||c->field148->slot!=SceneActorFieldSlot::Flag169||c->index96||c->model94!=s.model3e8)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;
        if(c->parent8c!=self||c->parent90!=self||c->field144->owner!=self||c->field148->owner!=self)return reject();
        if(!battle_initial_point_child_numbers(profile,s.ai_points150_15c[0],s.local_setup.retained1dbc,s.scalar_defaults.scale_b4,storage_->initial_program->vm.state().actor.steering1dfc,s.ai_flags168_16b[1],numbers,error))return false;
        return prepare_initial_cached_child_roots(lease,1,roots,error);
    }
    bool commit_initial_point_child(const SceneObjectLease& lease,HsdJointInstanceBank& roots,const BattleInitialPointChildNumbers& numbers,std::string* error)const{
        const auto objects=storage_->children_pool.lock();if(!objects||!objects->owns(lease)||roots.instances()!=2)return false;
        auto& r=objects->storage_->state.records[lease.index()];if(!r.point_child_update||r.point_child_update->initialized||r.registration85!=0||r.callback100_key!=0x80047aa4)return false;
        auto& runtime=*r.point_child_update;runtime.flags84=0x54;runtime.bone89=1;
        auto& fields=*r.actor_child;fields.scale_b4=1;fields.scale.fill(1);
        if(!attach_initial_cached_child_roots(lease,1,roots,error))return false;
        runtime.render82=0x31; //96+49; not the actor-render selector65
        runtime.numbers=numbers;fields.scale_b4=numbers.scale_b4;fields.scale_b8=numbers.scale_b8;
        runtime.generation=1;runtime.initialized=true;if(error)error->clear();return true;
    }
    bool prepare_initial_cached_child_roots(const SceneObjectLease& lease,u16 record,HsdJointInstanceBank* roots,std::string* error)const{
        const auto objects=storage_->children_pool.lock();const auto& prepared=*storage_->binding.storage_;
        if(!roots||!objects||!objects->owns(lease)||objects->inspect(lease)->descriptor_count99!=2||!prepared.bank_material||prepared.bank_material->source_cache_words()!=std::array<u32,2>{255,0}||!storage_->archive.bank()||!storage_->archive.bank()->same_source(prepared.bank)){if(error)*error="initial cached child: current archive/shared zero-cache/two descriptors required";return false;}
        for(unsigned i=0;i<2;++i)if(objects->storage_->descriptors->root(objects->storage_->leases[lease.index()][i])){if(error)*error="initial cached child: fresh descriptor roots required";return false;}
        const std::array<u16,2> records{record,record};return roots->construct_normal_records(prepared.bank,records,error);
    }
    bool attach_initial_cached_child_roots(const SceneObjectLease& lease,u16 record,HsdJointInstanceBank& roots,std::string* error)const{
        const auto objects=storage_->children_pool.lock();if(!objects||!objects->owns(lease)||roots.instances()!=2)return false;
        const auto& context=storage_->binding.storage_->bank_material;auto& descriptors=*objects->storage_->descriptors;
        for(unsigned i=0;i<2;++i){if(!roots.state(i)||roots.state(i)->record!=record)return false;}
        for(unsigned i=0;i<2;++i){HsdJointInstanceOwner root;if(!roots.take_instance(i,&root,error)||!SceneModelBindings::attach_actor(descriptors,objects->storage_->leases[lease.index()][i],storage_->archive,record,HsdJointInstanceStyle::Normal,std::move(root),context,error))return false;}
        return SceneModelBindings::style_initial_bank_child(descriptors,objects->storage_->leases[lease.index()][1],error);
    }
    bool prepare_initial_field_child(const BattleInitialFieldChildProfile& profile,const SceneObjectLease& lease,const std::array<const ForceInventory*,2>& owners,HsdJointInstanceBank* roots,BattleInitialFieldChildNumbers* numbers,std::string* error)const{
        auto reject=[&](){if(error)*error="initial field child: exact fresh070B row3/Flag168/current bone12 ownership required";return false;};
        if(!roots||!numbers||!profile.ready()||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;const auto* world=current_world(owners);
        if(!s.source_clear_seed||s.model_domain!=BattleModelDomain::InitialAi||s.model3e8!=0x70b||s.variant3ec||s.slot3e4||s.render_index96||s.native_revision!=19||s.world_generation!=1||s.effect_counts_a8[0]||!world||!world->evaluated||world->geometry.world.matrices.size()<=12||!r||!c||r->group01!=1||r->active00!=1||!r->header_installed||r->descriptor_count99!=2||r->header.update0c_key!=0x800c6394||r->header.draw10c_key!=0x800c6724||r->header.type11||c->field13!=3||r->field_child_update||r->registration85!=255||!c->field144||c->field148||c->field144->slot!=SceneActorFieldSlot::Flag168||c->index96||c->model94!=s.model3e8)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self||c->field144->owner!=self)return reject();
        if(!battle_initial_field_child_numbers(profile,world->geometry.world.matrices[12],s.angles70,s.scalar_defaults.scale_b4,s.ai_flags168_16b[0],numbers,error))return false;
        return prepare_initial_cached_child_roots(lease,1,roots,error);
    }
    bool commit_initial_field_child(const SceneObjectLease& lease,HsdJointInstanceBank& roots,const BattleInitialFieldChildNumbers& numbers,std::string* error)const{
        const auto objects=storage_->children_pool.lock();if(!objects||!objects->owns(lease))return false;
        auto& r=objects->storage_->state.records[lease.index()];if(!r.field_child_update||r.field_child_update->initialized||r.registration85!=0||r.callback100_key!=0x80047aa4)return false;
        auto& runtime=*r.field_child_update;runtime.flags84=0x1e;runtime.bone89=12;
        if(!attach_initial_cached_child_roots(lease,1,roots,error))return false;
        runtime.render82=0x41;runtime.numbers=numbers;runtime.fielda8=storage_->state.effect_counts_a8[0];r.actor_child->scale=numbers.scale58;
        //B4/B8 retain their constructor copies; only the high80 effect branch
        //would copy them again. Flag168==1 does not enter that branch.
        runtime.generation=1;runtime.initialized=true;if(error)error->clear();return true;
    }
    bool prepare_initial_sasuke_child(const BattleInitialSasukeChildrenProfile& profile,const SceneObjectLease& lease,const std::array<const ForceInventory*,2>& owners,HsdJointInstanceBank* roots,StartupAttachmentUpdate* step,std::string* error)const{
        auto reject=[&](){if(error)*error="initial Sasuke child: genuine unchanged slot1 parent/VM/type6-or7 source required";return false;};
        if(!roots||!step||!profile.ready()||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;const auto* p=profile.attachments();
        if(!s.source_clear_seed||s.model_domain!=BattleModelDomain::NormalPair||s.model3e8!=10||s.ai3e6||s.variant3ec||s.role490||s.mode4a0||s.flags48a||s.slot3e4!=1||s.render_index96!=1||s.native_revision!=19||s.world_generation!=1||s.retire_662||s.alternate_ae||!storage_->initial_program||!p||p->model_hash!=storage_->binding.storage_->klass.model_hash||!r||!c||r->active00!=1||r->group01!=2||!r->header_installed||r->descriptor_count99!=2||r->header.update0c_key!=0x8004d7fc||r->header.draw10c_key!=0x8004dabc||(r->header.type11!=6&&r->header.type11!=7)||r->bank_child_update||r->registration85!=255||c->field144||c->field148||c->index96!=1||c->model94!=s.model3e8)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self)return reject();
        StartupAttachmentParent parent;parent.model_id=s.model3e8;parent.mode6e8=storage_->initial_program->vm.state().actor.control0[2];parent.retire662=s.retire_662;parent.alternate_ae=s.alternate_ae;parent.selector3ec=s.variant3ec;parent.reload784=s.scalar_effects.reloads[2].current;
        const unsigned index=r->header.type11-6;
        if(!startup_attachment_state_step(*p,index,c->index96,parent,{},step,error)||step->unregister||step->released||step->state.phase!=1||step->state.render_group!=66)return reject();
        const auto* world=current_world(owners);if(!world||!world->evaluated||step->state.transform.bone>=world->geometry.world.matrices.size())return reject();
        return prepare_initial_cached_child_roots(lease,p->entries[index].record,roots,error);
    }
    bool commit_initial_bank_child(const SceneObjectLease& lease,HsdJointInstanceBank& roots,const StartupAttachmentUpdate& step,u16 record,std::string* error)const{
        const auto objects=storage_->children_pool.lock();if(!objects||!objects->owns(lease))return false;
        auto& r=objects->storage_->state.records[lease.index()];if(!r.bank_child_update||r.bank_child_update->initialized||r.registration85!=u8(0x10+storage_->state.slot3e4)||r.callback100_key!=0x80047aa4)return false;
        auto& runtime=*r.bank_child_update;runtime.state.transform.flags=step.state.transform.flags;
        r.actor_child->scale_b4=r.actor_child->scale_b8=1;r.actor_child->scale.fill(1);
        if(!attach_initial_cached_child_roots(lease,record,roots,error))return false;
        runtime.state=step.state;r.header.argument12=step.state.mode;runtime.record=record;runtime.type11=r.header.type11;runtime.draw_branch_visible=step.visible;runtime.draw_branch_alternate=step.alternate;
        runtime.generation=1;runtime.initialized=true;if(error)error->clear();return true;
    }
    bool prepare_initial_gred_child(const BattleInitialGRedChildrenProfile& profile,const SceneObjectLease& lease,const std::array<const ForceInventory*,2>& owners,HsdJointInstanceBank* roots,StartupAttachmentUpdate* step,std::string* error)const{
        auto reject=[&](){if(error)*error="initial G-Red child: unchanged actual slot3 parent/VM/type24-or25 required";return false;};
        if(!roots||!step||!profile.ready()||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;const auto* p=profile.attachments();
        if(!s.source_clear_seed||s.model_domain!=BattleModelDomain::NormalPair||s.model3e8!=0x615||s.ai3e6||s.variant3ec||s.role490||s.mode4a0||s.flags48a||s.slot3e4!=3||s.render_index96!=3||s.native_revision!=19||s.world_generation!=1||s.retire_662||s.alternate_ae||!storage_->initial_program||!p||p->model_hash!=storage_->binding.storage_->klass.model_hash||!r||!c||r->active00!=1||r->group01!=4||!r->header_installed||r->descriptor_count99!=2||r->header.update0c_key!=0x801860c4||r->header.draw10c_key!=0x80186480||(r->header.type11!=24&&r->header.type11!=25)||r->bank_child_update||r->registration85!=255||c->field144||c->field148||c->index96!=3||c->model94!=s.model3e8)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self)return reject();
        StartupAttachmentParent parent;parent.model_id=s.model3e8;parent.mode6e8=storage_->initial_program->vm.state().actor.control0[2];parent.retire662=s.retire_662;parent.alternate_ae=s.alternate_ae;parent.selector3ec=s.variant3ec;parent.reload784=s.scalar_effects.reloads[2].current;
        const unsigned index=r->header.type11-24;
        if(!battle_initial_gred_child_state(profile,index,c->index96,parent,step,error)||step->unregister||step->released||step->state.phase!=1||step->state.render_group!=68)return reject();
        const auto* world=current_world(owners);if(!world||!world->evaluated||step->state.transform.bone>=world->geometry.world.matrices.size())return reject();
        return prepare_initial_cached_child_roots(lease,p->entries[index].record,roots,error);
    }
    bool prepare_initial_shape_child(const BattleInitialShapeChildProfile& profile,const SceneObjectLease& lease,const HsdInitialLightConstructor& lights,const std::array<const ForceInventory*,2>& owners,BattleInitialShapeNumbers* numbers,std::string* error)const{
        return prepare_initial_type0_shape(profile,lease,lights,{0,0x70b,0xbdb1d4f4,{16,19}},19,owners,numbers,error);
    }
    bool prepare_shape_continuation(const BattleInitialShapeContinuationProfile& profile,const SceneObjectLease& lease,unsigned source_index,const HsdInitialLightConstructor& lights,const std::array<const ForceInventory*,2>& owners,BattleInitialShapeNumbers* numbers,std::string* error)const{
        if(!profile.ready()||!storage_||source_index>=2){if(error)*error="shape continuation: sealed source row/index required";return false;}
        const auto row=profile.owner(storage_->state.slot3e4);
        if(!row){if(error)*error="shape continuation: actual imported parent slot required";return false;}
        return prepare_initial_type0_shape(*profile.light_recipe(),lease,lights,*row,row->bones[source_index],owners,numbers,error);
    }
    bool prepare_initial_type0_shape(const BattleInitialShapeChildProfile& profile,const SceneObjectLease& lease,const HsdInitialLightConstructor& lights,const BattleInitialShapeOwnerRow& row,u8 bone,const std::array<const ForceInventory*,2>& owners,BattleInitialShapeNumbers* numbers,std::string* error)const{
        auto reject=[&](){if(error)*error="initial type0 shape: exact fresh source parent/slot/model/bone required";return false;};
        if(!profile.ready()||!numbers||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;const auto* world=current_world(owners);
        const bool ai=row.slot==0||row.slot==2;
        if(!s.source_clear_seed||s.model_domain!=(ai?BattleModelDomain::InitialAi:BattleModelDomain::NormalPair)||bool(s.ai3e6)!=ai||s.model3e8!=row.model||s.variant3ec||s.role490||s.mode4a0||s.flags48a||s.slot3e4!=row.slot||s.render_index96!=row.slot||s.native_revision!=19||s.world_generation!=1||s.phase18!=1||storage_->binding.storage_->klass.model_hash!=row.hash||!world||!world->evaluated||world->geometry.world.matrices.size()<=bone||!r||!c||r->group01!=25+row.slot||r->active00!=1||!r->header_installed||r->descriptor_count99||r->header.kind10!=6||r->header.type11||r->header.argument12||r->header.update0c_key!=0x8001c42c||r->header.draw10c_key!=0x8001c70c||r->shape_child_update||r->registration85!=255||c->field89!=bone||c->scale_b4!=1||c->index96!=row.slot||c->model94!=s.model3e8||c->field144||c->field148)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self)return reject();
        return battle_initial_shape_numbers(profile,lights,numbers,error);
    }
    bool prepare_initial_common_auxiliary(const BattleInitialCommonAuxiliaryProfile& profile,const SceneObjectLease& lease,const SceneArchiveLease& archive,const std::array<const ForceInventory*,2>& owners,HsdJointInstanceBank* roots,StartupAuxiliaryState* state,std::string* error)const{
        auto reject=[&](){if(error)*error="initial common auxiliary: actual G-Red/type40-or41/split-effect owner required";return false;};
        if(!profile.ready()||!roots||!state||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;const auto* bank=archive.bank();const auto* p=profile.auxiliary();
        if(!s.source_clear_seed||s.model_domain!=BattleModelDomain::NormalPair||s.model3e8!=0x615||s.ai3e6||s.variant3ec||s.role490||s.mode4a0||s.flags48a||s.kind83||s.slot3e4!=3||s.render_index96!=3||s.native_revision!=19||s.world_generation!=1||s.phase18!=1||!bank||bank->hash()!=p->model_hash||!objects||!archive.belongs_to(*objects->storage_->descriptors)||!r||!c||r->group01!=28||r->active00!=1||!r->header_installed||r->descriptor_count99!=2||r->header.kind10!=0x0e||(r->header.type11!=40&&r->header.type11!=41)||r->header.update0c_key!=0x800c7664||r->header.draw10c_key!=0x800c7954||r->common_auxiliary_update||r->registration85!=255||c->field13!=2||c->index96!=3||c->model94!=s.model3e8||c->field144||c->field148)return reject();
        const unsigned entry=r->header.type11-40;if(r->header.argument12!=p->entries[entry].channel||!s.auxiliary_children[entry]||!s.auxiliary_children[entry]->created)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self)return reject();
        if(!battle_initial_common_auxiliary_state(profile,c->index96,s.auxiliary_children[entry]->state,state,error))return false;
        for(unsigned slot=0;slot<2;++slot)if(objects->storage_->descriptors->root(objects->storage_->leases[lease.index()][slot]))return reject();
        const std::array<u16,2> records{p->entries[entry].parts[0].model,p->entries[entry].parts[1].model};
        return roots->construct_normal_records(*bank,records,error);
    }
    bool commit_initial_common_auxiliary(const SceneObjectLease& lease,const SceneArchiveLease& archive,HsdJointInstanceBank& roots,const StartupAuxiliaryState& state,std::string* error)const{
        const auto objects=storage_->children_pool.lock();if(!objects||!objects->owns(lease)||roots.instances()!=2)return false;
        auto& r=objects->storage_->state.records[lease.index()];if(!r.common_auxiliary_update||r.common_auxiliary_update->initialized||r.registration85!=0x13||r.callback100_key!=0x800c79f8)return false;
        for(unsigned slot=0;slot<2;++slot){HsdJointInstanceOwner root;if(!roots.take_instance(slot,&root,error)||!SceneModelBindings::attach_object_descriptor(*objects,lease,slot,archive,51,std::move(root),error))return false;}
        auto& update=*r.common_auxiliary_update;update.state=state;update.matrix54={{2,s32(lease.index())},lease.generation(),0x144,2};update.generation=1;update.initialized=true;
        if(error){error->clear();}return true;
    }
    bool initial_hud_child_current(const BattleInitialHudChildrenProfile& profile,const SceneObjectLease& lease,bool one,const std::array<const ForceInventory*,2>& owners,std::string* error)const{
        auto reject=[&](){if(error)*error="initial HUD: actual unparented fresh post-class child/current constructor required";return false;};
        if(!profile.ready()||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;
        if(!s.source_clear_seed||s.slot3e4>=4||s.render_index96!=s.slot3e4||s.variant3ec||s.role490||s.mode4a0||s.flags48a||s.kind83||s.native_revision!=19||s.world_generation!=1||s.phase18!=1||!s.post_class_children||!r||!c||r->group01!=56||r->active00!=1||!r->header_installed||r->kind83!=0x14||r->descriptor_count99!=(one?1:6)||r->header!=SceneObjectHeader{0,0,0,one?0x80081438u:0x80080874u,one?0x80081584u:0x80080a74u}||r->hud_child_update||r->registration85!=255||r->callback100_key||r->callback_previous104.present()||r->callback_next108.present())return reject();
        SceneObjectActorChildFields expected;expected.parent8c.scope_=expected.parent90.scope_=storage_->parent_scope;
        //813A8/807E4 copy ONLY parent8C/90. model94/index96 remain zero,
        //including children of slots1..3; they are not actor-cache bindings.
        if(*c!=expected)return reject();
        for(unsigned slot=0;slot<r->descriptor_count99;++slot)if(SceneModelBindings::object_root(*objects,lease,slot))return reject();
        return true;
    }
    bool initial_bar_child_current(const BattleInitialBarChildProfile& profile,const SceneObjectLease& lease,const std::array<const ForceInventory*,2>& owners,std::string* error)const{
        auto reject=[&](){if(error)*error="initial bar: exact unchanged slot3 G-Red/count5 constructor owner required";return false;};
        if(!profile.ready()||status_!=BattleActorConstructionStatus::FirstWorldReady||!current(owners))return reject();
        const auto& s=storage_->state;const auto objects=storage_->children_pool.lock();const auto* r=objects?objects->inspect(lease):nullptr;const auto* c=objects?objects->actor_child(lease):nullptr;
        if(!s.source_clear_seed||s.model_domain!=BattleModelDomain::NormalPair||s.model3e8!=0x615||s.ai3e6||s.variant3ec||s.role490||s.mode4a0||s.flags48a||s.slot3e4!=3||s.render_index96!=3||s.native_revision!=19||s.world_generation!=1||s.phase18!=1||!r||!c||r->group01!=28||r->active00!=1||!r->header_installed||r->descriptor_count99!=5||r->header.kind10!=0x1b||r->header.type11||r->header.update0c_key!=0x8013be88||r->header.draw10c_key!=0x8013c264||r->bar_child_update||r->registration85!=255||c->field13!=5||c->word54||c->index96!=3||c->model94!=s.model3e8)return reject();
        SceneActorReference self;self.scope_=storage_->parent_scope;if(c->parent8c!=self||c->parent90!=self)return reject();
        for(unsigned slot=0;slot<5;++slot)if(SceneModelBindings::object_root(*objects,lease,slot))return reject();
        return true;
    }
    bool roster_current(const std::array<const ForceInventory*,2>& owners)const{const auto r=storage_->roster.lock();return r&&r->current(owners);}
    bool class_sources_current()const{const auto& prepared=*storage_->binding.storage_;const auto data=prepared.resource->actor_data();return storage_->actor_data4ac.data==data.data&&storage_->actor_data4ac.size==data.size&&storage_->class_config4b0.data==prepared.setup.class_config.data()&&storage_->class_config4b0.size==prepared.setup.class_config.size();}
    bool resource_sources_current(const std::array<const ForceInventory*,2>& owners)const{const auto& prepared=*storage_->binding.storage_;if(storage_->resource_bank!=&prepared.instances||storage_->resource_motion!=&prepared.motion||storage_->resource_receipts.size()!=prepared.instances.instances()||storage_->material_contexts.size()!=prepared.instances.instances())return false;if(storage_->archive_bound){const auto* bank=storage_->archive.bank();if(!bank||!bank->same_source(prepared.bank))return false;}for(const auto& receipt:storage_->resource_receipts)if(!receipt.current(owners))return false;for(const auto& model:storage_->material_contexts)for(const auto& d:model)if(d.context!=prepared.bank_material.get())return false;return true;}
    struct MaterialContextBinding {u32 dobj=0;const BattleBankMaterialContext* context=nullptr;};
    struct InitialProgram {BattleActorInitialProgramProfile profile;CharacterPoseHeightProfile height;CharacterProgramVm vm;CharacterMotionDriver driver;CharacterProgramDirectionalControl controls;CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly;std::vector<CharacterProgramOperation> operations;};
    struct FirstUpdate {BattleActorFirstUpdateProfile profile;BattleActorWorldAfterLook world;};
    struct Publication {std::weak_ptr<const HsdJointWorldScope> scope;std::vector<BattleActorPublishedModel> models;CharacterModelParts parts;};
    struct Storage {BattleCharacterBinding binding;ChallengeCharacterDescriptorReceipt allocation;BattleActorBirthProfile profile;ChallengePartyMember selected;BattleActorBirthState state;u8 team=0;std::vector<BattleActorBirthWrite> journal;std::weak_ptr<challenge_battle_roster_detail::Storage> roster;bool member_joined=false,class_joined=false,resources_joined=false,level_cost_joined=false,defaults_joined=false,archive_bound=false,effects_joined=false,dimensions_joined=false,local_joined=false,program_seeded=false,timers_joined=false;SceneArchiveLease archive;ByteView actor_data4ac,class_config4b0;BattleActorResourceMaps resource_maps;std::vector<BattleBankInstanceReceipt> resource_receipts;std::vector<std::vector<MaterialContextBinding>> material_contexts;const HsdJointInstanceBank* resource_bank=nullptr;const CharacterMotionBank* resource_motion=nullptr;BattleActorLevelRow level_row;std::shared_ptr<const scene_actor_reference_detail::Scope> parent_scope=std::make_shared<const scene_actor_reference_detail::Scope>();std::weak_ptr<SceneObjectPool> children_pool;std::vector<BattleActorChildAttempt> children;BattleCommonResourceBinding common_motion;const CharacterMotionBank* specific_motion=nullptr;ByteView specific_motion_bytes,battle_data3fc;
        std::vector<BattleActorCollisionAttempt> collision_attempts;std::array<SceneCollisionLease,4> collision_slots;
        std::unique_ptr<InitialProgram> initial_program;
        std::unique_ptr<FirstUpdate> first_update;
        CharacterModelPartsProfile parts_profile;std::unique_ptr<Publication> publication;
    };
    std::unique_ptr<Storage> storage_;BattleActorConstructionStatus status_=BattleActorConstructionStatus::Closed;bool attempted_=false;
};
} // namespace gf
