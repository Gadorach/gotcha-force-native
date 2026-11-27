// Original allocator in declared RAM, followed by bounded contact-visual updates.
#include "retail_stationary_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 parent=0x81028000,effect=0x81030000,cells=0x81032000,pools=0x81033000;
constexpr u32 shape=0x81034000,origin=0x81035000,visual_object=0x81036000;
constexpr u32 visual=0x81037000,link=visual+0x40,table=visual+0x80,empty=visual+0x400;
constexpr u32 arena=0x81600000,arena_end=arena+0x20000,block=arena+32,payload=block+32;
constexpr u32 active_head=0x803C706C,scene=contact::scene,attack_count=0x8043B5A0-21328;
constexpr u32 os_table=0x80436408,os_current=0x80435C60,dispatch=0x803FA748;
constexpr u32 bucket=31,row=pools+3*28,cell_stride=0x80;
constexpr unsigned burst_masks[]{255,255,255,255,255,65535,65535,65535,65535,65535,
                                  65535,65535,65535,65535,65535,65535,65280,0,0,0};
constexpr unsigned piece_masks[]{3,7,15,31,63,63,57,49,33,1,0,0,0};
struct Case {unsigned selector,bursts,entries,count,total,retire;};
constexpr Case cases[]{
    {1,0,0,16,928,20},
    {1,31,511,1,96,19},
    {8,0,0,8,288,13},
    {8,32,512,8,288,13},
};

class Check {
    const Module& mod;
    const Case& spec;
    std::vector<u8> ram;
    u8* at(u32 address){return contact::at(ram,address);}
    u32 word(u32 address){return be32(at(address));}
    u16 half(u32 address){return contact::half(ram,address);}
    void call(u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;
        const auto stop=mod.run_to_or_return(cpu,ram,entry,{0x80264290},nullptr,4096);
        if(stop){
            std::fprintf(stderr,"storage entry=%08X assertion caller=%08X line=%u\n",entry,cpu.lr-4,cpu.gpr[4]);
            fail("contact visual crossed an unsupported assertion boundary");
        }
    }
    void init_heap(){
        put32(at(0x80000028),GC_MAIN_RAM_SIZE);
        CPUState init{};init.gpr[3]=arena;init.gpr[4]=arena_end;init.gpr[5]=2;
        mod.run(init,ram,0x801FFEC0);
        if(init.gpr[3]!=block||word(os_table)!=arena||word(os_table+4)!=2||word(arena)!=0xFFFFFFFF)
            fail("original OS table initialization did not use the declared arena");
        call(0x80200320,arena_end);
        call(0x80200328,block);
        call(0x80253DF0);
        constexpr u32 dispatch_entries[]{0x80253F94,0x80253FF4,0x8025404C,0x80254028,0x802540B0};
        for(unsigned i=0;i<std::size(dispatch_entries);++i)
            if(word(dispatch+4*i)!=dispatch_entries[i])fail("original HSD heap dispatch registration changed");
        if(word(os_current)||word(0x80435F40)||word(arena)!=arena_end-block||
           word(arena+4)!=block||word(arena+8)||word(block)||word(block+4)||word(block+8)!=arena_end-block){
            std::fprintf(stderr,"heap current=%08X hsd=%08X table=%08X/%08X/%08X block=%08X/%08X/%08X\n",
                         word(os_current),word(0x80435F40),word(arena),word(arena+4),word(arena+8),
                         word(block),word(block+4),word(block+8));
            fail("original HSD heap did not start with one free block and no owner");
        }
    }

    bool heap_owns_block(){
        auto cur=word(arena+8);
        for(unsigned i=0;i<64&&cur;++i){
            if(cur<block||cur>=arena_end||(cur&31))fail("contact visual heap list escaped the declared arena");
            if(cur==block)return true;
            cur=word(cur+4);
        }
        if(cur)fail("contact visual allocation list exceeded the walk bound");
        return false;
    }
    void allocation(bool at_frontier=false){
        if(word(effect+0xDC)!=visual_object||word(row+8)!=visual_object||word(row+12)||half(row+18)||
           at(visual_object)[8]!=3||at(visual_object)[9]!=1||word(visual_object+12)!=payload||
           !heap_owns_block()||word(block+8)!=spec.total||
           (at_frontier&&(word(arena+8)!=block||word(block)||word(block+4)||word(arena+4)!=block+spec.total||
            word(block+spec.total)||word(block+spec.total+4)||word(block+spec.total+8)!=arena_end-block-spec.total)))
            fail("original contact visual did not own the aligned heap block");
    }

