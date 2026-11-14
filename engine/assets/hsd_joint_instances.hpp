#pragma once
#include "engine/assets/hsd_record_animation.hpp"
#include <span>
namespace gf {
enum class HsdJointInstanceStyle {Normal,Alternate};
struct HsdJointInstanceState {
    u32 source_index=0;HsdJoint joint; //Authored local SRT; NOT an evaluated world.
};
struct HsdDObjInstanceState {
    u32 source_offset=0,joint=0,material_offset=0,flags=0;
    AttachmentAlternateState material;
    std::vector<u32> meshes; //Bank indices. PObjs on one DObj share this material.
};
struct HsdJointInstanceStateSet {
    u16 record=0;HsdJointInstanceStyle style=HsdJointInstanceStyle::Normal;
    std::vector<HsdJointInstanceState> joints;std::vector<HsdDObjInstanceState> dobjs;
    //Value-only render metadata, never copied vertex/index/image buffers.
    std::vector<std::pair<u32,HsdMeshProperties>> mesh_properties;
    bool animation_requested=false,world_evaluated=false;
};
struct HsdJointInstanceIdentity {};
//Weak lifetime token; a copied token cannot keep retired instance state current.
class HsdJointInstanceReceipt {
public:
    bool current()const{return !identity_.expired();}
    bool same_instance(const HsdJointInstanceReceipt& other)const{return current()&&other.current()&&!identity_.owner_before(other.identity_)&&!other.identity_.owner_before(identity_);}
private:std::weak_ptr<const HsdJointInstanceIdentity> identity_;friend class HsdJointInstanceBank;friend class HsdJointInstanceOwner;
};
//Exclusive native ownership of one already-prepared root. Transfer keeps its
//identity/source geometry; retiring destroys only this mutable metadata graph.
//Not a general HSD refcount/virtual callback, world, skin or renderer owner.
class HsdJointInstanceOwner {
public:
    HsdJointInstanceOwner()=default;HsdJointInstanceOwner(const HsdJointInstanceOwner&)=delete;HsdJointInstanceOwner& operator=(const HsdJointInstanceOwner&)=delete;
    HsdJointInstanceOwner(HsdJointInstanceOwner&&)=default;HsdJointInstanceOwner& operator=(HsdJointInstanceOwner&&)=default;
    bool current()const{return storage_&&storage_->identity;}
    const HsdJointInstanceStateSet* state()const{return current()?&storage_->state:nullptr;}
    HsdJointInstanceReceipt receipt()const{HsdJointInstanceReceipt r;if(current())r.identity_=storage_->identity;return r;}
    bool same_source(const HsdRecordAnimationBank& bank)const{return current()&&storage_->source==bank.data_;}
    const HsdMesh* source_mesh(unsigned index)const{return current()&&index<storage_->source->scene.meshes.size()?&storage_->source->scene.meshes[index]:nullptr;}
    const HsdTexture* source_texture(unsigned index)const{return current()&&index<storage_->source->scene.textures.size()?&storage_->source->scene.textures[index]:nullptr;}
    void retire(){storage_.reset();}
private:
    friend class HsdJointInstanceBank;
    struct Storage {std::shared_ptr<const hsd_record_detail::Bank> source;HsdJointInstanceStateSet state;std::shared_ptr<const HsdJointInstanceIdentity> identity;};
    std::unique_ptr<Storage> storage_;
};
//07030/HSD_LoadJoint preparation, normal-all-records then alternate-all-records.
//No Anim request, identity world, deformation clearing, rendering or actor ACK.
//The decoded source bank survives loader destruction; only small mutable instance
//fields are copied. Material identity is per DObj, NOT per PObj/source MObj.
class HsdJointInstanceBank {
    struct Request {u16 record=0;bool alternate=false;};
public:
    HsdJointInstanceBank()=default;HsdJointInstanceBank(const HsdJointInstanceBank&)=delete;HsdJointInstanceBank& operator=(const HsdJointInstanceBank&)=delete;
    HsdJointInstanceBank(HsdJointInstanceBank&&)=default;HsdJointInstanceBank& operator=(HsdJointInstanceBank&&)=default;
    bool construct(const HsdRecordAnimationBank& bank,const AttachmentAlternateProfile& alternate,std::string* error){
        retire();auto fail=[&](const char* why){if(error)*error=std::string("hsd joint instances: ")+why;return false;};
        if(!bank.data_||!attachment_alternate_profile_valid(alternate))return fail("loaded bank/source alternate profile required");
        if(bank.records()>4096)return fail("bank topology/budget");
        try{std::vector<Request> order;order.reserve(2*bank.records());for(unsigned style=0;style<2;++style)for(unsigned record=0;record<bank.records();++record)order.push_back({u16(record),style!=0});return construct_records(bank,&alternate,order,error);}
        catch(const std::exception&){return fail("uncertain allocation/construction; no partial publication");}
    }
    //06FB4/LoadJointDesc: explicit normal record copies, including repeated
    //records with independent mutable state. No alternate flags or pose request.
    bool construct_normal_records(const HsdRecordAnimationBank& bank,std::span<const u16> records,std::string* error){
        retire();if(!bank.data_||records.size()>256){if(error)*error="hsd joint instances: normal record/budget domain";return false;}
        try{std::vector<Request> order;order.reserve(records.size());for(u16 record:records)order.push_back({record,false});return construct_records(bank,nullptr,order,error);}
        catch(const std::exception&){if(error)*error="hsd joint instances: uncertain normal construction; no partial publication";return false;}
    }
private:
    bool construct_records(const HsdRecordAnimationBank& bank,const AttachmentAlternateProfile* alternate,std::span<const Request> order,std::string* error){
        retire();auto fail=[&](const char* why){if(error)*error=std::string("hsd joint instances: ")+why;return false;};
        if(!bank.data_)return fail("loaded bank required");
        for(const auto& request:order)if(request.alternate&&(!alternate||!attachment_alternate_profile_valid(*alternate)))return fail("source alternate profile required");
        try{
            auto next=std::make_unique<Storage>();next->source=bank.data_;const auto& full=next->source->scene;const auto& r=next->source->reader;
            if(full.mesh_joint.size()!=full.meshes.size()||next->source->records.size()>4096)return fail("bank topology/budget");
            for(const auto& request:order){const unsigned record=request.record;const bool style=request.alternate;if(record>=next->source->records.size())return fail("published record outside bank");
                Entry entry;auto& s=entry.state;s.record=u16(record);s.style=style?HsdJointInstanceStyle::Alternate:HsdJointInstanceStyle::Normal;
                std::vector<u32> remap(full.joints.size(),kHsdNoJoint);std::set<u32> joint_sources,dobj_sources;
                for(u32 index=0;index<full.joints.size();++index){const auto& original=full.joints[index];if(original.model_root!=record)continue;
                    if(s.joints.size()>=256||!joint_sources.insert(original.archive_offset).second||(original.flags&0x5020u))return fail("joint budget/instance/non-DObj union");
                    u32 klass=0,robj=0,dobj=0;
                    if(!r.has(original.archive_offset,64)||!hsd_record_detail::optional(r,original.archive_offset,&klass)||klass||!hsd_record_detail::optional(r,original.archive_offset+60,&robj)||robj||!hsd_record_detail::optional(r,original.archive_offset+16,&dobj))return fail("custom joint/constraint/pointers");
                    auto joint=original;
                    if(joint.parent!=kHsdNoJoint){if(joint.parent>=remap.size()||remap[joint.parent]==kHsdNoJoint)return fail("parent outside record");joint.parent=remap[joint.parent];}
                    else if(!s.joints.empty())return fail("multiple roots");
                    const u32 local=u32(s.joints.size());remap[index]=local;if(style)joint.flags=(joint.flags|alternate->words[0])&~alternate->words[1];s.joints.push_back({index,joint});
                    while(dobj){
                        if(s.dobjs.size()>=1024||!dobj_sources.insert(dobj).second||!r.has(dobj,16))return fail("DObj budget/cycle/shared destination");
                        HsdDObjInstanceState d;d.source_offset=dobj;d.joint=local;u32 material=0,pe=0,material_data=0,render_desc=0;
                        if(!hsd_record_detail::optional(r,dobj,&klass)||klass||!hsd_record_detail::optional(r,dobj+8,&material)||!material||!r.has(material,24)||!hsd_record_detail::optional(r,material,&klass)||klass||!hsd_record_detail::optional(r,material+12,&material_data)||!material_data||!hsd_record_detail::optional(r,material+16,&render_desc)||render_desc||!hsd_record_detail::optional(r,material+20,&pe))return fail("default nonnull material required");
                        d.material_offset=material;bool first=true;
                        for(u32 mesh=0;mesh<full.meshes.size();++mesh)if(full.mesh_joint[mesh]==index&&full.meshes[mesh].dobj_offset==dobj){
                            const auto& base=full.meshes[mesh];if(base.material_offset!=material)return fail("DObj/material identity");
                            if(first){const u32 category=base.render_mode&0x60000000u;if(category==0x20000000u||(base.render_mode&(0x04000000u|0x1000u)))return fail("unsupported material category/global branch");
                                d.flags=category==0?2:category==0x40000000u?8:4;d.material.joint_flags=original.flags;d.material.dobj_flags=d.flags;d.material.render_mode=base.render_mode;d.material.alpha=base.material_alpha;d.material.has_pe=pe!=0;
                                if(pe){if(!r.has(pe,12))return fail("PE range");std::copy_n(r.block.data+pe,12,d.material.pe.begin());}
                                if(style){AttachmentAlternateState changed;if(!attachment_alternate_apply(*alternate,d.material,&changed,error))return false;d.material=changed;d.flags=changed.dobj_flags;}first=false;
                            }
                            HsdMeshProperties properties=base;if(style&&!attachment_alternate_properties(base,d.material,&properties,error))return false;
                            d.meshes.push_back(mesh);s.mesh_properties.emplace_back(mesh,properties);
                        }
                        if(first)return fail("DObj without represented PObj material state");
                        s.dobjs.push_back(std::move(d));if(!hsd_record_detail::optional(r,dobj+4,&dobj))return fail("DObj next pointer");
                    }
                }
                if(s.joints.empty()||s.joints.front().joint.archive_offset!=next->source->records[record].root)return fail("published root missing");
                std::size_t expected=0;for(u32 mesh=0;mesh<full.meshes.size();++mesh){const u32 j=full.mesh_joint[mesh];if(j>=full.joints.size())return fail("mesh owner");if(full.joints[j].model_root==record)++expected;}
                if(s.mesh_properties.size()!=expected)return fail("unconsumed record geometry");
                entry.identity=std::make_shared<const HsdJointInstanceIdentity>();next->entries.push_back(std::move(entry));
            }
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){return fail("uncertain allocation/construction; no partial publication");}
    }
public:
    void retire(){storage_.reset();}
    std::size_t instances()const{return storage_?storage_->entries.size():0;}
    const HsdJointInstanceStateSet* state(unsigned index)const{return storage_&&index<instances()&&storage_->entries[index].identity?&storage_->entries[index].state:nullptr;}
    HsdJointInstanceReceipt receipt(unsigned index)const{HsdJointInstanceReceipt r;if(storage_&&index<instances())r.identity_=storage_->entries[index].identity;return r;}
    const HsdMesh* source_mesh(unsigned index)const{return storage_&&index<storage_->source->scene.meshes.size()?&storage_->source->scene.meshes[index]:nullptr;}
    const HsdTexture* source_texture(unsigned index)const{return storage_&&index<storage_->source->scene.textures.size()?&storage_->source->scene.textures[index]:nullptr;}
    bool take_instance(unsigned index,HsdJointInstanceOwner* out,std::string* error){
        if(!out||out->storage_||!storage_||index>=instances()||!storage_->entries[index].identity){if(error)*error="hsd joint instance transfer: fresh output/current unclaimed instance required";return false;}
        try{auto next=std::make_unique<HsdJointInstanceOwner::Storage>();auto& entry=storage_->entries[index];next->source=storage_->source;next->state=std::move(entry.state);next->identity=std::move(entry.identity);entry.state={};out->storage_=std::move(next);if(error)error->clear();return true;}
        catch(const std::exception&){if(error)*error="hsd joint instance transfer: allocation refused before transfer";return false;}
    }
private:
    struct Entry {HsdJointInstanceStateSet state;std::shared_ptr<const HsdJointInstanceIdentity> identity;};
    struct Storage {std::shared_ptr<const hsd_record_detail::Bank> source;std::vector<Entry> entries;};std::unique_ptr<Storage> storage_;
};
} // namespace gf
