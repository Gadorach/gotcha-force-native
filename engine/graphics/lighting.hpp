#pragma once
// Portable vertex lighting for the menu's declared ambient/infinite HSD rig.
// No platform or guest state. Point/spot rigs are explicitly unsupported here.
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "engine/core/types.hpp"

namespace gf {
struct SceneLight {
    u16 flags = 0; // type: ambient=0, infinite=1; diffuse=4, specular=8, alpha=16, hidden=32
    u8 colour[4] = {};
    f32 position[3] = {}; // infinite diffuse uses position * 2^20, as HSD does
    f32 interest[3] = {};
};
struct SceneLighting {
    bool declared = false;
    bool supported = true;
    bool animated = false; // archive contains animation, not automatically playing
    std::vector<SceneLight> lights;
};
struct LitChannels {
    u8 diffuse[4] = {255,255,255,255};
    u8 specular[3] = {};
};
inline bool scene_lighting_equal(const SceneLighting& a,const SceneLighting& b){
    if(a.declared!=b.declared||a.supported!=b.supported||a.animated!=b.animated||a.lights.size()!=b.lights.size())return false;
    for(unsigned i=0;i<a.lights.size();++i){const auto& x=a.lights[i];const auto& y=b.lights[i];if(x.flags!=y.flags)return false;
        for(unsigned k=0;k<4;++k)if(x.colour[k]!=y.colour[k])return false;
        for(unsigned k=0;k<3;++k)if(f32_to_bits(x.position[k])!=f32_to_bits(y.position[k])||f32_to_bits(x.interest[k])!=f32_to_bits(y.interest[k]))return false;
    }return true;
}
inline bool menu_light_rig_supported(const SceneLighting& rig) {
    if(!rig.supported||rig.lights.size()>256)return false;
    unsigned slots=0;
    for(const auto& light:rig.lights){
        if((light.flags&3u)>1 || (light.flags&~0x3fu))return false;
        f32 length2=0;
        for(unsigned k=0;k<3;++k){
            if(!std::isfinite(light.position[k])||!std::isfinite(light.interest[k])||
               std::fabs(light.position[k])>1e7f||std::fabs(light.interest[k])>1e7f)return false;
            length2+=light.position[k]*light.position[k];
        }
        if((light.flags&3u)==1){
            if(!(length2>0))return false;
            if(!(light.flags&32u)){slots+=(light.flags&4u)!=0;slots+=(light.flags&8u)!=0;}
        }
    }
    return slots<=8;
}
namespace lighting_detail {
inline bool normalize(f32 v[3]) {
    const f32 length=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    if(!std::isfinite(length)||length<=1e-12f){v[0]=v[1]=v[2]=0;return false;}
    for(unsigned k=0;k<3;++k)v[k]/=length;
    return true;
}
inline f32 dot(const f32 a[3],const f32 b[3]) {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline u8 byte(f32 x) {return static_cast<u8>(std::clamp(x,0.0f,255.0f)+0.5f);}
// Per-mesh scratch, rebuilt from the same rig/origin/view before vertex shading.
// It owns its vectors; no asset lifetime or cross-frame identity is inferred.
struct PreparedLightView {
    std::vector<std::array<f32,3>> half_vectors;
    void prepare(const SceneLighting& rig,const f32 origin[3],const f32 eye[3]){
        half_vectors.resize(rig.lights.size());
        f32 view[3];for(unsigned k=0;k<3;++k)view[k]=eye[k]-origin[k];normalize(view);
        for(unsigned i=0;i<rig.lights.size();++i){const auto& light=rig.lights[i];
            if((light.flags&32u)||(light.flags&3u)!=1||!(light.flags&8u))continue;
            f32 towards[3];for(unsigned k=0;k<3;++k)towards[k]=light.position[k]-light.interest[k];normalize(towards);
            auto& half=half_vectors[i];for(unsigned k=0;k<3;++k)half[k]=towards[k]+view[k];normalize(half.data());
        }
    }
};
}
// GX's specular attenuation is rational, NOT pow(N.H, shininess). HSD builds
// the half-vector at the joint origin, not independently at every vertex.
inline LitChannels light_vertex_prepared(const SceneLighting& rig,const f32 position[3],
                               const f32 normal[3],const f32 joint_origin[3],const f32 eye[3],
                               const u8 ambient[4],const u8 specular[4],f32 shininess,
                               bool diffuse_enabled,bool specular_enabled,
                               const lighting_detail::PreparedLightView* prepared) {
    using namespace lighting_detail;
    LitChannels result;
    f32 illumination[4]={},highlight[3]={};
    bool alpha_lit=false,ambient_found=false;
    for(const auto& light:rig.lights) {
        if(light.flags&32u)continue;
        const unsigned type=light.flags&3u;
        if(type==0){
            if(ambient_found)continue; // HSD selects the first active ambient
            ambient_found=true;
            if(light.flags&4u)for(unsigned k=0;k<3;++k)
                illumination[k]=std::floor(f32(ambient[k])*light.colour[k]/255.0f);
            if(light.flags&16u){illumination[3]=light.colour[3];alpha_lit=true;}
        }
    }
    f32 view[3]={};if(!prepared){for(unsigned k=0;k<3;++k)view[k]=eye[k]-joint_origin[k];normalize(view);}
    for(unsigned i=0;i<rig.lights.size();++i) {const auto& light=rig.lights[i];
        if((light.flags&32u)||(light.flags&3u)!=1)continue;
        f32 direction[3];for(unsigned k=0;k<3;++k)direction[k]=light.position[k]*1048576.0f-position[k];
        if(!normalize(direction))continue;
        const f32 ndotl=std::max(0.0f,dot(normal,direction));
        if(light.flags&4u)for(unsigned k=0;k<3;++k)
            illumination[k]+=std::round(light.colour[k]*ndotl);
        if((light.flags&0x14u)==0x14u){illumination[3]+=std::round(light.colour[3]*ndotl);alpha_lit=true;}
        if(specular_enabled && (light.flags&8u) && ndotl>0){
            f32 half[3];
            if(prepared)std::copy_n(prepared->half_vectors[i].data(),3,half);
            else{f32 towards[3];
                for(unsigned k=0;k<3;++k)towards[k]=light.position[k]-light.interest[k];
                normalize(towards);
                for(unsigned k=0;k<3;++k)half[k]=towards[k]+view[k];
                normalize(half);
            }
            const f32 cosine=std::clamp(dot(normal,half),0.0f,1.0f),square=cosine*cosine;
            const f32 k0=shininess*0.5f,denominator=square+k0*(1.0f-square);
            const f32 strength=denominator>0?square/denominator:0;
            for(unsigned k=0;k<3;++k)highlight[k]+=std::round(light.colour[k]*strength);
        }
    }
    if(diffuse_enabled){
        for(unsigned k=0;k<3;++k)result.diffuse[k]=byte(illumination[k]);
        result.diffuse[3]=alpha_lit?byte(illumination[3]):255;
    }
    if(specular_enabled)for(unsigned k=0;k<3;++k)
        result.specular[k]=byte(std::min(255.0f,highlight[k])*specular[k]/255.0f);
    return result;
}
inline LitChannels light_vertex(const SceneLighting& rig,const f32 position[3],
                               const f32 normal[3],const f32 joint_origin[3],const f32 eye[3],
                               const u8 ambient[4],const u8 specular[4],f32 shininess,
                               bool diffuse_enabled,bool specular_enabled){
    return light_vertex_prepared(rig,position,normal,joint_origin,eye,ambient,specular,shininess,diffuse_enabled,specular_enabled,nullptr);
}
} // namespace gf
