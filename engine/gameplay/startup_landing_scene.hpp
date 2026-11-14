#pragma once
// Source landing snapshots through the existing published-record renderer.
// Copies explicitly declared ground/anchor inputs; never borrows parent matrices
// or infers original actor20/history. Native capacity8 is NOT source pool parity.
#include "engine/gameplay/startup_landing.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
namespace gf {
enum class StartupLandingMode {DeclaredSourceGroundAndAnchorSnapshots};
class StartupLandingScene {
public:
    StartupLandingScene()=default;
    StartupLandingScene(const StartupLandingScene&)=delete;StartupLandingScene& operator=(const StartupLandingScene&)=delete;
    StartupLandingScene(StartupLandingScene&& other)noexcept{*this=std::move(other);}
    StartupLandingScene& operator=(StartupLandingScene&& other)noexcept{
        if(this!=&other){bank_=std::move(other.bank_);profile_=std::move(other.profile_);grids_=std::move(other.grids_);instances_=std::move(other.instances_);frame_=other.frame_;next_key_=other.next_key_;ready_=other.ready_;published_=other.published_;display_=other.display_;other.ready_=other.published_=other.display_=false;other.frame_=other.next_key_=0;}return *this;
    }
    bool load(const HsdRecordAnimationBank& bank,const StartupLandingProfile& profile,const std::array<StihGrid,3>& grids,StartupLandingMode mode,std::string* error){
        release();if(mode!=StartupLandingMode::DeclaredSourceGroundAndAnchorSnapshots||!startup_landing_profile_valid(profile)||bank.hash()!=profile.model_hash)return fail(error,"source resource/profile/snapshot scope");
        for(const auto& g:grids)if(!g.loaded())return fail(error,"all three original grid roles required");
        // Preflight ALL selected records before a request can create any draw.
        for(const auto& entry:profile.entries){AnimatedModelScene model;if(!model.load(bank,entry.record,0,0,0,error)||!model.request(0,0,error)||!model.replace_static_alpha(profile.one,error))return false;}
        bank_=bank;profile_=profile;grids_=grids;ready_=true;if(error)error->clear();return true;
    }
    bool step(u32 frame,std::span<const StartupLandingRequest> requests,f32 scene_delta,u8 scene34,std::string* error){
        published_=false;
        if(!ready_||frame==0||frame>8192||(frame_&&frame!=frame_+1)||requests.size()>8)return refuse(error,"consecutive scene frame/request budget");
        std::vector<StartupLandingState> candidate;candidate.reserve(instances_.size()+requests.size());
        for(const auto& instance:instances_){StartupLandingState next;if(!startup_landing_step(profile_,scene_delta,instance.state,&next,error))return refuse(error,"lifecycle/curve step");candidate.push_back(next);}
        const std::array<const StihGrid*,3> grids={&grids_[0],&grids_[1],&grids_[2]};
        for(const auto& request:requests){StartupLandingState next;if(!startup_landing_construct(profile_,grids,request,&next,error))return refuse(error,"source ground/construct");candidate.push_back(next);}
        unsigned allocated=0;for(const auto& c:candidate)allocated+=c.allocated;if(allocated>8)return refuse(error,"native live-instance budget, not silent allocation fallback");
        const auto previous=instances_.size();
        for(unsigned n=0;n<requests.size();++n){Instance next;next.key=++next_key_;const auto& state=candidate[previous+n];for(unsigned e=state.kind==2?2:0;e<(state.kind==2?3:2);++e){auto& model=next.models[e];if(!model.load(bank_,profile_.entries[e].record,0,0,0,error)||!model.request(0,0,error))return refuse(error,"source record instance");}instances_.push_back(std::move(next));}
        for(unsigned i=0;i<instances_.size();++i){const auto& state=candidate[i];auto& models=instances_[i].models;
            if(state.allocated&&state.registered){auto& model=models[state.entry];if(!model.set_root_world(state.world,error)||!model.replace_static_alpha(state.alpha,error))return refuse(error,"root/material publication");}
        }
        for(unsigned i=0;i<instances_.size();++i)instances_[i].state=candidate[i];
        instances_.erase(std::remove_if(instances_.begin(),instances_.end(),[](const auto& i){return !i.state.allocated;}),instances_.end());
        frame_=frame;display_=scene34==0;published_=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){if(!ready_)return 0;unsigned count=0;for(auto& i:instances_)for(auto& m:i.models)count+=m.upload_textures(render,base+count);return count;}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){if(!ready_)return 0;unsigned n=0;for(auto& i:instances_)for(auto& m:i.models)n+=m.upload_textures_shared(render,base);return n;}
    unsigned draw_model(unsigned i,RenderEngine& render,const Camera& camera){if(!ready_||!published_||!display_||i>=instances_.size()||!instances_[i].state.visible)return 0;auto& model=instances_[i].models[instances_[i].state.entry];model.set_enabled(true);return model.draw(render,camera);}
    unsigned draw(RenderEngine& render,const Camera& camera){if(!ready_||!published_||!display_)return 0;unsigned count=0;for(auto& i:instances_)if(i.state.visible){auto& model=i.models[i.state.entry];model.set_enabled(true);count+=model.draw(render,camera);}return count;}
    bool ready()const{return ready_;}u32 frame()const{return frame_;}std::size_t instances()const{return ready_?instances_.size():0;}
    const StartupLandingState* state(unsigned n)const{return ready_&&n<instances_.size()?&instances_[n].state:nullptr;}
    const AnimatedModelScene* model(unsigned n)const{return ready_&&n<instances_.size()?&instances_[n].models[instances_[n].state.entry]:nullptr;}
    u32 key(unsigned n)const{return ready_&&n<instances_.size()?instances_[n].key:0;}
    void release(){instances_.clear();bank_={};profile_={};grids_={};ready_=published_=display_=false;frame_=next_key_=0;}
private:
    struct Instance {StartupLandingState state;std::array<AnimatedModelScene,3> models;u32 key=0;};
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup landing scene: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const auto detail=error?*error:std::string{};ready_=published_=false;for(auto& i:instances_)for(auto& m:i.models)m.set_enabled(false);fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    HsdRecordAnimationBank bank_;StartupLandingProfile profile_;std::array<StihGrid,3> grids_;std::vector<Instance> instances_;
    u32 frame_=0,next_key_=0;bool ready_=false,published_=false,display_=false;
};
} // namespace gf
