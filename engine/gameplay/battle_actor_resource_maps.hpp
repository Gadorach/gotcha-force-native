#pragma once
#include "engine/assets/character_model_parts.hpp"
#include "engine/assets/hsd_joint_instances.hpp"
#include "engine/gameplay/character_pose.hpp"
namespace gf {
enum class BattleResourceWriteTarget:u8 {Resource,Actor,Descriptor,Motion};
enum class BattleResourceWriteKind:u8 {Byte,Word,FloatBits,WrapperReference,ModelReference,JointReference,ResourceRegionReference,MotionGroupPrepared};
//object/value identify NATIVE resources/instances/joints for reference kinds;
//they are never original pointer bits or host addresses truncated into words.
struct BattleResourceWrite {BattleResourceWriteTarget target=BattleResourceWriteTarget::Resource;BattleResourceWriteKind kind=BattleResourceWriteKind::Byte;u16 offset=0,object=0;u32 value=0;bool operator==(const BattleResourceWrite&)const=default;};
struct BattleResourceModelMap {u16 instance=0,record=0;u32 root=0;std::vector<u32> material_dobjs;};
struct BattleResourceBoneMap {u32 joint_offset=0;u16 instance_joint=0;std::array<u8,4> links{};std::array<f32,3> translation{};};
struct BattleActorResourceMaps {
    std::array<std::vector<BattleResourceModelMap>,2> models;
    std::array<std::vector<BattleResourceBoneMap>,2> bones;
    std::vector<std::array<u8,4>> links1994;std::vector<std::array<f32,3>> local_translations;
    std::array<std::array<u16,8>,32> parts{};std::array<u8,32> part_counts{};
    std::array<u16,16> motion_group_sizes{};u32 motion_archives=0;
    std::array<u32,2> cache_words{255,0};u16 records=0;u8 bone_count1d90=0;
    std::vector<BattleResourceWrite> writes;bool prepared=false;
};
//419EC/5380C/41B20 represented source bindings, using ALREADY constructed
//instances/rig/bank. Declared-input plan only, NOT current allocation authority,
//a world/local pose, motion request, source heap/callback or pool-capacity ACK.
inline bool battle_actor_resource_maps(const CharacterPoseRig& rig,const CharacterModelPartsProfile& parts,
                                      std::span<const HsdJointInstanceStateSet* const> instances,const CharacterMotionBank& motion,u8 slot,
                                      BattleActorResourceMaps* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle resource maps: ")+why;return false;};
    if(!out||slot>=6||!character_pose_detail::rig(rig)||!character_model_parts_valid(parts)||rig.binding.bones.size()<=11||instances.empty()||(instances.size()&1)||instances.size()>64)return fail("two-normal fresh rig/profile/record domain");
    const unsigned records=unsigned(instances.size()/2),count=unsigned(rig.binding.bones.size());
    const unsigned last_record=unsigned(parts.groups[1].base)+parts.groups[1].count-1;
    if(records<=last_record)return fail("part table exceeds complete bank");
    if(!motion.loaded||motion.groups[0].empty()||motion.groups[0][0]>=motion.archives.size()||motion.archives.empty()||motion.archives.size()>4096)return fail("prepared nonempty motion bank required");
    for(const auto& archive:motion.archives)if(archive.empty())return fail("empty prepared motion archive");
    for(const auto& group:motion.groups){if(group.size()>256)return fail("motion table budget");for(u32 index:group)if(index!=kCharacterNoMotion&&index>=motion.archives.size())return fail("motion alias index");}
    BattleActorResourceMaps p;p.records=u16(records);p.bone_count1d90=u8(count);for(auto& refs:p.parts)refs.fill(0xffff);p.links1994.resize(count);p.local_translations.resize(count);
    using T=BattleResourceWriteTarget;using K=BattleResourceWriteKind;auto put=[&](T target,K kind,unsigned at,unsigned object,u32 value){p.writes.push_back({target,kind,u16(at),u16(object),value});};
    put(T::Resource,K::WrapperReference,0x604,slot,0);put(T::Descriptor,K::WrapperReference,0x14,slot,0);put(T::Resource,K::Word,0xc28,0,255);put(T::Resource,K::Word,0xc2c,0,0);
    //Source419EC creates ALL normal records BEFORE ALL alternate records.
    for(unsigned style=0;style<2;++style)for(unsigned record=0;record<records;++record){const unsigned index=style*records+record;const auto* s=instances[index];
        if(!s||s->record!=record||s->style!=(style?HsdJointInstanceStyle::Alternate:HsdJointInstanceStyle::Normal)||s->animation_requested||s->world_evaluated||s->joints.empty()||s->joints.size()>256||s->dobjs.size()>1024||s->joints.front().joint.parent!=kHsdNoJoint)return fail("fresh ordered instance/record required");
        BattleResourceModelMap map;map.instance=u16(index);map.record=u16(record);map.root=s->joints.front().joint.archive_offset;if(!map.root)return fail("missing published root");
        for(unsigned j=0;j<s->joints.size();++j){const auto& joint=s->joints[j].joint;if(!joint.archive_offset||(j&&joint.parent>=j))return fail("instance hierarchy");for(float value:joint.translation)if(!character_pose_detail::finite(value))return fail("local translation domain");
            if(style){const auto* normal=instances[record];if(!normal||normal->joints.size()!=s->joints.size()||normal->joints[j].joint.archive_offset!=joint.archive_offset||normal->joints[j].joint.parent!=joint.parent||normal->joints[j].source_index!=s->joints[j].source_index)return fail("normal/alternate topology identity");}
        }
        for(const auto& d:s->dobjs){if(!d.source_offset||d.joint>=s->joints.size()||std::find(map.material_dobjs.begin(),map.material_dobjs.end(),d.source_offset)!=map.material_dobjs.end())return fail("DObj identity/context domain");map.material_dobjs.push_back(d.source_offset);}
        p.models[style].push_back(std::move(map));put(T::Resource,K::ModelReference,(style?0x788:0x708)+4*record,index,0);
    }
    //67E18 visits preorder, writes resource matrix TRANSLATION ONLY, wrapper
    //links to actor1994, and native joint refs to608/688. Alternate walk is LAST.
    for(unsigned style=0;style<2;++style){const auto* s=instances[style*records];if(s->joints.size()!=count)return fail("record0 bone count differs from rig");p.bones[style].resize(count);
        for(unsigned ordinal=0;ordinal<count;++ordinal){const unsigned logical=rig.binding.preorder_slots[ordinal];const auto& source=rig.binding.bones[logical];const auto& joint=s->joints[ordinal].joint;if(joint.archive_offset!=source.joint_offset)return fail("preorder/logical bone mismatch");
            BattleResourceBoneMap map;map.joint_offset=source.joint_offset;map.instance_joint=u16(ordinal);map.links={source.child,source.next,source.parent,u8(logical)};
            for(unsigned axis=0;axis<3;++axis){if(f32_to_bits(joint.translation[axis])!=f32_to_bits(source.translation[axis]))return fail("fresh instance translation differs from source rig");map.translation[axis]=joint.translation[axis];put(T::Resource,K::FloatBits,logical*48+12+16*axis,0,f32_to_bits(map.translation[axis]));}
            for(unsigned byte=0;byte<4;++byte)put(T::Actor,K::Byte,0x1994+logical*4+byte,0,map.links[byte]);
            put(T::Resource,K::JointReference,(style?0x688:0x608)+logical*4,style*records,ordinal);p.bones[style][logical]=map;p.links1994[logical]=map.links;p.local_translations[logical]=map.translation;
        }
    }
    put(T::Actor,K::Byte,0x1d90,0,count);put(T::Actor,K::ResourceRegionReference,0x1db4,slot,0x608);put(T::Actor,K::ResourceRegionReference,0x1db8,slot,0x688);
    //410BC clears the fresh3120-byte resource block BEFORE412C4. The per-bone
    //counts below therefore start at0, NOT at a guessed retained/respawn value.
    for(const auto& group:parts.groups){if(group.bone>=count||group.count>8)return fail("part-map count/bone domain");for(unsigned j=0;j<group.count;++j){const unsigned n=p.part_counts[group.bone],record=group.base+j;if(n>=8||record>=records)return fail("part-map source index");p.parts[group.bone][n]=u16(record);put(T::Resource,K::ModelReference,0x808+group.bone*32+n*4,record,0);p.part_counts[group.bone]=u8(n+1);put(T::Resource,K::Byte,0xc08+group.bone,0,n+1);}}
    //41B20/4BE08 native table preparation retains nulls/aliases/+1-tag semantics
    //without rewriting source bytes or assigning actor1D78 (that is4BB14 later).
    p.motion_archives=u32(motion.archives.size());for(unsigned group=0;group<16;++group){p.motion_group_sizes[group]=u16(motion.groups[group].size());put(T::Motion,K::MotionGroupPrepared,group*4,group,p.motion_group_sizes[group]);}
    p.prepared=true;*out=std::move(p);if(error)error->clear();return true;
}
} // namespace gf
