#pragma once
//Source Story+A4 is player-name text, NOT a map transform. Keyboard context0
//has maximum12 two-byte glyphs; context1's force-name maximum20 stays separate.
#include <string>
#include "engine/assets/source_font.hpp"
namespace gf {
inline constexpr unsigned kStoryPlayerNameGlyphs=12;
//Bounded portable prefix sufficient for12 glyphs+NUL. This is not a claim about
//the full original allocation/padding before the next known field+C4.
inline constexpr unsigned kStoryPlayerNamePrefixBytes=25;
inline bool story_player_name_text_valid(std::string_view encoded,const SourceFont& font){
    if(!font.ready()||encoded.size()%2||encoded.size()>kStoryPlayerNameGlyphs*2)return false;
    for(unsigned at=0;at<encoded.size();at+=2){
        if(!encoded[at]||!encoded[at+1])return false;
        const u16 code=u16(u16(u8(encoded[at]))<<8|u8(encoded[at+1]));if(code!=font.blank()&&font.glyph(code)<0)return false;
    }
    return true;
}
struct StoryPlayerNameProfile {
    std::string default_name;bool loaded=false;
    bool valid_for(const SourceFont& font)const{return loaded&&!default_name.empty()&&story_player_name_text_valid(default_name,font);}
    bool load(ByteView b,const SourceFont& font,std::string* error=nullptr){
        *this={};auto fail=[&](){if(error)*error="Story player name: default profile/schema/font";return false;};
        if(!b.data||b.size<16||b.size>40||b.be32(0)!=0x4746504eu||b.be16(4)!=1||b.be16(6)!=kStoryPlayerNameGlyphs||b.be16(8)!=kStoryPlayerNamePrefixBytes||b.be32(12)||b.size!=16+std::size_t(b.be16(10)))return fail();
        StoryPlayerNameProfile next;next.default_name.assign(reinterpret_cast<const char*>(b.data+16),b.be16(10));next.loaded=true;if(!next.valid_for(font))return fail();
        *this=std::move(next);if(error)error->clear();return true;
    }
};
} //namespace gf
