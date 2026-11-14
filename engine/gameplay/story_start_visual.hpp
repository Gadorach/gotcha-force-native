#pragma once
#include <limits>
#include "engine/assets/story_start_art.hpp"
#include "engine/gameplay/story_start.hpp"
namespace gf {
struct StoryStartVisualWidget {
    StoryStartWidget source{};u8 context=0,life=0,phase=0;s16 counter=0,last_selection=-1;
    u16 angle_z=0;f32 frame=0;bool animated=false,visible=false;
};
struct StoryStartWidgetPose {
    u16 record=0,id=0,angle_z=0;std::array<f32,3> position{};f32 frame=0;bool animated=false;
    bool operator==(const StoryStartWidgetPose&)const=default;
};
//Source801D23FC/type callbacks. One call per accepted UI callback AFTER controller
//state publication; drawing/pass iteration never advances these clocks.
class StoryStartVisual {
public:
    bool load(const StoryStartArt& art,const SourceFont& font,std::string* error){
        *this={};if(!art.valid(font))return fail(error,"validated source art/font required");art_=art;ready_=true;return true;
    }
    bool step(const StoryStartPlan& plan,bool source_story_menu_major_active,u64 serial,std::string* error){
        if(!ready_||serial==0||serial_==std::numeric_limits<u64>::max()||serial!=serial_+1||!story_start_detail::valid(plan.state))return fail(error,"consecutive UI callback/declared controller state required");
        if(plan.build_menu){
            if(built_||plan.state.page!=StoryStartPage::Menu||plan.state.phase!=1||plan.state.counter!=15)return fail(error,"fresh source menu construction required");
            const u8 context=story_start_special_visible(plan.state.facts)?1:0;append(art_.groups[context],context);built_=true;
        }
        if(plan.build_play_numbers){
            if(!built_||plan.state.page!=StoryStartPage::PlayNumber)return fail(error,"play-number construction context");
            for(const auto& w:widgets_)if(w.context==4&&w.life<2)return fail(error,"duplicate play-number instances");
            append(art_.groups[2],4);
        }
        if(widgets_.size()>24)return fail(error,"source widget ownership budget");
        auto next=widgets_;const auto& state=plan.state;
        next.erase(std::remove_if(next.begin(),next.end(),[](const auto& w){return w.life==3;}),next.end());
        for(auto& w:next){
            if(w.life==2){w.life=3;w.visible=false;continue;}
            if(w.life==0)w.life=1;
            if(!source_story_menu_major_active)w.life=2;
            const unsigned type=w.source.record;
            if(type==0||type==7){
                if(w.phase==0){w.phase=1;w.counter=0;w.visible=true;}
                if(w.phase==1){request(w,float(w.counter));if(++w.counter>20)w.phase=2;}
            }else if(type==1||type==2){
                if(delay(w)){
                    const unsigned offset=w.context==1?2:0;
                    if(unsigned(state.selection)>=unsigned(w.context==1?3:2))w.visible=false;
                    else{w.visible=true;w.source.position[1]=art_.cursor_y[offset+unsigned(state.selection)];}
                }
                if(type==2)w.angle_z=u16(w.angle_z-256);
            }else if(type>=3&&type<=5){
                if(w.phase==3){
                    request(w,(state.blink&4)?1.f:0.f);
                    if(!state.blink){w.phase=2;w.last_selection=-1;}
                }else if(delay(w)){
                    w.visible=true;
                    if(w.last_selection!=state.selection){w.last_selection=state.selection;request(w,state.selection==s16(w.source.id-3)?0.f:2.f);}
                    if(w.last_selection==s16(w.source.id-3)&&state.blink&&state.page==StoryStartPage::Menu)w.phase=3;
                }
            }else if(type==6)w.visible=true;
            else if(type==8){
                if(w.phase==3){
                    request(w,float(w.last_selection+((state.blink&4)?8:0)));
                    if(!state.blink){w.phase=2;w.last_selection=-1;}
                }else if(delay(w)){
                    w.visible=true;if(w.last_selection!=state.play_selection){w.last_selection=state.play_selection;request(w,float(state.play_selection));}
                    if(state.blink)w.phase=3;
                }
            }else if(type==9){
                if(w.phase==3){request(w,(state.blink&4)?1.f:0.f);if(!state.blink)w.phase=2;}
                else{if(w.phase==0)request(w,0);if(delay(w)){w.visible=true;if(state.blink)w.phase=3;}}
            }else if(type==10){
                if(w.phase==0&&w.source.id==11)w.angle_z=0x8000;
                if(delay(w)){
                    if(++w.counter>70)w.counter=0;
                    w.visible=w.counter<50;
                    const auto count=story_start_play_count(state.action,state.facts);
                    if((w.source.id==10&&state.play_selection==0)||(w.source.id==11&&u32(state.play_selection)>=count-1))w.visible=false;
                }
            }else return fail(error,"unrepresented source widget type");
            if(w.context==4&&state.page!=StoryStartPage::PlayNumber)w.life=2;
        }
        widgets_=std::move(next);serial_=serial;selection_=state.selection;if(error)error->clear();return true;
    }
    std::vector<StoryStartWidgetPose> poses()const{
        std::vector<StoryStartWidgetPose> out;if(!ready_)return out;
        for(const auto& w:widgets_)if(w.visible&&w.life<3)out.push_back({w.source.record,w.source.id,w.angle_z,w.source.position,w.frame,w.animated});
        return out;
    }
    bool footer_visible()const{if(!ready_)return false;for(const auto& w:widgets_)if(w.source.id==6&&w.context<2&&w.visible&&w.life<3)return true;return false;}
    s16 selection()const{return selection_;}bool ready()const{return ready_;}u64 serial()const{return serial_;}
    u32 bank_hash()const{return ready_?art_.bank_hash:0;}
    const std::vector<StoryStartVisualWidget>& widgets()const{return widgets_;}
private:
    static void request(StoryStartVisualWidget& w,f32 frame){w.frame=frame;w.animated=true;}
    static bool delay(StoryStartVisualWidget& w){
        if(w.phase==0){w.phase=1;w.counter=5;w.last_selection=-1;}
        if(w.phase==1){if(--w.counter>0)return false;w.phase=2;w.counter=0;}
        return w.phase==2;
    }
    void append(const std::vector<StoryStartWidget>& group,u8 context){for(const auto& source:group){StoryStartVisualWidget w;w.source=source;w.context=context;widgets_.push_back(w);}}
    bool fail(std::string* error,const char* why){ready_=false;widgets_.clear();if(error)*error=std::string("Story start visual: ")+why;return false;}
    StoryStartArt art_;std::vector<StoryStartVisualWidget> widgets_;u64 serial_=0;s16 selection_=0;bool ready_=false,built_=false;
};
} //namespace gf
