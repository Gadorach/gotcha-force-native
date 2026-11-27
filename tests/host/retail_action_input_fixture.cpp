// Original processed pad packet -> conditioned requests -> action arbiter.
// The scene, actor flags, and missing services are fixture inlets, not a match.
#include "retail_recomp_fixture.hpp"
#include <cstdint>

namespace {
using namespace retail_fixture;
constexpr u32 packet=0x803C727C, arbiter=0x800699D8;
constexpr u32 config=0x81014000, spec=0x81015000, contexts=0x81016000, rows=0x81016100,
              pairs=0x81016200, settings=0x81017000, mode_state=0x81019200, peer=0x8101A000;
constexpr u32 b=0x200, x=0x400, y=0x800;
struct Profile {u32 cfg,context,selector;};
constexpr Profile synthetic{config,contexts,pairs};
constexpr Profile red{0x8030A130,0x8030A380,0x8030A248}; // Machine Red, p076
constexpr Profile sasuke{0x802D3DC8,0x802D4148,0x802D3FF8}; // Sasuke, p012
constexpr Profile black[]{
    {0x80365AC8,0x80365EA8,0x80365CF8}, // G-Black, p088
    {0x80365BE0,0x80365EA8,0x80365CF8}, // G-Black, p089
};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 va){return ram.data()+va-GC_RAM_BASE;}
    u32 call(u32 entry){
        CPUState cpu{};
        cpu.gpr[3]=actor;
        mod.run(cpu,ram,entry);
        return cpu.gpr[3];
    }
    u32 relative_table(u32 root,u32 index){
        if(root<GC_RAM_BASE||root>=0x80440000||index>=128)fail("unsupported program table operand");
        const auto* p=at(root+2*index);
        const auto relative=std::bit_cast<int16_t>(u16(u16(p[0])<<8|p[1]));
        return root+u32(relative);
    }
