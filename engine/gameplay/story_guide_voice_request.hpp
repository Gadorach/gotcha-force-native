#pragma once
#include "engine/assets/story_guide_voice.hpp"
#include "engine/gameplay/story_editor_guide.hpp"
#include "engine/gameplay/source_byte_rng_owner.hpp"
namespace gf {
class StoryGuideVoiceRequests;
namespace story_guide_voice_detail {struct Scope {};}
//Native source-call/RNG evidence ONLY. No public delivered/completed boolean or
//voice-slot token. A future actual shared queue consumer must be qualified.
class StoryGuideVoiceRequest {
public:
    bool current(const StoryGuideVoiceLibrary& library,const SourceByteRngOwner& rng)const{return !scope_.expired()&&library_.same_resources(library)&&rng.same_owner(rng_);}
    const StoryGuideReference& command()const{return command_;}u32 source_bank()const{return 14;}u32 source_key()const{return key_;}u32 archive()const{return 0;}u32 member()const{return member_;}u8 consumed_random_byte()const{return random_;}u32 sequence()const{return sequence_;}
    SourceByteRngState before()const{return before_;}SourceByteRngState after()const{return after_;}
    bool requires_audio_consumer()const{return true;}
private:
    friend class StoryGuideVoiceRequests;
    std::weak_ptr<story_guide_voice_detail::Scope> scope_;StoryGuideVoiceLibrary library_;SourceByteRngSnapshot rng_;StoryGuideReference command_;u32 key_=0,member_=0,sequence_=0;u8 random_=0;SourceByteRngState before_{},after_{};
};
//The two slot4 source calls, in order0/5, including their unconditional055FC
//draw even with arg3=-1. This is NOT guide VM timing or software-player/ring ACK.
class StoryGuideVoiceRequests {
public:
    StoryGuideVoiceRequests()=default;StoryGuideVoiceRequests(const StoryGuideVoiceRequests&)=delete;StoryGuideVoiceRequests& operator=(const StoryGuideVoiceRequests&)=delete;
    bool begin(const StoryGuideBank& bank,const StoryGuideVoiceLibrary& library,const SourceByteRngOwner& rng){
        close();if(!bank.ready()||!library.ready()||!rng.ready())return false;
        for(unsigned i=0;i<2;++i){StoryGuideCommand c;if(!bank.command(4,i?5:0,&c)||c.opcode!=1||c.operands.size()!=2||c.operands[0]!=14||c.operands[1]!=60+i||!library.audio(60+i))return false;}
        bank_=bank;library_=library;rng_=rng.snapshot();scope_=std::make_shared<story_guide_voice_detail::Scope>();return true;
    }
    bool issue_unfulfilled(const StoryGuideReference& ref,SourceByteRngOwner& rng,StoryGuideVoiceRequest* out){
        if(out)*out={};
        if(!out||!scope_||next_>=2||!rng.same_owner(rng_)||!story_guide_reference_valid(ref,bank_)||ref.slot!=4||ref.byte_offset!=(next_?5u:0u))return false;
        StoryGuideCommand c;if(!bank_.command(4,ref.byte_offset,&c)||c.opcode!=1||c.operands.size()!=2||c.operands[0]!=14||c.operands[1]!=60+next_)return false;const auto* row=library_.row(60+next_);if(!row)return false;StoryGuideVoiceRequest request;request.scope_=scope_;request.library_=library_;request.rng_=rng_;request.command_=ref;request.key_=row->key;request.member_=row->member;request.sequence_=next_;request.before_=rng.state();u8 byte=0;if(!rng.next_byte(&byte))return false;request.random_=byte;request.after_=rng.state();++next_;*out=std::move(request);return true;
    }
    void close(){scope_.reset();bank_={};library_={};rng_={};next_=0;}
    unsigned issued()const{return next_;}bool ready()const{return bool(scope_);}
private:
    StoryGuideBank bank_;StoryGuideVoiceLibrary library_;SourceByteRngSnapshot rng_;std::shared_ptr<story_guide_voice_detail::Scope> scope_;unsigned next_=0;
};
} //namespace gf
