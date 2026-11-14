#include <cstdio>
#include <cstdlib>
#include <limits>
#include "tests/unit/story_narrative_fixture.hpp"
namespace {unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"Story narrative reference: %s\n",why);std::abort();}}}
int main(){using namespace gf;auto bytes=story_narrative_fixture::bank();StoryNarrativeBank bank;std::string error;
    check(bank.load({bytes.data(),bytes.size()},123,&error)&&bank.ready()&&bank.scripts().size()==3,"owned three logical roots");
    const StoryNarrativeKey key{StoryNarrativeOwner::Event,1,0};const auto* script=bank.find(key);check(script&&script->boundaries==std::vector<u32>{0,2,5,7},"all exact opcode boundaries");
    std::array<u8,20> encoded{};StoryNarrativeReference ref,decoded;
    check(story_narrative_reference_valid(ref,bank)&&story_narrative_reference_encode(ref,bank,encoded)&&story_narrative_reference_decode_declared({encoded.data(),encoded.size()},bank,&decoded)&&decoded==ref,"canonical inactive reference round-trip");
    for(u32 pc:{0u,2u,5u,7u}){StoryNarrativeCommand command;
        check(story_narrative_reference_from_declared(bank,key,pc,&ref)&&bank.command(ref.key,ref.word_offset,&command),"logical active lookup");
        check(command.terminal==(pc==7),"terminal boundary is structural, not VM completion");
        check(story_narrative_reference_encode(ref,bank,encoded)&&story_narrative_reference_decode_declared({encoded.data(),encoded.size()},bank,&decoded)&&ref==decoded,"active reference wire round-trip");
    }
    for(u32 pc:{1u,3u,4u,6u,8u,0x80353388u,std::numeric_limits<u32>::max()}){const auto before=ref;
        check(!story_narrative_reference_from_declared(bank,key,pc,&ref)&&ref==before,"operand/past-end/guest-address cursor rejected");}
    for(auto broken:{StoryNarrativeKey{StoryNarrativeOwner::Event,0,0},StoryNarrativeKey{StoryNarrativeOwner::Event,1,3},StoryNarrativeKey{StoryNarrativeOwner::Event,2,0},StoryNarrativeKey{StoryNarrativeOwner::Gate,1,1},StoryNarrativeKey{StoryNarrativeOwner(2),1,0}}){const auto before=ref;check(!story_narrative_reference_from_declared(bank,broken,0,&ref)&&ref==before,"unknown logical identity");}
    auto foreign=bytes;foreign[62]^=1;story_narrative_fixture::fingerprint(foreign);StoryNarrativeBank other;
    check(other.load({foreign.data(),foreign.size()},123,&error)&&other.fingerprint()!=bank.fingerprint(),"different bank same keys");
    check(!story_narrative_reference_decode_declared({encoded.data(),encoded.size()},other,&decoded)&&decoded==ref,"foreign bank refuses old reference without overwrite");
    auto wire=encoded;wire[7]=1;check(!story_narrative_reference_decode_declared({wire.data(),wire.size()},bank,&decoded),"reserved wire byte");
    wire=encoded;wire[6]=2;check(!story_narrative_reference_decode_declared({wire.data(),wire.size()},bank,&decoded),"bad active flag");
    wire=encoded;store_be32(wire.data()+16,1);check(!story_narrative_reference_decode_declared({wire.data(),wire.size()},bank,&decoded),"wire operand cursor");
    check(!story_narrative_reference_decode_declared({encoded.data(),encoded.size()-1},bank,&decoded),"short wire");
    check(!story_narrative_reference_from_declared(bank,key,0,nullptr),"no reference output");
    StoryNarrativeCommand command;check(!bank.command(key,1,&command)&&command.operands.empty(),"failed command no partial payload");
    for(unsigned opcode=0;opcode<256;++opcode){u8 count=255;const bool known=opcode>=0x14&&opcode<=0x2e;check(story_narrative_operands(u8(opcode),&count)==known,"bounded dialect opcode domain");}
    auto bad=bytes;bad[61]=0x13;story_narrative_fixture::fingerprint(bad);check(!other.load({bad.data(),bad.size()},123,&error)&&!other.ready(),"unsupported opcode");
    bad=bytes;bad[60]=2;story_narrative_fixture::fingerprint(bad);check(!other.load({bad.data(),bad.size()},123,&error),"header arity mismatch");
    bad=bytes;bad[75]=0x14;story_narrative_fixture::fingerprint(bad);check(!other.load({bad.data(),bad.size()},123,&error),"missing terminal");
    bad=bytes;bad[61]=0x2e;bad[60]=0;story_narrative_fixture::fingerprint(bad);check(!other.load({bad.data(),bad.size()},123,&error),"bytes after early terminal");
    bad=bytes;bad[26]=1;story_narrative_fixture::fingerprint(bad);check(!other.load({bad.data(),bad.size()},123,&error),"owner ID outside domain");
    bad=bytes;bad[37]=0;story_narrative_fixture::fingerprint(bad);check(!other.load({bad.data(),bad.size()},123,&error),"duplicate owner identity");
    bad=bytes;store_be32(bad.data()+28,2);story_narrative_fixture::fingerprint(bad);check(!other.load({bad.data(),bad.size()},123,&error),"noncontiguous script segment");
    bad=bytes;bad.back()^=1;check(!other.load({bad.data(),bad.size()},123,&error),"payload integrity");
    check(!other.load({bytes.data(),bytes.size()},124,&error),"foreign map");
    check(!other.load({nullptr,bytes.size()},123,&error),"missing bank data");
    std::printf("Story logical narrative references: %u owned checks PASS; no guest-pointer restore/VM/resource/voice ACK\n",checks);
}
