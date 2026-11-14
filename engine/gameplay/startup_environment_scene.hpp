#pragma once
// Source87CF0->87D2C->87F38: normal stff0, colour-mobj stff1, empty stff2.
// No new world transforms. Source8D8E8 multiplies completed material RGB then
// adds wrapper RGB0; alpha passes through. Contributing fog remains guarded.
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/gameplay/startup_scene_lighting.hpp"
namespace gf {
class StartupEnvironmentScene {
public:
    bool load(ByteView raw,const HsdRecordAnimationBank& bank,const StartupScenePassProfile& p,std::string* error){
        ready_=false;normal_={};coloured_={};props_.clear();
        if(bank.hash()!=p.bank_hash||!startup_scene_pass_valid(p))return fail(error,"source stage bank/profile");
        HsdScene full;if(!hsd_load(raw,&full,error)||full.model_roots.size()<3)return false;
        unsigned empty_joints=0;for(const auto& j:full.joints)if(j.model_root==2)++empty_joints;
        if(empty_joints!=1)return fail(error,"source record2 must be the declared single empty root");
        for(auto owner:full.mesh_joint)if(full.joints[owner].model_root==2)return fail(error,"source record2 is not empty; do not silently omit geometry");
        HsdRecordAnimationBinding a,b;if(!bank.select(0,0,0,0,&a,error)||!bank.select(1,0,0,0,&b,error)||!a.objects()->empty()||!b.objects()->empty()||!normal_.load(bank,0,0,0,0,error)||!coloured_.load(bank,1,0,0,0,error))return false;
        if(!normal_.request(0,0,error)||!coloured_.request(0,0,error))return false;
        // Source87924 stage17 ranges, not an archive-wide draw:3..34 ordinary
        // stage props and69..82 static XLU props. Source8784C computes object
        // world/bounds using flag0E;87B64/87C84 register normal models WITHOUT
        // 076D0, so their drawable root retains authored SRT, not that object world.
        for(unsigned record=3;record<=82;++record){if(record>34&&record<69)continue;Prop prop;prop.record=u16(record);if(!prop.model.load(bank,u16(record),0,0,0,error))return false;const auto& root=prop.model.asset()->joints[0];std::array<s16,3> angle;
            for(unsigned k=0;k<3;++k){const f32 value=f32(bits_to_f32(0x4622f983)*root.rotation[k]);if(!std::isfinite(value)||f64(value)<-2147483648.||f64(value)>2147483647.)return fail(error,"source rotation quantization domain");angle[k]=std::bit_cast<s16>(u16(u32(s32(value))));}
            Mat4 world=mat4_translation(root.translation);const f32 unit=bits_to_f32(0x38c90fdb);world=mat4_multiply(world,mat4_rotation_z(f32(angle[2])*unit));world=mat4_multiply(world,mat4_rotation_y(f32(angle[1])*unit));world=mat4_multiply(world,mat4_rotation_x(f32(angle[0])*unit));prop.object_world=world;if(!prop.model.request(0,0,error))return false;prop.model.set_enabled(true);props_.push_back(std::move(prop));}
        normal_.set_enabled(true);coloured_.set_enabled(true);ready_=true;if(error)error->clear();return true;
    }
    bool colour(const StartupSceneLighting& light,std::string* error){
        if(!ready_||!light.ready()||!light.channels()||!light.rig()||light.rig()->lights.size()!=3)return fail(error,"current source light state");
        std::array<u8,3> multiplier={255,255,255};
        if(light.channels()->at(0).active){const auto baseline=light.baseline_colours();for(unsigned k=0;k<3;++k){const u32 b=(baseline[1]>>(24-k*8))&255u;if(!b)return fail(error,"source colour-ratio divisor");const f32 current=f32((light.channels()->at(0).current[1]>>(24-k*8))&255u),value=f32(255.f*f32(current/f32(b)));if(!std::isfinite(value)||value<0||value>255)return fail(error,"source colour-ratio domain");multiplier[k]=u8(u32(value));}}
        coloured_.set_post_rgb_multiply(multiplier);if(error)error->clear();return true;
    }
    unsigned upload_textures_shared(RenderEngine& r,u32 base){if(!ready_)return 0;unsigned n=normal_.upload_textures_shared(r,base)+coloured_.upload_textures_shared(r,base);for(auto& p:props_)n+=p.model.upload_textures_shared(r,base);return n;}
    unsigned draw_model(unsigned model,RenderEngine& r,const Camera& c){return ready_?(model==0?normal_.draw(r,c):model==1?coloured_.draw(r,c):0):0;}
    unsigned draw_prop(unsigned index,RenderEngine& r,const Camera& c){return ready_&&index<props_.size()?props_[index].model.draw(r,c):0;}
    unsigned prop_count()const{return unsigned(props_.size());}u16 prop_record(unsigned i)const{return props_.at(i).record;}
    bool ready()const{return ready_;}
private:
    static bool fail(std::string* e,const char* why){if(e)*e=std::string("startup environment: ")+why;return false;}
    struct Prop {u16 record=0;Mat4 object_world;AnimatedModelScene model;};
    AnimatedModelScene normal_,coloured_;std::vector<Prop> props_;bool ready_=false;
};
} // namespace gf
