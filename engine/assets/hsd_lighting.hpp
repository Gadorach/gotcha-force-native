#pragma once
// Only published scene light roots are parsed. Reader supplies checked ranges
// and relocated pointers; the template keeps this helper independent of HSD's
// geometry decoder and lets owned fixtures exercise the light records directly.
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include "engine/graphics/lighting.hpp"
namespace gf::hsd_detail {
template<class Reader>
inline bool read_scene_lighting(const Reader& r,u32 scene_table,SceneLighting* out,std::string* error) {
    *out={};
    auto fail=[&](const char* message){*out={};if(error)*error=std::string("hsd lights: ")+message;return false;};
    u32 table=0;
    if(!r.ptr(std::size_t(scene_table)+8,&table))return fail("invalid light table pointer");
    if(!table)return true;
    SceneLighting rig;rig.declared=true;
    auto halfword=[&](std::size_t at){return static_cast<u16>((u16(r.block.data[at])<<8)|r.block.data[at+1]);};
    auto real=[&](std::size_t at){u32 bits=0;for(unsigned k=0;k<4;++k)bits=(bits<<8)|r.block.data[at+k];return bits_to_f32(bits);};
    std::vector<u32> visited;
    for(unsigned i=0;i<=256;++i){
        u32 record=0,light=0,animation=0;
        if(!r.ptr(std::size_t(table)+i*4,&record))return fail("invalid light root record pointer");
        if(!record){rig.supported=menu_light_rig_supported(rig);*out=std::move(rig);return true;}
        if(i==256||!r.has(record,8)||!r.ptr(record,&light)||!light||!r.ptr(record+4,&animation))
            return fail("invalid light root record");
        rig.animated=rig.animated||animation!=0;
        std::vector<u32> ancestors;
        while(light){
            if(ancestors.size()>=256||std::find(ancestors.begin(),ancestors.end(),light)!=ancestors.end())return fail("light chain cycle/budget");
            ancestors.push_back(light);
            if(std::find(visited.begin(),visited.end(),light)!=visited.end())break;
            if(visited.size()>=256||!r.has(light,28))return fail("truncated light/budget");
            visited.push_back(light);
            u32 klass=0,next=0,pos=0,interest=0,params=0;
            if(!r.ptr(light,&klass)||!r.ptr(light+4,&next)||!r.ptr(light+16,&pos)||
               !r.ptr(light+20,&interest)||!r.ptr(light+24,&params))return fail("unrelocated light field");
            SceneLight item;item.flags=halfword(light+8);
            for(unsigned k=0;k<4;++k)item.colour[k]=r.block.data[light+12+k];
            const unsigned type=item.flags&3;
            if(klass||type>1||(item.flags&~0x3fu))rig.supported=false;
            auto wobj=[&](u32 pointer,f32 xyz[3]){
                if(!pointer)return true;
                u32 class_name=0;
                if(!r.has(pointer,16)||!r.ptr(pointer,&class_name)||class_name||!r.finite(r.block,pointer+4,3))return false;
                for(unsigned k=0;k<3;++k)xyz[k]=real(pointer+4+k*4);
                return true;
            };
            if(!wobj(pos,item.position)||!wobj(interest,item.interest))return fail("invalid light WObj");
            if(type==1){
                if(!pos)return fail("infinite light missing position");
                const f32 length2=item.position[0]*item.position[0]+item.position[1]*item.position[1]+item.position[2]*item.position[2];
                if(!(length2>0))return fail("zero infinite light direction");
            }
            rig.lights.push_back(item);light=next;
        }
    }
    return fail("unterminated light table");
}
} // namespace gf::hsd_detail
