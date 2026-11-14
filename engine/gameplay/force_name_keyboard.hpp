#pragma once
#include "engine/assets/force_name_art.hpp"
#include "engine/animation/weighted_slide.hpp"
namespace gf {
enum class ForceNamePhase {Closed,Opening,Editing,Flash,Fade,Closing,Done};
struct ForceNameEvents {bool committed=false;u16 sound=0;};
class ForceNameKeyboard {
public:
    bool open(const ForceNameArt& art,const SourceFont& font,std::string_view encoded){
        *this={};if(!art.valid_for(font)||encoded.size()%2||encoded.size()>art.maximum*2)return false;
        for(std::size_t i=0;i<encoded.size();i+=2){const auto code=u16(u16(u8(encoded[i]))<<8|u8(encoded[i+1]));if(code!=font.blank()&&font.glyph(code)<0)return false;}
        art_=art;text_=encoded;cursor_=unsigned(text_.size()/2);phase_=ForceNamePhase::Opening;offset_=-art.slide;return true;
    }
    bool step(u32 repeated,u32 pressed,ForceNameEvents* out){
        if(!out)return false;
        *out={};if(phase_==ForceNamePhase::Closed||phase_==ForceNamePhase::Done)return false;
        ++age_;if(title_started_&&++title_tick_==88)title_tick_=24;
        if(phase_==ForceNamePhase::Opening){if(integrate()){phase_=ForceNamePhase::Editing;title_started_=true;title_tick_=0;}offset_=f32(-art_.slide+scaled(art_.slide));dim_=u8(dim_+5);return true;}
        if(phase_==ForceNamePhase::Closing){if(integrate()){phase_=ForceNamePhase::Done;return true;}offset_=scaled(-art_.slide);dim_=u8(dim_-5);return true;}
        if(phase_==ForceNamePhase::Flash){advance_confirm();return true;}
        if(phase_==ForceNamePhase::Fade){if(fade_)--fade_;if(!fade_){phase_=ForceNamePhase::Closing;index_=0;sum_=0;dim_=160;}return true;}
        blink_=(blink_+1)%31;
        if(pressed&0x80){if(text_.empty()){out->sound=art_.sounds[3];return true;}committed_=true;out->committed=true;out->sound=art_.sounds[1];phase_=ForceNamePhase::Flash;flash_=30;blink_=30;advance_confirm();return true;}
        if(pressed&0x100){insert(selected(),out);return true;}
        if(pressed&0x200){if(cursor_){text_.erase((cursor_-1)*2,2);--cursor_;blink_=0;out->sound=art_.sounds[2];}return true;}
        const auto length=unsigned(text_.size()/2);bool changed=false;
        if(repeated&0x40){if(length<art_.maximum&&cursor_){--cursor_;changed=true;}}
        else if(repeated&0x20){if(length<art_.maximum&&cursor_<length){++cursor_;changed=true;}}
        else if(repeated&0x20002){column_=(column_+1)%15;changed=true;}
        else if(repeated&0x10001){column_=(column_+14)%15;changed=true;}
        else if(repeated&0x40004){row_=(row_+1)%7;changed=true;}
        else if(repeated&0x80008){row_=(row_+6)%7;changed=true;}
        if(changed){blink_=0;out->sound=art_.sounds[0];}return true;
    }
    // Host text input selects the matching visible key, then performs the same
    // insertion operation. The caller feeds at most one character per callback.
    bool type_ascii(unsigned char c,ForceNameEvents* out){
        if(!out)return false;
        *out={};if(phase_!=ForceNamePhase::Editing)return false;
        if(c>=128||!art_.ascii[c])return true;
        const auto found=std::find(art_.keys.begin(),art_.keys.end(),art_.ascii[c]);if(found==art_.keys.end())return false;const auto index=unsigned(found-art_.keys.begin());column_=index%15;row_=index/15;insert(*found,out);return true;
    }
    bool cancel(){if(committed_||phase_==ForceNamePhase::Closed||phase_==ForceNamePhase::Done)return false;phase_=ForceNamePhase::Done;return true;}
    ForceNamePhase phase()const{return phase_;}const std::string& text()const{return text_;}unsigned cursor()const{return cursor_;}unsigned column()const{return column_;}unsigned row()const{return row_;}u16 selected()const{return art_.keys[row_*15+column_];}
    unsigned age()const{return age_;}unsigned title_tick()const{return title_tick_;}bool title_visible()const{return title_started_&&title_tick_!=0;}bool committed()const{return committed_;}
    bool text_visible()const{return phase_==ForceNamePhase::Opening||phase_==ForceNamePhase::Editing||phase_==ForceNamePhase::Flash||phase_==ForceNamePhase::Fade;}
    //Screen-space text does not inherit the keyboard's slide. Publish it only
    //after the authored opening has actually completed; sliding art is separate.
    bool screen_text_visible()const{return text_visible()&&phase_!=ForceNamePhase::Opening;}
    bool cursor_visible()const{return text_visible()&&text_.size()/2<art_.maximum&&blink_<20;}bool visible()const{return phase_!=ForceNamePhase::Closed&&phase_!=ForceNamePhase::Done;}
    f32 offset()const{return offset_;}u8 dim()const{return dim_;}f32 underline_alpha()const{return phase_==ForceNamePhase::Fade?fade_*art_.fade_step:1.f;}
    u8 underline_tint()const{return phase_==ForceNamePhase::Flash&&(flash_&4)?0x88:0xff;}
private:
    void insert(u16 code,ForceNameEvents* out){if(text_.size()/2>=art_.maximum)return;const char pair[2]={char(code>>8),char(code)};text_.insert(cursor_*2,pair,2);++cursor_;blink_=0;out->sound=art_.sounds[1];}
    bool integrate(){return weighted_slide_advance(art_.weights,2,index_,sum_);}
    f32 scaled(f32 scale)const{return weighted_slide_scale(scale,sum_,art_.total);}
    void advance_confirm(){if(flash_)--flash_;if(!flash_){phase_=ForceNamePhase::Fade;fade_=16;}}
    ForceNameArt art_;std::string text_;ForceNamePhase phase_=ForceNamePhase::Closed;unsigned cursor_=0,column_=0,row_=0,blink_=0,age_=0,title_tick_=0,index_=0,flash_=0,fade_=0;f32 sum_=0,offset_=0;u8 dim_=0;bool committed_=false,title_started_=false;
};
} // namespace gf
