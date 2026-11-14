#pragma once
// Graphics-only auxiliary40/41 consumer. Parent708 writes and sound obligations
// are HELD, never applied/acknowledged through this isolated snapshot entry.
#include "engine/gameplay/startup_actor_frame.hpp"
#include "engine/gameplay/startup_auxiliary_turn.hpp"
#include "engine/gameplay/startup_auxiliary_scene.hpp"
namespace gf {
enum class StartupThrusterMode {DeclaredIsolatedConstructionAndPlacement};
struct StartupThrusterSoundRequirement {
    u32 frame=0,parent_frame_before=0;u8 native_parent708=0,construction708=0,pre709=0,pre707=0;
    bool refresh_required=false,requires_parent_sound_completion=true;
};
class StartupThrusterScene {
public:
    bool bind(const StartupActorFrame& actor,ByteView model,const StartupAuxiliaryProfile& profile,
              const StartupAuxiliaryTurnProfile& turn,StartupThrusterMode mode,u8 constructor_scene,
              std::array<bool,2> allocation_available,std::string* error){
        storage_.reset();ready_=pending_=published_=false;
        if(mode!=StartupThrusterMode::DeclaredIsolatedConstructionAndPlacement||!actor.ready()||!actor.model()||!actor.state()||actor.state()->frames||actor.state()->scalars.field708||actor.state()->scalars.klass.model_id!=0x615||actor.state()->dispatch.state().render_index||!startup_auxiliary_profile_valid(profile)||!startup_auxiliary_turn_valid(turn))return fail(error,"fresh isolated G-Red/profile/held-mutation scope");
        for(const auto& entry:profile.entries)for(const auto& part:entry.parts)if(part.bone<0||unsigned(part.bone)>=actor.model()->rig().binding.bones.size())return fail(error,"this owner requires explicit main-actor logical bones");
        auto s=std::make_unique<Storage>();s->profile=profile;s->turn=turn;s->model_target=actor.model();s->generation=actor.model()->instance_binding();
        if(s->generation.expired()||!actor.state()->material.bound(*actor.model()))return fail(error,"native model/material binding");
        StartupAuxiliaryParent parent;parent.model94=actor.state()->scalars.klass.model_id;parent.phase18=actor.state()->scalars.phase18;parent.kind83=0;parent.stored_index=0;
        for(unsigned e=0;e<2;++e){StartupAuxiliaryConstruction plan;if(!startup_auxiliary_construct(profile,e,parent,constructor_scene,allocation_available[e],s->planned708,&plan,error))return false;s->planned708=plan.parent708;s->children[e]=plan.state;s->created[e]=plan.created;
            if(plan.created)for(auto& copy:s->models[e])if(!copy.load(model,profile,error))return false;}
        storage_=std::move(s);ready_=true;if(error)error->clear();return true;
    }
    //5568C reads709 after the command prefix, BEFORE this actor's VM update.
    // The local lease is a mirror of unfulfilled constructor mask plans, NOT a
    // forged accepted scalar state and NOT a write to the real native parent.
    bool before_actor(const StartupActorFrame& actor,const StartupActorFrameInput& entry,u32 frame,std::string* error){
        return before_actors(actor,entry,frame,true,error);
    }
    // Group18 callbacks run even while the director disables actor updates.
    // In that case no scalar lease preupdate is fabricated.
    bool before_actors(const StartupActorFrame& actor,const StartupActorFrameInput& entry,u32 frame,bool actor_updates,std::string* error){
        published_=false;
        if(!ready_||!storage_||pending_||!bound(actor)||!entry.isolated_external_mutations||!entry.placement_declared||actor.state()->frames!=storage_->parent_frames||actor.state()->scalars.field708||frame<=storage_->global_frame||(storage_->global_started&&frame!=storage_->global_frame+1)||frame>8192)return refuse(error,"source pre-actor stage/declared entry/held mask");
        for(f32 v:entry.position)if(!std::isfinite(v)||std::abs(v)>1e7f)return refuse(error,"declared source entry position");
        auto& s=*storage_;s.next_frame=frame;s.entry_position=entry.position;s.next_leases=s.leases;s.actor_updates=actor_updates;
        const u8 auxiliary=actor.state()->vm.state().actor.auxiliary;
        if(actor_updates)for(unsigned channel=0;channel<6;++channel){if((s.planned708&(1u<<channel))&&(auxiliary&(1u<<channel)))s.next_leases[channel]=2;if(s.next_leases[channel])--s.next_leases[channel];}
        pending_=true;if(error)error->clear();return true;
    }
    //Source group18 updates after actors, then89554 transforms from accepted
    //bone matrices. 5685C's entry copy supplies2C in this explicit no-mutation
    //placement subset; previous animated roots/previous frame positions are NOT2C.
    bool after_actors(const StartupActorFrame& actor,u32 frame,u8 scene34,std::string* error){
        if(!ready_||!storage_||!pending_||!bound(actor)||frame!=storage_->next_frame||actor.state()->frames!=storage_->parent_frames+unsigned(storage_->actor_updates)||actor.state()->scalars.field708)return refuse(error,"matching accepted source actor frame");
        bool active=false;for(auto lease:storage_->next_leases)active|=lease!=0;
        if((storage_->actor_updates||active)&&(!actor.state()->published||!actor.model()->ready()||!actor.state()->have_declared_position||actor.state()->declared_position!=storage_->entry_position))return refuse(error,"matching accepted source placement frame");
        auto& s=*storage_;auto children=s.children;const auto& source=*actor.state();
        StartupAuxiliaryParent parent;parent.model94=source.scalars.klass.model_id;parent.phase18=source.scalars.phase18;parent.kind83=0;parent.stored_index=source.dispatch.state().render_index;parent.bones=source.world.matrices;
        StartupAuxiliaryMotionObservation motion;if(s.actor_updates||active){motion.source_positions_declared=true;motion.current20=source.declared_position;motion.previous2c=s.entry_position;}
        for(unsigned e=0;e<2;++e){parent.active_count=s.next_leases[s.profile.entries[e].channel];StartupAuxiliaryState next;if(!startup_auxiliary_step_with_turning(s.profile,s.turn,e,parent,motion,children[e],&next,error))return refuse(error,"source auxiliary callback");children[e]=next;
            if(children[e].allocated&&parent.active_count){for(unsigned n=0;n<2;++n)if(!s.models[e][n].set_root(children[e].matrices[n],error))return refuse(error,"source auxiliary model publication");}
            if(!children[e].allocated)for(auto& model:s.models[e])model=StartupAuxiliaryModelScene{};
        }
        s.children=children;s.leases=s.next_leases;s.parent_frames=source.frames;s.global_frame=frame;s.global_started=true;s.display=scene34==0;s.last_motion=motion;pending_=false;published_=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){if(!ready_||!storage_)return 0;unsigned n=0;for(auto& pair:storage_->models)for(auto& model:pair)n+=model.upload_textures(render,base+n);return n;}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){if(!ready_||!storage_)return 0;unsigned n=0;for(auto& pair:storage_->models)for(auto& model:pair)n+=model.upload_textures_shared(render,base);return n;}
    unsigned draw_model(unsigned entry,unsigned copy,const StartupActorFrame& actor,RenderEngine& render,const Camera& camera){if(entry>1||copy>1||!ready_||!storage_||!published_||pending_||!storage_->display)return 0;if(!bound(actor)||actor.state()->frames!=storage_->parent_frames||(storage_->parent_frames&&(!actor.state()->published||!actor.model()->ready()))){refuse(nullptr,"parent changed after publication");return 0;}const auto& c=storage_->children[entry];return c.allocated&&c.render_group&&storage_->leases[storage_->profile.entries[entry].channel]?storage_->models[entry][copy].draw(render,camera):0;}
    unsigned draw(const StartupActorFrame& actor,RenderEngine& render,const Camera& camera){
        if(!ready_||!storage_||!published_||pending_||!storage_->display)return 0;
        if(!bound(actor)||actor.state()->frames!=storage_->parent_frames||(storage_->parent_frames&&(!actor.state()->published||!actor.model()->ready()))){refuse(nullptr,"parent generation/frame changed after publication");return 0;}
        unsigned n=0;for(unsigned e=0;e<2;++e){const auto& child=storage_->children[e];if(child.allocated&&child.render_group&&storage_->leases[storage_->profile.entries[e].channel])for(auto& model:storage_->models[e])n+=model.draw(render,camera);}return n;
    }
    bool ready()const{return ready_&&bool(storage_);}
    bool bound(const StartupActorFrame& actor)const{return storage_&&actor.ready()&&actor.model()&&actor.state()&&actor.model()==storage_->model_target&&storage_->generation.lock()==actor.model()->instance_binding()&&!storage_->generation.expired()&&actor.state()->scalars.klass.model_id==storage_->profile.parent_model;}
    const std::array<StartupAuxiliaryState,2>* states()const{return ready()&&!pending_?&storage_->children:nullptr;}
    const std::array<u8,6>* leases()const{return ready()&&!pending_?&storage_->leases:nullptr;}
    const StartupAuxiliaryMotionObservation* motion()const{return ready()&&!pending_?&storage_->last_motion:nullptr;}
    u8 held_parent708_plan()const{return storage_?storage_->planned708:0;}
    bool sound_preupdate_requirement(const StartupActorFrame& actor,u32 frame,StartupThrusterSoundRequirement* out,std::string* error)const{
        if(out)*out={};
        if(!out||!ready_||!storage_||!pending_||!bound(actor)||frame!=storage_->next_frame||actor.state()->frames!=storage_->parent_frames||actor.state()->scalars.field708)return fail(error,"matching native pre-actor/held construction sound boundary required");
        const auto& source=actor.state()->scalars;StartupThrusterSoundRequirement candidate;candidate.frame=frame;candidate.parent_frame_before=actor.state()->frames;candidate.native_parent708=source.field708;candidate.construction708=storage_->planned708;candidate.pre709=source.field709;candidate.pre707=source.field707;
        if(candidate.pre709!=actor.state()->vm.state().actor.auxiliary)return fail(error,"native pre-VM auxiliary/scalar coherence required");
        candidate.refresh_required=storage_->actor_updates&&u8(candidate.pre709&candidate.construction708&u8(~candidate.pre707));*out=candidate;if(error)error->clear();return true;
    }
    u32 parent_frames()const{return storage_?storage_->parent_frames:0;}
    const StartupAuxiliaryModelScene* model(unsigned entry,unsigned copy)const{return ready()&&!pending_&&entry<2&&copy<2?&storage_->models[entry][copy]:nullptr;}
    void release(){storage_.reset();ready_=pending_=published_=false;}
private:
    struct Storage {
        StartupAuxiliaryProfile profile;StartupAuxiliaryTurnProfile turn;std::array<std::array<StartupAuxiliaryModelScene,2>,2> models;
        std::array<StartupAuxiliaryState,2> children{};std::array<bool,2> created{};std::array<u8,6> leases{},next_leases{};
        const CharacterModelScene* model_target=nullptr;std::weak_ptr<const CharacterModelBinding> generation;std::array<f32,3> entry_position{};StartupAuxiliaryMotionObservation last_motion;
        u8 planned708=0;u32 parent_frames=0,global_frame=0,next_frame=0;bool display=false,actor_updates=false,global_started=false;
    };
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup thruster graphics: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const auto detail=error?*error:std::string{};ready_=pending_=published_=false;if(storage_)for(auto& pair:storage_->models)for(auto& model:pair)model.discard_pose();fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    std::unique_ptr<Storage> storage_;bool ready_=false,pending_=false,published_=false;
};
} // namespace gf
