#pragma once
// GFMP1 owns source-bound byte programs and two-lane descriptors, not guest
// addresses, clips or an actor. Unknown commands/modes refuse before execution.
#include <algorithm>
#include <array>
#include <utility>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
struct CharacterProgram {u8 group=0,choice=0;std::vector<u8> bytes;};
struct CharacterProgramDescriptor {u8 group=0,choice=0;std::array<u8,20> bytes{};};
struct CharacterProgramAsset {
    u32 model_hash=0;u8 lane_mask=0;std::array<u8,2> default_models{};
    std::vector<CharacterProgram> programs;
    std::vector<CharacterProgramDescriptor> descriptors;
    bool loaded=false,control_flow=false,request_prefix_only=false;
};
inline u32 character_program_model_hash(ByteView bytes){
    u32 hash=2166136261u;for(std::size_t i=0;i<bytes.size;++i){hash^=bytes.data[i];hash*=16777619u;}return hash;
}
namespace character_program_detail {
inline s8 signed_byte(u8 x){return std::bit_cast<s8>(x);}
inline s16 signed_word(const u8* p){return std::bit_cast<s16>(load_be16(p));}
inline unsigned command_size(u8 op,bool control_flow=false){switch(op){case 0:case 7:case 11:return 4;case 1:case 2:case 3:return 8;case 6:case 8:return control_flow?4:0;case 13:return control_flow?8:0;default:return 0;}}
inline const CharacterProgramDescriptor* descriptor(const CharacterProgramAsset& a,u8 group,u8 choice){
    for(const auto& d:a.descriptors)if(d.group==group&&d.choice==choice)return &d;
    return nullptr;
}
inline bool valid_impl(const CharacterProgramAsset& a,bool transitions,bool control_flow=false,bool request_prefix=false){
    if(!a.loaded||a.control_flow!=control_flow||a.request_prefix_only!=request_prefix||(request_prefix&&!control_flow)||(control_flow&&!transitions)||!a.lane_mask||(a.lane_mask&~3u)||a.programs.empty()||a.programs.size()>64||a.descriptors.empty()||a.descriptors.size()>128)return false;
    for(auto model:a.default_models)if(model>=128)return false;
    std::size_t total=0;
    for(unsigned i=0;i<a.descriptors.size();++i){const auto& d=a.descriptors[i];
        if((!transitions&&!d.group)||d.group>=128||d.choice>=128)return false;
        for(unsigned j=0;j<i;++j)if(d.group==a.descriptors[j].group&&d.choice==a.descriptors[j].choice)return false;
        for(unsigned lane=0;lane<2;++lane)if(d.bytes[6+lane*8]&~7u)return false;
    }
    for(unsigned i=0;i<a.programs.size();++i){const auto& p=a.programs[i];
        if((!transitions&&!p.group)||p.group>=128||p.choice>=128||p.bytes.empty()||p.bytes.size()>1024||p.bytes.size()>65536-total)return false;
        total+=p.bytes.size();
        for(unsigned j=0;j<i;++j)if(p.group==a.programs[j].group&&p.choice==a.programs[j].choice)return false;
        bool terminal=false;unsigned commands=0;std::vector<std::pair<std::size_t,unsigned>> visited;
        for(std::size_t at=0;at<p.bytes.size();){
            if(control_flow){auto prior=std::find_if(visited.begin(),visited.end(),[&](const auto& entry){return entry.first==at;});if(prior!=visited.end()){bool yield=false;for(auto it=prior;it!=visited.end();++it)yield|=p.bytes[it->first]==0;if(!yield)return false;terminal=true;break;}}
            const auto* c=p.bytes.data()+at;const auto size=command_size(c[0],control_flow);
            if(++commands>128||!size||size>p.bytes.size()-at||terminal)return false;
            if(control_flow){for(const auto& entry:visited)if(at<entry.first+entry.second&&entry.first<at+size)return false;visited.push_back({at,size});}
            // Initial subset: instant opcode1, no auxiliary mode, nonnegative
            // authored starts. Blends/negative authored starts require more
            // actor lifecycle branches; NEVER reinterpret them as instant.
            if(c[0]==1){
                const bool mode=transitions?(c[3]==0?c[4]==0:c[3]==2||(control_flow&&c[3]==1)):(c[3]==0&&c[4]==0);
                if(!mode||c[5]||signed_word(c+6)<0||signed_word(c+6)>8192||!descriptor(a,c[1],c[2]))return false;
            }
            if(c[0]==0){if(c[1]&~(control_flow?11u:3u))return false;if(request_prefix){if(at+size!=p.bytes.size())return false;terminal=true;break;}terminal=(c[1]&1)!=0;if(control_flow&&terminal)break;}
            if(control_flow&&c[0]==6){const s32 next=s32(at)+2-signed_word(c+2);if(next<0||std::size_t(next)>=p.bytes.size())return false;at=std::size_t(next);continue;}
            at+=size;
        }
        if(!terminal)return false;
    }
    return true;
}
inline bool valid(const CharacterProgramAsset& a){return valid_impl(a,false);}
inline bool valid_transitions(const CharacterProgramAsset& a){return valid_impl(a,true);}
inline bool valid_control_flow(const CharacterProgramAsset& a){return valid_impl(a,true,true);}
} // namespace character_program_detail
inline bool character_program_load_impl(ByteView bytes,u32 model_hash,CharacterProgramAsset* out,std::string* error,bool transitions,bool control_flow=false,bool request_prefix=false){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("character program: ")+why;return false;};
    if(!out||!bytes.data||bytes.size<32||bytes.size>65536||bytes.be32(0)!=0x47464d50||bytes.be16(4)!=(request_prefix?4:control_flow?3:transitions?2:1)||bytes.be32(8)!=model_hash||bytes.be32(16)!=bytes.size)
        return fail("header/version/model identity/size");
    const unsigned programs=bytes.data[7],descriptors=bytes.be16(12);
    const std::size_t ptable=32,dtable=ptable+programs*12,payload=dtable+descriptors*24;
    if(!programs||programs>64||!descriptors||descriptors>128||payload>bytes.size||bytes.be32(20)!=ptable||bytes.be32(24)!=dtable||bytes.be32(28)!=payload)return fail("table counts/extents");
    CharacterProgramAsset next;next.model_hash=model_hash;next.lane_mask=bytes.data[6];next.default_models={bytes.data[14],bytes.data[15]};
    for(unsigned i=0;i<descriptors;++i){const auto at=dtable+i*24;if(bytes.be16(at+2))return fail("descriptor reserved bytes");
        CharacterProgramDescriptor d;d.group=bytes.data[at];d.choice=bytes.data[at+1];std::copy_n(bytes.data+at+4,20,d.bytes.begin());next.descriptors.push_back(d);
    }
    std::size_t cursor=payload;
    for(unsigned i=0;i<programs;++i){const auto at=ptable+i*12;const auto start=bytes.be32(at+4),size=bytes.be32(at+8);
        if(bytes.be16(at+2)||start!=cursor||!bytes.contains(start,size))return fail("program reserved bytes/contiguous extent");
        CharacterProgram p;p.group=bytes.data[at];p.choice=bytes.data[at+1];p.bytes.assign(bytes.data+start,bytes.data+start+size);next.programs.push_back(std::move(p));cursor+=size;
    }
    next.loaded=true;next.control_flow=control_flow;next.request_prefix_only=request_prefix;
    if(cursor!=bytes.size||!character_program_detail::valid_impl(next,transitions,control_flow,request_prefix))return fail("unsupported/ambiguous program or descriptor");
    *out=std::move(next);return true;
}
inline bool character_program_load(ByteView bytes,u32 model_hash,CharacterProgramAsset* out,std::string* error){return character_program_load_impl(bytes,model_hash,out,error,false);}
// Explicit GFMP2: local group0 and mode2 duration blending. Legacy GFMP1 and
// its instantaneous command validation remain unchanged.
inline bool character_program_load_transitions(ByteView bytes,u32 model_hash,CharacterProgramAsset* out,std::string* error){return character_program_load_impl(bytes,model_hash,out,error,true);}
// GFMP3 owns validated reachable control flow. Opcode8/13 bytes can be recovered
// structurally, but execution still refuses until a qualified consumer exists.
inline bool character_program_load_control_flow(ByteView bytes,u32 model_hash,CharacterProgramAsset* out,std::string* error){return character_program_load_impl(bytes,model_hash,out,error,true,true);}
//GFMP4 ends at the FIRST real wait, preserving its flags/threshold verbatim.
//It authorizes one request prefix, never a continuation or a fabricated terminal.
inline bool character_program_load_request_prefix(ByteView bytes,u32 model_hash,CharacterProgramAsset* out,std::string* error){return character_program_load_impl(bytes,model_hash,out,error,true,true,true);}
} // namespace gf
