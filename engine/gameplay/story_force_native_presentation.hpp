#pragma once
#include "engine/gameplay/story_force_native_assets.hpp"
#include "engine/gameplay/story_force_selection.hpp"
#include "engine/gameplay/screen_fade.hpp"
namespace gf {
enum class StoryForceNativePhase {Closed,Load,Initialize,Enumerate,RevealWait,Enter,Selecting,Result,Failed};
enum class StoryForceNativeChoiceKind {None,Stored,Editor,Back,Bypass};
namespace story_force_native_detail {struct Scope {bool live=true;};}
class StoryForceNativeChoice {
public:
    bool current(const ForceInventory& owner)const{const auto s=scope_.lock();const auto binding=owner_.lock();return s&&s->live&&binding&&owner.ready()&&binding==owner.instance_binding().lock()&&owner.revision()==revision_&&(kind_!=StoryForceNativeChoiceKind::Editor||edit_.current(owner))&&(kind_!=StoryForceNativeChoiceKind::Stored||owner.current(roster_));}
    StoryForceNativeChoiceKind kind()const{return kind_;}u32 source_result()const{return result_;}unsigned force()const{return force_;}
    u8 port()const{return port_;}
    const StoryForceSetupFacts& facts()const{return facts_;}const ForceBattleRoster& stored()const{return roster_;}const StoryForceEditorRequest& editor()const{return edit_;}
private:
    friend class StoryForceNativePresentation;std::weak_ptr<story_force_native_detail::Scope> scope_;std::weak_ptr<const ForceInventoryScope> owner_;u64 revision_=0;StoryForceNativeChoiceKind kind_=StoryForceNativeChoiceKind::None;u32 result_=0;unsigned force_=kForceCount;u8 port_=0;StoryForceSetupFacts facts_;ForceBattleRoster roster_;StoryForceEditorRequest edit_;
};
struct StoryForceNativeEvent {
    std::array<u16,2> sounds{};unsigned sound_count=0;
    bool request_source_pools=false,request_music_stops=false,request_setup_music=false;u16 request_music_fade=0;
    //All service fields are unfulfilled requirements, not audio/pool ACKs.
    StoryForceNativeChoiceKind destination=StoryForceNativeChoiceKind::None;
};
//Owns bounded native assets, fade, input sampling and controller/page publication.
//Native synchronous I/O is NOT original asynchronous load duration or a campaign
//load receipt. The host must keep the same RenderEngine alive while this owns it.
class StoryForceNativePresentation {
public:
    StoryForceNativePresentation()=default;StoryForceNativePresentation(const StoryForceNativePresentation&)=delete;StoryForceNativePresentation& operator=(const StoryForceNativePresentation&)=delete;
    StoryForceNativePresentation(StoryForceNativePresentation&&)=default;StoryForceNativePresentation& operator=(StoryForceNativePresentation&&)=default;
    bool open(const StoryMapRebuildProfile& profile,const StoryForceSetupFacts& facts,const ForceInventory& owner,unsigned requested_force,u8 port,u8 source_controller_mask,const StoryForceNativeSpec& spec){
        close();StoryForceSetupPlan plan;if(!story_force_setup_plan(profile,facts,&plan)||!owner.ready()||requested_force>=kForceCount||port>=4)return false;
        if(plan.selector_enabled){if(!(source_controller_mask&15u)||(source_controller_mask&0xf0u))return false;unsigned first=0;while(!(source_controller_mask&(1u<<first)))++first;if(first!=port)return false;}
        profile_=profile;facts_=facts;spec_=spec;owner_=owner.instance_binding();revision_=owner.revision();requested_=requested_force;port_=port;energy_=plan.energy;enabled_=plan.selector_enabled;scope_=std::make_shared<story_force_native_detail::Scope>();phase_=StoryForceNativePhase::Load;return true;
    }
    bool step(IFileSystem& files,RenderEngine& render,const ForceInventory& owner,const std::array<SourceSetupPad,4>& pads,StoryForceNativeEvent* out,std::string* error){
        if(!out)return false;
        *out={};if(phase_==StoryForceNativePhase::Closed||phase_==StoryForceNativePhase::Failed||phase_==StoryForceNativePhase::Result)return false;
        if(!current(owner)||callbacks_>=8192||(assets_.ready()&&!assets_.bound(owner,render)))return fail(error,"live owner/renderer/callback budget");
        StoryForceNativeEvent effects;
        bool created=false,update_widgets=false;
        switch(phase_){
        case StoryForceNativePhase::Load:
            if(!enabled_){result_.kind_=StoryForceNativeChoiceKind::Bypass;effects.destination=result_.kind_;finish_result(owner);break;}
            effects.request_source_pools=true;effects.request_music_stops=true;
            if(!assets_.load(files,render,owner,spec_,error))return fail(error,"native asset preparation");
            phase_=StoryForceNativePhase::Initialize;break;
        case StoryForceNativePhase::Initialize:
            if(!assets_.bound(owner,render))return fail(error,"native prepared resource binding");
            fade_.clear();if(!fade_.request(0,60,ScreenFadeDirection::Reveal))return fail(error,"entry reveal request");effects.request_setup_music=true;phase_=StoryForceNativePhase::Enumerate;break;
        case StoryForceNativePhase::Enumerate:
            source_setup_input_step(pads[port_],&history_);shown_=requested_;
            if(!owner.state()->forces[shown_].count){shown_=kForceCount;for(unsigned i=0;i<kForceCount;++i)if(owner.state()->forces[i].count){shown_=i;break;}}
            if(shown_>=kForceCount||!assets_.open_widgets(owner,shown_,&widgets_))return fail(error,"nonempty rows/widget construction");
            has_page_=true;created=true;phase_=StoryForceNativePhase::RevealWait;break;
        case StoryForceNativePhase::RevealWait:
            source_setup_input_step(pads[port_],&history_);update_widgets=true;
            //Source F8248 reads COUNT, not active. Its Enter callback is later.
            if(!fade_.remaining()){fade_.clear();phase_=StoryForceNativePhase::Enter;}break;
        case StoryForceNativePhase::Enter:
            //The earlier enumeration was a read-only UI binding. Birth of the
            //prepared controller rebuilds/validates the same frozen inventory,
            //then its first step samples ONCE using the last reveal history.
            if(!selector_.open_prepared(profile_,facts_,owner,requested_,port_,history_))return fail(error,"prepared controller birth");
            phase_=StoryForceNativePhase::Selecting;
            [[fallthrough]];
        case StoryForceNativePhase::Selecting:{
            StoryForceEvent event;if(!selector_.step(owner,pads,fade_.remaining()==0,&event))return fail(error,"selector callback");history_=selector_.input();shown_=selector_.selected_force();effects.sounds=event.sounds;effects.sound_count=event.sound_count;
            if(event.request_cover){if(!fade_.request(0,event.request_cover,ScreenFadeDirection::Cover))return fail(error,"cover request");effects.request_music_fade=event.request_cover;}
            if(event.destination!=StoryForceDestination::None){
                result_.force_=event.destination==StoryForceDestination::Back?kForceCount:shown_;
                if(event.destination==StoryForceDestination::Stored){if(!selector_.take_roster(owner,&result_.roster_))return fail(error,"current stored roster candidate");result_.kind_=StoryForceNativeChoiceKind::Stored;result_.result_=shown_+1;}
                else if(event.destination==StoryForceDestination::Editor){result_.kind_=StoryForceNativeChoiceKind::Editor;result_.result_=0x8001u+shown_;result_.edit_=event.edit;}
                else{result_.kind_=StoryForceNativeChoiceKind::Back;result_.result_=0xffffffffu;}
                effects.destination=result_.kind_;finish_result(owner);break;
            }
            update_widgets=true;break;}
        default:return fail(error,"phase");
        }
        if(!current(owner))return fail(error,"owner changed during callback");
        if(has_page_){if(!created&&update_widgets&&!widgets_.step(owner,shown_,selector_.interactive_mask(),selector_.closing_mask()))return fail(error,"widget callback");if(!assets_.card_plan(owner,shown_,energy_,&cards_))return fail(error,"current card candidate");}
        //One owned advance AFTER this callback; no caller fade-finished bool.
        fade_.advance();++callbacks_;*out=effects;if(error)error->clear();return true;
    }
    bool draw(RenderEngine& render,const ForceInventory& owner,std::string* error){
        if(phase_==StoryForceNativePhase::Closed||phase_==StoryForceNativePhase::Failed)return false;
        if(!current(owner))return fail(error,"owner changed before draw");
        if(has_page_&&!assets_.draw(render,widgets_,cards_,owner,error))return fail(error,"native page draw");
        if(fade_.draw()){
            std::vector<Vertex> vertices;const auto colour=fade_.colour();ScreenDraw::textured_quad(vertices,0,0,640,480,0,0,0,1,1,{u8(colour>>24),u8(colour>>16),u8(colour>>8),u8(colour)});const auto saved=render.state();render.state()={};render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().depth_test=render.state().depth_write=render.state().cull_backface=false;Mat4 identity;if(!render.upload_matrix(0,identity.m)){render.state()=saved;return fail(error,"fade matrix upload");}render.state().position_matrix=0;const auto first=render.append_vertices(vertices.data(),vertices.size());const bool ok=render.draw(Primitive::Triangles,first,u32(vertices.size()));render.state()=saved;if(!ok)return fail(error,"native fade overlay");
        }
        return true;
    }
    bool take_choice(const ForceInventory& owner,StoryForceNativeChoice* out){if(out)*out={};if(!out||phase_!=StoryForceNativePhase::Result||taken_||!result_.current(owner))return false;*out=result_;taken_=true;return true;}
    void close(){if(scope_)scope_->live=false;scope_.reset();assets_.release();selector_=StoryForceSelection{};widgets_={};cards_={};result_={};history_={};fade_={};owner_.reset();revision_=0;callbacks_=0;phase_=StoryForceNativePhase::Closed;has_page_=taken_=false;}
    StoryForceNativePhase phase()const{return phase_;}unsigned callbacks()const{return callbacks_;}bool has_page()const{return has_page_;}bool assets_ready()const{return assets_.ready();}
    const ScreenFade& fade()const{return fade_;}const SourceSetupButtons& input()const{return history_;}StoryForcePhase selector_phase()const{return selector_.phase();}unsigned shown_force()const{return shown_;}
    const StoryForceSetupFacts& facts()const{return facts_;}
private:
    bool current(const ForceInventory& owner)const{const auto scope=owner_.lock();return scope_&&scope_->live&&scope&&owner.ready()&&scope==owner.instance_binding().lock()&&owner.revision()==revision_;}
    bool fail(std::string* error,const char* why){if(scope_)scope_->live=false;has_page_=false;assets_.release();selector_=StoryForceSelection{};widgets_={};cards_={};phase_=StoryForceNativePhase::Failed;if(error&&error->empty())*error=std::string("Story native force presentation: ")+why;return false;}
    void finish_result(const ForceInventory& owner){result_.scope_=scope_;result_.owner_=owner_;result_.revision_=owner.revision();result_.facts_=facts_;result_.port_=port_;fade_.clear();has_page_=false;assets_.release();phase_=StoryForceNativePhase::Result;}
    StoryForceNativeAssets assets_;StoryMapRebuildProfile profile_;StoryForceSetupFacts facts_;StoryForceNativeSpec spec_;StoryForceSelection selector_;StoryForceWidgets widgets_;StoryForceCardPlan cards_;ScreenFade fade_;SourceSetupButtons history_;StoryForceNativeChoice result_;
    std::weak_ptr<const ForceInventoryScope> owner_;std::shared_ptr<story_force_native_detail::Scope> scope_;u64 revision_=0;u32 callbacks_=0;unsigned requested_=0,shown_=kForceCount;u8 port_=0;s32 energy_=0;StoryForceNativePhase phase_=StoryForceNativePhase::Closed;bool enabled_=false,has_page_=false,taken_=false;
};
} //namespace gf