public:
    u8* const a;
    Check(const Module& module,const std::vector<u8>& original,Profile p=synthetic)
        :mod(module),ram(original),a(at(actor)){
        put32(a+0x4B0,p.cfg);put32(a+0x4AC,spec);put32(a+0x4EC,p.context);put32(a+0x4F0,p.selector);
        put32(at(0x8043B5A0-31852),settings);put32(at(0x8043B5A0-31856),mode_state);
        for(unsigned i=0;i<6;++i)put32(at(0x803C4E84)+4*i,i?peer+0x1E00*(i-1):actor);
        for(unsigned g=0;g<6;++g){
            put32(at(contexts)+4*g,rows+g*0x20);
            for(unsigned choice=0;choice<6;++choice){
                auto* row=at(rows)+g*0x20+4*choice;
                row[0]=1;row[1]=3;row[2]=u8(g);row[3]=u8(choice);
            }
        }
        at(pairs)[2]=61;
        // The original stock check is inert with a zero configured channel.
        // Scene+69 must stay below 5 while polling a human packet; the sound
        // refusal is supplied only at the later, separately invoked arbiter.
        a[0x584]=255;
        at(config)[0xBB]=255;at(config)[0xBC]=255;at(config)[0xBD]=255;
        put_float(a+0x768,1.f);
    }
    void disable(unsigned group){at(config)[0xB8+group]=255;}
    void refuse_first_row(unsigned group){at(rows)[group*0x20]=255;}
    void install(u32 entry,u16 model,Profile p,u32 main){
        put16(a+0x3E8,model);
        put32(at(0x803C4D94),spec); // Declared role-0 resource; no model/scene load.
        call(entry);
        if(be32(a+0x4B0)!=p.cfg||be32(a+0x4EC)!=p.context||be32(a+0x4F0)!=p.selector||
           be32(a+0x4B4)!=main||be32(a+0x4AC)!=spec)fail("unexpected original installer writes");
        // Fixture inlet from config+2, normally published by constructor 8004BB14.
        a[0x579]=at(p.cfg)[2];
    }
    void pre_callback(){call(be32(a+0x4BC));}
    u32 frontier(CPUState& cpu){
        if(a[0x5DA]!=61||a[0x5DB]!=0)fail("expected admitted first slot and no-op second slot");
        pre_callback();
        cpu.gpr[3]=actor;
        return mod.run_to(cpu,ram,0x80065DFC,{0x8004BEB8,0x80088AA0,0x8003E0A8});
    }
    void request(u32 site,u32 group,u32 choice,u32 effective_mask,u32 program){
        CPUState cpu{};
        const auto reached=frontier(cpu);
        if(reached!=0x8004BEB8||cpu.lr!=site+4||cpu.gpr[3]!=actor||cpu.gpr[4]!=15||
           cpu.gpr[5]!=group||cpu.gpr[6]!=choice||cpu.gpr[7]!=0xFFFFFFFFu||cpu.gpr[8]!=0xFFFFFFFFu||
           (cpu.gpr[4]&a[0x579])!=effective_mask){
            std::fprintf(stderr,"request site=%08X mask=%u group=%u choice=%u r7=%08X r8=%08X effective=%u\n",
                         cpu.lr-4,cpu.gpr[4],cpu.gpr[5],cpu.gpr[6],cpu.gpr[7],cpu.gpr[8],cpu.gpr[4]&a[0x579]);
            fail("first program request mismatch");
        }
        // Qualify the local bank address from the observed operands without
        // giving the cut-off original request a result or executing cues.
        const auto local=relative_table(be32(a+0x1D80),cpu.gpr[5]);
        if(relative_table(local,cpu.gpr[6])!=program)fail("first request's local program address mismatch");
    }
    void charge(unsigned channel,float limit=2.f){
        if(be32(a+0x4B0)==config)at(config)[0xBB+channel]=0;
        put_float(at(spec)+0x184+4*channel,limit);
        // Declare an already-bound ancillary descriptor, so the original
        // same-sample path updates its existing slot without starting audio.
        put32(at(0x803D3B80),be32(at(0x8031B698)));
    }
    void input(u32 buttons,u32 held,u32 pressed,u32 released,u32 flags,const char* name){
        at(settings)[69]=0;
        put32(at(packet),buttons);
        call(0x80056900);
        call(0x800685C0);
        if(be32(a+0x5B4)!=held||be32(a+0x5BC)!=pressed||be32(a+0x5C0)!=released||
           be32(a+0x5D4)!=flags){
            std::fprintf(stderr,"%s: held=%X pressed=%X released=%X request=%X\n",
                         name,be32(a+0x5B4),be32(a+0x5BC),be32(a+0x5C0),be32(a+0x5D4));
            fail("processed packet or conditioned request mismatch");
        }
    }
    void expect(int group,u8 state,const char* name){
        at(settings)[69]=5;
        const u32 result=call(arbiter);
        if(result!=u32(group>=0)||(group>=0&&(a[0x584]!=group||a[0x580]!=state||
                                             a[0x5DA]!=61||a[0x5DB]!=0))){
            std::fprintf(stderr,"%s: arbiter=%u group=%u state=%u slots=%u:%u\n",
                         name,result,a[0x584],a[0x580],a[0x5DA],a[0x5DB]);
            fail("action arbitration mismatch");
        }
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: action-input-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    {
        Check c(mod,original,red);
        c.input(0,0,0,0,0,"neutral");
        c.expect(-1,0,"neutral Machine Red");
        c.input(b,b,b,0,0x60,"fresh B");
        c.expect(0,0,"Machine Red actual ranged B row");
        c.input(b,b,0,0,0,"held B");
        c.expect(-1,0,"held B does not start another ordinary move");
        c.input(0,0,0,b,0,"ordinary B release");
        c.expect(-1,0,"ordinary B release does not start an action");
    }
    {
        Check c(mod,original,sasuke);
        c.a[0x7D2]=8;
        c.input(b,b,b,0,0x60,"fresh B with close-admit flag");
        c.expect(1,1,"Sasuke actual close-B row");
    }
    {
        Check c(mod,original,red);
        c.input(b|x,b|x,b|x,0,0xE0,"fresh B plus X");
        c.expect(2,2,"Machine Red selects X over eligible B");
    }
    {
        Check c(mod,original);
        c.refuse_first_row(2);
        c.input(b|x,b|x,b|x,0,0xE0,"same press with refused X row");
        c.expect(0,0,"refused X falls through to B");
    }
    {
        Check c(mod,original);
        c.a[0x58E]=4;
        c.input(b|x,b|x,b|x,0,0xE0,"same press with actor X inhibit");
        c.expect(0,0,"actor-inhibited X falls through to B");
    }
    {
        Check c(mod,original);
        c.a[0x7D2]=8;
        c.disable(1);
        c.input(b,b,b,0,0x60,"same B with close mode refused");
        c.expect(0,0,"refused close branch falls through to ranged B");
    }
    {
        Check c(mod,original);
        c.input(y,y,y,0,0x200,"fresh Y");
        c.expect(-1,0,"Y flag is not a row request in this arbiter");
    }
    {
        Check c(mod,original);
        c.a[0x488]=3;
        c.input(b|x,b|x,0,0,0,"source switch suppresses edges");
        c.expect(-1,0,"source switch cannot manufacture a fresh attack");
    }
    for(auto channel:{0u,2u}){
        Check c(mod,original);
        c.charge(channel);
        const u32 button=channel==0?b:x;
        const u32 fresh=channel==0?0x60:0x80;
        const u32 release=channel==0?0x400:0x1000;
        c.input(button,button,button,0,fresh,"short charge press");
        c.input(0,0,0,button,0,"release before charge threshold");
        c.expect(-1,0,"short release cannot start charged action");
        c.input(button,button,button,0,fresh,"start charge hold");
        for(unsigned i=0;i<14;++i)c.input(button,button,0,0,0,"continue charge hold");
        // Combine a fresh opposing button with release on the same update.
        const u32 other=channel==0?x:b;
        c.input(other,other,other,button,release|(channel==0?0x80:0x60),"charged release plus fresh press");
        c.expect(channel==0?3:5,channel==0?3:5,"mature charged release wins over plain attack");
    }
    for(unsigned profile=0;profile<2;++profile){
        Check c(mod,original,black[profile]);
        // Limits extracted from the installer-selected role-0 record.
        // Keep the actor idle deliberately; this is not a live cancel path.
        c.charge(0,60.f);c.charge(2,120.f);
        c.input(b|x,b|x,b|x,0,0xE0,"G-Black fresh B and X");
        for(unsigned i=0;i<140;++i)c.input(b|x,b|x,0,0,0,"G-Black simultaneous hold");
        c.input(0,0,0,b|x,profile?0x1400:0x400,"G-Black two-profile release");
        c.expect(profile?5:3,profile?4:3,"G-Black uses each profile's original charged row");
        c.input(0,0,0,0,0,"no repeated charge pulse after release");
    }
    {
        Check c(mod,original);
        c.install(0x800C91BC,0x0600,red,0x800C94D4);
        c.a[0x6EE]=5;
        c.pre_callback();
        if(c.a[0x6EE])fail("Machine Red neutral pre-callback did not seed choice");
        c.input(b,b,b,0,0x60,"installed Machine Red B");
        c.expect(0,0,"installed Machine Red B row");
        c.request(0x800C9634,2,0,7,0x8030903E);
    }
    {
        constexpr Profile installed_sasuke{0x802D3EE0,0x802D4148,0x802D3FF8};
        Check c(mod,original);
        c.install(0x8006F4F8,0x000A,installed_sasuke,0x8006FACC);
        c.a[0x7D2]=8;
        c.input(b,b,b,0,0x60,"installed Sasuke close B");
        c.expect(1,1,"installed Sasuke close row");
        c.request(0x800FEEA4,3,1,3,0x802BE3B6);
    }
    for(const bool use_x:{false,true}){
        Check c(mod,original);
        // The real model-gated installer selects p089 for model 062A.
        c.install(0x8018CCFC,0x062A,black[1],0x8018D24C);
        c.pre_callback();
        c.charge(0,60.f);c.charge(2,120.f);
        const u32 button=use_x?b|x:b;
        c.input(button,button,button,0,use_x?0xE0:0x60,"installed G-Black fresh press");
        for(unsigned i=0;i<140;++i)c.input(button,button,0,0,0,"installed G-Black hold");
        c.input(0,0,0,button,use_x?0x1400:0x400,"installed G-Black release");
        c.expect(use_x?5:3,use_x?4:3,"installed G-Black charged row");
        CPUState cpu{};
        const auto reached=c.frontier(cpu);
        if(cpu.gpr[3]!=actor||
           (use_x?(reached!=0x8003E0A8||cpu.lr!=0x800677F4||cpu.gpr[4]!=actor+0x20):
                  (reached!=0x80088AA0||cpu.lr!=0x8016CC54||cpu.gpr[4]!=3||cpu.gpr[5]!=8||
                   cpu.gpr[6]!=0||cpu.gpr[7]!=1)))fail("G-Black did not reach its expected pre-request service");
    }
    std::puts("action input: 12 scoped sequences, 2 program requests and 2 pre-request boundaries passed");
}
