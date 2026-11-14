#pragma once
//Structural map/event narrative dialect from8017CA3C and prefetch8017C7E0.
//Not the illustrated-introduction stream or a script execution/voice consumer.
#include <array>
#include <span>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
enum class StoryNarrativeOwner : u8 { Event,Gate };
struct StoryNarrativeKey {
    StoryNarrativeOwner owner=StoryNarrativeOwner::Event;
    u16 id=0;u8 phase=0; //event slots10/14/18hex -> phases0/1/2; gate slot8 -> phase0
    bool operator==(const StoryNarrativeKey&)const=default;
};
inline bool story_narrative_key_valid(const StoryNarrativeKey& key){
    return key.owner==StoryNarrativeOwner::Event?(key.id>0&&key.id<256&&key.phase<3):
        key.owner==StoryNarrativeOwner::Gate&&(key.id>0&&key.id<64&&key.phase==0);
}
inline bool story_narrative_operands(u8 opcode,u8* out){
    if(out)*out=0;
    constexpr std::array<u8,27> counts{0,1,0,2,2,1,0,1,2,1,1,1,2,1,2,2,2,2,1,2,3,0,1,1,1,1,0};
    if(!out||opcode<0x14||opcode>0x2e)return false;
    *out=counts[opcode-0x14];return true;
}
struct StoryNarrativeScript {StoryNarrativeKey key;std::vector<u16> words;std::vector<u32> boundaries;};
struct StoryNarrativeCommand {u8 opcode=0;std::span<const u16> operands;bool terminal=false;};
class StoryNarrativeBank {
public:
    bool load(ByteView b,u32 expected_map_fingerprint,std::string* error=nullptr){
        *this={};auto fail=[&](const char* why){if(error)*error=std::string("Story narrative bank: ")+why;return false;};
        if(!b.data||b.size<24||b.size>(2u<<20)||b.be32(0)!=0x47465342u||b.be16(4)!=1||b.be32(20)||!expected_map_fingerprint||b.be32(12)!=expected_map_fingerprint)return fail("schema/map identity");
        const auto count=b.be16(6);const auto payload=b.be32(8);const std::size_t base=24+std::size_t(count)*12;
        if(!count||count>1024||payload>(1u<<20)||base>b.size||b.size-base!=payload)return fail("bounded entry/payload extents");
        u32 hash=2166136261u;for(std::size_t at=24;at<b.size;++at)hash=(hash^b.data[at])*16777619u;
        if(!hash||hash!=b.be32(16))return fail("content fingerprint"); //scope/corruption guard, not authentication
        std::vector<StoryNarrativeScript> scripts;std::size_t end=0;u32 previous_rank=0;
        for(unsigned i=0;i<count;++i){const std::size_t at=24+i*12;StoryNarrativeScript script;
            script.key={StoryNarrativeOwner(b.data[at]),b.be16(at+2),b.data[at+1]};
            if(!story_narrative_key_valid(script.key))return fail("logical owner/phase/ID");
            const u32 rank=(u32(script.key.owner)<<24)|(u32(script.key.id)<<8)|script.key.phase;
            if(i&&rank<=previous_rank)return fail("duplicate/unordered logical identity");
            previous_rank=rank;
            const auto offset=b.be32(at+4),size=b.be32(at+8);
            if(offset!=end||size<2||size>8192||size%2||offset>payload||size>payload-offset)return fail("script segment extents");
            for(unsigned k=0;k<size;k+=2)script.words.push_back(b.be16(base+offset+k));
            std::size_t pc=0;bool terminal=false;
            while(pc<script.words.size()){
                const u16 header=script.words[pc];const u8 opcode=u8(header);u8 operands=0;
                if(!story_narrative_operands(opcode,&operands)||u8(header>>8)!=operands||1+std::size_t(operands)>script.words.size()-pc)return fail("opcode/operand span");
                script.boundaries.push_back(u32(pc));pc+=1+operands;
                if(opcode==0x2e){terminal=true;if(pc!=script.words.size())return fail("bytes after terminal");break;}
            }
            if(!terminal)return fail("missing structural terminal");
            end+=size;scripts.push_back(std::move(script));
        }
        if(end!=payload)return fail("unclaimed payload");
        scripts_=std::move(scripts);fingerprint_=hash;map_fingerprint_=expected_map_fingerprint;if(error)error->clear();return true;
    }
    bool ready()const{return fingerprint_!=0;}u32 fingerprint()const{return fingerprint_;}u32 map_fingerprint()const{return map_fingerprint_;}
    const std::vector<StoryNarrativeScript>& scripts()const{return scripts_;}
    const StoryNarrativeScript* find(const StoryNarrativeKey& key)const{
        if(!ready()||!story_narrative_key_valid(key))return nullptr;
        for(const auto& script:scripts_)if(script.key==key)return &script;
        return nullptr;
    }
    bool command(const StoryNarrativeKey& key,u32 word_offset,StoryNarrativeCommand* out)const{
        if(out)*out={};
        if(!out)return false;
        const auto* script=find(key);if(!script)return false;
        bool boundary=false;for(auto at:script->boundaries)if(at==word_offset){boundary=true;break;}if(!boundary)return false;
        const u16 header=script->words[word_offset];const unsigned operands=header>>8;
        out->opcode=u8(header);out->operands=std::span<const u16>(script->words).subspan(word_offset+1,operands);out->terminal=out->opcode==0x2e;return true;
    }
private:
    std::vector<StoryNarrativeScript> scripts_;u32 fingerprint_=0,map_fingerprint_=0;
};
} //namespace gf
