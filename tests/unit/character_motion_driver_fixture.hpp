#pragma once
#include "engine/gameplay/character_motion_driver.hpp"
#include "tests/unit/character_model_fixture.hpp"
#include "tests/unit/character_program_fixture.hpp"
namespace character_motion_driver_fixture {
inline void little(std::vector<gf::u8>& data,float value){const auto bits=gf::f32_to_bits(value);for(unsigned i=0;i<4;++i)data.push_back(gf::u8(bits>>(i*8)));}
inline gf::HsdAnimTrack linear(float first,float last,unsigned wait){gf::HsdAnimTrack t;t.channel=5;
    for(unsigned i=0;i<3;++i){t.data.push_back(2);little(t.data,i?last:first);t.data.push_back(gf::u8(wait));}return t;}
struct Fixture {
    std::vector<gf::u8> model=character_model_fixture::archive();
    gf::CharacterPoseRig rig;gf::CharacterProgramAsset profile;std::vector<gf::CharacterMotionClipBinding> clips;
    bool load(std::string* error){
        auto prefix=character_model_fixture::prefix();gf::HsdModelBones bones;
        if(!gf::hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bones,error))return false;
        rig.binding=bones;rig.order={0,1};rig.mirror={0,1};rig.cuts={0,1,2,2,2};rig.lane_mask=3;rig.loaded=true;
        auto bytes=character_program_fixture::bytes();const auto hash=gf::character_program_model_hash({model.data(),model.size()});gf::store_be32(bytes.data()+8,hash);
        if(!gf::character_program_load({bytes.data(),bytes.size()},hash,&profile,error))return false;
        for(unsigned index=0;index<2;++index){gf::CharacterMotionClipBinding entry;entry.index=gf::u8(index);entry.clip.loaded=true;entry.clip.public_bone_count=2;entry.clip.end_frame=index?2:4;
            for(unsigned bone=0;bone<2;++bone){gf::HsdBoneMotion b;b.joint_offset=bones.bones[bone].joint_offset;b.has_aobj=true;
                const float first=float(index*100+bone*10);b.tracks.push_back(linear(first,first+entry.clip.end_frame,unsigned(entry.clip.end_frame)));entry.clip.bones.push_back(std::move(b));}
            clips.push_back(std::move(entry));
        }
        return true;
    }
    bool driver(gf::CharacterMotionDriver& out,std::string* error)const{return out.load({model.data(),model.size()},rig,profile,clips,error);}
};
} // namespace character_motion_driver_fixture
