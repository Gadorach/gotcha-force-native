#pragma once
//8019813C/81B0 and82AC/833C/83F0: five authored reveal/shine instances.
// Explicit native snapshot scope, NOT the complete actor constructor/scene owner.
#include "engine/assets/startup_reveal.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/gameplay/character_model_scene.hpp"
#include "engine/gameplay/startup_director.hpp"
namespace gf {
enum class StartupRevealMode {DeclaredNativeActorSnapshots};
struct StartupRevealActor {
    const CharacterModelScene* model=nullptr;
    const CharacterWorldPose* world=nullptr; // current source bone1/actor+904 only when hidden
    u32 frame=0;u8 render_group=0;
};
struct StartupRevealState {
    u8 phase=0,subphase=0;u16 remaining=0;bool allocated=true,visible=false,registered=false;
    u32 animation_updates=0,root_copies=0;Mat4 root;
};
struct StartupRevealSound {u16 id=377;u8 record=0;}; // pending requirement, never acknowledged/played here
class StartupRevealScene {
public:
    bool load(const HsdRecordAnimationBank& bank,const StartupRevealProfile& profile,
              const std::array<const CharacterModelScene*,2>& actors,StartupRevealMode mode,std::string* error){
        ready_=pending_=published_=false;storage_.reset();
        if(mode!=StartupRevealMode::DeclaredNativeActorSnapshots||!startup_reveal_profile_valid(profile)||bank.hash()!=profile.bank_hash||bank.records()!=profile.bank_records)return fail(error,"explicit profile/bank/snapshot scope");
        auto s=std::make_unique<Storage>();s->profile=profile;
        for(unsigned i=0;i<2;++i){if(!actors[i])return fail(error,"native actor instance");const auto b=actors[i]->instance_binding();if(!b||b->model_hash!=profile.actor_model_hashes[i])return fail(error,"actor resource binding");s->actors[i]=actors[i];s->generations[i]=b;}
        if(actors[0]==actors[1]||s->generations[0].lock()==s->generations[1].lock())return fail(error,"distinct actor generations required");
        for(unsigned i=0;i<5;++i){if(!s->models[i].load(bank,u16(i),3,0,0,error)||s->models[i].asset()->model_roots!=std::vector<u32>{profile.roots[i]})return fail(error,"published reveal record/root");s->models[i].set_untextured_billboards(i>=3);}
        storage_=std::move(s);ready_=true;if(error)error->clear();return true;
    }
    // Group28 is updated before80057AD0. No actor matrix is read at this stage.
    bool before_actors(const StartupDirectorState& director,u8 mode,bool resource_ready,std::string* error){
        published_=false;
        if(!ready_||!storage_||pending_||director.updates!=storage_->frames+1||director.updates>8192||director.phase>12)return refuse(error,"consecutive director frame/before-actor stage");
        auto& s=*storage_;s.candidate=s.states;s.cues.clear();s.next_frame=director.updates;s.mode=mode;s.phase=director.phase;s.resource_ready=resource_ready;
        for(unsigned record:{2u,1u,0u})if(!update(record,error))return refuse(error,"late graphics callback failed");
        pending_=true;if(error)error->clear();return true;
    }
    // Group10 then89554 run after actors. Copy while actor82==0, even for hidden
    // graphics in phases2/3; unregister/free only on the later phase3 callback.
    bool after_actors(const std::array<StartupRevealActor,2>& actors,u8 scene34,
                      std::vector<StartupRevealSound>* out,std::string* error){
        if(out)out->clear();
        if(!ready_||!storage_||!pending_||!out)return refuse(error,"matching after-actor stage/output");
        auto& s=*storage_;
        for(unsigned record:{4u,3u})if(!update(record,error))return refuse(error,"early graphics callback failed");
        for(unsigned record:{4u,3u}){auto& state=s.candidate[record];if(!state.allocated||!state.registered)continue;
            const unsigned slot=record==3?0:1;const auto& actor=actors[slot];
            if(!current_actor(slot,actor,s.next_frame))return refuse(error,"current actor generation/frame required by registered callback");
            if(actor.render_group)continue; // source reads no matrix for a visible actor
            if(!actor.world||actor.world->matrices.size()<2||actor.world->matrices.size()!=actor.model->rig().binding.bones.size()||actor.world->joint_offsets.size()!=actor.world->matrices.size())return refuse(error,"hidden actor current world/bone1");
            for(unsigned i=0;i<actor.world->joint_offsets.size();++i)if(actor.world->joint_offsets[i]!=actor.model->rig().binding.bones[i].joint_offset)return refuse(error,"hidden actor rig identity");
            const auto& matrix=actor.world->matrices[1];if(!s.models[record].set_root_world(matrix,error))return refuse(error,"hidden actor bone1 root copy failed");state.root=matrix;++state.root_copies;
        }
        for(unsigned i=0;i<5;++i)s.models[i].set_enabled(s.candidate[i].allocated&&s.candidate[i].visible&&scene34==0);
        s.states=s.candidate;s.frames=s.next_frame;s.display=scene34==0;*out=s.cues;s.cues.clear();pending_=false;published_=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){
        if(!ready_||!storage_)return 0;
        unsigned n=0;for(auto& model:storage_->models)n+=model.upload_textures(render,base+n);return n;
    }
    unsigned upload_textures_shared(RenderEngine& render,u32 base){if(!ready_||!storage_)return 0;unsigned n=0;for(auto& m:storage_->models)n+=m.upload_textures_shared(render,base);return n;}
    unsigned draw_model(unsigned record,RenderEngine& render,const Camera& camera,const std::array<StartupRevealActor,2>& actors){
        if(record>=5||!ready_||!storage_||!published_||pending_||!storage_->display)return 0;
        if(record>=3&&storage_->states[record].visible&&!current_actor(record-3,actors[record-3],storage_->frames)){refuse(nullptr,"actor changed after reveal publication");return 0;}return storage_->models[record].draw(render,camera);
    }
    unsigned draw(RenderEngine& render,const Camera& camera,const std::array<StartupRevealActor,2>& actors){
        if(!ready_||!storage_||!published_||pending_||!storage_->display)return 0;
        for(unsigned record:{3u,4u})if(storage_->states[record].allocated&&storage_->states[record].visible){const unsigned slot=record==3?0:1;if(!current_actor(slot,actors[slot],storage_->frames)){refuse(nullptr,"actor generation/frame changed after publication");return 0;}}
        unsigned n=0;for(unsigned record:{2u,1u,0u,4u,3u})n+=storage_->models[record].draw(render,camera);return n;
    }
    const std::array<StartupRevealState,5>* states()const{return ready_&&storage_&&!pending_?&storage_->states:nullptr;}
    const AnimatedModelScene* model(unsigned record)const{return ready_&&storage_&&!pending_&&record<5?&storage_->models[record]:nullptr;}
    u32 frames()const{return storage_?storage_->frames:0;}
    std::span<const StartupRevealSound> pending_source_sounds()const{return ready_&&storage_&&pending_?std::span<const StartupRevealSound>(storage_->cues):std::span<const StartupRevealSound>{};}
    bool ready()const{return ready_&&bool(storage_);}
    void release(){storage_.reset();ready_=pending_=published_=false;}
private:
    struct Storage {
        StartupRevealProfile profile;std::array<AnimatedModelScene,5> models;std::array<StartupRevealState,5> states{},candidate{};
        std::array<const CharacterModelScene*,2> actors{}; // comparison-only; never dereferenced without matching current input
        std::array<std::weak_ptr<const CharacterModelBinding>,2> generations;
        std::vector<StartupRevealSound> cues;u32 frames=0,next_frame=0;u8 phase=0,mode=0;bool resource_ready=false,display=false;
    };
    bool current_actor(unsigned slot,const StartupRevealActor& actor,u32 frame)const{
        const auto generation=storage_->generations[slot].lock();return actor.model&&actor.model==storage_->actors[slot]&&generation&&generation==actor.model->instance_binding()&&actor.frame==frame;
    }
    bool update(unsigned record,std::string* error){
        auto& s=*storage_;auto& state=s.candidate[record];auto& model=s.models[record];if(!state.allocated)return true;
        if(state.phase==0){if(!s.resource_ready)return true;if(!model.request(s.profile.start,s.profile.rate,error))return false;state.phase=1;state.registered=record>=3;}
        else if(state.phase==1){
            if(state.subphase==0){if(s.phase>(record<3?10:2)){state.subphase=1;state.remaining=record<3?120:40;state.visible=true;if(record<3)s.cues.push_back({377,u8(record)});}}
            else{state.remaining=u16(state.remaining-1);if(std::bit_cast<s16>(state.remaining)<1||s.mode!=1){state.phase=2;state.visible=false;}
                else{if(!model.advance(s.profile.rate,error))return false;++state.animation_updates;}}
        }else if(state.phase==2){state.phase=3;state.visible=false;}
        else if(state.phase==3){state.allocated=state.registered=state.visible=false;model.release();}
        else return false;
        return true;
    }
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup reveal: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const auto detail=error?*error:std::string{};ready_=pending_=published_=false;if(storage_)for(auto& model:storage_->models)model.set_enabled(false);fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    std::unique_ptr<Storage> storage_;bool ready_=false,pending_=false,published_=false;
};
} // namespace gf
