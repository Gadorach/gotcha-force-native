#pragma once
#include <span>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
struct StoryGuideCommand {u8 opcode=0;std::span<const u8> operands;u32 next_offset=0;bool terminal=false,external_wait=false;};
struct StoryGuideProgram {u8 slot=0;std::vector<u8> bytes;std::vector<u32> boundaries;};
//Fixed guide/popup BYTE dialect 801D11D8. Not the map/event halfword dialect,
//illustrated introduction or native VM/resource/voice completion authority.
class StoryGuideBank {
public:
    bool load(ByteView b,u32 panel_hash,u32 text_hash,std::string* error=nullptr){
        *this={};auto fail=[&](){*this={};if(error)*error="Story guide bank: schema/binding/command span";return false;};
        if(!b.data||b.size<24||b.size>(1u<<20)||b.be32(0)!=0x47464744||b.be16(4)!=1||!panel_hash||!text_hash||b.be32(12)!=panel_hash||b.be32(16)!=text_hash)return fail();
        const auto count=b.be16(6);const std::size_t base=24+std::size_t(count)*12;if(!count||count>16||base>b.size||b.size-base!=b.be32(8))return fail();
        u32 hash=2166136261;for(std::size_t i=24;i<b.size;++i)hash=(hash^b.data[i])*16777619u;if(!hash||hash!=b.be32(20))return fail();
        std::size_t end=0;int previous=-1;
        for(unsigned i=0;i<count;++i){const std::size_t at=24+i*12;const auto slot=b.data[at];const auto offset=b.be32(at+4),size=b.be32(at+8);if(slot<=previous||slot>=16||b.data[at+1]||b.be16(at+2)||offset!=end||size<2||size>256||offset>b.size-base||size>b.size-base-offset)return fail();previous=slot;
            StoryGuideProgram p;p.slot=slot;p.bytes.assign(b.data+base+offset,b.data+base+offset+size);std::size_t pc=0;bool terminal=false;
            while(pc<p.bytes.size()){
                const u8 opcode=p.bytes[pc];const unsigned width=opcode==1?3:opcode<=3?2:1;if(width>p.bytes.size()-pc)return fail();p.boundaries.push_back(u32(pc));pc+=width;if(opcode==2){terminal=true;if(pc!=p.bytes.size())return fail();break;}
            }
            if(!terminal)return fail();
            end+=size;programs_.push_back(std::move(p));
        }
        if(end!=b.size-base)return fail();
        //Logical references bind the complete profile, including resource hashes,
        //not merely identical instruction bytes paired with a different panel.
        u32 identity=2166136261;for(std::size_t i=0;i<b.size;++i)identity=(identity^b.data[i])*16777619u;
        if(!identity)return fail();
        fingerprint_=identity;panel_hash_=panel_hash;text_hash_=text_hash;if(error)error->clear();return true;
    }
    bool ready()const{return fingerprint_!=0;}u32 fingerprint()const{return fingerprint_;}u32 panel_hash()const{return panel_hash_;}u32 text_hash()const{return text_hash_;}
    const auto& programs()const{return programs_;}
    const StoryGuideProgram* find(unsigned slot)const{if(!ready())return nullptr;for(const auto& p:programs_)if(p.slot==slot)return &p;return nullptr;}
    //Operand spans borrow this bank, valid only until its next load/destruction.
    bool command(unsigned slot,u32 offset,StoryGuideCommand* out)const{
        if(out)*out={};
        if(!out)return false;
        const auto* p=find(slot);if(!p)return false;bool found=false;for(auto at:p->boundaries)if(at==offset)found=true;if(!found)return false;const auto op=p->bytes[offset];const unsigned width=op==1?3:op<=3?2:1;*out={op,std::span<const u8>(p->bytes).subspan(offset+1,width-1),offset+(op==2?1:width),op==2,op>=4};return true;
    }
private:
    std::vector<StoryGuideProgram> programs_;u32 fingerprint_=0,panel_hash_=0,text_hash_=0;
};
} //namespace gf
