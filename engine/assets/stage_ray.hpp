#pragma once
#include <string>
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
struct StageRayParameters {f32 min_x=0,max_x=0,min_z=0,max_z=0,cell_size=0;bool declared=false;};
inline bool stage_ray_parameters_valid(const StageRayParameters& p){return p.declared&&std::isfinite(p.min_x)&&std::isfinite(p.max_x)&&std::isfinite(p.min_z)&&std::isfinite(p.max_z)&&std::abs(p.min_x)<=1e7&&std::abs(p.max_x)<=1e7&&std::abs(p.min_z)<=1e7&&std::abs(p.max_z)<=1e7&&p.min_x<p.max_x&&p.min_z<p.max_z&&f32_to_bits(p.cell_size)==0x43fa0000;}
inline bool stage_ray_parameters_load(ByteView b,StageRayParameters* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="stage ray: parameter extent/domain";return false;};
    if(!out||!b.data||b.size!=48||b.be32(0)!=0x47465352||b.be32(4)!=1||b.be32(8)!=48||b.be32(12)||b.be32(36)||b.be32(40)!=0x80000000u||b.be32(44))return fail();
    StageRayParameters p{b.bef32(16),b.bef32(20),b.bef32(24),b.bef32(28),b.bef32(32),true};if(!stage_ray_parameters_valid(p))return fail();*out=p;if(error)error->clear();return true;
}
} // namespace gf
