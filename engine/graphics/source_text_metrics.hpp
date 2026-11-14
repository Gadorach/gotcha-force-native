#pragma once
#include <array>
#include <cmath>
#include "engine/assets/source_font.hpp"
namespace gf {
//Native measured text layout. The immutable font pixels remain owned by the
//existing SourceFont/SourceTextRenderer, not an alternative font/render engine.
inline u32 source_text_metrics_key(const SourceFont& font){
    if(!font.ready())return 0;
    u32 hash=2166136261u;const auto byte=[&](u8 b){hash=(hash^b)*16777619u;};
    for(u32 value:{font.image().width,font.image().height,u32(font.cell()),u32(font.columns())})for(unsigned shift:{24u,16u,8u,0u})byte(u8(value>>shift));
    for(auto b:font.image().rgba)byte(b);
    for(auto code:font.codes()){byte(u8(code>>8));byte(u8(code));}return hash;
}
struct SourceTextMeasure {f32 left=0,top=0,right=0,bottom=0,advance=0;bool ink=false;};
class SourceTextMetrics {
public:
    bool load(ByteView profile,const SourceFont& font,std::string* error){
        *this={};const auto fail=[&](const char* why){if(error)*error=std::string("source text metrics: ")+why;return false;};
        if(!font.ready()||!profile.data||profile.size<20||profile.be32(0)!=0x4746544du||profile.be32(4)!=1)return fail("header/font");
        const auto count=profile.be16(12),cell=profile.be16(14),space=profile.be16(16);const auto key=source_text_metrics_key(font);
        if(!count||count!=font.codes().size()||cell!=font.cell()||!space||space>cell||profile.be16(18)||profile.size!=20+std::size_t(count)*20||profile.be32(8)!=key)return fail("binding/count/extent");
        std::vector<Glyph> next;
        for(unsigned i=0;i<count;++i){const auto at=20+i*20;Glyph glyph;glyph.code=profile.be16(at);glyph.advance=profile.be16(at+2);for(unsigned k=0;k<4;++k){glyph.ink[k]=profile.be16(at+4+k*2);glyph.alpha[k]=profile.be16(at+12+k*2);}
            const auto box=[&](const auto& b){return b[0]<b[2]&&b[1]<b[3]&&b[2]<=cell&&b[3]<=cell;};
            if(glyph.code!=font.codes()[i]||!glyph.advance||glyph.advance>cell||!box(glyph.ink)||!box(glyph.alpha)||glyph.advance<=glyph.ink[2]-glyph.ink[0]||glyph.alpha[0]>glyph.ink[0]||glyph.alpha[1]>glyph.ink[1]||glyph.alpha[2]<glyph.ink[2]||glyph.alpha[3]<glyph.ink[3])return fail("glyph bounds/advance");
            if(!glyph.alpha[0]||!glyph.alpha[1]||glyph.alpha[2]>=cell||glyph.alpha[3]>=cell)return fail("transparent sampler gutter");
            std::array<u16,4> actual_ink={cell,cell,0,0},actual_alpha=actual_ink;const auto include=[&](auto& b,unsigned x,unsigned y){b[0]=std::min(b[0],u16(x));b[1]=std::min(b[1],u16(y));b[2]=std::max(b[2],u16(x+1));b[3]=std::max(b[3],u16(y+1));};
            for(unsigned y=0;y<cell;++y)for(unsigned x=0;x<cell;++x){const auto pixel=((std::size_t(i/font.columns())*cell+y)*font.image().width+(i%font.columns())*cell+x)*4;const auto* rgba=font.image().rgba.data()+pixel;if(rgba[3]){include(actual_alpha,x,y);if(rgba[0]||rgba[1]||rgba[2])include(actual_ink,x,y);}}
            if(actual_ink!=glyph.ink||actual_alpha!=glyph.alpha)return fail("atlas ink/alpha bounds differ");
            next.push_back(glyph);
        }
        glyphs_=std::move(next);key_=key;cell_=cell;space_=space;blank_=font.blank();ready_=true;return true;
    }
    bool for_font(const SourceFont& font)const{return ready_&&key_==source_text_metrics_key(font);}
    bool ready()const{return ready_;}
    f32 advance(u16 code,f32 extent)const{if(!ready_)return 0;if(code==blank_)return f32(space_)*extent/cell_;for(const auto& glyph:glyphs_)if(glyph.code==code)return f32(glyph.advance)*extent/cell_;return 0;}
    bool measure(std::string_view encoded,f32 extent,SourceTextMeasure* out,bool include_outline=true)const{
        if(out)*out={};
        if(!out||!ready_||encoded.size()%2||encoded.size()>1024||!std::isfinite(extent)||extent<=0||extent>10000)return false;
        SourceTextMeasure next;f32 pen=0;const f32 scale=extent/cell_;
        for(std::size_t at=0;at<encoded.size();at+=2){const u16 code=u16(u16(u8(encoded[at]))<<8|u8(encoded[at+1]));if(code!=blank_){const Glyph* item=nullptr;for(const auto& g:glyphs_)if(g.code==code){item=&g;break;}if(!item)return false;const auto& b=include_outline?item->alpha:item->ink;const f32 left=pen+b[0]*scale,top=b[1]*scale,right=pen+b[2]*scale,bottom=b[3]*scale;if(!next.ink){next.left=left;next.top=top;next.right=right;next.bottom=bottom;next.ink=true;}else{next.left=std::min(next.left,left);next.top=std::min(next.top,top);next.right=std::max(next.right,right);next.bottom=std::max(next.bottom,bottom);}}
            pen+=advance(code,extent);if(!std::isfinite(pen)||pen>1e7f)return false;
        }next.advance=pen;*out=next;return true;
    }
private:
    struct Glyph {u16 code=0,advance=0;std::array<u16,4> ink{},alpha{};};
    std::vector<Glyph> glyphs_;u32 key_=0;u16 cell_=0,space_=0,blank_=0;bool ready_=false;
};
} //namespace gf
