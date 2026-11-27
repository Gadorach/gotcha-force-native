// Original damage-input production before actor construction or stage selection.
#include "retail_recomp_fixture.hpp"
#include <array>
#include <bit>

namespace {
using namespace retail_fixture;
constexpr u32 mode=0x81018000,scene=0x81019000,task=0x8101C000,config=0x81022000,
              settings=0x803C4E60;

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* ptr(u32 va){return ram.data()+va-GC_RAM_BASE;}
    u16 half(u32 va){const auto* b=ptr(va);return u16(u16(b[0])<<8|b[1]);}
    void run(u32 entry,unsigned budget=256){
        CPUState cpu{};
        mod.run(cpu,ram,entry,budget);
    }
    u8 ordinary_bank(unsigned slot,u8 team){
        auto* a=ptr(actor);
        a[0x3E4]=u8(slot);a[0x3E5]=u8(1u<<slot);a[0x88]=team;
        a[0x3E6]=255;
        CPUState bank{};
        bank.gpr[31]=actor;
        mod.run_to(bank,ram,0x8005438C,{0x80055F90});
        const u8 expected=(ptr(mode)[2]&(1u<<slot))==0;
        if(bank.lr!=0x80054418||a[0x3E6]!=expected||a[0x488]!=expected||
           a[0x3F3]!=unsigned(expected&&ptr(mode)[50]!=2)||a[0x88]!=team)
            fail("produced input mask did not flow to the ordinary constructor bank");
        return a[0x3E6];
    }
public:
    Check(const Module& module,const std::vector<u8>& original):mod(module),ram(original){
        put32(ptr(0x8043B5A0-31856),mode);
        put32(ptr(0x8043B5A0-31852),scene);
        put32(ptr(0x8043B5A0-31844),task);
        put32(ptr(0x8043B5A0-31836),config);
        put32(ptr(0x8043B5A0-31824),settings);
    }
    void automatic_setup(){
        // An isolated fresh byte stream, not a supplied draw or a claim
        // about the stream at any point in a live match.
        run(0x800055E0);
        ptr(mode)[2]=ptr(mode)[50]=255;
        for(unsigned slot=0;slot<6;++slot)ptr(scene)[62+slot]=255;
        for(unsigned i=0;i<4;++i)ptr(settings)[i]=255;
        run(0x80018494,4096);
        if(ptr(mode)[2]!=0||ptr(mode)[50]!=2||ptr(scene)[5593]!=0x0F||ptr(scene)[5594]!=4||
           ptr(scene)[5595]!=0x0F||ptr(scene)[5596]!=4)fail("automatic setup mode or active roster mismatch");
        for(unsigned slot=0;slot<4;++slot){
            if(ptr(settings)[slot]!=7||ptr(scene)[62+slot]||ptr(scene)[203+slot]!=(slot&1u)||
               half(scene+16+slot*2)==65535u)fail("automatic team, settings or rank data mismatch");
            // Feed the produced mask and rank bytes into the original
            // writer slices using this declared ordinary actor slot.
            if(ordinary_bank(slot,ptr(scene)[203+slot])!=1)fail("automatic bank should be 1");
            CPUState rank{};rank.gpr[3]=actor;
            // Rank slice reads this pointer even though the checked
            // rank byte is from the Scene. The scratch spec stays zero.
            put32(ptr(actor+0x4AC),0x81014000);ptr(actor)[0x43A]=255;
            mod.run_to(rank,ram,0x80056180,{0x8006826C});
            if(rank.lr!=0x8005621C||ptr(actor)[0x43A]!=0)fail("automatic setup did not seed rank zero");
        }
        std::printf("automatic: %u %u %u %u %u %u %u %u\n",ptr(mode)[50],ptr(mode)[2],
                    ptr(scene)[5593],ptr(scene)[5594],ptr(settings)[0],ptr(settings)[1],
                    ptr(scene)[62],ptr(actor)[0x3E6]);
    }
    void selected_slot(unsigned slot,unsigned port,bool request_enabled){
        ptr(mode)[2]=5; // independent input mask; this slice must not rewrite it.
        ptr(scene)[5593]=0x20;ptr(scene)[5594]=1;
        for(unsigned i=0;i<4;++i){
            ptr(config)[40+i]=u8(i);
            ptr(config)[32+i]=0x40;
            ptr(scene)[62+i]=7;
        }
        ptr(config)[40+slot]=u8(port);
        ptr(config)[32+port]=request_enabled?0:0x40;
        ptr(config)[256+slot*128+97]=0xFF; // no custom selection; ordinary fallback arm
        CPUState cpu{};
        // Declared loop index/strides; do not recreate the front-end that
        // produced the four port entries or let the next random draw run.
        cpu.gpr[22]=slot;cpu.gpr[15]=slot*128;cpu.gpr[16]=slot*2;
        cpu.gpr[23]=slot*41;cpu.gpr[24]=slot*840;
        mod.run_to(cpu,ram,0x80173680,{0x800055FC});
        const auto expected_mask=0x20u+(request_enabled?(1u<<slot):0);
        if(cpu.lr!=(request_enabled?0x80173A08u:0x80173DB4u)||ptr(mode)[2]!=5||
           ptr(scene)[5593]!=expected_mask||ptr(scene)[5594]!=unsigned(request_enabled?2:1)||
           ptr(scene)[62+slot]!=unsigned(request_enabled?3:7))
            fail("selected-slot producer path, active mask or rank mismatch");
        const auto team=ptr(scene)[203+slot];
        if(request_enabled&&(team!=(port&1u)||ptr(scene)[32+slot]!=slot||ptr(scene)[50+slot]!=4))
            fail("selected-slot producer lost its slot or team");
        std::printf("selected-slot: %u %u %u %u %u %u %u %u\n",slot,port,unsigned(request_enabled),
                    ptr(mode)[2],ptr(scene)[5593],ptr(scene)[5594],ptr(scene)[62+slot],team);
    }
    void port_enrollment(u8 current,u8 prior_mask,u16 wanted,const std::array<u16,4>& words,u8 pick){
        ptr(config)[20]=current;ptr(config)[25]=255;
        ptr(mode)[2]=prior_mask;ptr(mode)[3]=255;ptr(mode)[50]=2;
        for(unsigned i=0;i<words.size();++i)put16(ptr(config+270+i*128),words[i]);
        CPUState cpu{};cpu.gpr[3]=wanted;
        mod.run(cpu,ram,0x8019052C);
        const u8 expected=pick==255?prior_mask:u8(prior_mask|(1u<<pick));
        if(ptr(config)[25]!=pick||ptr(config)[20]!=(pick==255?current:pick)||ptr(mode)[2]!=expected||
           ptr(mode)[3]!=std::popcount(unsigned(expected&15))||ptr(mode)[50]!=2)
            fail("original port search changed the wrong slot, mask or count");
        unsigned banks=0;
        for(unsigned slot=0;slot<4;++slot)banks|=unsigned(ordinary_bank(slot,u8(slot&1u)))<<slot;
        std::printf("ports: %u %u %u %u %u %u %u\n",current,prior_mask,wanted,ptr(config)[25],
                    ptr(mode)[2],ptr(mode)[3],banks);
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: scene-damage-inputs qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    {
        Check c(mod,original);c.automatic_setup();
    }
    for(const unsigned slot:{0u,3u})for(const unsigned port:{2u,3u})for(const bool enabled:{false,true}){
        Check c(mod,original);c.selected_slot(slot,port,enabled);
    }
    struct Ports {u8 current,mask;u16 wanted;std::array<u16,4> flags;u8 picked;};
    for(const auto p:{Ports{0,1,2,{2,2,2,0},1},Ports{1,3,2,{0,2,2,0},2},
                      Ports{1,3,4,{0,2,2,0},255},Ports{1,3,2,{0,2,0,0},255},
                      Ports{2,0x84,2,{0,2,0,0},1},Ports{2,7,2,{0,2,2,0},1}}){
        Check c(mod,original);c.port_enrollment(p.current,p.mask,p.wanted,p.flags,p.picked);
    }
}
