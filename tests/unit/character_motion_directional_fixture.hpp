#pragma once
#include "tests/unit/character_motion_driver_fixture.hpp"
namespace character_motion_directional_fixture {
struct Fixture:character_motion_driver_fixture::Fixture {
    gf::CharacterLocalPose initial;
    bool load(std::string* error){
        if(!character_motion_driver_fixture::Fixture::load(error))return false;
        auto third=clips[0];third.index=2;third.clip.end_frame=8;
        for(unsigned bone=0;bone<2;++bone)third.clip.bones[bone].tracks={character_motion_driver_fixture::linear(float(200+bone*10),float(208+bone*10),8)};
        clips.push_back(std::move(third));auto descriptor=profile.descriptors[0];descriptor.choice=2;descriptor.bytes[5]=descriptor.bytes[13]=2;profile.descriptors.push_back(descriptor);
        for(unsigned index=0;index<clips.size();++index){auto& d=profile.descriptors[index];d.bytes[9]=gf::u8(index*2+4);d.bytes[10]=gf::u8(index*2+5);
            for(auto& bone:clips[index].clip.bones){auto z=character_motion_driver_fixture::linear(float(index)*.2f,float(index)*.2f+.1f,unsigned(clips[index].clip.end_frame));z.channel=3;bone.tracks.push_back(std::move(z));}
        }
        for(const auto& bone:rig.binding.bones){gf::CharacterBonePose pose;pose.joint_offset=bone.joint_offset;std::copy_n(bone.translation,3,pose.translation);initial.bones.push_back(pose);}
        return true;
    }
    bool directional(gf::CharacterMotionDriver& driver,std::string* error,std::array<gf::s8,2> model_bones={0,-1})const{
        return driver.load_directional_follow({model.data(),model.size()},rig,profile,clips,initial,model_bones,error);
    }
};
inline constexpr std::array<gf::u8,6> controls={5,5,5,1,0,2};
} // namespace character_motion_directional_fixture
