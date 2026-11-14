#pragma once
// Archive-bound native menu controller content, exported offline from the
// original root/angle tables. No guest memory or executable code is loaded.
#include "engine/assets/hsd.hpp"
namespace gf {
inline bool menu_motion_load(ByteView file,u32 archive_hash,const HsdScene& scene,
                             HsdScene::MenuMotion* out,std::string* error){
    if(!out){if(error)*error="menu motion: missing output";return false;}
    *out={};
    auto fail=[&](const char* why){if(error)*error=std::string("menu motion: ")+why;return false;};
    if(!file.data||file.size!=172)return fail("invalid content size");
    std::size_t at=0;
    auto word=[&](){const u32 x=hsd_detail::be32(file,at);at+=4;return x;};
    auto signed_word=[&](){const u32 x=word();return x<=0x7FFFFFFFu?s32(x):s32(s64(x)-0x100000000ll);};
    if(word()!=0x47464D56u||word()!=1||word()!=archive_hash||word()!=7)return fail("magic/version/archive/count mismatch");
    HsdScene::MenuMotion result;result.present=true;result.angle_unit=bits_to_f32(word());
    result.selector_step=signed_word();result.identifier_step=signed_word();result.selector_joint=word();
    if(!std::isfinite(result.angle_unit)||std::fabs(result.angle_unit*65536.0f-6.28318530718f)>0.001f)
        return fail("invalid angle unit");
    auto step_ok=[](s32 n){return n>=-32768&&n<=32767;};
    if(!step_ok(result.selector_step)||!step_ok(result.identifier_step))return fail("angle step out of range");
    auto root=[&](u32 descriptor){
        unsigned count=0;
        for(const auto& j:scene.joints)if(j.archive_offset==descriptor&&j.parent==kHsdNoJoint)++count;
        return count==1;
    };
    if(!root(result.selector_joint))return fail("selector root missing or ambiguous");
    std::vector<u32> used{result.selector_joint};
    for(unsigned i=0;i<7;++i){
        HsdScene::MenuMotion::Binding b;b.item=word();b.gear=word();b.identifier=word();b.caption=word();b.step=signed_word();
        if(b.item>=7||!step_ok(b.step))return fail("invalid item or step");
        for(const auto& old:result.bindings)if(old.item==b.item)return fail("duplicate item");
        for(u32 descriptor:{b.gear,b.identifier,b.caption}){
            if(!root(descriptor)||std::find(used.begin(),used.end(),descriptor)!=used.end())return fail("missing/ambiguous/reused model root");
            used.push_back(descriptor);
        }
        result.bindings.push_back(b);
    }
    *out=std::move(result);return true;
}
} // namespace gf
