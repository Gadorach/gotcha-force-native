#pragma once
#include "engine/gameplay/challenge_battle_spawn.hpp"
namespace challenge_battle_spawn_fixture {
inline void p32(std::vector<gf::u8>& b,unsigned at,gf::u32 x){for(unsigned i=0;i<4;++i)b[at+i]=gf::u8(x>>(24-8*i));}
inline std::vector<gf::u8> grid(bool floor){
    using namespace gf;std::vector<u8> b(108);p32(b,0,0x53544948);p32(b,4,108);p32(b,8,500);p32(b,12,500);p32(b,16,1);p32(b,20,1);p32(b,32,32);p32(b,36,44);p32(b,40,36);p32(b,44,floor?0:0xffffffffu);p32(b,48,0xffffffffu);p32(b,52,7);const f32 values[13]={0,10,0,0,10,500,500,10,0,0,1,0,-10};for(unsigned i=0;i<13;++i)p32(b,56+4*i,f32_to_bits(values[i]));return b;
}
inline std::vector<gf::u8> profile_bytes(){
    using namespace gf;std::vector<u8> b(132);p32(b,0,0x47464357);p32(b,4,1);b[9]=1;b[11]=68;p32(b,12,68);const f32 values[4]={5000,2000,1000,3000};for(unsigned i=0;i<4;++i)p32(b,16+4*i,f32_to_bits(values[i]));p32(b,64,7);for(unsigned t=0;t<2;++t)for(unsigned c=0;c<4;++c){p32(b,68+(t*8+c*2)*4,f32_to_bits(f32(50+20*c+50*t)));p32(b,72+(t*8+c*2)*4,f32_to_bits(50));}return b;
}
inline gf::ChallengeDeclaredTeamInput teams(unsigned mask){
    gf::ChallengeDeclaredTeamInput out;out.humans=gf::u8(mask);unsigned allies=0,enemies=0;for(unsigned i=0;i<4;++i)allies+=(mask>>i)&1u;for(unsigned slot=0;slot<6;++slot){if(mask&(1u<<slot))out.teams[slot]=0;else if(allies<2){out.teams[slot]=0;++allies;}else if(enemies<2){out.teams[slot]=1;++enemies;}if(out.teams[slot]>=0)out.active|=gf::u8(1u<<slot);}return out;
}
} // namespace challenge_battle_spawn_fixture
