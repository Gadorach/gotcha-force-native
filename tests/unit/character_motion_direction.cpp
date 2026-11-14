#include <bit>
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/character_motion_direction.hpp"
namespace {
using namespace gf;unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"motion direction: %s\n",why);std::abort();}}
bool target(const CharacterMotionTarget& actual,u8 group,u8 index){return actual.group==group&&actual.index==index;}
}
int main(){using namespace gf;const std::array<u8,6> controls={3,5,7,11,13,17};
    for(s32 value=-32768;value<=32767;++value){const auto result=character_motion_direction(controls,s16(value));
        const s32 magnitude=value<0?-value:value;const unsigned side=value<0?2:0;
        check(target(result.centre,5,13)&&target(result.side,controls[side],controls[side+3])&&
              f64(result.side_weight)==f64(magnitude)/16384.0,"all signed steering values, exact rational weight and target identity");
    }
    for(unsigned slot=0;slot<6;++slot)for(unsigned byte=0;byte<256;++byte)for(s16 steering:std::array<s16,5>{-32768,-1,0,1,32767}){
        auto input=controls;input[slot]=u8(byte);const auto original=input;const auto result=character_motion_direction(input,steering);const unsigned side=steering<0?2:0;
        check(input==original&&target(result.centre,input[1],input[4])&&target(result.side,input[side],input[side+3]),"all byte slots preserve group/index roles and immutable input; no invented clip guard");
    }
    const auto zero=character_motion_direction(controls,0),minimum=character_motion_direction(controls,-32768);
    check(target(zero.side,3,11)&&std::bit_cast<u32>(zero.side_weight)==0,"zero selects nonnegative side with positive zero");
    check(minimum.side_weight==2&&target(minimum.side,7,17),"signed minimum never overflows or clamps");
    const std::array<u8,6> identical={255,255,255,255,255,255};const auto alias=character_motion_direction(identical,-16384);
    check(target(alias.centre,255,255)&&target(alias.side,255,255)&&alias.side_weight==1,"numeric selector preserves identical and unresolved targets");
    std::printf("directional target selector: %u checks PASS (65536 numeric steering values, not actors)\n",checks);return 0;
}
