#pragma once
#include "engine/graphics/source_text_metrics.hpp"
namespace gf {
//Original fixed-advance font measurement, including alpha/outline ink and one
//atlas texel of conservative linear-sampling support. Unlike measured word-font
//advances this permits intentional kerning/overlap and never changes the atlas.
class SourceFixedTextMetrics {
public:
    bool load(const SourceFont& font){
        *this={};if(!font.ready())return false;cell_=font.cell();blank_=font.blank();
        const auto& image=font.image();
        for(unsigned g=0;g<font.codes().size();++g){
            Glyph item;item.code=font.codes()[g];item.box={cell_,cell_,0,0};
            for(unsigned y=0;y<cell_;++y)for(unsigned x=0;x<cell_;++x){
                const auto at=((std::size_t(g/font.columns())*cell_+y)*image.width+(g%font.columns())*cell_+x)*4;
                if(image.rgba[at+3]){item.ink=true;item.box[0]=std::min(item.box[0],u16(x));item.box[1]=std::min(item.box[1],u16(y));item.box[2]=std::max(item.box[2],u16(x+1));item.box[3]=std::max(item.box[3],u16(y+1));}
            }
            if(item.ink){item.box[0]=item.box[0]?u16(item.box[0]-1):0;item.box[1]=item.box[1]?u16(item.box[1]-1):0;item.box[2]=std::min(cell_,u16(item.box[2]+1));item.box[3]=std::min(cell_,u16(item.box[3]+1));}
            glyphs_.push_back(item);
        }
        key_=source_text_metrics_key(font);ready_=true;return true;
    }
    bool for_font(const SourceFont& font)const{return ready_&&key_==source_text_metrics_key(font);}
    bool measure(std::string_view encoded,f32 extent,f32 advance,SourceTextMeasure* out)const{
        if(out)*out={};
        if(!out||!ready_||encoded.size()%2||encoded.size()>1024||!std::isfinite(extent)||extent<=0||extent>10000||!std::isfinite(advance)||advance<=0||advance>10000)return false;
        SourceTextMeasure next;f32 pen=0;const f32 scale=extent/cell_;
        for(unsigned at=0;at<encoded.size();at+=2){
            const u16 code=u16(u16(u8(encoded[at]))<<8|u8(encoded[at+1]));
            if(code!=blank_){const Glyph* found=nullptr;for(const auto& g:glyphs_)if(g.code==code){found=&g;break;}if(!found)return false;
                if(found->ink){const auto& b=found->box;const f32 l=pen+b[0]*scale,t=b[1]*scale,r=pen+b[2]*scale,d=b[3]*scale;
                    if(!next.ink){next.left=l;next.top=t;next.right=r;next.bottom=d;next.ink=true;}
                    else{next.left=std::min(next.left,l);next.top=std::min(next.top,t);next.right=std::max(next.right,r);next.bottom=std::max(next.bottom,d);}}
            }
            pen+=advance;if(!std::isfinite(pen)||pen>1e7f)return false;
        }
        next.advance=pen;*out=next;return true;
    }
private:
    struct Glyph {u16 code=0;std::array<u16,4> box{};bool ink=false;};
    std::vector<Glyph> glyphs_;u32 key_=0;u16 cell_=0,blank_=0;bool ready_=false;
};
} //namespace gf
