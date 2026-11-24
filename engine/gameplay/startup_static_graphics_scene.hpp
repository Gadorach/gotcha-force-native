#pragma once
//80197DD4/E40 and7F0C/7FA4/8094; no global scene/render-queue or audio admission.
#include "engine/assets/startup_static_graphics.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/gameplay/startup_camera.hpp"
#include "engine/graphics/camera_transform.hpp"
namespace gf {
struct StartupStaticGraphicsState {u8 phase=0,substate=0;bool visible=false;f32 alpha=0,material_alpha=0,slope=0;u32 alpha_writes=0;Mat4 world;};
enum class StartupStaticGraphicsMode {DeclaredSceneObservations};
class StartupStaticGraphicsScene {
public:
    bool load(const HsdRecordAnimationBank& bank,const StartupStaticGraphicsProfile& p,StartupStaticGraphicsMode mode,std::string* error){
        storage_.reset();ready_=pending_=published_=false;
        if(mode!=StartupStaticGraphicsMode::DeclaredSceneObservations||!startup_static_graphics_profile_valid(p)||bank.hash()!=p.bank_hash||bank.records()!=p.bank_records)return fail(error,"bank/profile/scope");
        auto s=std::make_unique<Storage>();s->profile=p;s->screen=mat4_multiply(mat4_translation(p.screen_position.data()),mat4_rotation_x(p.screen_rotation_x));
        for(unsigned i=0;i<32;++i){const auto& e=p.entries[i];HsdRecordAnimationBinding bind;if(!bank.select(e.record,3,0,0,&bind,error)||!bind.objects()->empty()||bind.scene()->model_roots!=std::vector<u32>{e.root}||!s->models[i].load(bank,e.record,0,0,0,error))return fail(error,"static published record/root required");}
        storage_=std::move(s);ready_=true;if(error)error->clear();return true;
    }
    bool before_actors(const StartupDirectorState& d,u8 mode,bool resource_ready,std::string* error){
        published_=false;if(!ready_||!storage_||pending_||d.updates!=storage_->frames+1||d.updates>8192||d.phase>12)return refuse(error,"consecutive source group28 frame");
        auto& s=*storage_;s.candidate=s.states;s.next_frame=d.updates;
        for(unsigned i=0;i<32;++i){auto& state=s.candidate[i];const auto kind=s.profile.entries[i].kind;auto& model=s.models[i];bool initialized=false;
            if(state.phase==0){if(!resource_ready)continue;if(!model.request(0,1,error))return refuse(error,"static constructor pose");state.phase=1;state.material_alpha=model.animation().pose()->materials[0].alpha;state.visible=!kind;initialized=true;}
            else if(state.phase==2){state.phase=3;state.visible=false;}
            else if(state.phase==3){state.phase=4;state.visible=false;model.release();}
            if(state.phase!=1)continue;
            if(!kind){if(mode!=1&&mode!=2){state.phase=2;state.visible=false;}}
            else if(!initialized){if(state.substate==0){if(d.phase==12){state.substate=1;state.visible=true;}}
                else if(state.substate==1){if(mode!=1){state.substate=2;state.alpha=s.profile.alpha_start;state.slope=s.profile.alpha_step;if(!model.replace_static_alpha(state.alpha,error))return refuse(error,"caption alpha start");state.material_alpha=state.alpha;++state.alpha_writes;}}
                else if(state.substate==2){state.alpha=f32(state.alpha+state.slope);if(state.alpha>s.profile.zero){if(!model.replace_static_alpha(state.alpha,error))return refuse(error,"caption alpha update");state.material_alpha=state.alpha;++state.alpha_writes;}else{state.phase=2;state.visible=false;}}}
        }
        pending_=true;if(error)error->clear();return true;
    }
    // Render registration is after source camera/scene updates. Record15 alone
    // receives inverse(current view)*Tscreen*Rx. TM16 keeps its authored world.
    bool register_camera(u32 frame,const StartupCameraPoseState& camera,u8 scene34,std::string* error){
        if(!ready_||!storage_||!pending_||frame!=storage_->next_frame)return refuse(error,"matching camera-registration stage");
        auto& s=*storage_;auto next=s.candidate;
        bool needs_view=false;for(unsigned i=0;i<32;++i)needs_view|=s.profile.entries[i].kind==1&&next[i].visible&&scene34==0;
        Mat4 view,inverse;if(needs_view){if(!camera.initialized||camera.main_binding!=s.profile.main_camera_hash||camera.roll!=0||!camera_view_matrix(camera.camera,&view)||!mat4_inverse_affine(view,&inverse))return refuse(error,"bound current camera view/inverse required");}
        for(unsigned i=0;i<32;++i){if(s.profile.entries[i].kind==1&&next[i].visible&&scene34==0){next[i].world=mat4_multiply(inverse,s.screen);if(!s.models[i].set_root_world(next[i].world,error))return refuse(error,"copyright camera-relative publication");}s.models[i].set_enabled(next[i].visible&&scene34==0);}
        s.states=next;s.frames=frame;s.display=scene34==0;s.camera_required=needs_view;if(needs_view)s.camera=camera.camera;pending_=false;published_=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){if(!ready_||!storage_)return 0;unsigned n=0;for(auto& model:storage_->models)n+=model.upload_textures(render,base+n);return n;}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){if(!ready_||!storage_)return 0;unsigned n=0;for(auto& model:storage_->models)n+=model.upload_textures_shared(render,base);return n;}
    unsigned draw_model(unsigned slot,RenderEngine& render,const Camera& camera){if(slot>=32||!ready()||!published_||pending_||!storage_->display)return 0;if(storage_->camera_required&&!same_camera(camera,storage_->camera)){refuse(nullptr,"camera changed after copyright registration");return 0;}return storage_->models[slot].draw(render,camera);}
    unsigned draw(RenderEngine& render,const Camera& camera){if(!ready_||!storage_||!published_||pending_||!storage_->display)return 0;if(storage_->camera_required&&!same_camera(camera,storage_->camera)){refuse(nullptr,"camera changed after copyright registration");return 0;}unsigned n=0;for(auto& model:storage_->models)n+=model.draw(render,camera);return n;}
    bool ready()const{return ready_&&bool(storage_);}
    u32 frames()const{return storage_?storage_->frames:0;}
    const std::array<StartupStaticGraphicsState,32>* states()const{return ready()&&!pending_?&storage_->states:nullptr;}
    const AnimatedModelScene* model(unsigned slot)const{return ready()&&!pending_&&slot<32?&storage_->models[slot]:nullptr;}
    void release(){storage_.reset();ready_=pending_=published_=false;}
private:
    struct Storage {StartupStaticGraphicsProfile profile;std::array<AnimatedModelScene,32> models;std::array<StartupStaticGraphicsState,32> states{},candidate{};Mat4 screen;Camera camera;u32 frames=0,next_frame=0;bool display=false,camera_required=false;};
    static bool same_camera(const Camera& a,const Camera& b){return a.cache_key()==b.cache_key();}
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup static graphics: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const auto detail=error?*error:std::string{};ready_=pending_=published_=false;if(storage_)for(auto& model:storage_->models)model.set_enabled(false);fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    std::unique_ptr<Storage> storage_;bool ready_=false,pending_=false,published_=false;
};
} // namespace gf
