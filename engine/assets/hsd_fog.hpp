#pragma once
// Explicit published first FogDesc: source scene+12 -> binding[0] -> FogDesc.
// Old geometry loaders do not acquire new fog/type acceptance implicitly.
#include "engine/graphics/fog.hpp"
#include <string>
namespace gf::hsd_detail {
template<class Reader> inline bool read_scene_linear_fog(const Reader& r,u32 scene,LinearFog* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("hsd linear fog: ")+why;return false;};
    if(!out||!r.has(scene,16))return fail("published scene/output required");
    auto pointer=[&](std::size_t at,u32* value){if(!r.has(at,4))return false;const auto word=r.block.be32(at);if((word==0||word==0xffffffffu)&&std::binary_search(r.fields.begin(),r.fields.end(),u32(at)))return false;return r.ptr(at,value);};
    u32 binding=0,descriptor=0,range=0,additional=0;if(!pointer(std::size_t(scene)+12,&binding))return fail("fog binding relocation");
    if(!binding)return true;
    if(!r.has(binding,8)||!pointer(binding,&descriptor)||!descriptor||!pointer(binding+4,&additional)||additional||!r.has(descriptor,20)||!pointer(descriptor+4,&range)||range)return fail("static single FogDesc without animation/range adjustment required");
    if(r.block.be32(descriptor)!=2)return fail("only perspective linear FogDesc type2 admitted");
    LinearFog fog;fog.enabled=true;fog.start=r.block.bef32(descriptor+8);fog.end=r.block.bef32(descriptor+12);for(unsigned i=0;i<3;++i)fog.colour[i]=r.block.data[descriptor+16+i];
    if(!linear_fog_valid(fog))return fail("finite bounded ordered fog range");
    *out=fog;return true;
}
} // namespace gf::hsd_detail
