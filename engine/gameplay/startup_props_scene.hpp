#pragma once
// Eight moving source-kind1 props only. Shared renderer, source scene policy.
#include "engine/gameplay/startup_props.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/gameplay/startup_director.hpp"
namespace gf {
enum class StartupPropMode {DeclaredSceneObservations};
class StartupPropScene {
public:
    bool load(const HsdRecordAnimationBank& bank,const StartupPropProfile& profile,StartupPropMode mode,std::string* error){
        storage_.reset();ready_=pending_=published_=false;
        if(mode!=StartupPropMode::DeclaredSceneObservations||!startup_prop_profile_valid(profile)||bank.hash()!=profile.bank_hash||bank.records()!=profile.bank_records)return fail(error,"source resource/profile/scope");
        auto s=std::make_unique<Storage>();s->profile=profile;
        for(unsigned i=0;i<8;++i){const auto& e=profile.parts[i];HsdRecordAnimationBinding binding;
            if(!bank.select(e.record,3,0,0,&binding,error)||!binding.objects()->empty()||binding.scene()->joints.size()!=1||binding.scene()->model_roots!=std::vector<u32>{e.root}||!s->models[i].load(bank,e.record,0,0,0,error))return fail(error,"single-root static prop record required");
            s->roots[i]=binding.scene()->joints[0];}
        storage_=std::move(s);ready_=true;if(error)error->clear();return true;
    }
    bool before_actors(const StartupDirectorState& director,u8 mode,std::string* error){
        published_=false;if(!ready_||!storage_||pending_||director.updates!=storage_->frames+1||director.updates>8192||director.phase>12)return refuse(error,"consecutive source group28 update");
        auto& s=*storage_;s.next_frame=director.updates;s.candidate=s.states;s.cues.clear();
        for(unsigned i=0;i<8;++i){auto state=s.candidate[i];if(!state.initialized){if(!startup_prop_initialize(s.profile,i,s.roots[i],director.phase,mode,&state,error)||!s.models[i].request(0,1,error))return refuse(error,"source constructor failed");}
            std::vector<StartupPropSound> cues;if(!startup_prop_update(s.profile,i,director.phase,mode,state,&s.candidate[i],&cues,error))return refuse(error,"prop update failed");s.cues.insert(s.cues.end(),cues.begin(),cues.end());}
        pending_=true;if(error)error->clear();return true;
    }
    bool after_actors(u32 frame,u8 scene34,std::vector<StartupPropSound>* out,std::string* error){
        if(out)out->clear();
        if(!ready_||!storage_||!pending_||!out||frame!=storage_->next_frame)return refuse(error,"matching registered-transform stage/output");
        auto& s=*storage_;auto poses=s.candidate;
        for(unsigned i=0;i<8;++i)if(!startup_prop_transform(s.profile,s.roots[i],s.candidate[i],&poses[i],error))return refuse(error,"registered transform failed");
        for(unsigned i=0;i<8;++i){if(!s.models[i].set_root_world(poses[i].transform.world,error))return refuse(error,"shared scene publication failed");s.models[i].set_enabled(poses[i].enabled&&scene34==0);}
        s.states=poses;s.frames=frame;s.display=scene34==0;*out=s.cues;s.cues.clear();pending_=false;published_=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){if(!ready_||!storage_)return 0;unsigned n=0;for(auto& model:storage_->models)n+=model.upload_textures(render,base+n);return n;}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){if(!ready_||!storage_)return 0;unsigned n=0;for(auto& m:storage_->models)n+=m.upload_textures_shared(render,base);return n;}
    unsigned draw_model(unsigned part,RenderEngine& render,const Camera& camera){return part<8&&ready_&&storage_&&!pending_&&published_&&storage_->display?storage_->models[part].draw(render,camera):0;}
    unsigned draw(RenderEngine& render,const Camera& camera){if(!ready_||!storage_||pending_||!published_||!storage_->display)return 0;unsigned n=0;for(auto& model:storage_->models)n+=model.draw(render,camera);return n;}
    const std::array<StartupPropState,8>* states()const{return ready_&&storage_&&!pending_?&storage_->states:nullptr;}
    const AnimatedModelScene* model(unsigned part)const{return ready_&&storage_&&!pending_&&part<8?&storage_->models[part]:nullptr;}
    bool ready()const{return ready_&&bool(storage_);}
    u32 frames()const{return storage_?storage_->frames:0;}
    std::span<const StartupPropSound> pending_source_sounds()const{return ready_&&storage_&&pending_?std::span<const StartupPropSound>(storage_->cues):std::span<const StartupPropSound>{};}
    void release(){storage_.reset();ready_=pending_=published_=false;}
private:
    struct Storage {StartupPropProfile profile;std::array<AnimatedModelScene,8> models;std::array<HsdJoint,8> roots;std::array<StartupPropState,8> states{},candidate{};std::vector<StartupPropSound> cues;u32 frames=0,next_frame=0;bool display=false;};
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup prop scene: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const auto detail=error?*error:std::string{};ready_=pending_=published_=false;if(storage_)for(auto& model:storage_->models)model.set_enabled(false);fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    std::unique_ptr<Storage> storage_;bool ready_=false,pending_=false,published_=false;
};
} // namespace gf
