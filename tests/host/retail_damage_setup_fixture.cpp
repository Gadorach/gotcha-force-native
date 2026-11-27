// Exercise the original writers of damage settings, banks, and ranks.
#include "retail_recomp_fixture.hpp"
#include <array>

namespace {
using namespace retail_fixture;
constexpr u32 receiver=0x8101A000,mode=0x81018000,scene=0x81019000,settings=0x803C4E60;
constexpr u32 spec_a=0x81014000,spec_b=0x81015000,amount_entry=0x8003CD5C;
using Spec=std::array<u8,432>;

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* ptr(u32 va){return ram.data()+va-GC_RAM_BASE;}
public:
    Check(const Module& module,const std::vector<u8>& original):mod(module),ram(original){
        put32(ptr(0x8043B5A0-31856),mode);
        put32(ptr(0x8043B5A0-31852),scene);
        put32(ptr(0x8043B5A0-31824),settings);
        for(unsigned i=0;i<5;++i)ptr(settings)[i]=u8(211+i);
        put32(ptr(actor+0x4AC),spec_a);put32(ptr(receiver+0x4AC),spec_b);
    }
    std::array<u8,4> setting_write(u8 state,int bias){
        ptr(mode)[50]=state;
        put16(ptr(scene+70),u16(bias));
        CPUState cpu{};
        mod.run(cpu,ram,0x80006ABC);
        if(ptr(settings)[4]!=215)fail("damage settings writer exceeded four cells");
        return {ptr(settings)[0],ptr(settings)[1],ptr(settings)[2],ptr(settings)[3]};
    }
    u8 controller_source(u32 who,unsigned slot,u8 mask,u8 state){
        ptr(mode)[2]=mask;ptr(mode)[50]=state;
        auto* a=ptr(who);
        a[0x3E4]=u8(slot);
        a[0x3E5]=u8(1u<<slot);
        a[0x3E6]=a[0x488]=a[0x3F3]=0xFFu;
        const auto team=a[0x88];
        CPUState cpu{};cpu.gpr[31]=who;
        // Enter the ordinary constructor at its controller selection
        // slice, with the slot bit already assigned. Do not run stock,
        // rig, receiver or other initialization after this boundary.
        mod.run_to(cpu,ram,0x8005438C,{0x80055F90});
        const u8 expected=(mask&(1u<<slot))==0;
        if(cpu.lr!=0x80054418||cpu.gpr[3]!=who||a[0x88]!=team||
           a[0x3E6]!=expected||a[0x488]!=expected||a[0x3F3]!=u8(expected&&state!=2))
            fail("original ordinary constructor controller selection mismatch");
        return a[0x3E6];
    }
    u8 rank_copy(u32 who,unsigned slot,u8 rank){
        ptr(who)[0x3E4]=u8(slot);
        ptr(who)[0x43A]=0xFF;
        // Give each slot a distinct value so an adjacent-byte read
        // cannot accidentally satisfy the isolated comparison.
        for(unsigned i=0;i<6;++i)ptr(scene)[62+i]=i==slot?rank:u8((rank+i+1)&7);
        CPUState cpu{};cpu.gpr[3]=who;
        mod.run_to(cpu,ram,0x80056180,{0x8006826C});
        if(cpu.lr!=0x8005621C||ptr(who)[0x43A]!=rank)
            fail("rank initializer did not copy the selected Scene slot byte");
        return ptr(who)[0x43A];
    }
    void pair(const Spec& spec){
        std::memcpy(ptr(spec_a),spec.data(),spec.size());
        std::memcpy(ptr(spec_b),spec.data(),spec.size());
        ptr(receiver)[0x88]=1;
        for(const u32 obj:{actor,receiver}){
            auto* fighter=ptr(obj);
            fighter[0]=fighter[0x18]=1;
            put16(fighter+0x3E8,0x000A);
            put32(fighter+0x90,obj);
            put_float(fighter+0xB4,1.f);put_float(fighter+0xC4,1.f);
            put16(fighter+0x1C4,180);put16(fighter+0x1C6,180);
        }
        put16(ptr(0x803B069C),180);put16(ptr(0x803B069C+2),180);
        for(unsigned team=0;team<2;++team){
            put32(ptr(0x803B068C+4*team),180);
            put32(ptr(scene+team*60+0x114),180);
            put32(ptr(scene+team*60+0x118),180);
        }
    }
    u32 amount(){
        CPUState cpu{};
        cpu.gpr[3]=0x802D4190;cpu.gpr[4]=actor;cpu.gpr[5]=receiver;
        mod.run(cpu,ram,amount_entry);
        if(ptr(actor)[0x88]!=0||ptr(receiver)[0x88]!=1)fail("source setup changed the declared teams");
        return cpu.gpr[3];
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: retail-damage-setup qualified.dol unmodified-recomp.so < spec");
    Spec spec{};
    if(std::fread(spec.data(),1,spec.size(),stdin)!=spec.size()||std::fgetc(stdin)!=EOF)
        fail("damage spec inlet extent");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    unsigned setting_checks=0,controller_checks=0,rank_checks=0;
    for(const int bias:{-32768,-1,0,1,7,16,31,32,32767}){
        Check c(mod,original);
        const std::array<u8,4> expected{u8(unsigned(bias)&31),u8(unsigned(31-bias)&31),213,214};
        if(c.setting_write(1,bias)!=expected)fail("mode 1 did not wrap only the first two settings");
        ++setting_checks;
    }
    for(const u8 state:{0,2,3,255}){
        Check c(mod,original);
        const auto expected=state==2?std::array<u8,4>{7,7,7,7}:std::array<u8,4>{211,212,213,214};
        if(c.setting_write(state,16)!=expected)fail("mode 2 or no-write settings branch mismatch");
        ++setting_checks;
    }
    for(const u8 state:{0,1,2,3}){
        for(const u8 mask:{0,1,0x25,0x3F}){
            Check c(mod,original);
            for(unsigned slot=0;slot<6;++slot){c.controller_source(actor,slot,mask,state);++controller_checks;}
        }
    }
    {
        Check c(mod,original);
        for(unsigned slot=0;slot<6;++slot)
            for(const u8 value:{0,3,7,9,255}){c.rank_copy(actor,slot,value);++rank_checks;}
    }
    std::printf("setup-checks: %u %u %u\n",setting_checks,controller_checks,rank_checks);
    struct Joined {u8 state,bias,mask,attack_rank,receive_rank,amount;};
    for(const auto setup:{Joined{2,0,3,0,0,35},Joined{2,0,1,0,0,40},
                         Joined{2,0,1,3,0,47},Joined{1,0,1,0,0,79}}){
        Check c(mod,original);c.pair(spec);
        const auto cells=c.setting_write(setup.state,setup.bias);
        const auto bank0=c.controller_source(actor,0,setup.mask,setup.state);
        const auto bank1=c.controller_source(receiver,1,setup.mask,setup.state);
        const auto rank0=c.rank_copy(actor,0,setup.attack_rank);
        const auto rank1=c.rank_copy(receiver,1,setup.receive_rank);
        const auto result=c.amount();
        if(result!=setup.amount)fail("original calculation disagrees with its produced setup rows");
        std::printf("setup-amount: %u %u %u %u %u %u %u %u %u %u\n",setup.state,setup.bias,setup.mask,
                    cells[0],cells[1],bank0,bank1,rank0,rank1,result);
    }
}
