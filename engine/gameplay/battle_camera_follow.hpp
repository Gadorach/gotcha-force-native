#pragma once
#include "engine/assets/battle_camera_follow.hpp"
#include "engine/gameplay/battle_camera_pose.hpp"
#include "engine/core/source_heading.hpp"
namespace gf {
struct BattleCameraFollowInput {
    BattleFirstCameraInput actor;std::array<f32,3> target50c{};f32 width668=0;
    bool target508_bound=false;u8 lock7c9=0;
};
struct BattleCameraFollowState {
    std::array<f32,3> eye{},target{},horizontal318{};
    f32 distance348=0,height350=0,scale354=0;std::array<s16,3> angles70{};
    u8 extended35e=0;
};
struct BattleCameraFollowPlan {
    BattleCameraFollowState state;std::array<f32,3> previous_eye{},previous_target{},height_probe_eye{},up{};
    Mat4 world114,view394;s16 actor5b0=0;
    f32 minimum=0,maximum=0,pitch_factor=0,pre_clamp_distance=0;
    bool direction_fallback=false,angle_fallback=false,pitch_clamped=false,radius_clamped=false;
};
namespace battle_camera_follow_detail {
using namespace source_intersection_detail;
inline bool normalize(Vec& v){const auto squared=dot(v,v);if(!std::isfinite(squared)||squared<=0)return false;const auto n=f32(std::sqrt(squared));for(auto& x:v)x=f32(x/n);return valid(v);}
inline bool fallback(Vec& v,s16 yaw){const f32 e=bits_to_f32(0x3a83126f);if(dot(v,v)>f32(e*e))return false;v={source_angle_sin(yaw),0,source_angle_cos(yaw)};return true;}
inline bool eye_from_orbit(BattleCameraFollowState& s){auto v=sub(s.horizontal318,s.target);if(!normalize(v))return false;v=scale(v,s.distance348);for(unsigned k=0;k<3;++k)s.eye[k]=f32(s.target[k]+v[k]);return valid(s.eye);}
inline bool contract_orbit(BattleCameraFollowState& s){
    auto v=sub(s.horizontal318,s.target);v[1]=0;if(!normalize(v))return false;v=scale(v,s.distance348);
    std::array<f32,3> desired{f32(s.target[0]+v[0]),std::fma(s.distance348,source_angle_tan(s.angles70[0]),s.target[1]),f32(s.target[2]+v[2])};
    for(unsigned k=0;k<3;++k)s.horizontal318[k]=f32(std::fma(3.f,s.horizontal318[k],desired[k])*.25f);
    return valid(s.horizontal318)&&eye_from_orbit(s);
}
}
//Numeric0C918→CB8C(0→2)→CDC0 first-follow slice. The native owner performs
//10664's actual source-floor query at height_probe_eye before committing.
inline bool battle_camera_first_follow(const BattleCameraFollowProfile& profile,const BattleCameraFollowInput& in,const BattleCameraFollowState& before,BattleCameraFollowPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("camera first follow: ")+why;return false;};
    using namespace battle_camera_follow_detail;
    if(!out||!profile.ready()||in.actor.group582||in.actor.layout29>2||in.actor.split3d>1||in.target508_bound||in.lock7c9||before.extended35e||before.angles70[0]!=1536||before.angles70[2]||!valid(in.actor.actor_position)||!valid(in.target50c)||!valid(before.eye)||!valid(before.target)||!valid(before.horizontal318))return fail("fresh ordinary unbound-target camera branch required");
    for(f32 x:{in.actor.offset6d0,in.actor.scale_b4,in.width668,in.actor.values88c[0],in.actor.values88c[2],in.actor.values88c[3],before.distance348,before.height350})if(!std::isfinite(x)||std::abs(x)>1e7f)return fail("numeric operand domain");
    if(in.actor.scale_b4<=0||in.width668<=0||in.actor.values88c[2]<=0||in.actor.values88c[3]<in.actor.values88c[2])return fail("source radius/width bounds");
    BattleCameraFollowPlan p;p.state=before;p.previous_eye=before.eye;p.previous_target=before.target;auto& s=p.state;
    s.scale354=std::fma(.5f,f32(in.actor.scale_b4-1),1.f);
    //FC2C: retain the source target-Y addition order.
    s.height350=f32(std::fma(in.actor.values88c[0],s.scale354,s.height350)*.5f);
    s.target=in.actor.actor_position;s.target[1]=f32(f32(in.actor.offset6d0+f32(s.height350+f32(p.previous_target[1]+in.actor.actor_position[1])))*.5f);
    auto delta=sub(s.horizontal318,in.actor.actor_position);delta[1]=0;const auto separation=dot(delta,delta);
    p.minimum=f32(in.actor.values88c[2]*s.scale354);p.maximum=f32(in.actor.values88c[3]*s.scale354);
    if((in.actor.layout29==1&&!in.actor.split3d)||in.actor.layout29==2){p.minimum=f32(p.minimum*bits_to_f32(0x3fa66666));p.maximum=f32(p.maximum*bits_to_f32(0x3fa66666));}
    if(!std::isfinite(separation)||!std::isfinite(p.maximum)||p.maximum>1e7f||p.minimum<=0)return fail("derived radius domain");
    s.distance348=separation>f32(p.maximum*p.maximum)?p.maximum:separation<f32(p.minimum*p.minimum)?p.minimum:f32(std::sqrt(separation));s.extended35e=0;
    //CDC0: horizontal and vertical smoothing use different source divisors.
    s.angles70[0]=1536;delta=sub(s.horizontal318,s.target);delta[1]=0;p.direction_fallback=fallback(delta,s.angles70[1]);
    if(!p.direction_fallback&&!normalize(delta))return fail("horizontal direction normalization");
    delta=scale(delta,s.distance348);
    s.horizontal318[0]=f32(std::fma(4.f,s.horizontal318[0],f32(s.target[0]+delta[0]))/5.f);
    s.horizontal318[1]=f32(std::fma(9.f,s.horizontal318[1],std::fma(s.distance348,source_angle_tan(1536),s.target[1]))/10.f);
    s.horizontal318[2]=f32(std::fma(4.f,s.horizontal318[2],f32(s.target[2]+delta[2]))/5.f);
    if(!eye_from_orbit(s))return fail("source orbit normalization");
    //101C8 reads50C even with a null508 relationship.
    delta=sub(s.target,s.horizontal318);p.angle_fallback=fallback(delta,s.angles70[1]);
    const f32 horizontal=f32(f32(delta[0]*delta[0])+f32(delta[2]*delta[2]));
    if(!std::isfinite(horizontal)||horizontal<0)return fail("source pitch distance");
    s.distance348=f32(std::sqrt(horizontal));
    if(!source_heading_s16(-delta[1],s.distance348,&s.angles70[0])||!source_heading_s16(delta[0],delta[2],&s.angles70[1]))return fail("source signed angle conversion");
    const auto target_delta=sub(in.target50c,s.target);const auto target_squared=dot(target_delta,target_delta);if(!std::isfinite(target_squared)||target_squared<0)return fail("source50C distance");
    const f32 factor=f32(bits_to_f32(0x3e99999a)*f32(f32(1.5f*f32(std::sqrt(target_squared)))/in.width668));
    p.pitch_factor=std::clamp(factor,bits_to_f32(0x3e99999a),1.f);
    const auto limit=s16(s32(f32(12288.f*p.pitch_factor)));const auto low=s16(s32(f32(-12288.f*p.pitch_factor)));
    const auto old_pitch=s.angles70[0];s.angles70[0]=std::clamp(old_pitch,low,limit);p.pitch_clamped=old_pitch!=s.angles70[0];
    if(p.pitch_clamped&&!contract_orbit(s))return fail("source pitch clamp geometry");
    p.actor5b0=std::bit_cast<s16>(u16(u32(std::bit_cast<u16>(s.angles70[1]))+0x8000u));
    //1063C/10664 observe this eye before FFFC's radius contraction.
    p.height_probe_eye=s.eye;p.pre_clamp_distance=s.distance348;
    if(s.distance348<p.minimum||s.distance348>p.maximum){s.distance348=std::clamp(s.distance348,p.minimum,p.maximum);p.radius_clamped=true;if(!contract_orbit(s))return fail("source radius clamp geometry");}
    if(!valid(s.eye)||!valid(s.target)||!valid(s.horizontal318)||!battle_camera_pose_detail::basis(s.eye,s.target,&p.up,&p.world114,&p.view394,error))return fail("final source view basis");
    *out=p;if(error)error->clear();return true;
}
}
