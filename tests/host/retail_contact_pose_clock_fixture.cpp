// Original local body clock with declared rate modifiers and link states.
#include "retail_contact_pose_clock.hpp"

namespace {
using namespace retail_contact_pose_clock;
constexpr u32 leader=actor,follower=contact::receiver;

void output(std::vector<u8>& ram,u32 body,unsigned scenario,unsigned visit){
    const auto at=[&](u32 offset){return contact::at(ram,body+offset);};
    std::printf("pose-clock: %u %u %u %08X %08X %08X\n",scenario,visit,at(timer)[0],
                be32(at(base)),be32(at(primary)),be32(at(secondary)));
}

void ordinary(const Module& mod,std::vector<u8> ram,unsigned scenario,unsigned seed,bool boosted){
    factors(ram,leader);
    contact::at(ram,leader+0x6FC)[0]=u8(boosted);
    contact::at(ram,leader+timer)[0]=u8(seed);
    for(unsigned visit=1;visit<=seed+2;++visit){
        tick(mod,ram,leader);
        const auto remaining=visit<seed?seed-visit:0;
        const auto b=boosted?boost:unit,s=boosted?boosted_secondary:neutral_secondary;
        check_clocks(ram,leader,remaining,b,remaining?slow:b,remaining?slow:s);
        output(ram,leader,scenario,visit);
    }
}

void linked(const Module& mod,std::vector<u8> ram){
    constexpr unsigned scenario=4;
    const auto at=[&](u32 address){return contact::at(ram,address);};
    factors(ram,leader);factors(ram,follower);
    at(leader+0x6FC)[0]=1;
    tick(mod,ram,leader);
    at(leader+timer)[0]=4;
    at(follower+timer)[0]=7;
    at(follower+link_kind)[0]=2;
    put32(at(follower+link),leader);
    const auto sample=[&](unsigned n,unsigned count,u32 b,u32 p,u32 s){
        tick(mod,ram,follower);
        check_clocks(ram,follower,count,b,p,s);
        output(ram,follower,scenario,n);
    };
    sample(1,7,boost,boost,boosted_secondary); // Leader has not consumed its seed.
    tick(mod,ram,leader);
    check_clocks(ram,leader,3,boost,slow,slow);
    sample(2,7,boost,slow,slow);
    at(follower+override_flag)[0]=1;
    put32(at(follower+link),0);
    sample(3,7,unit,unit,unit); // This branch takes precedence over a link.
    at(follower+override_flag)[0]=0;
    put32(at(follower+link),leader);
    sample(4,7,boost,slow,slow);
    for(unsigned i=0;i<3;++i)tick(mod,ram,leader);
    check_clocks(ram,leader,0,boost,boost,boosted_secondary);
    sample(5,7,boost,boost,boosted_secondary);
    at(follower+link_kind)[0]=0;
    put32(at(follower+link),0);
    sample(6,6,unit,slow,slow);
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: contact-pose-clock-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    ordinary(mod,original,0,0,false);
    ordinary(mod,original,1,1,false);
    ordinary(mod,original,2,2,true);
    ordinary(mod,original,3,8,true);
    linked(mod,original);
}
