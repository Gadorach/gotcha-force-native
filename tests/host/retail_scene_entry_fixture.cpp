// Declared scene/mode and six physical actor records; stop before actor dispatch.
#include "retail_scene_entry.hpp"

namespace {
using namespace retail_fixture;
namespace source=retail_scene_entry;
namespace contact=retail_stationary_contact;
constexpr u32 mode_storage=0x81071000,actors=0x803C4E84,stride=0x1E00;
void seed(std::vector<u8>& ram,bool fast=false,bool extended=false){
    const auto at=[&](u32 p){return contact::at(ram,p);};
    put32(at(source::mode_ptr),mode_storage);
    put32(at(0x8043B5A0-21024),0x81078000); // Empty SDK presentation pool.
    for(unsigned i=0;i<6;++i){
        const auto body=actor+i*stride;
        put32(at(actors+i*4),body);
        at(body)[0]=1;
        at(body+0x743)[0]=1;
        put16(at(body+0x1C6),0); // Readiness is not a current-HP check.
    }
    source::prepare(ram,fast,extended);
}
void output(const char* tag,unsigned key,unsigned n,source::Visit v){
    std::printf("%s: %u %u %08X %u %u %d %u\n",tag,key,n,v.handler,v.status,v.substep,v.timer,v.hud_held);
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: scene-entry-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    for(unsigned seat=0;seat<6;++seat){
        auto ram=original;
        seed(ram);
        const auto body=actor+seat*stride;
        contact::at(ram,body+0x743)[0]=0;
        put16(contact::at(ram,source::scene+8),37);
        output("scene-seat",seat,0,source::advance(mod,ram));
        contact::at(ram,body)[0]=0;
        output("scene-seat",seat,1,source::advance(mod,ram));
    }
    constexpr unsigned releases[]{93,212,2};
    for(unsigned kind=0;kind<3;++kind){
        auto ram=original;
        seed(ram,kind==2,kind==1);
        unsigned last=0;
        for(unsigned visit=1;visit<=releases[kind];++visit){
            const auto row=source::advance(mod,ram);
            if(visit<=2||row.substep!=last||row.timer==1||visit==releases[kind])
                output("scene-clock",kind,visit,row);
            if(row.status!=(visit==releases[kind]?4u:3u))
                fail("scene status changed outside the expected scheduler visit");
            last=row.substep;
        }
    }
    auto ram=original;
    seed(ram);
    contact::at(ram,source::scene)[0]=3;
    contact::at(ram,source::scene+1)[0]=1;
    contact::at(ram,source::scene+69)[0]=4;
    put16(contact::at(ram,source::scene+8),2);
    output("scene-ending",0,1,source::advance(mod,ram));
}
