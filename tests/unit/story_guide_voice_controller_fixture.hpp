#pragma once
#include "engine/gameplay/story_guide_voice_controller.hpp"
#include "tests/unit/story_guide_voice_pcm_fixture.hpp"
namespace guide_voice_controller_fixture {
using namespace gf;using guide_voice_pcm_fixture::hash;
inline SourceAdxControlProfile profile(){SourceAdxControlProfile p;p.lengths={4,2,2,5};for(auto& gains:p.gains)gains={.6f,.7f,.8f,.9f,1};p.full=127;p.half=.5f;p.loaded=true;const std::array<std::pair<unsigned,s32>,6> anchors={{{0,-9600},{76,-440},{88,-310},{101,-190},{114,-90},{127,0}}};for(unsigned n=1;n<anchors.size();++n){const auto [a,x]=anchors[n-1];const auto [b,y]=anchors[n];for(unsigned i=a;i<=b;++i)p.volume[i]=x+s32((s64(y-x)*(i-a))/(b-a));}if(!p.valid())throw std::runtime_error("owned source-control profile");return p;}
inline StoryGuideBank bank(){const std::array<u8,12> program={1,14,60,0,13,1,14,61,0,14,2,0};std::vector<u8> b(48);store_be32(b.data(),0x47464744);store_be16(b.data()+4,1);store_be16(b.data()+6,1);store_be32(b.data()+8,12);store_be32(b.data()+12,11);store_be32(b.data()+16,22);b[24]=4;store_be32(b.data()+32,12);std::copy(program.begin(),program.end(),b.begin()+36);store_be32(b.data()+20,hash(std::span<const u8>(b).subspan(24)));StoryGuideBank out;if(!out.load({b.data(),b.size()},11,22))throw std::runtime_error("owned fixed guide program");return out;}
struct Harness {
    guide_voice_pcm_fixture::Sink sink;PcmQueue queue{sink};PcmMixer mixer;StoryGuideVoiceLibrary library;SourceAdxControlProfile control_profile;StoryGuideBank guide;SourceByteRngOwner rng;StoryGuideVoicePcmSource pcm;StoryGuideVoiceController controller;std::string error;
    explicit Harness(u32 enabled=1,unsigned frames=64,SourceByteRngState seed={17,99}):library(guide_voice_pcm_fixture::voices(frames)),control_profile(profile()),guide(bank()){
        if(!rng.initialize_explicit(seed)||!queue.start()||!pcm.load(library,guide_voice_pcm_fixture::gain(),SourceAdxSdkOutput::Stereo,48000,queue,&error)||!controller.load(control_profile,library,pcm,rng,enabled,&error)||!controller.begin_guide(guide,rng,&error))throw std::runtime_error("new native guide-controller fixture");
    }
    StoryGuideReference ref(unsigned n)const{return {guide.fingerprint(),n?5u:0u,4};}
    bool enqueue(u32 opcode,std::array<u32,4> args,u8 known=1,SourceAdxEnqueueOutcome* output=nullptr,bool* alias_output=nullptr){SourceAdxEnqueueOutcome outcome;bool alias=false;const bool ok=controller.enqueue({opcode,args,known},&outcome,&alias,&error);if(output)*output=outcome;if(alias_output)*alias_output=alias;return ok;}
    bool consume(){StoryGuideVoiceCommandEvent e;return controller.consume_one(&e,&error)&&e.consumed;}
    bool pump_and_drain(){if(!pcm.pump(mixer,&error))return false;sink.consume(4096);return pcm.observe_queue(&error);}
};
}
