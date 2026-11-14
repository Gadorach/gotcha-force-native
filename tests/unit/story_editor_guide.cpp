#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include "engine/gameplay/story_editor_guide.hpp"
#include "engine/assets/source_messages.hpp"
#include "engine/assets/hsd_record_animation.hpp"
namespace {
using namespace gf;unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"Story guide: %s\n",why);std::abort();}}
u32 hash(std::span<const u8> b){u32 h=2166136261;for(u8 x:b)h=(h^x)*16777619u;return h;}
ByteView view(const std::vector<u8>& b){return {b.data(),b.size()};}
std::vector<u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("guide fixture read");return {std::istreambuf_iterator<char>(f),{}};}
std::vector<u8> profile(std::vector<u8> commands={1,14,60,0,13,3,125,4,255,2,7}){
    std::vector<u8> b(36+commands.size());store_be32(b.data(),0x47464744);store_be16(b.data()+4,1);store_be16(b.data()+6,1);store_be32(b.data()+8,u32(commands.size()));store_be32(b.data()+12,11);store_be32(b.data()+16,22);b[24]=4;store_be32(b.data()+32,u32(commands.size()));std::copy(commands.begin(),commands.end(),b.begin()+36);store_be32(b.data()+20,hash(std::span<const u8>(b).subspan(24)));return b;
}
void rehash(std::vector<u8>& b){store_be32(b.data()+20,hash(std::span<const u8>(b).subspan(24)));}
void owned(){
    auto b=profile();StoryGuideBank bank;std::string error;check(bank.load(view(b),11,22,&error)&&bank.ready()&&error.empty()&&bank.programs().size()==1&&bank.fingerprint()==hash(b),"complete bank identity");
    StoryGuideCommand cmd;check(bank.command(4,0,&cmd)&&cmd.opcode==1&&cmd.operands.size()==2&&cmd.operands[0]==14&&cmd.operands[1]==60&&cmd.next_offset==3,"voice structural request only");
    check(bank.command(4,5,&cmd)&&cmd.opcode==3&&cmd.operands[0]==125&&cmd.next_offset==7,"wait operand");check(bank.command(4,7,&cmd)&&cmd.external_wait&&cmd.operands.empty()&&cmd.next_offset==8,"external selector0");check(bank.command(4,8,&cmd)&&cmd.opcode==255&&cmd.external_wait,"external selector251");
    check(bank.command(4,9,&cmd)&&cmd.terminal&&cmd.operands[0]==7&&cmd.next_offset==10&&!bank.command(4,10,&cmd)&&cmd.operands.empty(),"terminal reads operand but advances one; operand NOT cursor");
    StoryGuideReference ref{bank.fingerprint(),0,4},decoded{77,88,9};std::array<u8,16> wire{};check(story_guide_reference_encode(ref,bank,wire)&&story_guide_reference_decode({wire.data(),wire.size()},bank,&decoded)&&decoded==ref,"logical16B roundtrip");
    const auto kept=decoded;wire[7]=1;check(!story_guide_reference_decode({wire.data(),wire.size()},bank,&decoded)&&decoded==kept,"reserved wire failure no publication");wire[7]=0;store_be32(wire.data()+12,0x8035b1d4);check(!story_guide_reference_decode({wire.data(),wire.size()},bank,&decoded),"guest address not a cursor");
    auto foreign=b;store_be32(foreign.data()+12,12);StoryGuideBank other;check(other.load(view(foreign),12,22)&&other.fingerprint()!=bank.fingerprint()&&!story_guide_reference_valid(ref,other),"same bytes different panel not same bank");foreign=b;store_be32(foreign.data()+16,23);check(other.load(view(foreign),11,23)&&!story_guide_reference_valid(ref,other),"same bytes different text not same bank");
    check(!bank.load(view(b),12,22,&error)&&!bank.ready()&&bank.programs().empty()&&!story_guide_reference_valid(ref,bank),"binding refusal clears all bank state");
    for(std::size_t n=0;n<b.size();++n){check(!bank.load({b.data(),n},11,22)&&!bank.ready()&&bank.programs().empty(),"every truncated extent refuses");}
    for(unsigned which=0;which<12;++which){auto bad=b;
        if(which==0)bad[0]^=1;
        if(which==1)bad[5]=2;
        if(which==2)bad[7]=0;
        if(which==3)bad[24]=16;
        if(which==4)bad[25]=1;
        if(which==5)bad[27]=1;
        if(which==6)store_be32(bad.data()+28,1);
        if(which==7)store_be32(bad.data()+32,257);
        if(which==8)bad[45]=1; //terminal becomes truncated voice
        if(which==9)bad[36]=2; //terminal before suffix
        if(which==10)store_be32(bad.data()+8,999);
        if(which==11)bad[20]^=1;
        if(which!=11)rehash(bad);
        check(!bank.load(view(bad),11,22)&&bank.programs().empty()&&!bank.ready(),"malformed bank refuses and clears");
    }
    check(bank.load(view(b),11,22),"restore owned bank");const auto original=bank.programs()[0].bytes;b[36]=255;check(bank.programs()[0].bytes==original,"immutable input ownership");
    StoryEditorGuideState s;s.source54=77;s.selector84=9;s.editor148=8;s.counter8=-123;StoryEditorGuidePlan p;check(story_editor_guide_plan(s,{1,0},&p)&&p.state==s&&!p.request_popup0&&!p.request_text_bank1,"local editor readiness not fabricated");
    check(story_editor_guide_plan(s,{0,1},&p)&&p.state.phase2==1&&p.state.editor148==1&&p.state.counter8==-123&&p.request_panel_bank1&&p.request_text_bank1&&!p.request_popup0,"resource requests before primary readiness");
    s=p.state;check(story_editor_guide_plan(s,{1,0},&p)&&p.state.phase2==2&&p.state.counter8==39&&!p.request_text_bank1,"primary readiness loads40 then immediately decrements39");
    s=p.state;for(unsigned i=0;i<39;++i){check(story_editor_guide_plan(s,{},&p)&&p.request_popup0==(i==38),"fortieth countdown callback only");s=p.state;}
    check(s.phase2==3&&s.counter8==0&&s.selector84==4&&s.source54==77&&story_editor_guide_plan(s,{},&p)&&p.state==s&&!p.request_popup0,"one popup request with source54 retained");
    s.phase2=2;s.counter8=std::bit_cast<s16>(u16(0x8000));check(story_editor_guide_plan(s,{},&p)&&p.state.counter8==32767&&p.state.phase2==2&&!p.request_popup0,"signed16 STORE wraps before compare");
    s.completed68=1;check(story_editor_guide_plan(s,{1,1},&p)&&p.state==s&&!p.request_text_bank1,"completed bypass");s.completed68=0;s.source54=0;check(story_editor_guide_plan(s,{1,1},&p)&&p.state==s,"zero54 bypass");check(!story_editor_guide_plan(s,{},nullptr),"null output refuses");
    std::printf("Story guide owned: %u checks PASS; structural/declared only\n",checks);
}
struct Writer {FILE* f=nullptr;explicit Writer(const std::string& p){f=std::fopen(p.c_str(),"wx");if(!f)throw std::runtime_error("exclusive guide wire");}void bytes(std::span<const u8> b){if(std::fwrite(b.data(),1,b.size(),f)!=b.size())throw std::runtime_error("guide wire write");}void word(u32 x){u8 b[4];store_be32(b,x);bytes(b);}void close(){if(std::fclose(f))throw std::runtime_error("guide wire close");f=nullptr;}};
void emit(Writer& w,const StoryEditorGuideState& s,const StoryEditorGuideObservation& o){StoryEditorGuidePlan p;if(!story_editor_guide_plan(s,o,&p))throw std::runtime_error("guide scalar plan");for(u32 x:{p.state.completed68,p.state.source54,p.state.selector84,u32(p.state.phase2),u32(p.state.editor148),u32(u16(p.state.counter8)),u32(p.request_text_bank1),u32(p.request_panel_bank1),u32(p.request_popup0)})w.word(x);}
void corpus(const std::string& base){Writer w(base+".states");unsigned count=0;
    for(unsigned bits=0;bits<65536;++bits){StoryEditorGuideState s;s.source54=77;s.phase2=2;s.counter8=std::bit_cast<s16>(u16(bits));s.selector84=123;s.editor148=8;emit(w,s,{});++count;}
    for(unsigned phase=0;phase<256;++phase)for(unsigned completed:{0u,2u})for(unsigned opaque:{0u,77u})for(s16 primary:{s16(-1),s16(0),s16(1),s16(2)})for(s16 local:{s16(-1),s16(0),s16(1),s16(2)})for(s16 counter:{s16(-32768),s16(-1),s16(0),s16(1),s16(40),s16(32767)}){StoryEditorGuideState s;s.phase2=u8(phase);s.completed68=completed;s.source54=opaque;s.selector84=123;s.editor148=8;s.counter8=counter;emit(w,s,{primary,local});++count;}
    w.close();std::printf("Story guide declared states: %u records\n",count);
}
void source(const std::string& directory,const std::string& base){
    auto programs=read(directory+"/guide-programs.gfgd"),panel=read(directory+"/fmg00_mdl.arc"),text=read(directory+"/guide-text.gftx"),fp=read(directory+"/guide-font.gffn"),atlas=read(directory+"/guide-font.tpl");std::string error;
    SourceFont font;SourceMessageBank messages;StoryGuideBank bank;check(font.load(view(fp),view(atlas),&error)&&font.codes().size()==89&&messages.load(view(text),font,&error)&&messages.groups()==1&&messages.message(0,0)&&messages.message(0,1)&&!messages.message(0,2),"actual89 glyphs and compact two messages through shared readers");
    check(bank.load(view(programs),hash(panel),messages.hash(),&error)&&bank.programs().size()==5,"actual5 byte streams with panel/text binding");
    Writer commands(base+".commands");unsigned accepted=0,refused=0;for(const auto& p:bank.programs())for(u32 at=0;at<=p.bytes.size()+1;++at){StoryGuideCommand c;const bool ok=bank.command(p.slot,at,&c);for(u32 x:{u32(p.slot),at,u32(ok),u32(c.opcode),c.next_offset,u32(c.terminal),u32(c.external_wait),u32(c.operands.size()),u32(c.operands.size()>0?c.operands[0]:0),u32(c.operands.size()>1?c.operands[1]:0)})commands.word(x);
        StoryGuideReference ref{bank.fingerprint(),at,p.slot},decoded;std::array<u8,16> b{};if(ok){check(story_guide_reference_encode(ref,bank,b)&&story_guide_reference_decode({b.data(),b.size()},bank,&decoded)&&decoded==ref,"actual boundary reference");commands.bytes(b);++accepted;}else{check(!story_guide_reference_valid(ref,bank)&&!story_guide_reference_encode(ref,bank,b),"actual operand/nonboundary reference refuses");commands.bytes(b);++refused;}}
    commands.close();Writer glyphs(base+".glyphs");for(auto code:font.codes())glyphs.word(code);glyphs.close();Writer rgba(base+".rgba");rgba.bytes(font.image().rgba);rgba.close();Writer texts(base+".text");for(unsigned i=0;i<2;++i){const auto* lines=messages.message(0,i);texts.word(u32(lines->size()));for(const auto& line:*lines){texts.word(u32(line.size()));texts.bytes({reinterpret_cast<const u8*>(line.data()),line.size()});}}texts.close();
    HsdRecordAnimationBank panels;check(panels.load(view(panel),&error),"actual panel archive bounded shared metadata reader");HsdScene scene;check(hsd_load(view(panel),&scene,&error)&&scene.model_roots.size()==3,"all3 published panel model roots");Writer metadata(base+".panel");metadata.word(u32(scene.model_roots.size()));metadata.word(u32(scene.joints.size()));metadata.word(u32(scene.meshes.size()));for(auto root:scene.model_roots)metadata.word(root);metadata.close();
    std::printf("Story guide source: %u boundary refs / %u operand refusals; 89 glyphs, 2 messages, 3 panel records; no VM/popup/voice ACK\n",accepted,refused);
}
}
int main(int argc,char** argv){owned();if(argc==3){corpus(argv[2]);source(argv[1],argv[2]);}else if(argc!=1)return 2;return 0;}
