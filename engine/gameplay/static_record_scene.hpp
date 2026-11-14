#pragma once
#include <map>
#include "engine/gameplay/animated_model_scene.hpp"
namespace gf {
struct StaticRecordAppearance {f32 alpha=1;std::array<u8,3> multiply{255,255,255};};
// Shared editor/setup static-record path. The caller owns the bank, camera,
// texture namespace and cache; clear that cache when rebinding any of them.
// No animation clock, source placement policy or inventory mutation lives here.
inline bool draw_static_record(RenderEngine& render,const HsdRecordAnimationBank& bank,std::map<u16,AnimatedModelScene>& cache,u16 id,const Mat4& world,const Camera& camera,u32 texture_base,std::string* error,bool always=false,bool compact=true,const StaticRecordAppearance* appearance=nullptr){
    auto found=cache.find(id);if(found==cache.end()){AnimatedModelScene instance;if(!(compact?instance.load_compact_static(bank,id,error):instance.load(bank,id,0,0,0,error))||!instance.request(0,0,error))return false;instance.set_enabled(true);instance.set_untextured_billboards(true);instance.upload_textures_shared(render,texture_base);found=cache.emplace(id,std::move(instance)).first;}
    if(appearance){if(!found->second.replace_static_alpha(appearance->alpha,error))return false;found->second.set_post_rgb_multiply(appearance->multiply);}
    if(always){if(!found->second.set_depth_compare(7))return false;}else found->second.clear_depth_compare();
    if(!found->second.set_root_world(world,error))return false;
    found->second.draw(render,camera);return true;
}
} // namespace gf
