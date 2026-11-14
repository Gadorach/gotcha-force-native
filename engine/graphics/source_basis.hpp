#pragma once
//45918/45EF4 six source axis-basis recipes. Shared numerical operations only;
//not a renderer, actor authority or PS/frsqrte/FPSCR hardware implementation.
#include <algorithm>
#include <span>
#include "engine/graphics/source_intersection.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct SourceBasisTrace {u8 mode=0,fallback_mode=0;bool fallback=false,retained_basis=false;};
namespace source_basis_detail {
using Vec=source_intersection_detail::Vec;
inline Vec column(const Mat4& m,unsigned c){return {m.at(0,c),m.at(1,c),m.at(2,c)};}
inline bool number(f32 x){return std::isfinite(x)&&std::abs(x)<=1e7f;}
inline f32 length(const Vec& v){return f32(std::sqrt(f64(source_intersection_detail::dot(v,v))));}
inline bool normalize(Vec* v){const f32 n=length(*v);if(!std::isfinite(n)||n<=0)return false;for(auto& x:*v)x=f32(x/n);return source_intersection_detail::valid(*v);}
inline bool basis(u8 mode,const Vec& primary,const Vec& hint,f32 threshold,Mat4* m,bool* built){
    using namespace source_intersection_detail;*built=false;std::array<Vec,3> cols;unsigned cross_col=0;
    switch(mode){
    case 0:cols[0]=primary;cols[1]=hint;cols[2]=cross(cols[0],cols[1]);cross_col=2;break;
    case 1:cols[0]=primary;cols[2]=hint;cols[1]=cross(cols[2],cols[0]);cross_col=1;break;
    case 2:cols[1]=primary;cols[0]=hint;cols[2]=cross(cols[0],cols[1]);cross_col=2;break;
    case 3:cols[1]=primary;cols[2]=hint;cols[0]=cross(cols[1],cols[2]);cross_col=0;break;
    case 4:cols[2]=primary;cols[0]=hint;cols[1]=cross(cols[2],cols[0]);cross_col=1;break;
    case 5:cols[2]=primary;cols[1]=hint;cols[0]=cross(cols[1],cols[2]);cross_col=0;break;
    default:return false;
    }
    const f32 squared=dot(cols[cross_col],cols[cross_col]);if(!std::isfinite(squared))return false;
    if(!(squared>threshold))return true; //source returns1 BEFORE matrix writes
    switch(mode){case 0:cols[1]=cross(cols[2],cols[0]);break;case 1:cols[2]=cross(cols[0],cols[1]);break;case 2:cols[0]=cross(cols[1],cols[2]);break;case 3:cols[2]=cross(cols[0],cols[1]);break;case 4:cols[0]=cross(cols[1],cols[2]);break;case 5:cols[1]=cross(cols[2],cols[0]);break;default:return false;}
    for(auto& col:cols)if(!normalize(&col))return false;
    for(unsigned c=0;c<3;++c)for(unsigned r=0;r<3;++r)m->at(r,c)=cols[c][r];
    *built=true;return true;
}
}
//Keeps source T and original column LENGTHS (not determinant/reflection signs).
//The fallback uses the next source column ONCE; if both attempts fail, source
//still post-scales the retained basis. Do not invent a universal fallback axis.
inline bool source_basis_align(Mat4 input,const std::array<f32,3>& direction,s8 primary_axis,s8 hint_axis,std::span<const u8,9> modes,f32 threshold,Mat4* out,SourceBasisTrace* trace=nullptr){
    if(trace)*trace={};
    if(!out||!source_intersection_detail::valid(direction)||!std::isfinite(threshold)||threshold<0)return false;
    for(f32 x:input.m)if(!source_basis_detail::number(x))return false;
    if(input.at(3,0)!=0||input.at(3,1)!=0||input.at(3,2)!=0||input.at(3,3)!=1)return false;
    int a=primary_axis,b=hint_axis;if(a==0||a< -3||a>3||b==0||b< -3||b>3)return false;
    const std::array<f32,3> lengths{source_basis_detail::length(source_basis_detail::column(input,0)),source_basis_detail::length(source_basis_detail::column(input,1)),source_basis_detail::length(source_basis_detail::column(input,2))};
    auto target=direction;auto hint=source_basis_detail::column(input,unsigned(std::abs(b)-1));if(a<0){for(auto& x:target)x=-x;a=-a;}if(b<0){for(auto& x:hint)x=-x;b=-b;}
    SourceBasisTrace t;auto result=input;bool built=false;t.mode=modes[unsigned((a-1)*3+b-1)];if(!source_basis_detail::basis(t.mode,target,hint,threshold,&result,&built))return false;
    if(!built){t.fallback=true;b=b==3?1:b+1;hint=source_basis_detail::column(input,unsigned(b-1));t.fallback_mode=modes[unsigned((a-1)*3+b-1)];if(!source_basis_detail::basis(t.fallback_mode,target,hint,threshold,&result,&built))return false;}
    t.retained_basis=!built;result=mat4_multiply(result,mat4_scale(lengths.data()));for(f32 x:result.m)if(!source_basis_detail::number(x))return false;
    *out=result;if(trace)*trace=t;return true;
}
} // namespace gf
