#pragma once
// Source45460 vector turn +45BC8 POSITIVE-Y alignment + C79F8 turn block.
// Host bounded math, NOT PPC rsqrt/libm/FMA/FPSCR equivalence. Legacy auxiliary
// entry still refuses active40; this opt-in entry requires source20/2C inputs.
#include <algorithm>
#include "engine/gameplay/startup_auxiliary.hpp"
#include "engine/assets/startup_auxiliary_turn.hpp"
namespace gf {
struct StartupAuxiliaryMotionObservation {bool source_positions_declared=false;std::array<f32,3> current20{},previous2c{};};
namespace startup_auxiliary_turn_detail {
using Vec=std::array<f32,3>;
inline bool vector(const Vec& v){for(auto x:v)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;return true;}
inline f32 dot(const Vec& a,const Vec& b){const f32 x=f32(a[0]*b[0]),y=f32(a[1]*b[1]),z=f32(a[2]*b[2]);return f32(f32(x+y)+z);}
inline Vec cross(const Vec& a,const Vec& b){return {f32(f32(a[1]*b[2])-f32(a[2]*b[1])),f32(f32(a[2]*b[0])-f32(a[0]*b[2])),f32(f32(a[0]*b[1])-f32(a[1]*b[0]))};}
inline bool magnitude(const Vec& v,f32* length){if(!vector(v))return false;f64 sum=0;for(auto x:v)sum+=f64(x)*x;*length=f32(std::sqrt(sum));return std::isfinite(*length)&&*length>1e-12f;}
inline bool normalize(Vec v,Vec* out){f32 length=0;if(!magnitude(v,&length))return false;for(unsigned k=0;k<3;++k)(*out)[k]=f32(v[k]/length);return vector(*out);}
inline bool angle_axis(const StartupAuxiliaryTurnProfile& p,const Vec& from,const Vec& to,f32 limit,f32* angle,Vec* axis){
    f32 a=0,b=0;if(!magnitude(from,&a)||!magnitude(to,&b)||!std::isfinite(limit)||limit<0)return false;
    const f32 denominator=f32(a*b);if(!std::isfinite(denominator)||denominator<=0)return false;
    const f32 cosine=std::clamp(f32(dot(from,to)/denominator),p.cos_min,p.cos_max);*angle=std::min(f32(std::acos(cosine)),limit);*axis=cross(from,to);
    if(!vector(*axis))return false;
    if(dot(*axis,*axis)<=p.cross_epsilon_sq)*axis=p.fallback;
    return true;
}
inline bool rotation(Vec axis,f32 angle,Mat4* out){
    Vec a;if(!normalize(axis,&a)||!std::isfinite(angle))return false;const f32 c=std::cos(angle),s=std::sin(angle),t=f32(1-c);Mat4 r;
    r.at(0,0)=f32(c+t*a[0]*a[0]);r.at(0,1)=f32(t*a[0]*a[1]-s*a[2]);r.at(0,2)=f32(t*a[0]*a[2]+s*a[1]);
    r.at(1,0)=f32(t*a[1]*a[0]+s*a[2]);r.at(1,1)=f32(c+t*a[1]*a[1]);r.at(1,2)=f32(t*a[1]*a[2]-s*a[0]);
    r.at(2,0)=f32(t*a[2]*a[0]-s*a[1]);r.at(2,1)=f32(t*a[2]*a[1]+s*a[0]);r.at(2,2)=f32(c+t*a[2]*a[2]);*out=r;return attachment_transform_detail::affine(r);
}
inline bool turn(const StartupAuxiliaryTurnProfile& p,Vec from,Vec to,f32 limit,Vec* out){
    f32 angle=0;Vec axis;Mat4 r;if(!angle_axis(p,from,to,limit,&angle,&axis)||!rotation(axis,angle,&r))return false;Vec next;mat4_transform_direction(r,from.data(),next.data());if(!vector(next))return false;*out=next;return true;
}
inline bool inverse_direction(const Mat4& basis,Vec world,Vec* local){
    f64 cof[3][3];for(unsigned r=0;r<3;++r)for(unsigned s=0;s<3;++s)cof[r][s]=f64(basis.at((r+1)%3,(s+1)%3))*basis.at((r+2)%3,(s+2)%3)-f64(basis.at((r+1)%3,(s+2)%3))*basis.at((r+2)%3,(s+1)%3);
    const f64 det=f64(basis.at(0,0))*cof[0][0]+f64(basis.at(0,1))*cof[0][1]+f64(basis.at(0,2))*cof[0][2];if(!std::isfinite(det)||std::abs(det)<1e-12)return false;
    Mat4 inverse;for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)inverse.at(r,k)=f32(cof[k][r]/det);
    Vec v;mat4_transform_direction(inverse,world.data(),v.data());if(!vector(v))return false;*local=v;return true;
}
inline bool align_y(const StartupAuxiliaryTurnProfile& p,Mat4 input,Vec target,f32 limit,Mat4* out){
    Vec from={input.at(0,1),input.at(1,1),input.at(2,1)},axis;f32 angle=0;if(!angle_axis(p,from,target,limit,&angle,&axis))return false;
    Mat4 normalized;for(unsigned col=0;col<3;++col){Vec v={input.at(0,col),input.at(1,col),input.at(2,col)},n;if(!normalize(v,&n))return false;for(unsigned row=0;row<3;++row)normalized.at(row,col)=n[row];}
    //800451B8 calls8020B078: full inverse, NOT transpose for a sheared basis.
    Vec local;Mat4 r;if(!inverse_direction(normalized,axis,&local)||!rotation(local,angle,&r))return false;const auto next=mat4_multiply(input,r);if(!attachment_transform_detail::affine(next))return false;*out=next;return true;
}
}
inline bool startup_auxiliary_step_with_turning(const StartupAuxiliaryProfile& p,const StartupAuxiliaryTurnProfile& turn_profile,
                                               unsigned entry,const StartupAuxiliaryParent& parent,const StartupAuxiliaryMotionObservation& motion,
                                               const StartupAuxiliaryState& input,StartupAuxiliaryState* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup auxiliary turning: ")+why;return false;};
    if(!out||!startup_auxiliary_profile_valid(p)||!startup_auxiliary_turn_valid(turn_profile)||entry>1||parent.stored_index>1||input.phase>3||(!input.allocated&&input.phase!=0&&input.phase!=3))return fail("profile/state/parent domain");
    bool has_turn=false;for(const auto& d:p.entries[entry].parts)has_turn|=d.turn!=0;
    if(!has_turn||!parent.active_count||!input.allocated||input.phase==3)return startup_auxiliary_step(p,entry,parent,input,out,error);
    // Active turning callback state prefix. No zeroed turn operands/profile
    // mutation to force the legacy non-turn entry through its refusal.
    auto s=input;
    if(s.phase==0){s.phase=1;s.counter=0;s.cached_a8=s.previous_a9=0;s.render_group=1;}
    else if(s.phase==1){s.counter=std::bit_cast<s16>(u16(u16(s.counter)+1u));s.previous_a9=s.cached_a8;
        if(s.cached_a8){if(!s.matrices_valid)return fail("previous matrices required by history");for(unsigned n=0;n<2;++n){f64 length=0;for(unsigned k=0;k<3;++k){const auto v=s.matrices[n].at(k,1);if(!std::isfinite(v))return fail("history finite");length+=f64(v)*v;}length=std::sqrt(length);if(!std::isfinite(length)||length<1e-12)return fail("history normalization domain");for(unsigned k=0;k<3;++k)s.history[n][k]=f32(s.matrices[n].at(k,1)/length);}}
        s.cached_a8=parent.active_count;
        if(std::bit_cast<s8>(parent.phase18)>1||parent.model94!=s.captured94){s.phase=2;s.render_group=0;}
    }else{s.phase=3;s.render_group=0;}
    using namespace startup_auxiliary_turn_detail;
    if(!motion.source_positions_declared||!vector(motion.current20)||!vector(motion.previous2c))return fail("explicit source actor20/2C observations required");
    Vec movement;for(unsigned k=0;k<3;++k)movement[k]=f32(motion.current20[k]-motion.previous2c[k]);if(!vector(movement))return fail("movement difference domain");
    f64 squared=0;for(auto x:movement)squared+=f64(x)*x;const f32 length=f32(std::sqrt(squared));const bool moving=length>=turn_profile.movement_threshold;Vec direction;
    if(moving&&!normalize(movement,&direction))return fail("movement normalization domain");
    for(unsigned n=0;n<2;++n){const auto& d=p.entries[entry].parts[n];Mat4 m;
        if(parent.kind83==0&&d.bone>=0){if(parent.bones.empty()||parent.bones.size()>32||unsigned(d.bone)>=parent.bones.size())return fail("logical parent bone");m=parent.bones[unsigned(d.bone)];}else m=parent.object_world;
        if(!attachment_transform_detail::affine(m))return fail("parent affine domain");
        m=mat4_multiply(m,mat4_translation(d.position.data()));m=mat4_multiply(m,mat4_rotation_x(f32(d.angle_x)*p.angle_unit));m=mat4_multiply(m,mat4_rotation_z(f32(d.angle_z)*p.angle_unit));
        if(d.turn){Vec current;if(!normalize({m.at(0,1),m.at(1,1),m.at(2,1)},&current))return fail("current axis normalization");
            if(moving){const f32 limit=f32(f32(turn_profile.pi*f32(d.turn))/turn_profile.binary_divisor);if(!turn(turn_profile,current,direction,limit,&current))return fail("movement turn domain");}
            Vec target=current;if(s.previous_a9&&!turn(turn_profile,s.history[n],current,turn_profile.history_step,&target))return fail("retained-history turn domain");
            const f32 limit=f32(turn_profile.orientation_factor*f32(d.turn));if(!align_y(turn_profile,m,target,limit,&m))return fail("source positive-Y inverse/alignment domain");
        }
        const unsigned pulse=((u32(s32(s.counter))+n)>>1)&1u;std::array<f32,3> scale;for(unsigned k=0;k<3;++k)scale[k]=f32(p.pulse[pulse][k]*d.size);m=mat4_multiply(m,mat4_scale(scale.data()));if(!attachment_transform_detail::affine(m))return fail("derived matrix domain");s.matrices[n]=m;
    }
    s.matrices_valid=true;*out=s;if(error)error->clear();return true;
}
} // namespace gf
