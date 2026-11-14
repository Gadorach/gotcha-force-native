#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/story_editor_guide.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"Story guide edge: %s\n",why);std::abort();}}
u32 hash(std::span<const u8> b){u32 h=2166136261;for(auto x:b)h=(h^x)*16777619u;return h;}
void rehash(std::vector<u8>& b){store_be32(b.data()+20,hash(std::span<const u8>(b).subspan(24)));}
std::vector<u8> bank_bytes(){std::vector<u8> b(52);store_be32(b.data(),0x47464744);store_be16(b.data()+4,1);store_be16(b.data()+6,2);store_be32(b.data()+8,4);store_be32(b.data()+12,11);store_be32(b.data()+16,22);b[24]=0;b[36]=4;store_be32(b.data()+32,2);store_be32(b.data()+40,2);store_be32(b.data()+44,2);b[48]=b[50]=2;rehash(b);return b;}
}
int main(){using namespace gf;auto b=bank_bytes();StoryGuideBank bank;StoryGuideCommand c;check(bank.load({b.data(),b.size()},11,22)&&bank.command(0,0,&c)&&bank.command(4,0,&c),"two canonical programs");const StoryGuideReference ref{bank.fingerprint(),0,4};
    for(unsigned edge=0;edge<8;++edge){auto bad=b;
        if(edge==0)bad[36]=0; //duplicate after first accepted program
        if(edge==1)bad[37]=1;
        if(edge==2)bad[50]=1;
        if(edge==3)store_be32(bad.data()+40,1);
        if(edge==4)store_be32(bad.data()+44,0);
        if(edge==5)store_be32(bad.data()+44,0xffffffff);
        if(edge==6)store_be32(bad.data()+40,0xffffffff);
        if(edge==7)bad[50]=255;
        rehash(bad);check(!bank.load({bad.data(),bad.size()},11,22)&&!bank.ready()&&bank.programs().empty()&&!bank.find(0)&&!story_guide_reference_valid(ref,bank),"late second-program failure clears prior candidates");
    }
    check(bank.load({b.data(),b.size()},11,22),"restore");std::array<u8,16> bytes{};check(story_guide_reference_encode(ref,bank,bytes),"valid encoding");
    for(unsigned offset:{0u,4u,6u,7u,8u,12u}){auto corrupt=bytes;corrupt[offset]^=1;StoryGuideReference out{7,8,9},keep=out;check(!story_guide_reference_decode({corrupt.data(),corrupt.size()},bank,&out)&&out==keep,"wire schema/binding/boundary/slot corruption no partial publication");}
    for(unsigned n=0;n<16;++n){auto before=bytes;check(!story_guide_reference_encode(ref,bank,std::span<u8>(bytes).first(n))&&bytes==before,"bad encode length retains destination");StoryGuideReference out{7,8,9},keep=out;check(!story_guide_reference_decode({bytes.data(),n},bank,&out)&&out==keep,"bad decode length retains output");}
    check(!story_guide_reference_decode({nullptr,16},bank,nullptr)&&!bank.command(0,0,nullptr),"null requests");
    StoryEditorGuideState initial;initial.source54=77;initial.selector84=123;initial.counter8=42;StoryEditorGuidePlan p;check(story_editor_guide_plan(initial,{1,1},&p)&&p.request_text_bank1&&p.request_panel_bank1&&p.state.counter8==39&&p.state.phase2==2&&!p.request_popup0,"both readiness observations same call still one immediate countdown");
    auto s=initial;s.phase2=255;check(story_editor_guide_plan(s,{1,1},&p)&&p.state==s&&!p.request_text_bank1,"negative source char phase inert");
    std::printf("Story guide edges: %u checks PASS; no readiness/voice ACK\n",checks);return 0;
}
