#pragma once
// Source1AB6C/1ADBC/1AF94/1B0C8/1B308. Procedural history geometry through
// shared projection/RenderEngine, not an HSD mesh/renderer fork or fitted path.
#include "engine/assets/startup_trail.hpp"
#include "engine/animation/cubic_history.hpp"
#include "engine/gameplay/startup_actor_frame.hpp"
#include "engine/graphics/projected_triangle.hpp"
namespace gf {
struct StartupTrailParentFields {
    bool declared=false;s8 field_af=0,phase18=0;u16 identity94=0,identity3e8=0;u8 selector3ec=0;
}; // Explicit observation, NOT original full constructor/actor history proof.
struct StartupTrailState {
    u32 key=0,birth=0,history_updates=0;u8 parent=0,phase=0,subphase=0,motion_bank=0,motion_index=0;
    u16 captured94=0;bool registered=false,visible=false,head_known=false,history_known=false;
    std::array<HistoryPoint,16> left{},right{};
};
inline bool startup_trail_parent_matches(const StartupTrailState& s,const StartupTrailParentFields& fields,const CharacterProgramLane& lane){
    // Source4871C stores lookup group/choice at1AFC/FD, but4D244 copies the
    // descriptor payload into1AFE/FF. Trail1AF94 compares the LATTER pair.
    return fields.declared&&fields.selector3ec<2&&fields.field_af!=-1&&fields.field_af!=126&&fields.phase18<2&&fields.identity3e8==s.captured94&&lane.metadata[0]==s.motion_bank&&lane.metadata[1]==s.motion_index;
}
class StartupTrailScene {
public:
    bool load(const StartupTrailProfile& profile,ByteView atlas,const std::array<const StartupActorFrame*,2>& actors,std::string* error){
        ready_=published_=pending_=false;states_.clear();candidate_.clear();frame_=next_frame_=next_key_=texture_=0;profile_={};atlas_={};actors_={};generations_={};
        if(!startup_trail_valid(profile)||!atlas.data||atlas.size>(4u<<20)||startup_trail_hash(atlas)!=profile.atlas_hash)return fail(error,"profile/atlas binding");
        u32 count=0;if(!tpl_image_count(atlas,&count,error)||count!=1||!tpl_decode(atlas,0,&atlas_,error)||atlas_.width!=profile.width||atlas_.height!=profile.height)return fail(error,"single source atlas/dimensions");
        for(unsigned i=0;i<2;++i){if(!actors[i]||!actors[i]->ready()||!actors[i]->model())return fail(error,"native actor required");actors_[i]=actors[i];generations_[i]=actors[i]->model()->instance_binding();if(generations_[i].expired())return fail(error,"actor generation");}
        if(actors[0]==actors[1]||generations_[0].lock()==generations_[1].lock())return fail(error,"distinct native parents");
        profile_=profile;ready_=true;if(error)error->clear();return true;
    }
    // Group18 update is BEFORE actors. A newly allocated trail is not initialized
    // until the NEXT group callback. Descriptor changes are observed at this cut.
    bool before_actors(u32 frame,const std::array<const StartupActorFrame*,2>& actors,const std::array<StartupTrailParentFields,2>& fields,std::string* error){
        published_=false;
        if(!ready_||pending_||frame!=frame_+1||frame>8192||!bound(actors))return refuse(error,"consecutive before-actor stage/generation");
        for(const auto& f:fields)if(!f.declared||f.selector3ec>1)return refuse(error,"explicit supported parent style/lifecycle observations");
        candidate_=states_;
        for(auto& s:candidate_){const auto& a=fields[s.parent];const auto& lane=actors[s.parent]->state()->vm.state().lanes[0];
            if(s.phase==0){s.phase=1;s.registered=true;}
            else if(s.phase==1){s.visible=startup_trail_parent_matches(s,a,lane);if(!s.visible)s.phase=2;}
            else if(s.phase==2){s.phase=3;s.visible=false;}
            else if(s.phase==3){s.registered=s.visible=false;}
            else return refuse(error,"child phase");}
        candidate_.erase(std::remove_if(candidate_.begin(),candidate_.end(),[](const auto& s){return s.phase==3&&!s.registered;}),candidate_.end());
        next_frame_=frame;pending_=true;if(error)error->clear();return true;
    }
    // Registered1B0C8 runs after accepted actor worlds, including hidden phases.
    // Born requests capture descriptor identity but do NOT read matrices yet.
    bool after_actors(u32 frame,const std::array<const StartupActorFrame*,2>& actors,const std::array<StartupTrailParentFields,2>& fields,std::span<const u8> requests,std::string* error){
        if(!ready_||!pending_||frame!=next_frame_||!bound(actors)||requests.size()>2)return refuse(error,"matching after-actor stage/request budget");
        for(auto& s:candidate_)if(s.registered){const auto* a=actors[s.parent]->state();if(!a->published||!a->have_world||!actors[s.parent]->model()->ready()||a->world.matrices.size()<2)return refuse(error,"accepted current bone1 required");
            std::array<HistoryPoint,2> head;for(unsigned side=0;side<2;++side){mat4_transform_point(a->world.matrices[1],profile_.endpoints[side].data(),head[side].data());for(f32 x:head[side])if(!std::isfinite(x)||std::abs(x)>1e7f)return refuse(error,"world endpoint domain");}
            if(s.subphase==0){s.subphase=1;}
            else if(s.subphase==1){if(!s.head_known)return refuse(error,"previous head required");for(unsigned i=0;i<15;++i){s.left[i]=s.left[15];s.right[i]=s.right[15];}s.subphase=2;s.history_known=true;}
            else if(s.subphase==2){for(unsigned i=0;i<15;++i){s.left[i]=s.left[i+1];s.right[i]=s.right[i+1];}}
            else return refuse(error,"history subphase");
            s.left[15]=head[0];s.right[15]=head[1];s.head_known=true;++s.history_updates;
        }
        std::array<bool,2> used{};u32 key=next_key_;
        for(u8 parent:requests){if(parent>1||used[parent]||!fields[parent].declared||fields[parent].selector3ec>1||candidate_.size()>=16)return refuse(error,"declared unique parent request/native capacity/style");used[parent]=true;const auto& a=*actors[parent]->state();const auto& lane=a.vm.state().lanes[0];if(!a.published||!lane.motion.selected)return refuse(error,"accepted trail request/primary descriptor");StartupTrailState s;s.key=++key;s.birth=frame;s.parent=parent;s.captured94=fields[parent].identity94;s.motion_bank=lane.metadata[0];s.motion_index=lane.metadata[1];candidate_.push_back(s);}
        states_=std::move(candidate_);next_key_=key;frame_=frame;pending_=false;published_=true;parent_frames_={actors[0]->state()->frames,actors[1]->state()->frames};if(error)error->clear();return true;
    }
    bool upload_texture(RenderEngine& render,u32 handle){if(!ready_||!handle||!render.upload_texture_shared(handle,{atlas_.width,atlas_.height,atlas_.rgba.data(),1,1,true})){ready_=false;return false;}texture_=handle;return true;}
    bool strip(const StartupTrailState& s,std::vector<Vertex>* out)const{
        if(out)out->clear();
        if(!out||!ready_||!s.history_known||s.subphase!=2)return false;
        std::array<HistoryPoint,16> left,right;for(unsigned i=0;i<16;++i){left[i]=s.left[15-i];right[i]=s.right[15-i];}std::vector<HistoryCubic> a,b;if(!history_natural_cubic(left,&a)||!history_natural_cubic(right,&b))return false;
        const auto alpha=u8(u32(f32(255.f*profile_.alpha)));unsigned index=0;
        auto pair=[&](const HistoryPoint& l,const HistoryPoint& r){for(unsigned side=0;side<2;++side){const auto& p=side?r:l;Vertex v;v.x=p[0];v.y=p[1];v.z=p[2];v.r=v.g=v.b=255;v.a=alpha;v.u=f32(profile_.uv[index][side*2])/profile_.width;v.v=f32(profile_.uv[index][side*2+1])/profile_.height;out->push_back(v);}++index;};
        HistoryPoint last_left{},last_right{};for(unsigned i=0;i<15;++i)for(unsigned j=0;j<2;++j){last_left=history_cubic_sample(a[i],f32(j)*.5f);last_right=history_cubic_sample(b[i],f32(j)*.5f);pair(last_left,last_right);}
        // Source1B684 repeats the last evaluated pair; it does NOT evaluate u=1
        // or substitute the oldest history point as a fitted terminal endpoint.
        pair(last_left,last_right);return out->size()==62;
    }
    bool draw(RenderEngine& render,const Camera& camera,const std::array<const StartupActorFrame*,2>& actors,unsigned* count,std::string* error){
        if(count)*count=0;
        if(!count||!ready_||!published_||pending_||!texture_||!bound(actors)||actors[0]->state()->frames!=parent_frames_[0]||actors[1]->state()->frames!=parent_frames_[1])return refuse(error,"published frame/texture/native parent binding");
        // Actor-group callbacks register newest-first; source50694 prepends into
        // queue2, so consumption reverses that order (Sasuke, then G-Red here).
        std::vector<const StartupTrailState*> order;for(const auto& s:states_)if(s.visible)order.push_back(&s);std::stable_sort(order.begin(),order.end(),[](const auto* a,const auto* b){return a->parent!=b->parent?a->parent>b->parent:a->birth<b->birth;});
        const auto saved=render.state();RenderState state;state.position_matrix=kIdentityPositionMatrix;state.texture=texture_;state.depth_test=true;state.depth_compare=6;state.depth_write=false;state.cull_backface=false;state.blend=true;state.blend_src=5;state.blend_dst=2;state.texture_colour_mode=4;state.texture_alpha_mode=3;
        for(const auto* s:order){std::vector<Vertex> vertices,projected;if(!strip(*s,&vertices)){render.state()=saved;return refuse(error,"bounded cubic strip");}for(unsigned i=0;i+2<vertices.size();++i){Vertex triangle[3]={vertices[i+(i&1)],vertices[i+1-(i&1)],vertices[i+2]};project_triangle(camera,triangle,projected);}if(projected.empty())continue;render.state()=state;const auto base=render.append_vertices(projected.data(),projected.size());if(!render.draw(Primitive::Triangles,base,u32(projected.size()))){render.state()=saved;return refuse(error,"shared projected strip submission");}++*count;}
        render.state()=saved;if(error)error->clear();return true;
    }
    const std::vector<StartupTrailState>& states()const{return states_;}bool ready()const{return ready_;}u32 frame()const{return frame_;}
private:
    bool bound(const std::array<const StartupActorFrame*,2>& actors)const{for(unsigned i=0;i<2;++i)if(!actors[i]||actors[i]!=actors_[i]||!actors[i]->ready()||!actors[i]->model()||generations_[i].expired()||generations_[i].lock()!=actors[i]->model()->instance_binding())return false;return true;}
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup trail: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=published_=pending_=false;return fail(error,why);}
    StartupTrailProfile profile_;TplImage atlas_;std::array<const StartupActorFrame*,2> actors_{};std::array<std::weak_ptr<const CharacterModelBinding>,2> generations_;std::array<u32,2> parent_frames_{};
    std::vector<StartupTrailState> states_,candidate_;u32 frame_=0,next_frame_=0,next_key_=0,texture_=0;bool ready_=false,published_=false,pending_=false;
};
} // namespace gf
