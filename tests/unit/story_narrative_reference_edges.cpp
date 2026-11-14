#include <cstdio>
#include <cstdlib>
#include "tests/unit/story_narrative_fixture.hpp"
namespace {unsigned checks=0;void check(bool x){++checks;if(!x)std::abort();}}
int main(){using namespace gf;
    const std::array<u16,9> words{0x0227,0x615,1,0x0328,0x615,0,0,0x0029,0x002e};
    std::vector<u8> bytes(36+words.size()*2);store_be32(bytes.data(),0x47465342);store_be16(bytes.data()+4,1);store_be16(bytes.data()+6,1);store_be32(bytes.data()+8,u32(words.size()*2));store_be32(bytes.data()+12,123);store_be16(bytes.data()+26,1);store_be32(bytes.data()+32,u32(words.size()*2));
    for(unsigned i=0;i<words.size();++i)store_be16(bytes.data()+36+i*2,words[i]);
    story_narrative_fixture::fingerprint(bytes);
    StoryNarrativeBank bank;std::string error;check(bank.load({bytes.data(),bytes.size()},123,&error));
    const StoryNarrativeKey key{StoryNarrativeOwner::Event,1,0};check(bank.find(key)->boundaries==std::vector<u32>{0,3,7,8});
    for(auto [pc,opcode,count]:std::array<std::array<unsigned,3>,4>{{{0,0x27,2},{3,0x28,3},{7,0x29,0},{8,0x2e,0}}}){
        StoryNarrativeCommand command;check(bank.command(key,pc,&command)&&command.opcode==opcode&&command.operands.size()==count);u8 n=99;check(story_narrative_operands(u8(opcode),&n)&&n==count);
        StoryNarrativeReference ref,decoded;std::array<u8,20> wire{};check(story_narrative_reference_from_declared(bank,key,pc,&ref));check(story_narrative_reference_encode(ref,bank,wire));check(story_narrative_reference_decode_declared({wire.data(),wire.size()},bank,&decoded)&&decoded==ref);
    }
    StoryNarrativeReference ref;check(story_narrative_reference_from_declared(bank,key,3,&ref));auto before=ref;std::array<u8,20> wire{};check(story_narrative_reference_encode(ref,bank,wire));const auto old=wire;
    check(!story_narrative_reference_encode(ref,bank,std::span<u8>(wire.data(),19))&&wire==old);
    check(!story_narrative_reference_decode_declared({nullptr,20},bank,&ref)&&ref==before);
    check(!story_narrative_reference_decode_declared({wire.data(),20},bank,nullptr));
    StoryNarrativeReference noncanonical;noncanonical.key.id=1;check(!story_narrative_reference_valid(noncanonical,bank));
    auto bad=bytes;store_be16(bad.data()+42,0x0228);story_narrative_fixture::fingerprint(bad);StoryNarrativeBank rejected;check(!rejected.load({bad.data(),bad.size()},123,&error)); //28 must have3 operands, not2
    bad=bytes;store_be16(bad.data()+36,0x0327);story_narrative_fixture::fingerprint(bad);check(!rejected.load({bad.data(),bad.size()},123,&error)); //27 must have2 operands
    bad=bytes;store_be16(bad.data()+50,0x0129);story_narrative_fixture::fingerprint(bad);check(!rejected.load({bad.data(),bad.size()},123,&error)); //29 must have0 operands
    bad=bytes;store_be16(bad.data()+6,1025);check(!rejected.load({bad.data(),bad.size()},123,&error));
    check(!story_narrative_reference_decode_declared({wire.data(),20},rejected,&ref)&&ref==before);
    std::printf("Story narrative structural edges: %u NEW checks PASS; opcode28 owned-only/raw contract, not original VM execution\n",checks);
}