    void no_collider(){
        if(half(contact::free_count+8)!=1||word(contact::free_head+16)!=shape||word(active_head+49*4)||
           half(attack_count)||half(contact::receiver_count)||std::memcmp(at(effect+0x20),at(origin),12))
            fail("bounded contact visual published a collider or moved its origin");
    }
    void retirement(unsigned descriptors){
        if(word(effect+0xDC)||word(row+8)||word(row+12)!=visual_object||half(row+18)!=1||
           at(visual_object)[9]||word(visual_object+12)||heap_owns_block()||word(arena+4)!=block||
           word(block+8)!=(spec.selector==1?spec.total:arena_end-block)||
           word(active_head+4*bucket)||half(contact::free_count+4)!=1||word(contact::free_head+8)!=effect||
           half(pools+18)!=descriptors)
            fail("contact visual failed to return a heap block, its type-3 cell or a battle resource");
        auto cur=word(pools+12);
        unsigned seen=0;
        for(unsigned i=0;i<descriptors;++i){
            if(cur<cells||(cur-cells)%cell_stride||(cur-cells)/cell_stride>=descriptors||word(cur+12))
                fail("contact visual returned a descriptor with an unexpected address or joint");
            const auto bit=1u<<((cur-cells)/cell_stride);
            if(seen&bit)fail("contact visual returned a duplicate descriptor");
            seen|=bit;cur=word(cur);
        }
        if(cur)fail("contact visual returned a malformed descriptor list");
    }
    void check_burst_joints(){
        u32 joint_root=word(cells+12);
        if(joint_root<payload+spec.total||joint_root>=arena_end||word(cells+20)!=2)
            fail("original burst did not attach the record-2 joint group");
        u32 cur=word(joint_root+16);
        for(unsigned i=0;i<spec.count;++i){
            if(cur<payload+spec.total||cur>=arena_end||cur==joint_root){
                std::fprintf(stderr,"burst joint index=%u address=%08X root=%08X\n",i,cur,joint_root);
                fail("original burst joint group was shorter than the admitted burst");
            }
            cur=word(cur+8);
        }
        if(cur)fail("original burst joint group exceeded the admitted burst");
    }
public:
    Check(const Module& m,const std::vector<u8>& original,const Case& c):mod(m),spec(c),ram(original){
        init_heap();
        at(parent)[0]=1;at(actor)[0]=1;at(effect)[2]=2;at(shape)[2]=4;
        put32(at(parent+0x90),actor);put_float(at(parent+0xB4),1.f);
        put_float(at(actor+0x1DC8),1.f);
        for(unsigned i=0;i<3;++i)put_float(at(origin+4*i),std::array{260.f,120.f,880.f}[i]);
        put32(at(0x8043B5A0-31852),scene);
        put16(at(scene+484),u16(spec.bursts));put16(at(scene+486),u16(spec.entries));
        put32(at(0x803C4E84),actor);
        for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
        put32(at(0x80436380),pools);put32(at(0x8043B5A0-21024),pools);
        const unsigned descriptors=spec.selector==8?8:1;
        put32(at(pools+12),cells);put16(at(pools+18),u16(descriptors));
        for(unsigned i=1;i<descriptors;++i)put32(at(cells+(i-1)*cell_stride),cells+i*cell_stride);
        constexpr u32 source_row=0x8036F60C;
        if(half(source_row+16)!=16||word(source_row+20)||word(source_row+24)!=0x80006F1C)
            fail("source type-3 pool layout changed");
        // Declared backing for one original row-3 header. Keep source
        // stride and destructor; only its capacity and base differ.
        put32(at(row),visual_object);put32(at(row+4),16);
        std::memcpy(at(row+16),at(source_row+16),12);
        call(0x80197950,3);
        if(word(row+8)||word(row+12)!=visual_object||half(row+18)!=1||at(visual_object)[8]!=3)
            fail("original visual row initialization did not publish its single free cell");
        put32(at(contact::free_head+8),effect);put32(at(contact::free_tail+8),effect);
        put16(at(contact::free_count+4),1);
        put32(at(contact::free_head+16),shape);put32(at(contact::free_tail+16),shape);
        put16(at(contact::free_count+8),1);
        put32(at(0x803BB374),visual);put32(at(visual+16),link);put32(at(link),table);
        for(auto id:{53u,54u})put32(at(table+4*id),empty);
        if(spec.selector==1){
            constexpr u32 record=empty+0x20,joint=empty+0x80;
            // Original HSD ID pool and declared single-joint template
            // (no children, animation or geometry).
            call(0x80259644);
            put32(at(table+4*2),record);
            put32(at(record),joint);
            for(unsigned i=0;i<3;++i)put_float(at(joint+32+4*i),1.f);
        }

        CPUState request{};request.gpr[3]=parent;request.gpr[4]=origin;request.gpr[5]=spec.selector;
        mod.run(request,ram,0x80019550);
        if(request.gpr[3]!=effect||word(active_head+4*bucket)!=effect||half(pools+18)||
           half(contact::free_count+4)||at(effect+0x13)[0]!=descriptors)
            fail("fixed-bucket visual constructor did not claim its battle resources");
    }

