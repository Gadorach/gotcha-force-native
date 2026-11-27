// Bound the first update and retirement when dynamic visual storage is unavailable.
#include "retail_stationary_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 parent=0x81028000,effect=0x81030000,cells=0x81032000,pools=0x81033000;
constexpr u32 shape=0x81034000,origin=0x81035000,visual_object=0x81036000;
constexpr u32 active_head=0x803C706C,helper=0x80019550,allocate_visual=0x80006DC8,allocate_heap=0x8025AFA4;
constexpr u32 rng=0x8043B5A0-21722,scene=contact::scene;
constexpr u32 attack_count=0x8043B5A0-21328;
constexpr unsigned bucket=31,visual_row=3*28;

struct Case {
    unsigned selector,bursts,particles,seed,request,count_a,count_b,count_byte;
};
constexpr Case cases[]{
    {1,0,0,0,896,8,8,16},
    {1,31,0,0x37AB,1064,10,9,19},
    {1,31,496,0x37AB,896,8,8,16}, // Same draw selects 10+9, clamped to 16.
    {1,31,511,0x37AB,56,1,0,1},
    {1,32,0,0x37AB,0,0,0,1},
    {1,0,512,0x37AB,0,0,0,1},
    {8,0,0,0x37AB,256,0,0,8},
    {8,32,512,0x37AB,256,0,0,8},
};

