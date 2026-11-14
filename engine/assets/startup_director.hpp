#pragma once
// Owned GFDR1 script/counter inputs. No guest pointers or resource side effects.
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
inline constexpr std::array<u8,22> kStartupOperandBytes={1,5,0,0,0,2,1,3,1,1,0,0,1,3,1,1,0,2,0,2,1,2};
struct StartupDirectorProfile {
    std::vector<s16> actor_ids;
    std::vector<f32> camera_values;
    std::array<u32,8> config_words{};
    std::vector<u8> script;
    std::vector<u32> command_offsets;
    bool loaded=false;
};
namespace startup_director_detail {
inline s16 signed_word(u16 x){return std::bit_cast<s16>(x);}
inline bool finite(f32 x){return std::isfinite(x)&&std::abs(x)<=1e7f;}
inline bool valid(const StartupDirectorProfile& p){
    if(!p.loaded||p.actor_ids.empty()||p.actor_ids.size()>8||p.camera_values.empty()||p.camera_values.size()>32||p.script.empty()||p.script.size()>4096)return false;
    for(auto id:p.actor_ids)if(id< -1)return false;
    for(auto v:p.camera_values)if(!finite(v)||v<0||v>100000)return false;
    if(!finite(bits_to_f32(p.config_words[2])))return false;
    std::vector<u32> offsets;bool terminal=false,marked=false;
    auto actor=[&](u8 index){return index<p.actor_ids.size()&&p.actor_ids[index]>=0;};
    auto camera=[&](u8 index){return index<p.camera_values.size();};
    for(std::size_t at=0;at<p.script.size();){const u8 op=p.script[at];
        if(terminal||op>=kStartupOperandBytes.size()||offsets.size()>=1024||std::size_t(1+kStartupOperandBytes[op])>p.script.size()-at)return false;
        offsets.push_back(u32(at));const auto* c=p.script.data()+at;
        if(op==4)terminal=true;
        if(op==6&&c[1]!=255&&c[1]>=32)return false;
        if(op==7&&(!camera(c[1])||(c[2]!=255&&!camera(c[2]))||!camera(c[3])))return false;
        if(op==8&&c[1]!=255&&!camera(c[1]))return false;
        if((op==9||op==12||op==15)&&c[1]>1)return false;
        if(op==13){if(c[1]>2||!actor(c[1]==2?c[3]:c[2])||(c[1]==0&&c[3]>=128))return false;}
        if(op==14){if(c[1]>1||(c[1]==1&&!marked))return false;if(c[1]==0)marked=true;}
        if(op==17&&signed_word(load_be16(c+1))<=0)return false; // bounded positive ramp subset
        if((op==19||op==21)&&signed_word(load_be16(c+1))<0)return false;
        at+=1+kStartupOperandBytes[op];
    }
    return terminal&&offsets==p.command_offsets;
}
} // namespace startup_director_detail
inline bool startup_director_load(ByteView bytes,StartupDirectorProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup director profile: ")+why;return false;};
    if(!out||!bytes.data||bytes.size<32||bytes.size>8192||bytes.be32(0)!=0x47464452||bytes.be16(4)!=1||bytes.be32(8)!=bytes.size||bytes.be16(14)!=8)return fail("header/version/size");
    const unsigned actors=bytes.data[6],values=bytes.data[7];const std::size_t floats=32,config=floats+values*4,script=config+32+actors*2;
    if(!actors||actors>8||!values||values>32||bytes.be32(16)!=floats||bytes.be32(20)!=config||bytes.be32(24)!=script||!bytes.contains(script,bytes.be32(28))||script+bytes.be32(28)!=bytes.size)return fail("table extents/counts");
    StartupDirectorProfile next;
    for(unsigned i=0;i<actors;++i)next.actor_ids.push_back(startup_director_detail::signed_word(bytes.be16(config+32+i*2)));
    for(unsigned i=0;i<values;++i)next.camera_values.push_back(bytes.bef32(floats+i*4));
    for(unsigned i=0;i<8;++i)next.config_words[i]=bytes.be32(config+i*4);
    next.script.assign(bytes.data+script,bytes.data+bytes.size);
    for(std::size_t at=0;at<next.script.size();){const auto op=next.script[at];if(op>=22||1u+kStartupOperandBytes[op]>next.script.size()-at)return fail("opcode/operand extent");next.command_offsets.push_back(u32(at));at+=1+kStartupOperandBytes[op];}
    next.loaded=true;if(next.command_offsets.size()!=bytes.be16(12)||!startup_director_detail::valid(next))return fail("commands/identities/supported domain");
    *out=std::move(next);return true;
}
} // namespace gf
