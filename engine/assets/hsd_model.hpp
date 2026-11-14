#pragma once
#include "engine/assets/hsd.hpp"
namespace gf {
// Source 80041678 selects model + BE32(model). Character ARC files have a
// prefix with skeleton lookup metadata; they are not bare HSD at byte zero.
// This is an explicit import view, never a first-plausible-header scan.
inline bool hsd_model_archive(ByteView model,ByteView* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("hsd model: ")+why;return false;};
    if(!out||!model.data||model.size<12||model.size>(64u<<20))return fail("model budget/destination");
    const u32 at=model.be32(0);
    if(at<12||(at&3)||at>model.size||model.size-at<32)return fail("declared HSD offset");
    const u32 size=model.be32(at);
    if(size<32||size>model.size-at)return fail("declared HSD extent");
    for(std::size_t i=std::size_t(at)+size;i<model.size;++i)if(model.data[i])return fail("nonzero trailing model data");
    *out={model.data+at,size};return true;
}
} // namespace gf
