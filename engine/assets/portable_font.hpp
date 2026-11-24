#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <string>
#include <string_view>
#include <vector>
#include "engine/assets/borg_package.hpp" // Shared local-path and manifest row rules.

namespace gf {
inline constexpr std::size_t kPortableFontManifestBytes=256u<<10;
inline constexpr unsigned kPortableFontAtlasMaxDimension=1024,kPortableFontMaxGlyphs=4096;
inline constexpr u32 kPortableFontReplacement=0xfffd;

struct PortableGlyph {
    u32 scalar=0;
    u16 x=0,y=0,width=0,height=0;
    s16 bearing_x=0,bearing_y=0;
    u16 advance=0;
    bool ink()const{return width&&height;}
};

// Immutable parsed metadata after successful load; loading raw coverage is
// separate so a host can use the validated path and exact byte limit.
class PortableFontMetrics {
public:
    static bool parse(std::string_view text,PortableFontMetrics* out,std::string* error=nullptr){
        auto fail=[&](const char* why){if(error)*error=why;return false;};
        if(!out||text.size()>kPortableFontManifestBytes)return fail("font manifest unavailable or over budget");
        if(borg_package_detail::line(text)!="GFFONT 1")return fail("unsupported font manifest header");
        PortableFontMetrics next;
        bool ascent_seen=false;
        while(!text.empty()){
            const auto row=borg_package_detail::line(text);
            if(row.empty())continue;
            if(row.starts_with('#')){
                if(!comment_valid(row))return fail("font comment contains invalid UTF-8 or controls");
                continue;
            }
            const auto equal=row.find('=');
            if(equal==std::string_view::npos)return fail("malformed font row");
            const auto key=row.substr(0,equal),value=row.substr(equal+1);
            if(key=="atlas"){
                if(!next.atlas_path_.empty()||!borg_package_asset_path_valid(value))return fail("invalid or duplicate atlas path");
                next.atlas_path_=value;
            }else if(key=="size"){
                std::array<std::string_view,2> fields;
                if(next.width_||!borg_package_detail::split_fields(value,&fields)||
                   !number(fields[0],1,kPortableFontAtlasMaxDimension,&next.width_)||
                   !number(fields[1],1,kPortableFontAtlasMaxDimension,&next.height_))return fail("invalid or duplicate atlas size");
            }else if(key=="line-height"){
                if(next.line_height_||!number(value,1,256,&next.line_height_))return fail("invalid or duplicate font line height");
            }else if(key=="ascent"){
                if(ascent_seen||!number(value,0,256,&next.ascent_))return fail("invalid or duplicate font ascent");
                ascent_seen=true;
            }else if(key=="glyph"){
                if(next.glyphs_.size()>=kPortableFontMaxGlyphs)return fail("font glyph count exceeds budget");
                PortableGlyph glyph;
                if(!read_glyph(value,&glyph))return fail("invalid font glyph row");
                next.glyphs_.push_back(glyph);
            }else return fail("unknown font field");
        }
        if(next.atlas_path_.empty()||!next.width_||!next.line_height_||!ascent_seen||next.ascent_>next.line_height_)
            return fail("font requires atlas, size, line height and ascent");
        std::sort(next.glyphs_.begin(),next.glyphs_.end(),[](const auto& a,const auto& b){return a.scalar<b.scalar;});
        u32 previous=0;
        for(const auto& glyph:next.glyphs_){
            if(glyph.scalar==previous)return fail("duplicate font glyph");
            if(glyph.ink()&&(unsigned(glyph.x)+glyph.width>next.width_||unsigned(glyph.y)+glyph.height>next.height_))
                return fail("font glyph rectangle outside atlas");
            if(glyph.ink()){
                const s32 top=s32(next.ascent_)-glyph.bearing_y;
                next.ink_top_=std::min(next.ink_top_,top);
                next.ink_bottom_=std::max(next.ink_bottom_,top+glyph.height);
            }
            previous=glyph.scalar;
        }
        const auto* space=next.find(0x20);
        if(!space||space->ink()||!space->advance)return fail("font requires a positive-advance blank space glyph");
        const auto* replacement=next.find(kPortableFontReplacement);
        if(!replacement||!replacement->ink()||!replacement->advance)return fail("font requires an inked positive-advance replacement glyph");
        *out=std::move(next);if(error)error->clear();return true;
    }

    bool ready()const{return !atlas_path_.empty();}
    std::string_view atlas_path()const{return atlas_path_;}
    u16 width()const{return width_;}
    u16 height()const{return height_;}
    std::size_t atlas_bytes()const{return std::size_t(width_)*height_;}
    u16 line_height()const{return line_height_;}
    u16 ascent()const{return ascent_;}
    s32 ink_top()const{return ready()?ink_top_:0;}
    s32 ink_height()const{return ink_bottom_>ink_top_?ink_bottom_-ink_top_:0;}
    const PortableGlyph* find(u32 scalar)const{
        const auto it=std::lower_bound(glyphs_.begin(),glyphs_.end(),scalar,
            [](const PortableGlyph& glyph,u32 code){return glyph.scalar<code;});
        return it!=glyphs_.end()&&it->scalar==scalar?&*it:nullptr;
    }
private:
    static bool comment_valid(std::string_view text){
        while(!text.empty()){
            const auto scalar=utf8_front(text);
            if(!scalar.bytes||(scalar.value!='\t'&&!unicode_display_scalar(scalar.value)))return false;
            text.remove_prefix(scalar.bytes);
        }
        return true;
    }
    template<class T>static bool number(std::string_view text,s32 low,s32 high,T* out){
        if(text.empty()||text.size()>5)return false;
        s32 value=0;const auto end=text.data()+text.size();
        const auto result=std::from_chars(text.data(),end,value);
        if(result.ec!=std::errc{}||result.ptr!=end||value<low||value>high)return false;
        *out=T(value);return true;
    }
    static bool codepoint(std::string_view token,u32* out){
        if(!token.starts_with("U+"))return false;
        token.remove_prefix(2);
        if(token.size()<4||token.size()>6||(token.size()>4&&token.front()=='0'))return false;
        u32 scalar=0;
        for(char c:token){
            if(c>='0'&&c<='9')scalar=(scalar<<4)|u32(c-'0');
            else if(c>='A'&&c<='F')scalar=(scalar<<4)|u32(c-'A'+10);
            else return false;
        }
        if(!unicode_display_scalar(scalar))return false;
        *out=scalar;return true;
    }
    static bool read_glyph(std::string_view text,PortableGlyph* out){
        std::array<std::string_view,8> f;
        auto& g=*out;
        if(!borg_package_detail::split_fields(text,&f)||!codepoint(f[0],&g.scalar)||
           !number(f[1],0,kPortableFontAtlasMaxDimension,&g.x)||!number(f[2],0,kPortableFontAtlasMaxDimension,&g.y)||
           !number(f[3],0,kPortableFontAtlasMaxDimension,&g.width)||!number(f[4],0,kPortableFontAtlasMaxDimension,&g.height)||
           !number(f[5],-256,256,&g.bearing_x)||!number(f[6],-256,256,&g.bearing_y)||!number(f[7],0,256,&g.advance))return false;
        return g.ink()||(!g.x&&!g.y&&!g.width&&!g.height&&!g.bearing_x&&!g.bearing_y);
    }

    std::string atlas_path_;
    u16 width_=0,height_=0,line_height_=0,ascent_=0;
    s32 ink_top_=std::numeric_limits<s32>::max(),ink_bottom_=std::numeric_limits<s32>::min();
    std::vector<PortableGlyph> glyphs_;
};
}
