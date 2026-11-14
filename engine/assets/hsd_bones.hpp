#pragma once
// Character-wrapper bone binding, not an animator or world-matrix builder.
// Original 80041678 retains the wrapper; 8005380C walks model record0 twice.
// 80067E18 visits node -> child -> next, reads a SIGNED order-table byte,
// copies record bytes0..2 and descriptor translation into that logical slot.
#include <array>
#include "engine/assets/hsd_anim.hpp"
namespace gf {
inline constexpr u8 kHsdNoBone=0xff;
inline constexpr std::size_t kHsdMaxCharacterBones=32; // resource +608..688 arrays
struct HsdModelBone {
    u32 joint_offset=0; // archive-relative identity, NEVER a guest/host pointer
    u8 child=kHsdNoBone,next=kHsdNoBone,parent=kHsdNoBone;
    f32 translation[3]={}; // initial LOCAL translation only, not a complete pose
};
struct HsdModelBones {
    u32 root_offset=0;
    std::vector<u8> preorder_slots;
    std::vector<HsdModelBone> bones; // indexed by logical slot, NOT HSD traversal
    bool loaded=false;
};
inline bool hsd_bones_load(ByteView prefix,ByteView archive,HsdModelBones* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("hsd bones: ")+why;return false;};
    if(!out||!prefix.contains(0,12)||prefix.size>(64u<<10)||prefix.be32(0)!=prefix.size||
       (prefix.size&3)||!archive.contains(0,32)||archive.size>(64u<<20))return fail("input budget/prefix extent/destination");
    const u32 publics=archive.be32(12),externs=archive.be32(16),size=archive.be32(0);
    if(publics>4096||externs>4096)return fail("symbol budget");
    const std::size_t records=prefix.be32(4),order=prefix.be32(8);
    if(records<12||(records&3)||order<12||records>=prefix.size||order>=prefix.size)return fail("wrapper table offsets");
    hsd_detail::Reader r;if(!hsd_anim_detail::open_container(archive,&r,error))return false;
    const std::size_t symbols=32+r.block.size+std::size_t(archive.be32(8))*4;
    const std::size_t strings=symbols+(std::size_t(publics)+externs)*8;
    u32 scene=0;bool found=false;
    for(u32 i=0;i<publics;++i){
        const u32 name=archive.be32(symbols+i*8+4);
        if(name>=size-strings)return fail("symbol name offset");
        const std::size_t begin=strings+name;std::size_t end=begin;
        while(end<size&&archive.data[end]){if(end-begin>=256)return fail("symbol name budget");++end;}
        if(end==size)return fail("unterminated symbol name");
        if(std::string(reinterpret_cast<const char*>(archive.data+begin),end-begin)=="scene_data"){
            if(found)return fail("ambiguous scene export");
            scene=archive.be32(symbols+i*8);found=true;
        }
    }
    u32 table=0,record=0,root=0;
    if(!found||!r.ptr(scene,&table)||!table||!r.ptr(table,&record)||!record||
       !r.has(record,12)||!r.ptr(record,&root)||!root)return fail("missing published model record0");
    struct Node{u32 offset,parent,child,next;};
    std::vector<Node> nodes;std::vector<std::pair<u32,u32>> stack{{root,0}};
    HsdModelBones candidate;candidate.root_offset=root;
    std::array<bool,kHsdMaxCharacterBones> used{};
    while(!stack.empty()){
        const auto [at,parent]=stack.back();stack.pop_back();
        if(nodes.size()>=kHsdMaxCharacterBones)return fail("bone budget");
        for(const auto& n:nodes)if(n.offset==at)return fail("cycle/shared joint identity");
        u32 klass=0,child=0,next=0;
        if(!r.has(at,0x40)||!r.ptr(at,&klass)||klass||!r.ptr(at+8,&child)||!r.ptr(at+12,&next)||
           !r.finite(r.block,at+0x2c,3))return fail("joint descriptor/class/translation");
        // Instance loading changes child ownership; do not pretend descriptor
        // traversal is runtime traversal for that path.
        if(r.block.be32(at+4)&0x1000u)return fail("instance joint requires separate binding");
        const std::size_t ordinal=nodes.size();
        if(!prefix.contains(order+ordinal,1))return fail("truncated order table");
        const u8 slot=prefix.data[order+ordinal];
        if(slot>=kHsdMaxCharacterBones||used[slot])return fail("negative/out-of-budget/duplicate logical slot");
        used[slot]=true;candidate.preorder_slots.push_back(slot);nodes.push_back({at,parent,child,next});
        if(next)stack.push_back({next,parent});
        if(child)stack.push_back({child,at});
    }
    const std::size_t count=nodes.size();
    if(!prefix.contains(records,count*4)||!prefix.contains(order,count)||
       (records<order+count&&order<records+count*4))return fail("table extent/overlap");
    for(std::size_t i=0;i<count;++i)if(!used[i])return fail("sparse logical slots unsupported");
    auto slot_of=[&](u32 offset){
        for(std::size_t i=0;i<count;++i)if(nodes[i].offset==offset)return candidate.preorder_slots[i];
        return kHsdNoBone;
    };
    candidate.bones.resize(count);
    for(std::size_t i=0;i<count;++i){
        const auto& node=nodes[i];const u8 slot=candidate.preorder_slots[i];const auto* links=prefix.data+records+slot*4;
        // The source copies these bytes without interpreting them here. This
        // native subset additionally requires their child/next/parent topology
        // to agree with the selected descriptor tree; no guessed fixups.
        if(links[0]!=slot_of(node.child)||links[1]!=slot_of(node.next)||links[2]!=slot_of(node.parent))
            return fail("wrapper links disagree with selected model topology");
        auto& bone=candidate.bones[slot];bone.joint_offset=node.offset;
        bone.child=links[0];bone.next=links[1];bone.parent=links[2];
        for(unsigned k=0;k<3;++k)bone.translation[k]=r.block.bef32(node.offset+0x2c+k*4);
        // Input record byte3 is deliberately ignored: 80067E18 writes slot there.
    }
    candidate.loaded=true;*out=std::move(candidate);return true;
}
} // namespace gf
