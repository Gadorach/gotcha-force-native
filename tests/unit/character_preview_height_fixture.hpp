#pragma once
#include "engine/gameplay/character_preview_frame.hpp"
#include "tests/unit/startup_attachments_fixture.hpp"
namespace character_preview_height_fixture {
using namespace gf;
inline CharacterPreviewProfile preview(){CharacterPreviewProfile p;p.loaded=true;p.scene_hash=123;p.models={0x615,10};p.pitch=0x800;p.position={-60,15,0};p.one=1;p.radius=bits_to_f32(0x439e199a);p.near_plane=10;p.far_plane=40000;p.fov=41;p.aspect=640.f/480;p.angle_unit=bits_to_f32(0x38c90fdb);return p;}
inline CharacterPoseHeightProfile height(){CharacterPoseHeightProfile p;p.loaded=true;p.half=.5f;p.negative_factor=bits_to_f32(0xbd4ccccd);p.gain=bits_to_f32(0x3f333333);p.step=5;return p;}
struct Fixture {
    startup_attachments_fixture::Fixture source;std::vector<u8> program;
    bool load(std::string* error,bool immediate=false,u8 operand=236){
        if(!source.load(error))return false;
        auto& f=source.frame;f.setup[480+15]=1;for(unsigned i=0;i<4;++i)f.setup[480+0x22+i*8]=255;
        auto y=character_motion_driver_fixture::linear(50,54,4);y.channel=6;f.motion.clips[0].clip.bones[1].tracks.push_back(std::move(y));
        std::vector<u8> commands={1,0,0,2,10,0,0,0};if(!immediate)commands.insert(commands.end(),{0,0,0,2});commands.insert(commands.end(),{3,0,0,0,0,0,0,64,8,0,0,operand,0,0,3,232,0,1,255,255});
        program.assign(68,0);store_be32(program.data(),0x47464d50);store_be16(program.data()+4,3);program[6]=3;program[7]=1;store_be32(program.data()+8,character_program_model_hash({f.motion.model.data(),f.motion.model.size()}));store_be16(program.data()+12,1);program[14]=2;program[15]=3;store_be32(program.data()+16,68+commands.size());store_be32(program.data()+20,32);store_be32(program.data()+24,44);store_be32(program.data()+28,68);store_be32(program.data()+36,68);store_be32(program.data()+40,commands.size());
        auto d=f.motion.profile.descriptors[0].bytes;d[6]=d[14]=5;std::copy(d.begin(),d.end(),program.begin()+48);program.insert(program.end(),commands.begin(),commands.end());return true;
    }
    CharacterPreviewAssets assets()const{const auto& f=source.frame;return {{f.motion.model.data(),f.motion.model.size()},{f.prefix.data(),f.prefix.size()},{f.pose.data(),f.pose.size()},{program.data(),program.size()},{f.setup.data(),f.setup.size()},{f.runtime.data(),f.runtime.size()},{f.classes.data(),f.classes.size()},{f.fade.data(),f.fade.size()},{source.attachments.data(),source.attachments.size()},f.motion.clips};}
};
} // namespace character_preview_height_fixture
