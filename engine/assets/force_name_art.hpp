#pragma once
#include "engine/assets/source_font.hpp"
#include <array>
#include <cmath>
namespace gf {
struct ForceNameWidget {u16 record=0,type=0;std::array<f32,3> position{};};
struct ForceNameArt {
    u32 bank_hash=0;u16 maximum=0,cursor_code=0;std::vector<ForceNameWidget> widgets;
    std::array<u16,105> keys{};std::array<u16,128> ascii{};std::array<f32,15> x{};std::array<f32,7> y{};std::array<s16,15> screen_x{};std::array<s16,7> screen_y{};
    std::array<f32,64> weights{};f32 total=0,slide=0,title_start=0,title_delta=0,title_y=0,title_pulse=0,fade_step=0,underline_divisor=0,cell=0,name_scale=0,preview_scale=0,angle_unit=0,screen_width=0,screen_height=0,name_advance=0,name_x=0,name_y=0,preview_x=0,preview_y=0;
    // Source style table: KONST/fill (high texture intensity), then TEV0/glow.
    std::array<std::array<u32,2>,2> colours{};u32 dim_colour=0;std::array<u16,4> sounds{};
    bool load(ByteView b,const SourceFont& font,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="name art: GFNK extent/domain";return false;};
        if(!b.data||b.size<20||b.size>8192||b.be32(0)!=0x47464e4bu||b.be32(4)!=1||!font.ready())return fail();
        bank_hash=b.be32(8);maximum=b.be16(12);const auto count=b.be16(14);if(maximum!=20||!count||count>64||b.be16(16)!=15||b.be16(18)!=7)return fail();
        std::size_t at=20;bool valid=true;auto word=[&](){if(!b.contains(at,4)){valid=false;return u32(0);}auto v=b.be32(at);at+=4;return v;};auto half=[&](){if(!b.contains(at,2)){valid=false;return u16(0);}auto v=b.be16(at);at+=2;return v;};auto real=[&](){auto v=bits_to_f32(word());if(!std::isfinite(v)||std::abs(v)>1e6f)valid=false;return v;};auto glyph=[&](u16 c){return c==font.blank()||font.glyph(c)>=0;};
        for(unsigned i=0;i<count;++i){const auto record=word(),type=word();if(record>=4096||type>9)return fail();ForceNameWidget w;w.record=u16(record);w.type=u16(type);for(auto& p:w.position)p=real();widgets.push_back(w);}
        for(auto& key:keys){key=half();if(!glyph(key))return fail();}for(auto& key:ascii){key=half();if(key&&std::find(keys.begin(),keys.end(),key)==keys.end())return fail();}
        for(auto& v:x)v=real();
        for(auto& v:y)v=real();
        for(auto& v:screen_x)v=std::bit_cast<s16>(half());
        for(auto& v:screen_y)v=std::bit_cast<s16>(half());
        for(auto& w:weights){w=real();if(w<=0)return fail();total=f32(total+w);}
        slide=real();title_start=real();title_delta=real();title_y=real();title_pulse=real();fade_step=real();underline_divisor=real();cell=real();name_scale=real();preview_scale=real();angle_unit=real();screen_width=real();screen_height=real();name_advance=real();name_x=real();name_y=real();preview_x=real();preview_y=real();
        for(auto& pair:colours)for(auto& c:pair)c=word();
        cursor_code=half();if(half())return fail();dim_colour=word();for(auto& s:sounds)s=half();
        if(!valid||at!=b.size||!bank_hash||!glyph(cursor_code)||!std::isfinite(total)||total<=0||slide<=0||title_start<=0||title_y<=0||title_pulse<0||title_pulse>=1||fade_step!=.0625f||underline_divisor<=0||cell<=0||name_scale<=0||preview_scale<=0||angle_unit<=0||screen_width<=0||screen_height<=0||name_advance<=0)return fail();
        if(!valid_for(font))return fail();
        if(error)error->clear();
        return true;
    }
    bool valid_for(const SourceFont& font)const{
        if(!font.ready()||!bank_hash||maximum!=20||widgets.empty()||widgets.size()>64)return false;
        auto finite=[](f32 v){return std::isfinite(v)&&std::abs(v)<=1e6f;};
        auto glyph=[&](u16 c){return c&&((c&255)!=0)&&(c==font.blank()||font.glyph(c)>=0);};
        for(const auto& w:widgets){if(w.record>=4096||w.type>9)return false;for(auto v:w.position)if(!finite(v))return false;}
        for(auto c:keys)if(!glyph(c))return false;
        for(auto c:ascii)if(c&&std::find(keys.begin(),keys.end(),c)==keys.end())return false;
        for(auto v:x)if(!finite(v))return false;
        for(auto v:y)if(!finite(v))return false;
        f32 sum=0;for(auto w:weights){if(!finite(w)||w<=0)return false;sum=f32(sum+w);}
        if(!finite(total)||sum!=total||total<=0||!glyph(cursor_code))return false;
        for(auto v:{slide,title_start,title_delta,title_y,title_pulse,fade_step,underline_divisor,cell,name_scale,preview_scale,angle_unit,screen_width,screen_height,name_advance,name_x,name_y,preview_x,preview_y})if(!finite(v))return false;
        return slide>0&&title_start>0&&title_y>0&&title_pulse>=0&&title_pulse<1&&fade_step==.0625f&&underline_divisor>0&&cell>0&&name_scale>0&&preview_scale>0&&angle_unit>0&&screen_width>0&&screen_height>0&&name_advance>0;
    }
};
} // namespace gf
