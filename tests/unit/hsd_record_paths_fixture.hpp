#pragma once
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace hsd_record_paths_fixture {
struct Fixture:hsd_record_animation_fixture::Fixture {
    Fixture(){
        ptr(0x14c,0xe00);word(0xe04,0x4000);for(unsigned k=0;k<3;++k)real(0xe20+k*4,1);ptr(0xe10,0xe40);
        ptr(0x70c,0xe00);block[0x74c]=4;stream(0x780,0,1);
        gf::store_be16(block.data()+0xe42,2);real(0xe4c,10);ptr(0xe48,0xe60);ptr(0xe50,0xe90);ptr(0xe54,0xea0);
        real(0xe6c,10);real(0xe94,1);real(0xeb0,100);
    }
};
} // namespace hsd_record_paths_fixture
