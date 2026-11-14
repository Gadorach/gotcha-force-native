#pragma once
//Pointer-free declared cursor representation. A reference round-trip does not
//resume a VM, acknowledge resource/voice completion or authorize an event.
#include "engine/assets/story_narrative.hpp"
namespace gf {
struct StoryNarrativeReference {
    bool active=false;u32 bank_fingerprint=0;StoryNarrativeKey key;u32 word_offset=0;
    bool operator==(const StoryNarrativeReference&)const=default;
};
inline bool story_narrative_reference_valid(const StoryNarrativeReference& ref,const StoryNarrativeBank& bank){
    if(!bank.ready())return false;
    if(!ref.active)return ref.bank_fingerprint==0&&ref.key==StoryNarrativeKey{}&&ref.word_offset==0;
    StoryNarrativeCommand command;return ref.bank_fingerprint==bank.fingerprint()&&bank.command(ref.key,ref.word_offset,&command);
}
inline bool story_narrative_reference_from_declared(const StoryNarrativeBank& bank,const StoryNarrativeKey& key,
                                                    u32 word_offset,StoryNarrativeReference* out){
    if(!out)return false;
    StoryNarrativeReference next{true,bank.fingerprint(),key,word_offset};if(!story_narrative_reference_valid(next,bank))return false;
    *out=next;return true;
}
inline bool story_narrative_reference_encode(const StoryNarrativeReference& ref,const StoryNarrativeBank& bank,std::span<u8> out){
    if(out.size()!=20||!story_narrative_reference_valid(ref,bank))return false;
    store_be32(out.data(),0x47465352u);store_be16(out.data()+4,1);out[6]=u8(ref.active);out[7]=0;store_be32(out.data()+8,ref.bank_fingerprint);
    out[12]=u8(ref.key.owner);out[13]=ref.key.phase;store_be16(out.data()+14,ref.key.id);store_be32(out.data()+16,ref.word_offset);return true;
}
inline bool story_narrative_reference_decode_declared(ByteView b,const StoryNarrativeBank& bank,StoryNarrativeReference* out){
    if(!out||!b.data||b.size!=20||b.be32(0)!=0x47465352u||b.be16(4)!=1||b.data[6]>1||b.data[7])return false;
    StoryNarrativeReference next{b.data[6]!=0,b.be32(8),{StoryNarrativeOwner(b.data[12]),b.be16(14),b.data[13]},b.be32(16)};
    if(!story_narrative_reference_valid(next,bank))return false;
    *out=next;return true;
}
} //namespace gf
