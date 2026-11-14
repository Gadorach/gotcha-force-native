#pragma once
#include "engine/assets/battle_actor_birth.hpp"
#include "engine/gameplay/battle_character_binding.hpp"
#include "engine/gameplay/challenge_battle_roster.hpp"
#include "engine/gameplay/battle_actor_class_selection.hpp"
#include "engine/gameplay/battle_actor_resource_maps.hpp"
#include "engine/gameplay/battle_actor_level_cost.hpp"
#include "engine/gameplay/battle_actor_scalar_defaults.hpp"
namespace gf {
enum class BattleActorConstructionStatus {Closed,Cleared,AwaitMemberInitializer,Failed,Retired,AwaitClassSelection,AwaitModelBinding,AwaitScalarSetup,AwaitScalarDefaults,AwaitScalarEffects};
enum class BattleActorBirthWriteKind:u8 {Byte,Half,Word,FloatBits,Clear,SourceKey,ActorDataResource,SelfReference,SourceBindingStage};
struct BattleActorBirthWrite {BattleActorBirthWriteKind kind=BattleActorBirthWriteKind::Byte;u16 offset=0,extent=0;u32 value=0;bool operator==(const BattleActorBirthWrite&)const=default;};
//Represented fields ONLY, not a general1E00 source-image/guest-memory codec. Self
//references are native ownership relationships, never host pointers in u32 words.
//No Mat4 is defaulted to an actor world, and no local/program pose is claimed.
struct BattleActorBirthState {
    u8 active00=0,kind83=0,phase18=0,phase19=0,phase1a=0,phase1b=0,field493=0;
    u8 slot3e4=255,mask3e5=0,resource3e7=255,variant3ec=0,variant3ed=0,selected492=255,team88=0;
    u16 model3e8=0,model3ea=0,render_model94=0;u8 render_index96=255,resource_index97=255;
    u8 field3f2=0,field486=0,field501=0,role490=0,mode4a0=0,generation3f0=0;
    std::array<f32,3> position20{};f32 offset6d0=0;std::array<f32,4> fields44_48_4c_50{};
    std::array<s16,3> angles7c_7e_80{};std::array<u32,4> fields530_to_53c{};
    std::array<u8,128> cleared144{};std::array<u8,916> cleared540{};
    bool self8c=false,self90=false,source_clear_seed=false;u64 native_revision=0;
    u8 level3ee=0,level3ef=0;u32 acquired3f4=0,experience3f8=0;std::array<s16,3> shorts1c4_1c6_1c8{};
    BattleActorClassFields class_fields;u8 flags48a=0;
    u8 ai3e6=0,field488=0,field3f3=0,field26f=0,render_enabled82=0,bone_count1d90=0;
    u32 callback10c_key=0,visible76c=0,field5a0=0;
    s16 cost4aa=0;
    BattleActorScalarDefaults scalar_defaults;
};
struct BattleActorConstructionPending {
    bool member_68344=true,class_selection_56ce0=true,actor_model_motion_binding=true,scalar_setup=true,local_pose_4bb14=true;
    bool timer_and_target_rng=true,class_initializer_4b8=true,effects=true,colliders=true,program=true,placement_history=true,full_constructor=true;
    bool model_records_419ec=true,bone_parts_5380c=true,motion_tables_41b20=true;
    bool level_cost_55f90=true;
    bool defaults_56180=true;
};
//Declared numeric adapter for owned/source checks, never native actor authority.
inline bool battle_actor_birth_seed(const BattleActorBirthProfile& p,u8 slot,u8 selected,u16 model,u8 variant,BattleActorBirthState* out,std::string* error){
    if(out)*out={};
    if(!out||!battle_actor_birth_profile_valid(p)||slot>=6||selected>=30||(model!=0x615&&model!=10)||variant){if(error)*error="battle actor birth seed: two-normal fresh source domain";return false;}
    BattleActorBirthState s;s.active00=1;s.slot3e4=s.resource3e7=s.render_index96=s.resource_index97=slot;s.mask3e5=u8(1u<<slot);s.model3e8=s.model3ea=s.render_model94=model;s.variant3ec=s.variant3ed=variant;s.selected492=selected;s.self8c=s.self90=s.source_clear_seed=true;*out=s;if(error)error->clear();return true;
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
    s.cleared144.fill(0);s.cleared540.fill(0);s.offset6d0=0;put(K::Clear,p.words[1],p.words[2],0);put(K::Clear,p.words[3],p.words[4],0);
    s.fields44_48_4c_50.fill(bits_to_f32(p.words[5]));for(unsigned at:{0x50u,0x48u,0x4cu,0x44u})put(K::FloatBits,at,4,p.words[5]);
    s.angles7c_7e_80.fill(0);for(unsigned at:{0x80u,0x7eu,0x7cu})put(K::Half,at,2,0);
    s.field501=0;put(K::Byte,0x501,1,0);s.fields530_to_53c.fill(0);for(unsigned at:{0x530u,0x534u,0x538u,0x53cu})put(K::Word,at,4,0);
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
    if(!out||!writes||!input.source_clear_seed||input.native_revision!=2||input.phase18!=1||input.role490||input.mode4a0||input.flags48a||input.class_fields.installed||input.slot3e4>=6||input.resource3e7!=input.slot3e4||input.model3e8!=row.model||input.variant3ec!=row.variant){if(error)*error="battle actor class: ordinary member-complete source cursor required";return false;}
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
    if(!out||!writes||!input.source_clear_seed||input.native_revision!=4||!input.class_fields.installed||input.phase18!=1||input.role490||input.mode4a0||input.flags48a||input.model3e8!=row.model||input.variant3ec!=row.variant||input.level3ee!=row.level||row.model_hash!=model_hash){if(error)*error="battle actor level cost: current ordinary member/resource row required";return false;}
    BattleActorLevelCostPlan plan;if(!battle_actor_level_cost(row,input.ai3e6,input.field486,&plan,error))return false;
    auto next=input;next.shorts1c4_1c6_1c8=plan.shorts1c4_1c6_1c8;next.cost4aa=plan.cost4aa;next.native_revision=5;std::vector<BattleActorBirthWrite> journal;for(const auto& write:plan.writes)journal.push_back({BattleActorBirthWriteKind::Half,write.offset,2,write.value});*out=next;*writes=std::move(journal);if(error)error->clear();return true;
}
class BattleActorConstruction {
public:
    BattleActorConstruction()=default;BattleActorConstruction(const BattleActorConstruction&)=delete;BattleActorConstruction& operator=(const BattleActorConstruction&)=delete;
    BattleActorConstruction(BattleActorConstruction&&)=default;BattleActorConstruction& operator=(BattleActorConstruction&&)=default;
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
        //First construction is one-shot per current allocation slot, even if a
        //caller prepared a second model instance for that same receipt. This is
        //serialized native ownership, NOT a global source actor-pool/respawn ACK.
        if(batch->construction_claimed[slot])return reject("allocation slot already claimed for first construction");
        batch->construction_claimed[slot]=true; //never reset/replayed after uncertain work or retire
        try{auto next=std::make_unique<Storage>();next->profile=profile;next->allocation=allocation;next->selected=members[index];next->team=u8(batch->party.team(slot));if(!battle_actor_birth_seed(profile,u8(slot),index,resource->selection().model,resource->variant(),&next->state,error))return false;
            next->binding=std::move(prepared);if(!next->binding.current(owners)||allocation.data(owners)!=resource)return reject("allocation retired during transfer; no replay");storage_=std::move(next);status_=BattleActorConstructionStatus::Cleared;if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native allocation/transfer; no replay");}
    }
    bool current(const std::array<const ForceInventory*,2>& owners)const{return (status_==BattleActorConstructionStatus::Cleared||status_==BattleActorConstructionStatus::AwaitMemberInitializer||status_==BattleActorConstructionStatus::AwaitClassSelection||status_==BattleActorConstructionStatus::AwaitModelBinding||status_==BattleActorConstructionStatus::AwaitScalarSetup||status_==BattleActorConstructionStatus::AwaitScalarDefaults||status_==BattleActorConstructionStatus::AwaitScalarEffects)&&storage_&&storage_->binding.current(owners)&&storage_->allocation.data(owners)==storage_->binding.storage_->resource&&(!storage_->member_joined||roster_current(owners))&&(!storage_->class_joined||class_sources_current())&&(!storage_->resources_joined||resource_sources_current(owners));}
    bool advance_direct_prefix(const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=BattleActorConstructionStatus::Cleared){if(error)*error="battle actor construction: prefix already applied or owner closed";return false;}
        if(!current(owners)){status_=BattleActorConstructionStatus::Failed;if(error)*error="battle actor construction: allocation/party no longer current";return false;}
        try{BattleActorBirthState next;std::vector<BattleActorBirthWrite> journal;if(!battle_actor_birth_prefix(storage_->profile,storage_->state,storage_->team,&next,&journal,error)){status_=BattleActorConstructionStatus::Failed;return false;}if(!current(owners)){status_=BattleActorConstructionStatus::Failed;if(error)*error="battle actor construction: owner retired before prefix commit";return false;}
            storage_->state=next;storage_->journal=std::move(journal);status_=BattleActorConstructionStatus::AwaitMemberInitializer;if(error)error->clear();return true;
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
        const auto& prepared=*storage_->binding.storage_;CharacterModelPartsProfile parts;
        //Pure metadata refusal is BEFORE source work, preserving the pending cursor.
        if(!character_model_parts_load(part_fields,prepared.klass.model_hash,&parts,error)||parts.model_id!=storage_->state.model3e8)return reject("same-model qualified part table required");
        try{const auto& bank=prepared.instances;std::vector<const HsdJointInstanceStateSet*> instances;for(unsigned i=0;i<bank.instances();++i)instances.push_back(bank.state(i));BattleActorResourceMaps maps;
            if(prepared.bank_material.source_cache_words!=std::array<u32,2>{255,0}||!battle_actor_resource_maps(prepared.model.rig(),parts,instances,prepared.motion,storage_->state.resource_index97,&maps,error)){status_=BattleActorConstructionStatus::Failed;return false;}
            std::vector<BattleBankInstanceReceipt> receipts;std::vector<std::vector<MaterialContextBinding>> contexts;
            for(unsigned style=0;style<2;++style)for(const auto& model:maps.models[style]){auto receipt=storage_->binding.bank_instance_receipt(model.instance);if(!receipt.current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("instance allocation changed during binding");}receipts.push_back(receipt);std::vector<MaterialContextBinding> rows;for(u32 dobj:model.material_dobjs)rows.push_back({dobj,&prepared.bank_material});contexts.push_back(std::move(rows));}
            const auto batch=storage_->allocation.batch_.lock();BattleActorBirthState next;std::vector<BattleActorBirthWrite> tail;if(!batch||!battle_actor_birth_resource_tail(storage_->state,batch->party.humans(),&next,&tail,error)){status_=BattleActorConstructionStatus::Failed;return false;}next.bone_count1d90=maps.bone_count1d90;
            auto journal=storage_->journal;for(u32 call:{0x800419ecu,0x8005380cu,0x80041b20u})journal.push_back({BattleActorBirthWriteKind::SourceBindingStage,0,0,call});journal.insert(journal.end(),tail.begin(),tail.end());
            if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("retired before resource commit");}
            storage_->state=next;storage_->journal=std::move(journal);storage_->resource_maps=std::move(maps);storage_->resource_receipts=std::move(receipts);storage_->material_contexts=std::move(contexts);storage_->resource_bank=&bank;storage_->resource_motion=&prepared.motion;storage_->resources_joined=true;status_=BattleActorConstructionStatus::AwaitScalarSetup;if(error)error->clear();return true;
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
            auto next=state;next.scalar_defaults=std::move(defaults);next.native_revision=6;auto journal=storage_->journal;
            for(const auto& w:next.scalar_defaults.writes)journal.push_back({w.extent==1?BattleActorBirthWriteKind::Byte:w.extent==2?BattleActorBirthWriteKind::Half:BattleActorBirthWriteKind::FloatBits,w.offset,w.extent,w.value});
            if(!current(owners)){status_=BattleActorConstructionStatus::Failed;return reject("retired before defaults commit");}
            storage_->state=std::move(next);storage_->journal=std::move(journal);storage_->defaults_joined=true;status_=BattleActorConstructionStatus::AwaitScalarEffects;if(error)error->clear();return true;
        }catch(const std::exception&){status_=BattleActorConstructionStatus::Failed;return reject("uncertain defaults allocation; no replay");}
    }
    //Inspection only. A copied snapshot is not a step/frame/collider capability.
    const BattleActorBirthState* state(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?&storage_->state:nullptr;}
    const ChallengePartyMember* selected_member(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?&storage_->selected:nullptr;}
    std::span<const BattleActorBirthWrite> writes(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?std::span<const BattleActorBirthWrite>{storage_->journal}:std::span<const BattleActorBirthWrite>{};}
    std::weak_ptr<const CharacterModelBinding> model_binding()const{return storage_?storage_->binding.model_binding():std::weak_ptr<const CharacterModelBinding>{};}
    const BattleCharacterBinding* prepared_binding(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?&storage_->binding:nullptr;}
    const BattleActorResourceMaps* resource_maps(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->resources_joined?&storage_->resource_maps:nullptr;}
    std::size_t bound_material_contexts(const std::array<const ForceInventory*,2>& owners)const{if(!current(owners)||!storage_->resources_joined)return 0;std::size_t count=0;for(const auto& model:storage_->material_contexts)count+=model.size();return count;}
    const BattleActorLevelRow* selected_level_row(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&storage_->level_cost_joined?&storage_->level_row:nullptr;}
    BattleActorConstructionStatus status()const{return status_;}BattleActorConstructionPending pending()const{BattleActorConstructionPending p;if(storage_&&storage_->member_joined)p.member_68344=false;if(storage_&&storage_->class_joined)p.class_selection_56ce0=false;if(storage_&&storage_->resources_joined)p.model_records_419ec=p.bone_parts_5380c=p.motion_tables_41b20=false;if(storage_&&storage_->level_cost_joined)p.level_cost_55f90=false;if(storage_&&storage_->defaults_joined)p.defaults_56180=false;return p;}
    u32 next_source_call()const{return status_==BattleActorConstructionStatus::AwaitMemberInitializer&&storage_?storage_->profile.words[6]:status_==BattleActorConstructionStatus::AwaitClassSelection&&storage_?0x80056ce0:status_==BattleActorConstructionStatus::AwaitModelBinding&&storage_?0x800419ec:status_==BattleActorConstructionStatus::AwaitScalarSetup&&storage_?0x80055f90:status_==BattleActorConstructionStatus::AwaitScalarDefaults&&storage_?0x8005614c:status_==BattleActorConstructionStatus::AwaitScalarEffects&&storage_?0x800562b8:0;}
    void retire(){storage_.reset();status_=BattleActorConstructionStatus::Retired;attempted_=true;}
private:
    bool roster_current(const std::array<const ForceInventory*,2>& owners)const{const auto r=storage_->roster.lock();return r&&r->current(owners);}
    bool class_sources_current()const{const auto& prepared=*storage_->binding.storage_;const auto data=prepared.resource->actor_data();return storage_->actor_data4ac.data==data.data&&storage_->actor_data4ac.size==data.size&&storage_->class_config4b0.data==prepared.setup.class_config.data()&&storage_->class_config4b0.size==prepared.setup.class_config.size();}
    bool resource_sources_current(const std::array<const ForceInventory*,2>& owners)const{const auto& prepared=*storage_->binding.storage_;if(storage_->resource_bank!=&prepared.instances||storage_->resource_motion!=&prepared.motion||storage_->resource_receipts.size()!=prepared.instances.instances()||storage_->material_contexts.size()!=prepared.instances.instances())return false;for(const auto& receipt:storage_->resource_receipts)if(!receipt.current(owners))return false;for(const auto& model:storage_->material_contexts)for(const auto& d:model)if(d.context!=&prepared.bank_material)return false;return true;}
    struct MaterialContextBinding {u32 dobj=0;const BattleBankMaterialContext* context=nullptr;};
    struct Storage {BattleCharacterBinding binding;ChallengeCharacterDescriptorReceipt allocation;BattleActorBirthProfile profile;ChallengePartyMember selected;BattleActorBirthState state;u8 team=0;std::vector<BattleActorBirthWrite> journal;std::weak_ptr<challenge_battle_roster_detail::Storage> roster;bool member_joined=false,class_joined=false,resources_joined=false,level_cost_joined=false,defaults_joined=false;ByteView actor_data4ac,class_config4b0;BattleActorResourceMaps resource_maps;std::vector<BattleBankInstanceReceipt> resource_receipts;std::vector<std::vector<MaterialContextBinding>> material_contexts;const HsdJointInstanceBank* resource_bank=nullptr;const CharacterMotionBank* resource_motion=nullptr;BattleActorLevelRow level_row;};
    std::unique_ptr<Storage> storage_;BattleActorConstructionStatus status_=BattleActorConstructionStatus::Closed;bool attempted_=false;
};
} // namespace gf
