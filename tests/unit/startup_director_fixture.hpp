#pragma once
#include "engine/gameplay/startup_director.hpp"
namespace startup_director_fixture {
inline std::vector<gf::u8> encode(const std::vector<gf::u8>& script){
    const unsigned float_at=32,config=52,script_at=88;std::vector<gf::u8> b(script_at,0);unsigned commands=0;
    for(std::size_t at=0;at<script.size();at+=1+gf::kStartupOperandBytes[script[at]])++commands;
    gf::store_be32(b.data(),0x47464452);gf::store_be16(b.data()+4,1);b[6]=2;b[7]=5;gf::store_be16(b.data()+12,gf::u16(commands));gf::store_be16(b.data()+14,8);
    gf::store_be32(b.data()+16,float_at);gf::store_be32(b.data()+20,config);gf::store_be32(b.data()+24,script_at);gf::store_be32(b.data()+28,script.size());
    for(unsigned i=0;i<5;++i)gf::store_be_f32(b.data()+float_at+i*4,float(i));
    gf::store_be_f32(b.data()+config+8,.5f);gf::store_be16(b.data()+84,7);gf::store_be16(b.data()+86,9);
    b.insert(b.end(),script.begin(),script.end());gf::store_be32(b.data()+8,b.size());return b;
}
inline gf::StartupDirectorObservations observations(float end=96){gf::StartupDirectorObservations o;o.camera_end_valid.fill(true);o.camera_end.fill(end);return o;}
} // namespace startup_director_fixture
