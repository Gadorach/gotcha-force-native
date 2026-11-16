#pragma once
#include <cmath>
#include <initializer_list>
#include "engine/core/types.hpp"
namespace gf {
struct BattlePoint {f32 x=0,z=0;friend bool operator==(const BattlePoint&,const BattlePoint&)=default;};
//Shared swept point/expanded-circle collision, including initial overlap.
inline bool battle_swept_hit(BattlePoint start,BattlePoint delta,BattlePoint center,f32 radius,f32* fraction){
    if(!fraction||!std::isfinite(radius)||radius<=0)return false;
    for(f32 v:{start.x,start.z,delta.x,delta.z,center.x,center.z})if(!std::isfinite(v)||std::abs(v)>1e6f)return false;
    const f64 x=f64(start.x)-center.x,z=f64(start.z)-center.z,dx=delta.x,dz=delta.z;
    const f64 c=x*x+z*z-f64(radius)*radius;if(c<=0){*fraction=0;return true;}
    const f64 a=dx*dx+dz*dz;if(a==0)return false;
    const f64 b=x*dx+z*dz,disc=b*b-a*c;if(disc<0)return false;
    const f64 t=(-b-std::sqrt(disc))/a;if(t<0||t>1)return false;*fraction=f32(t);return true;
}
}