void observe(const Module& mod,std::vector<u8> ram,unsigned scenario){
    const auto& row=cases[scenario];
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    const auto initial_cells=row.selector==8?8u:1u;
    constexpr u32 stride=0x80;
    at(actor)[0]=1;at(parent)[0]=1;at(effect)[2]=2;at(shape)[2]=4;
    put32(at(parent+0x90),actor);put_float(at(parent+0xB4),1.f);
    put_float(at(actor+0x1DC8),1.f);
    constexpr contact::Point point{260.f,120.f,880.f};
    for(unsigned i=0;i<point.size();++i)put_float(at(origin+4*i),point[i]);
    put32(at(0x8043B5A0-31852),scene);
    put16(at(scene+484),u16(row.bursts));put16(at(scene+486),u16(row.particles));
    at(rng)[0]=u8(row.seed);at(rng)[1]=u8(row.seed>>8);
    put32(at(0x80436380),pools);
    put32(at(0x8043B5A0-21024),pools);
    // Class-2 battle descriptors and the type-3 visual-object inlet use
    // different rows. Leave the latter empty so no backend allocator runs.
    put32(at(pools+12),cells);put16(at(pools+18),u16(initial_cells));
    for(unsigned i=1;i<initial_cells;++i)put32(at(cells+(i-1)*stride),cells+i*stride);
    put32(at(contact::free_head+8),effect);put32(at(contact::free_tail+8),effect);
    put16(at(contact::free_count+4),1);
    put32(at(contact::free_head+16),shape);put32(at(contact::free_tail+16),shape);
    put16(at(contact::free_count+8),1);
    put32(at(0x803C4E84),actor);
    for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);

    CPUState create{};create.gpr[3]=parent;create.gpr[4]=origin;create.gpr[5]=row.selector;
    mod.run(create,ram,helper);
    if(create.gpr[3]!=effect||word(active_head+4*bucket)!=effect||word(effect+0x0C)!=0x800196D0||
       at(effect+0x11)[0]!=(row.selector==8?3:1)||at(effect+0x13)[0]!=initial_cells||
       half(contact::free_count+4)||half(pools+18))
        fail("contact visual did not pass the first, battle-object admission");
    for(unsigned i=0;i<initial_cells;++i)
        if(word(effect+0xE0+4*i)!=cells+i*stride)fail("contact visual lost its ordered descriptor inlet");

    auto cut_ram=ram;
    CPUState cut{};cut.gpr[3]=bucket;
    const auto reached=mod.run_to_or_return(cut,cut_ram,0x80089A94,{allocate_visual});
    const auto* inlet=contact::at(cut_ram,effect);
    if(reached!=(row.request?allocate_visual:0)||
       (reached&&(cut.lr!=(row.selector==8?0x8001A740:0x80019AF8)||cut.gpr[3]!=row.request||
                  inlet[0x18]!=1||inlet[0x13]!=row.count_byte||
                  contact::half(cut_ram,effect+0x1C)!=row.count_a||
                  contact::half(cut_ram,effect+0x1E)!=row.count_b))){
        const auto* next_rng=contact::at(cut_ram,rng);
        std::fprintf(stderr,"impact init case=%u cut=%08X lr=%08X bytes=%u phase=%u count=%u split=%u/%u seed=%02X%02X -> %02X%02X\n",
                     scenario,reached,cut.lr,cut.gpr[3],inlet[0x18],inlet[0x13],
                     contact::half(cut_ram,effect+0x1C),contact::half(cut_ram,effect+0x1E),
                     at(rng)[1],at(rng)[0],next_rng[1],next_rng[0]);
        fail("contact visual changed the second allocation request or cap boundary");
    }

    unsigned heap_bytes=0;
    if(reached){
        // Separate fork: give the original object allocator one cell and
        // stop before the heap call, without substituting its result.
        std::memcpy(cut_ram.data(),ram.data(),ram.size());
        const auto mem=[&](u32 address){return contact::at(cut_ram,address);};
        put32(mem(pools+visual_row+12),visual_object);
        put16(mem(pools+visual_row+18),1);
        CPUState heap{};heap.gpr[3]=bucket;
        mod.run_to(heap,cut_ram,0x80089A94,{allocate_heap});
        heap_bytes=heap.gpr[3];
        if(heap.lr!=0x80006E08||heap_bytes!=((row.request+31)&~31u)||
           be32(mem(pools+visual_row+8))!=visual_object||be32(mem(pools+visual_row+12))||
           contact::half(cut_ram,pools+visual_row+18)||mem(visual_object)[9]!=1||be32(mem(effect+0xDC))||
           contact::half(cut_ram,scene+484)!=row.bursts||contact::half(cut_ram,scene+486)!=row.particles)
            fail("contact visual heap frontier did not follow the separate type-3 object claim");
    }

    for(unsigned visit=1;visit<=3;++visit){
        CPUState tick{};tick.gpr[3]=bucket;
        mod.run(tick,ram,0x80089A94,1024);
        for(u32 entry:{0x80089554u,0x8008A28Cu,0x80089AECu}){
            CPUState pass{};mod.run(pass,ram,entry,1024);
        }
        const bool retired=visit==3;
        if(at(effect+0x18)[0]!=(visit==1?2:3)||bool(at(effect)[0])==retired||at(effect+0x82)[0]||
           at(effect+0x13)[0]!=row.count_byte||
           word(effect+0xDC)||word(active_head+4*bucket)!=(retired?0:effect)||
           half(contact::free_count+4)!=unsigned(retired)||word(contact::free_head+8)!=(retired?effect:0)||
           half(pools+18)!=(retired?initial_cells:0)||half(pools+visual_row+18)||word(pools+visual_row+8)||
           half(scene+484)!=row.bursts||half(scene+486)!=row.particles||
           half(contact::free_count+8)!=1||word(contact::free_head+16)!=shape||word(active_head+49*4)||
           half(attack_count)||half(contact::receiver_count)||std::memcmp(at(effect+0x20),at(origin),12))
            fail("contact visual storage refusal leaked a cell or published a collider");
        if(retired){
            auto cur=word(pools+12);
            unsigned seen=0;
            for(unsigned i=0;i<initial_cells;++i){
                if(cur<cells||(cur-cells)%stride||(cur-cells)/stride>=initial_cells)
                    fail("contact visual returned an unexpected descriptor");
                const auto bit=1u<<((cur-cells)/stride);
                if(seen&bit)fail("contact visual returned a duplicate descriptor");
                seen|=bit;cur=word(cur);
            }
            if(cur)fail("contact visual returned a malformed descriptor list");
        }
    }
    const auto seed=unsigned(at(rng)[0])+256u*at(rng)[1];
    const auto draw=(row.seed*3>>8)&0xFF;
    const auto expected_seed=row.selector==1&&row.request?256*draw+((row.seed+draw)&0xFF):row.seed;
    if(seed!=expected_seed)fail("contact visual changed the expected RNG debit");
    std::printf("impact-init: %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                scenario,row.selector,row.bursts,row.particles,row.seed,seed,row.request,heap_bytes,row.count_a,row.count_b,
                initial_cells,row.count_byte,3u);
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: contact-effect-init-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(unsigned i=0;i<std::size(cases);++i)observe(mod,original,i);
}
