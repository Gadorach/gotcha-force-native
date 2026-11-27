// Shared read-only archive inlet and original SDK bootstrap for contact visual fixtures.
#pragma once
#include "retail_stationary_contact.hpp"

namespace retail_contact_effect_archive {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
inline constexpr u32 arena=0x81600000,arena_end=arena+0x20000,block=arena+32,payload=block+32;
inline constexpr u32 source_bank=0x81400000,source_globals=0x803BB374,draw_heads=0x803BB150;
inline constexpr u32 source_bank_basis=0x8043B5A0-21352,source_bank_delta=0x7AC400;
inline constexpr u32 bucket=31;
struct Source {std::vector<u8> bytes;unsigned slot=0;};

// Self-sized prefix. The fixture caller must enforce EOF after its own payload.
inline Source read_source(){
    std::array<u8,4> header{};
    if(std::fread(header.data(),1,header.size(),stdin)!=header.size())
        fail("common impact archive input extent");
    const auto size=be32(header.data());
    if(size!=417955&&size!=408295)fail("common impact archive input size");
    std::vector<u8> bytes(size+1);
    std::memcpy(bytes.data(),header.data(),header.size());
    if(std::fread(bytes.data()+4,1,bytes.size()-4,stdin)!=bytes.size()-4)
        fail("common impact archive input extent");
    const bool split=size==408295;
    const u32 data_size=split?389516:399084,relocs=split?4682:4705;
    if(be32(bytes.data()+4)!=data_size||be32(bytes.data()+8)!=relocs||
       be32(bytes.data()+12)!=1||be32(bytes.data()+16)||bytes.back())
        fail("common impact archive input header");
    std::vector<bool> seen(data_size/4);
    for(unsigned i=0;i<relocs;++i){
        const auto offset=be32(bytes.data()+32+data_size+i*4);
        if(offset%4||offset>data_size-4||seen[offset/4]||be32(bytes.data()+32+offset)>=data_size)
            fail("common impact archive relocation extent");
        seen[offset/4]=true;
    }
    const auto symbol=32+data_size+relocs*4;
    constexpr char name[]="scene_data";
    if(be32(bytes.data()+symbol)>data_size-16||be32(bytes.data()+symbol+4)||
       symbol+8+sizeof(name)!=size||std::memcmp(bytes.data()+symbol+8,name,sizeof(name)))
        fail("common impact archive scene symbol extent");
    return {std::move(bytes),split?2u:1u};
}

class Setup {
    const Module& mod;
    std::vector<u8>& ram;
    u8* at(u32 address){return contact::at(ram,address);}
    u32 word(u32 address){return be32(at(address));}
    u16 half(u32 address){return contact::half(ram,address);}
public:
    Setup(const Module& m,std::vector<u8>& mem):mod(m),ram(mem){}
    void call(u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;
        if(mod.run_to_or_return(cpu,ram,entry,{0x80264290},nullptr,4096)){
            std::fprintf(stderr,"visual entry=%08X assertion caller=%08X line=%u\n",entry,cpu.lr-4,cpu.gpr[4]);
            fail("contact visual crossed an unsupported assertion boundary");
        }
    }
    void init_heap(){
        constexpr u32 os_table=0x80436408,os_current=0x80435C60,dispatch=0x803FA748;
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
           word(arena+4)!=block||word(arena+8)||word(block)||word(block+4)||word(block+8)!=arena_end-block)
            fail("original HSD heap did not start with one free block and no owner");
    }
    void init_visual_header(u32 pools,u32 header){
        constexpr u32 source_pool=0x8036F60C;
        const auto row=pools+3*28;
        if(half(source_pool+16)!=16||word(source_pool+20)||word(source_pool+24)!=0x80006F1C)
            fail("source type-3 pool layout changed");
        put32(at(row),header-word(source_bank_basis));put32(at(row+4),16);
        std::memcpy(at(row+16),at(source_pool+16),12);
        call(0x80197950,3);
        if(word(row+8)||word(row+12)!=header||half(row+18)!=1||at(header)[8]!=3)
            fail("original visual row initialization did not publish its single free cell");
    }
    void publish(const Source& source,u32 pools,u32 source_owner,u32 render_context){
        constexpr u32 source_pool=0x8036F5D4,destinations=0x803B0760;
        const auto owner_row=pools+28;
        if(half(source_pool+16)!=32||word(source_pool+20)!=0x80006E3C||word(source_pool+24)!=0x80006E8C)
            fail("source common archive header pool row changed");
        put32(at(owner_row),source_owner-word(source_bank_basis));put32(at(owner_row+4),32);
        std::memcpy(at(owner_row+16),at(source_pool+16),12);
        call(0x80197950,1);
        put32(at(0x8043B5A0-31856),render_context);
        at(render_context+41)[0]=u8(source.slot==2);
        put32(at(source_bank_basis),source_bank-source_bank_delta);
        CPUState stage{};
        mod.run_to(stage,ram,0x80042878,{0x8000B07C});
        const auto inactive_slot=source.slot==1?2u:1u;
        if(stage.lr!=0x80042904||stage.gpr[3]!=697||stage.gpr[4]!=destinations||
           word(destinations)!=source_bank-0x8800||
           word(destinations+4*source.slot)!=source_bank||word(destinations+4*inactive_slot)!=0xF0000000||
           word(source_globals)||word(source_globals+4)!=source_bank)
            fail("original common effect archive staging selected the wrong PZZ slot");
        // Decompressed bytes at the selected output, not a fabricated
        // archive wrapper or a pre-relocated scene descriptor.
        std::memcpy(at(source_bank),source.bytes.data(),source.bytes.size());
        call(0x80042954);
        const auto context=word(source_owner+12),scene_symbol=word(source_owner+16);
        const auto data_extent=be32(source.bytes.data()+4),relocs=be32(source.bytes.data()+8);
        const auto* reloc=source.bytes.data()+32+data_extent;
        if(word(source_globals)!=source_owner||word(owner_row+8)!=source_owner||half(owner_row+18)||
           at(source_owner)[8]!=1||at(source_owner)[9]!=1||at(source_owner)[10]!=1||
           word(source_owner+20)!=source_bank||half(source_owner+26)!=157||
           context!=payload||word(context+32)!=source_bank+32||
           scene_symbol!=source_bank+32+be32(reloc+4*relocs))
            fail("original common effect archive did not publish its relocated scene and owner");
        for(unsigned i=0;i<relocs;++i){
            const auto offset=be32(reloc+4*i);
            if(word(source_bank+32+offset)!=source_bank+32+be32(source.bytes.data()+32+offset))
                fail("original common effect archive relocation changed");
        }
        const auto records=word(scene_symbol);
        if(records<source_bank+32||records>source_bank+32+data_extent-158*4||word(records+157*4))
            fail("original common effect scene table extent");
    }
    void prepare_view_zero(){
        call(0x80009790);call(0x80089554);call(0x8008A16C);call(0x80089CD0,24);call(0x80089EB4);
    }
};
}
