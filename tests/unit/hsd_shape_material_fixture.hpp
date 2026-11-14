#pragma once
#include "tests/unit/hsd_shape_fixture.hpp"
namespace hsd_shape_material_fixture {
inline auto fixture(){
    auto f=hsd_shape_fixture::fixture();
    f.ptr(0x204,0x210);f.ptr(0x21c,0x500);f.ptr(0x508,0x400);f.word(0x50c,0x90000003);f.ptr(0x510,0x480);f.ptr(0x514,0xd60);
    f.ptr(0xed0,0xf90);f.ptr(0xf94,0xee0);
    f.ptr(0x38,0x60);f.word(0x800,0);f.fields.erase(std::remove(f.fields.begin(),f.fields.end(),0x800u),f.fields.end());f.ptr(0x900,0x920);
    f.stream(0xc00,0,1);f.stream(0xc20,0,1);
    return f;
}
}
