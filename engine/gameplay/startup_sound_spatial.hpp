#pragma once
// F036C/F0414/F061C/F06F0 -> F0A74/F0C24/F0D70 + GXProject8022BB74.
// Explicit DECLARED snapshot, bounded host sqrt/inverse/F32 model, not PPC/GX
// hardware proof. Produces scalar sound commands, NOT fulfilled voices/PCM.
#include <algorithm>
#include <span>
#include "engine/gameplay/startup_sound_manager.hpp"
#include "engine/assets/startup_sound_spatial.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
using StartupSoundPosition=std::array<f32,3>;
struct StartupSoundOwnerObservation {
    u64 generation=0,epoch=0;bool live=false,position_declared=false;s8 actor96=0;StartupSoundPosition position20{};
};
struct StartupSoundViewerObservation {
    u64 epoch=0;bool actor_declared=false,position_declared=false,facing_declared=false,camera_declared=false;
    s8 actor2e5=0;StartupSoundPosition position2e8{};Mat4 matrix364,matrix394;
};
struct StartupSoundSpatialSnapshot {
    u64 epoch=0;bool scene_declared=false,bias_selection_declared=false,projection_declared=false;
    std::array<s8,4> viewers0c{};u8 mode29=0,override33=0;s8 bias_viewer28=0;
    std::array<StartupSoundViewerObservation,4> viewers{};
    std::array<f32,7> projection{};std::array<f32,6> viewport{};
};
struct StartupSoundScalarCommand {
    bool update=false; s16 cue=0;u64 owner=0;s8 actor96=0;u8 volume=0,pan=0;u16 pitch=8192;
    bool position_used=false,bias_used=false,reflection_used=false;StartupSoundPosition position{},pan_position{};
    f32 nearest_distance=-1,screen_x=0; // diagnostics, not a free gain/pan input
};
struct StartupSoundSpatialBatch {std::array<StartupSoundScalarCommand,48> commands{};unsigned count=0;};
namespace startup_sound_spatial_detail {
using Vec=StartupSoundPosition;
inline bool bounded(f32 v){return std::isfinite(v)&&std::abs(v)<=1e7f;}
inline bool vector(const Vec& v){for(auto x:v)if(!bounded(x))return false;return true;}
inline f32 dot(const Vec& a,const Vec& b){return f32(f32(f32(a[0]*b[0])+f32(a[1]*b[1]))+f32(a[2]*b[2]));}
inline bool magnitude(const Vec& v,f32* out){if(!vector(v))return false;f64 sum=0;for(auto x:v)sum+=f64(x)*x;*out=f32(std::sqrt(sum));return bounded(*out);}
inline bool normalize(Vec v,Vec* out){f32 length=0;if(!magnitude(v,&length)||length<=1e-12f)return false;for(unsigned k=0;k<3;++k)(*out)[k]=f32(v[k]/length);return vector(*out);}
inline bool affine(const Mat4& m){for(auto x:m.m)if(!bounded(x))return false;return m.at(3,0)==0&&m.at(3,1)==0&&m.at(3,2)==0&&m.at(3,3)==1;}
inline bool point(const Mat4& m,Vec v,Vec* out){
    if(!affine(m)||!vector(v))return false;
    for(unsigned r=0;r<3;++r)(*out)[r]=f32(f32(f32(f32(m.at(r,0)*v[0])+f32(m.at(r,1)*v[1]))+f32(m.at(r,2)*v[2]))+m.at(r,3));
    return vector(*out);
}
inline bool inverse(const Mat4& m,Mat4* out){
    if(!affine(m))return false;
    f64 cof[3][3];for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)cof[r][c]=f64(m.at((r+1)%3,(c+1)%3))*m.at((r+2)%3,(c+2)%3)-f64(m.at((r+1)%3,(c+2)%3))*m.at((r+2)%3,(c+1)%3);
    const f64 det=f64(m.at(0,0))*cof[0][0]+f64(m.at(0,1))*cof[0][1]+f64(m.at(0,2))*cof[0][2];if(!std::isfinite(det)||std::abs(det)<1e-12)return false;
    Mat4 result;for(unsigned r=0;r<3;++r){for(unsigned c=0;c<3;++c)result.at(r,c)=f32(cof[c][r]/det);
        result.at(r,3)=-f32(f32(f32(result.at(r,0)*m.at(0,3))+f32(result.at(r,1)*m.at(1,3)))+f32(result.at(r,2)*m.at(2,3)));
    }
    if(!affine(result))return false;
    *out=result;return true;
}
inline bool viewer(const StartupSoundSpatialSnapshot& s,s8 id,const StartupSoundViewerObservation** out){
    if(id<0||id>=4)return false;
    const auto& v=s.viewers[unsigned(id)];if(!s.epoch||v.epoch!=s.epoch)return false;*out=&v;return true;
}
inline bool classify(const StartupSoundSpatialProfile& p,const StartupSoundSpatialSnapshot& s,Vec position,s8 id,s32* out){
    if(id<0){*out=-1;return true;}
    const StartupSoundViewerObservation* v=nullptr;if(!viewer(s,id,&v)||!v->facing_declared)return false;
    Vec forward,origin;for(unsigned r=0;r<3;++r){forward[r]=v->matrix364.at(r,2);origin[r]=v->matrix364.at(r,3);}
    f32 a=0,b=0;if(!magnitude(forward,&a))return false;
    if(a<=p.degenerate_length){*out=-1;return true;}
    if(!magnitude(origin,&b))return false;
    if(b<=p.degenerate_length){*out=-1;return true;}
    Vec delta;for(unsigned r=0;r<3;++r)delta[r]=f32(position[r]-origin[r]);
    Vec d,n;if(!normalize(delta,&d)||!normalize(forward,&n))return false;
    *out=dot(d,n)<p.zero?1:0;return true;
}
inline bool volume(const StartupSoundSpatialProfile& p,const StartupSoundSpatialSnapshot& s,Vec position,u8* out,f32* nearest){
    if(!s.scene_declared)return false;
    *nearest=p.invalid_distance;if(s.viewers0c[0]<0){*out=127;return true;}
    const unsigned count=s.mode29==0||s.override33==1?1:s.mode29==2?4:2;
    for(unsigned i=0;i<count;++i){s32 facing=0;if(!classify(p,s,position,s.viewers0c[i],&facing))return false;
        f32 distance=p.invalid_distance;
        if(facing>=0){const StartupSoundViewerObservation* v=nullptr;if(!viewer(s,s.viewers0c[i],&v)||!v->position_declared||!vector(v->position2e8))return false;
            Vec delta;for(unsigned k=0;k<3;++k)delta[k]=f32(position[k]-v->position2e8[k]);if(!magnitude(delta,&distance))return false;
        }
        if(distance>p.zero&&(distance<*nearest||*nearest<p.zero))*nearest=distance;
    }
    if(*nearest<p.zero){*out=32;return true;}
    const f32 attenuation=std::min(f32(*nearest/p.distance_unit),p.attenuation_max);
    *out=u8(std::max(32,127-s32(attenuation)));return true;
}
inline bool project_x(const StartupSoundSpatialProfile& p,const StartupSoundSpatialSnapshot& s,const Mat4& camera,Vec position,f32* out){
    if(!s.projection_declared)return false;
    for(auto x:s.projection)if(!bounded(x))return false;
    for(auto x:s.viewport)if(!bounded(x))return false;
    if((s.projection[0]!=p.perspective&&s.projection[0]!=p.one)||s.viewport[2]<=0||s.viewport[3]<=0||s.viewport[2]>8192||s.viewport[3]>8192)return false;
    // GXProject adds Z product to (X product+Y product), then translation.
    Vec c;if(!point(camera,position,&c))return false;
    const auto& q=s.projection;f32 scale=p.one,px,py,pz;
    if(q[0]==p.perspective){if(std::abs(c[2])<=1e-12f)return false;scale=f32(p.one/-c[2]);px=f32(f32(c[0]*q[1])+f32(c[2]*q[2]));py=f32(f32(c[1]*q[3])+f32(c[2]*q[4]));}
    else{px=f32(q[2]+f32(c[0]*q[1]));py=f32(q[4]+f32(c[1]*q[3]));}
    pz=f32(q[6]+f32(c[2]*q[5]));
    const auto& v=s.viewport;
    const f32 x=f32(f32(v[0]+f32(scale*f32(f32(px*v[2])*p.half)))+f32(v[2]*p.half));
    const f32 y=f32(f32(v[1]+f32(scale*f32(f32(-py*v[3])*p.half)))+f32(v[3]*p.half));
    const f32 z=f32(v[5]+f32(scale*f32(pz*f32(v[5]-v[4]))));
    if(!bounded(x)||!bounded(y)||!bounded(z))return false;
    *out=x;return true;
}
inline bool pan(const StartupSoundSpatialProfile& p,const StartupSoundSpatialSnapshot& s,Vec position,StartupSoundScalarCommand* out){
    if(!s.scene_declared)return false;
    if(s.viewers0c[0]<0||(s.mode29!=0&&s.override33!=1)){out->pan=64;return true;}
    const StartupSoundViewerObservation* v=nullptr;if(!viewer(s,s.viewers0c[0],&v)||!v->camera_declared)return false;
    s32 facing=0;if(!classify(p,s,position,s.viewers0c[0],&facing))return false;
    Vec projected=position;
    if(facing==1){Mat4 inv;Vec local;if(!inverse(v->matrix364,&inv)||!point(inv,position,&local))return false;local[2]=-local[2];if(!point(v->matrix364,local,&projected))return false;out->reflection_used=true;}
    if(!project_x(p,s,v->matrix394,projected,&out->screen_x))return false;
    out->pan_position=projected;
    out->pan=out->screen_x<=p.zero?0:out->screen_x>=p.screen_max?127:u8(s32(f32(out->screen_x/p.pan_unit)));
    return true;
}
} // namespace startup_sound_spatial_detail
inline bool startup_sound_spatial_resolve(const StartupSoundSpatialProfile& p,const StartupSoundPendingCall& call,
                                          const StartupSoundSpatialSnapshot& snapshot,std::span<const StartupSoundOwnerObservation> owners,
                                          StartupSoundScalarCommand* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound spatial: ")+why;return false;};
    if(!out||!startup_sound_spatial_profile_valid(p)||call.cue<0||call.cue>=384||unsigned(call.kind)>unsigned(StartupSoundCallKind::PauseCentered)||owners.size()>48)return fail("profile/call/observation budget");
    using namespace startup_sound_spatial_detail;
    const bool biased=call.kind==StartupSoundCallKind::StartBiased||call.kind==StartupSoundCallKind::UpdateBiased;
    if(f32_to_bits(call.viewer_bias)!=(biased?f32_to_bits(p.bias):0u))return fail("pending call bias contract");
    StartupSoundScalarCommand result;result.update=call.kind==StartupSoundCallKind::UpdateDirect||call.kind==StartupSoundCallKind::UpdateBiased||call.kind==StartupSoundCallKind::PauseCentered;result.cue=call.cue;result.owner=call.owner;
    if(!call.owner){if(call.kind==StartupSoundCallKind::PauseCentered)return fail("pause requires live owner");result.volume=127;result.pan=64;*out=result;if(error)error->clear();return true;}
    const StartupSoundOwnerObservation* owner=nullptr;
    for(const auto& candidate:owners)if(candidate.generation==call.owner){if(owner)return fail("ambiguous native generation");owner=&candidate;}
    if(!owner||!owner->live||!snapshot.epoch||owner->epoch!=snapshot.epoch||owner->actor96<0||owner->actor96>=6)return fail("live same-snapshot owner required");
    result.actor96=owner->actor96;
    if(call.kind==StartupSoundCallKind::PauseCentered){result.volume=0;result.pan=64;*out=result;if(error)error->clear();return true;}
    if(!owner->position_declared||!vector(owner->position20))return fail("source actor20 position required");
    result.position=owner->position20;result.position_used=true;
    if(biased){const StartupSoundViewerObservation* v=nullptr;
        if(!snapshot.bias_selection_declared||!viewer(snapshot,snapshot.bias_viewer28,&v)||!v->actor_declared)return fail("bias viewer selector/actor observation required");
        if(v->actor2e5==owner->actor96){if(!v->position_declared||!vector(v->position2e8))return fail("bias viewer position required");
            for(unsigned k=0;k<3;++k)result.position[k]=f32(v->position2e8[k]+f32(p.bias*f32(v->position2e8[k]-owner->position20[k])));
            result.bias_used=true;
        }
    }
    if(!vector(result.position)||!volume(p,snapshot,result.position,&result.volume,&result.nearest_distance)||!pan(p,snapshot,result.position,&result))return fail("viewer/distance/inverse/projection domain");
    *out=result;if(error)error->clear();return true;
}
inline bool startup_sound_spatial_resolve_batch(const StartupSoundSpatialProfile& p,std::span<const StartupSoundPendingCall> calls,
                                                const StartupSoundSpatialSnapshot& snapshot,std::span<const StartupSoundOwnerObservation> owners,
                                                StartupSoundSpatialBatch* out,std::string* error){
    if(out)*out={};
    if(!out||!startup_sound_spatial_profile_valid(p)||calls.size()>48||owners.size()>48){if(error)*error="startup sound spatial batch: profile/budget";return false;}
    StartupSoundSpatialBatch result;
    for(const auto& call:calls){if(!startup_sound_spatial_resolve(p,call,snapshot,owners,&result.commands[result.count],error))return false;++result.count;}
    *out=result;if(error)error->clear();return true;
}
} // namespace gf
