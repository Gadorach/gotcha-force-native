#pragma once
// Source80005AF4/05FE0: unit-interval natural cubic through supplied history.
// No path fitting, resampling clock, actor inference or renderer dependency.
#include <array>
#include <cmath>
#include <span>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
using HistoryPoint=std::array<f32,3>;
struct HistoryCubic {std::array<HistoryPoint,4> coefficients{};}; // a,b,c,d
inline bool history_natural_cubic(std::span<const HistoryPoint> points,std::vector<HistoryCubic>* out){
    if(out)out->clear();
    if(!out||points.size()<2||points.size()>16)return false;
    for(const auto& p:points)for(f32 x:p)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;
    const unsigned n=unsigned(points.size());std::array<HistoryPoint,16> derivatives{};
    for(unsigned k=0;k<3;++k){std::array<f32,16> rhs{},gamma{},q{};rhs[0]=f32(3.f*f32(points[1][k]-points[0][k]));rhs[n-1]=f32(3.f*f32(points[n-1][k]-points[n-2][k]));
        for(unsigned i=1;i+1<n;++i)rhs[i]=f32(3.f*f32(f32(points[i][k]-points[i-1][k])+f32(points[i+1][k]-points[i][k])));
        f32 pivot=2;gamma[0]=.5f;q[0]=f32(rhs[0]/pivot);
        for(unsigned i=1;i<n;++i){pivot=std::fma(-1.f,gamma[i-1],i+1==n?2.f:4.f);if(!std::isfinite(pivot)||pivot==0)return false;gamma[i]=f32(1.f/pivot);q[i]=f32(std::fma(-1.f,q[i-1],rhs[i])/pivot);}
        derivatives[n-1][k]=q[n-1];for(unsigned i=n-1;i--;)derivatives[i][k]=std::fma(-gamma[i],derivatives[i+1][k],q[i]);
    }
    std::vector<HistoryCubic> result(n-1);
    for(unsigned i=0;i+1<n;++i)for(unsigned k=0;k<3;++k){auto& c=result[i].coefficients;c[0][k]=f32(f32(2.f*f32(points[i][k]-points[i+1][k]))+f32(derivatives[i][k]+derivatives[i+1][k]));c[1][k]=f32(f32(3.f*f32(points[i+1][k]-points[i][k]))-std::fma(2.f,derivatives[i][k],derivatives[i+1][k]));c[2][k]=derivatives[i][k];c[3][k]=points[i][k];for(const auto& term:c)if(!std::isfinite(term[k])||std::abs(term[k])>1e9f)return false;}
    *out=std::move(result);return true;
}
inline HistoryPoint history_cubic_sample(const HistoryCubic& c,f32 u){HistoryPoint p{};for(unsigned k=0;k<3;++k)p[k]=std::fma(u,std::fma(u,std::fma(u,c.coefficients[0][k],c.coefficients[1][k]),c.coefficients[2][k]),c.coefficients[3][k]);return p;}
} // namespace gf