    void burst_frontier(unsigned scenario){
        CPUState visit{};visit.gpr[3]=bucket;
        mod.run_to(visit,ram,0x80089A94,{0x80007198});
        allocation(true);
        no_collider();
        if(visit.lr!=0x80019B90||visit.gpr[3]!=visual||visit.gpr[4]!=cells||visit.gpr[5]!=2||visit.gpr[6]!=spec.count||
           at(effect+0x18)[0]!=1||at(effect+0x82)[0]||half(scene+484)!=spec.bursts+1||half(scene+486)!=spec.entries+spec.count)
            fail("selector 1 changed its original scene charge or model preparation frontier");
        std::printf("effect-storage-frontier: %u %u %u %u %u %u\n",
                    scenario,word(block+8),half(scene+484),half(scene+486),visit.gpr[5],visit.gpr[6]);
    }

    void lifetime(unsigned scenario){
        // Only the fixed bucket update. Do not call the drawing services;
        // selector 8 deliberately resolves its model records to null.
        for(unsigned visit=1;visit<=spec.retire;++visit){
            call(0x80089A94,bucket);
            no_collider();
            const bool burst=spec.selector==1,retired=visit==spec.retire;
            const bool charged=burst&&visit<spec.retire-2;
            const unsigned phase=visit<spec.retire-2?1:visit==spec.retire-2?2:3;
            const auto mask=burst?word(effect+0x158):unsigned(half(effect+0x1C));
            const unsigned expected_mask=!burst?piece_masks[visit-1]:
                spec.count==1?unsigned(charged):burst_masks[visit-1];
            if(at(effect+0x18)[0]!=phase||bool(at(effect)[0])==retired||at(effect+0x82)[0]!=unsigned(phase!=3)||
               mask!=expected_mask||half(scene+484)!=spec.bursts+unsigned(charged)||
               half(scene+486)!=spec.entries+spec.count*unsigned(charged))
                fail("contact visual changed its phase, activity mask or scene charge");
            if(!retired){
                allocation(!burst);
                if(half(pools+18)||half(contact::free_count+4)||word(active_head+4*bucket)!=effect)
                    fail("contact visual returned battle resources before retirement");
                if(burst)check_burst_joints();
            }else{
                retirement(burst?1:8);
            }
            std::printf("effect-storage-visit: %u %u %u %u %u %u %u %u\n",scenario,visit,phase,mask,
                        at(effect+0x82)[0],half(scene+484),half(scene+486),retired?0:word(block+8));
        }
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: contact-effect-storage-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(unsigned i=0;i<std::size(cases);++i){
        if(cases[i].selector==1)Check(mod,original,cases[i]).burst_frontier(i);
        Check(mod,original,cases[i]).lifetime(i);
    }
}
