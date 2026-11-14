#pragma once
#include "engine/gameplay/story_guide_popup.hpp"
#include "engine/gameplay/story_guide_indicator.hpp"
#include "engine/gameplay/source_byte_rng_owner.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
namespace gf {
struct StoryGuideMessageEvent {bool indicator_born=false,indicator_released=false,random_consumed=false,selector_requested=false;u8 random_byte=0,selector=0;};
//Native prepared message+indicator ownership. Original voice/script/editor
//readiness/campaign seed/save/mission authority remain separate and unfulfilled.
class StoryGuideMessage {
public:
    StoryGuideMessage()=default;StoryGuideMessage(const StoryGuideMessage&)=delete;StoryGuideMessage& operator=(const StoryGuideMessage&)=delete;
    bool load(ByteView archive,const StoryGuidePopupArt& popup_art,const StoryGuideIndicatorArt& art,const SourceFont& font,const SourceMessageBank& text,RenderEngine& render,const SourceByteRngOwner& rng,std::string* error){
        close();if(!rng.ready()||!art.valid()||!popup_art.ready||art.bank_hash!=popup_art.bank_hash||!bank_.load(archive,error,true)||bank_.hash()!=art.bank_hash||bank_.records()!=3||!popup_.load(archive,popup_art,font,text,render,error))return fail(error,"bound original resources/current RNG owner");
        HsdRecordAnimationBinding binding;if(!bank_.select_compact_textures(art.record,2,0,0,&binding,error)||!binding.scene()||binding.scene()->joints.size()!=1||binding.scene()->meshes.size()!=2)return fail(error,"complete indicator record");
        const auto& source=*binding.scene();if(source.joints[0].flags!=0x20080000u)return fail(error,"source C54 root flags already present");for(const auto& mesh:source.meshes)if(mesh.render_mode!=0x60002011u||!mesh.blend||mesh.depth_write)return fail(error,"source XLU indicator state");
        art_=art;camera_=popup_art.camera;render_=&render;rng_=rng.snapshot();ready_=true;return true;
    }
    bool begin(unsigned source_message,const SourceByteRngOwner& rng,std::string* error){
        if(!ready_||!rng.same_owner(rng_)||!popup_.begin(source_message,error))return false;
        indicator_={};model_.release();if(error)error->clear();return true;
    }
    bool callback(RenderEngine& render,SourceByteRngOwner& rng,bool fresh_a,StoryGuideMessageEvent* out,std::string* error){
        if(out)*out={};
        if(!out||!ready_||&render!=render_||!rng.same_owner(rng_)||render.model_pass())return fail(error,"same serialized render/RNG owners");
        const auto prior=popup_.frame();if(!popup_.step(fresh_a,error))return fail(error,"native parent callback");const auto& parent=popup_.frame();const bool birth=prior.phase==1&&parent.phase==2&&parent.counter==8;const f32 width=parent.maximum_line<10?5.f:parent.maximum_line>40?20.f:f32(parent.maximum_line)*.5f;
        StoryGuideIndicatorPlan plan;if(!story_guide_indicator_plan(art_,indicator_,birth,parent.characters,width,&plan))return fail(error,"indicator callback plan");StoryGuideMessageEvent event;event.indicator_born=plan.birth;event.indicator_released=plan.release;event.selector_requested=plan.request_selector;
        if(plan.birth){if(!model_.load_compact_textures(bank_,art_.record,2,0,0,error)||!model_.request(0,0,error))return fail(error,"fresh indicator model");model_.set_enabled(true);model_.upload_textures_shared(render,111000);if(render.shared_texture_upload_failed())return fail(error,"checked indicator upload");}
        if(plan.random_required){u8 byte=0;if(!rng.next_byte(&byte))return fail(error,"current native byte RNG consumption");plan.state.selector=u8(byte%3);event.random_consumed=true;event.random_byte=byte;}
        //Source RNG effects above are real native stream effects. A later native
        //draw/request refusal invalidates this component; NEVER rewind/replay.
        if(plan.request_selector&&!model_.request_again(f32(plan.state.selector),0,error))return fail(error,"same indicator source selector");
        if(plan.release)model_.release();
        if(plan.state.render_group){const f32 p[3]={plan.state.x,art_.position[1],art_.position[2]};if(!model_.set_root_world(mat4_translation(p),error))return fail(error,"source indicator world");}
        if(!popup_.draw(render,error))return fail(error,"native parent draw");
        //Both components use the same shared passes/camera. This explicit native
        //composition is not original GObj traversal/pool-order/GPU completion.
        if(plan.state.render_group)for(u8 pass:{u8(1),u8(4),u8(2)}){if(!render.begin_model_pass(pass))return fail(error,"indicator pass");model_.draw(render,camera_);if(!render.end_model_pass())return fail(error,"indicator submission");}
        indicator_=plan.state;event.selector=indicator_.selector;*out=event;if(error)error->clear();return true;
    }
    void close(){model_.release();bank_={};popup_=StoryGuidePopup{};rng_={};render_=nullptr;ready_=false;indicator_={};}
    bool ready()const{return ready_;}bool active()const{return ready_&&popup_.active();}const StoryGuidePopupFrame& frame()const{return popup_.frame();}const StoryGuideIndicatorState& indicator()const{return indicator_;}const HsdAnimPose* indicator_pose()const{return ready_&&model_.ready()?model_.pose():nullptr;}
private:
    bool fail(std::string* error,const char* why){ready_=false;model_.release();if(error)*error=std::string("Story guide message: ")+why;return false;}
    StoryGuidePopup popup_;StoryGuideIndicatorArt art_;StoryGuideIndicatorState indicator_;HsdRecordAnimationBank bank_;AnimatedModelScene model_;Camera camera_;RenderEngine* render_=nullptr;SourceByteRngSnapshot rng_;bool ready_=false;
};
} //namespace gf
