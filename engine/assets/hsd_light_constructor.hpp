#pragma once
#include <string>
#include <optional>
#include "engine/graphics/lighting.hpp"
namespace gf {
struct HsdInitialLightNode {
    SceneLight light;u32 source_descriptor=0;std::optional<u16> next;
    u32 source_parameters=0;std::optional<std::array<u32,3>> point_parameters;
};
struct HsdInitialLightConstructor {
    std::vector<HsdInitialLightNode> nodes;std::vector<u16> loaded_heads;
    std::optional<u16> head,primary_infinite;
    u64 generation=0;bool animation_descriptors_present=false,animation_requested=false;
};
namespace hsd_detail {
//Shared default-node input copy. Point parameters are retained ONLY for the
//explicit cockpit-input entry; this never enables point-light rendering.
template<class Reader>
bool append_initial_light_chain(const Reader& r,u32 descriptor,bool cockpit_point_inputs,HsdInitialLightConstructor* next,u16* head,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("initial light chain: ")+why;return false;};
    if(!next||!head||!descriptor)return fail("current output/default root required");
    *head=u16(next->nodes.size());std::optional<u16> previous;std::vector<u32> chain;
    while(descriptor){
        if(next->nodes.size()>=256||std::find(chain.begin(),chain.end(),descriptor)!=chain.end()||!r.has(descriptor,28))return fail("source chain cycle/node budget/range");
        chain.push_back(descriptor);u32 klass=0,following=0,position=0,interest=0,params=0;
        if(!r.ptr(descriptor,&klass)||klass||!r.ptr(descriptor+4,&following)||!r.ptr(descriptor+16,&position)||!r.ptr(descriptor+20,&interest)||!r.ptr(descriptor+24,&params))return fail("default relocated light fields required");
        HsdInitialLightNode node;node.source_descriptor=descriptor;node.source_parameters=params;node.light.flags=r.block.be16(descriptor+8);
        const unsigned type=node.light.flags&3u;
        if(type>(cockpit_point_inputs?2u:1u)||(node.light.flags&~0x3fu))return fail("unsupported default light kind/flags");
        for(unsigned k=0;k<4;++k)node.light.colour[k]=r.block.data[descriptor+12+k];
        auto wobj=[&](u32 p,f32* values){
            if(!p)return true;
            u32 kind=0;if(!r.has(p,16)||!r.ptr(p,&kind)||kind||!r.finite(r.block,p+4,3))return false;
            for(unsigned k=0;k<3;++k)values[k]=r.block.bef32(p+4+4*k);
            return true;
        };
        if(!wobj(position,node.light.position)||!wobj(interest,node.light.interest)||(type&&!position))return fail("default bounded WObj positions required");
        for(unsigned k=0;k<3;++k)if(std::abs(node.light.position[k])>1e7f||std::abs(node.light.interest[k])>1e7f)return fail("WObj numeric domain");
        if(type==2){
            if(!params||!r.has(params,12)||!r.finite(r.block,params,2))return fail("point constructor parameter extent");
            const f32 brightness=r.block.bef32(params),distance=r.block.bef32(params+4);
            if(brightness<=0||brightness>=1||distance<=0||distance>1e7f||r.block.be32(params+8)>3)return fail("unreviewed point constructor parameter domain");
            node.point_parameters=std::array<u32,3>{r.block.be32(params),r.block.be32(params+4),r.block.be32(params+8)};
        }
        const u16 id=u16(next->nodes.size());next->nodes.push_back(node);
        if(previous)next->nodes[*previous].next=id;
        previous=id;descriptor=following;
    }
    return true;
}
//08A94/245F20 default-class constructor inputs. Each root load owns a fresh
//chain, even for repeated descriptors. Replacing a prior head's next pointer
//does NOT silently destroy its now-unreached nodes or qualify SDK cleanup.
template<class Reader>
bool read_initial_light_constructor(const Reader& r,u32 scene,HsdInitialLightConstructor* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("initial light constructor: ")+why;return false;};
    if(!out)return fail("output required");
    u32 table=0;if(!r.ptr(std::size_t(scene)+8,&table)||!table)return fail("published light table required");
    HsdInitialLightConstructor next;std::optional<u16> previous_head;
    for(unsigned root=0;root<=256;++root){
        u32 record=0,descriptor=0,animation=0;
        if(!r.ptr(std::size_t(table)+4*root,&record))return fail("light-root table range");
        if(!record){next.generation=1;*out=std::move(next);if(error)error->clear();return true;}
        if(root==256||!r.has(record,8)||!r.ptr(record,&descriptor)||!descriptor||!r.ptr(record+4,&animation))return fail("default light root/budget");
        next.animation_descriptors_present=next.animation_descriptors_present||animation!=0;
        u16 head=0;if(!append_initial_light_chain(r,descriptor,false,&next,&head,error))return false;
        next.loaded_heads.push_back(head);
        if(!next.head)next.head=head;
        if(previous_head)next.nodes[*previous_head].next=head;
        //The caller tests only the newly returned root, not descendants or root0.
        if(root&&(next.nodes[head].light.flags&3u)==1)next.primary_infinite=head;
        previous_head=head;
    }
    return fail("unterminated root table");
}
}
//08B90's four INDEPENDENT ambient+selected constructor-input groups. These are
//not the stage's08A94 chain and never establish/replace primary_infinite.
struct HsdInitialCockpitLightInputs {
    std::array<HsdInitialLightConstructor,4> groups;
    u64 generation=0;bool point_light_rendering_supported=false;
};
namespace hsd_detail {
template<class Reader>
bool read_initial_cockpit_light_inputs(const Reader& r,u32 scene,HsdInitialCockpitLightInputs* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("cockpit light inputs: ")+why;return false;};
    if(!out)return fail("output required");
    u32 table=0,first=0;if(!r.ptr(std::size_t(scene)+8,&table)||!table||!r.ptr(table,&first))return fail("source root table required");
    HsdInitialCockpitLightInputs next;
    for(unsigned group=0;group<4;++group){
        auto& inputs=next.groups[group];inputs.generation=1;
        if(!first)continue;
        std::optional<u16> previous_head;
        for(unsigned index:{0u,group+1}){
            u32 record=0,descriptor=0,animation=0;
            if(!r.ptr(std::size_t(table)+4*index,&record))return fail("source group root extent");
            if(!record)continue;
            if(!r.has(record,8)||!r.ptr(record,&descriptor)||!descriptor||!r.ptr(record+4,&animation))return fail("default group source root");
            inputs.animation_descriptors_present=inputs.animation_descriptors_present||animation!=0;
            u16 head=0;if(!append_initial_light_chain(r,descriptor,true,&inputs,&head,error))return false;
            inputs.loaded_heads.push_back(head);if(!inputs.head)inputs.head=head;
            if(previous_head)inputs.nodes[*previous_head].next=head;
            previous_head=head;
        }
    }
    next.generation=1;*out=std::move(next);if(error)error->clear();return true;
}
}
}
