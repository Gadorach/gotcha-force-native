#pragma once
#include "tests/unit/character_preview_height_fixture.hpp"
namespace character_preview_controls_fixture {
using namespace gf;
inline std::vector<u8> encode(const CharacterProgramAsset& p){const unsigned pt=32,dt=pt+unsigned(p.programs.size())*12,body=dt+unsigned(p.descriptors.size())*24;std::vector<u8> b(body);store_be32(b.data(),0x47464d50);store_be16(b.data()+4,3);b[6]=p.lane_mask;b[7]=u8(p.programs.size());store_be32(b.data()+8,p.model_hash);store_be16(b.data()+12,u16(p.descriptors.size()));b[14]=p.default_models[0];b[15]=p.default_models[1];store_be32(b.data()+20,pt);store_be32(b.data()+24,dt);store_be32(b.data()+28,body);
    for(unsigned i=0;i<p.descriptors.size();++i){const auto at=dt+i*24;b[at]=p.descriptors[i].group;b[at+1]=p.descriptors[i].choice;std::copy(p.descriptors[i].bytes.begin(),p.descriptors[i].bytes.end(),b.begin()+at+4);}
    for(unsigned i=0;i<p.programs.size();++i){const auto at=pt+i*12;b[at]=p.programs[i].group;b[at+1]=p.programs[i].choice;store_be32(b.data()+at+4,u32(b.size()));store_be32(b.data()+at+8,u32(p.programs[i].bytes.size()));b.insert(b.end(),p.programs[i].bytes.begin(),p.programs[i].bytes.end());}store_be32(b.data()+16,u32(b.size()));return b;
}
inline std::vector<u8> events(u32 hash){std::vector<u8> b(36);store_be32(b.data(),0x47465345);store_be16(b.data()+4,1);store_be16(b.data()+6,2);store_be32(b.data()+12,hash);store_be16(b.data()+16,0x615);store_be32(b.data()+24,0x3e4ccccd);store_be32(b.data()+28,0x3f333333);store_be32(b.data()+32,0x40000000);for(u8 id:{u8(24),u8(25)}){const u8 row[]={id,1,0,0,0,1,1,0,0,u8(id==24?36:18),255,0};b.insert(b.end(),std::begin(row),std::end(row));}store_be32(b.data()+8,u32(b.size()));return b;}
struct Fixture:character_preview_height_fixture::Fixture {
    std::vector<u8> sound;
    bool load(std::string* error){if(!character_preview_height_fixture::Fixture::load(error))return false;auto& frame=source.frame;frame.setup[480+0x42]=0;frame.setup[480+0x43]=255;auto& action=frame.motion.clips[1].clip;for(unsigned k=1;k<3;++k){auto t=character_motion_driver_fixture::linear(0,action.end_frame*f32(k+1),unsigned(action.end_frame));t.channel=u8(5+k);action.bones[0].tracks.push_back(t);}
        CharacterProgramAsset p;if(!character_program_load_control_flow({program.data(),program.size()},character_program_model_hash({frame.motion.model.data(),frame.motion.model.size()}),&p,error))return false;
        p.descriptors[0].bytes[19]=0;p.descriptors[0].bytes[8]=p.descriptors[0].bytes[16]=255;auto d=frame.motion.profile.descriptors[1];d.group=0;d.bytes[8]=24;d.bytes[16]=25;d.bytes[19]=1;p.descriptors.push_back(d);d.choice=3;d.bytes[16]=26;p.descriptors.push_back(d);
        p.programs={{0,0,{1,0,0,0,0,0,0,0,0,0,0,100,0,1,255,255}},
                    {0,1,{3,0,0,0,0,0,0,64,1,0,1,0,0,0,0,0,0,0,0,100,0,1,255,255}},
                    {0,2,{13,0,1,0,1,0,0,0,0,0,0,100,0,1,255,255}},
                    {0,3,{1,0,3,0,0,0,0,0,0,0,0,100,0,1,255,255}},
                    {0,4,{1,0,1,0,0,0,0,0,7,0,4,5,0,0,0,100,0,1,255,255}},
                    {0,5,{1,0,1,0,0,0,0,0,6,0,255,254,0,0,0,100,0,1,255,255}},
                    {0,40,{1,0,1,0,0,0,0,0,0,0,0,100,0,1,255,255}}};program=encode(p);sound=events(p.model_hash);return character_program_detail::valid_control_flow(p);
    }
    ByteView event_view()const{return {sound.data(),sound.size()};}
};
} //namespace character_preview_controls_fixture
