#pragma once
// Per-native-model material owner. Source actor clocks are read from the
// initialized scalar state; no caller-supplied fade RGB or free delta argument.
#include "engine/gameplay/startup_actor_bookkeeping.hpp"
#include "engine/gameplay/character_model_scene.hpp"
namespace gf {
class StartupActorMaterial {
public:
    bool bind(CharacterModelScene& model,const StartupActorSetupProfile& setup,const ActorFadeProfile& profile){
        ready_=published_=false;target_=nullptr;binding_.reset();state_={};
        const auto binding=model.instance_binding();if(!binding||!startup_actor_setup_detail::valid(setup)||binding->model_hash!=setup.model_hash||!actor_fade_profile_valid(profile))return false;
        if(!actor_fade_initialize(profile,&state_))return false;
        profile_=profile;model_id_=setup.model_id;target_=&model;binding_=binding;ready_=true;model.discard_pose();return true;
    }
    bool request(const CharacterModelScene& model,u8 curve,unsigned channel=1){
        published_=false;
        if(!ready_||!bound(model)){ready_=false;return false;}
        ActorFadeState next;if(!actor_fade_request(profile_,state_,curve,channel,&next)){ready_=false;return false;}state_=next;return true;
    }
    bool clear(const CharacterModelScene& model){
        published_=false;
        if(!ready_||!bound(model)){ready_=false;return false;}
        ActorFadeState next;if(!actor_fade_clear(profile_,state_,&next)){ready_=false;return false;}state_=next;return true;
    }
    bool tick(const CharacterModelScene& model,const StartupActorBookkeeping& actor){
        published_=false;const auto binding=binding_.lock();
        if(!ready_||!binding||!bound(model)||!startup_actor_bookkeeping_detail::valid(actor)||actor.model_hash!=binding->model_hash||actor.klass.model_id!=model_id_){ready_=false;return false;}
        ActorFadeState next;if(!actor_fade_update(profile_,state_,actor.motion_delta,&next)){ready_=false;return false;}state_=next;return true;
    }
    bool publish(CharacterModelScene& model,const CharacterWorldPose& world,const CharacterModelRenderState& render_state,std::string* error){
        published_=false;
        if(!ready_||!bound(model)){ready_=false;if(error)*error="startup actor material: instance lifetime/binding";return false;}
        ActorFadeMaterial material;if(!actor_fade_material(profile_,state_,&material)||!model.set_world_pose_fade(world,render_state,material,error)){ready_=false;model.discard_pose();return false;}published_=true;return true;
    }
    unsigned draw(CharacterModelScene& model,RenderEngine& render,const Camera& camera){return ready_&&published_&&bound(model)?model.draw(render,camera):0;}
    bool bound(const CharacterModelScene& model)const{const auto binding=binding_.lock();return target_==&model&&binding&&binding==model.instance_binding();}
    bool ready()const{return ready_;}
    const ActorFadeState& state()const{return state_;}
private:
    const CharacterModelScene* target_=nullptr; // comparison only, never dereferenced
    std::weak_ptr<const CharacterModelBinding> binding_;
    ActorFadeProfile profile_;ActorFadeState state_;u16 model_id_=0;bool ready_=false,published_=false;
};
} // namespace gf
