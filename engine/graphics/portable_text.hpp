#pragma once
#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "engine/assets/portable_font.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"

namespace gf {
// The host passes a reserved shared slot between frames. Content never
// supplies a handle. The renderer retains the immutable texture for its
// session; the font object retains immutable metrics and a borrowed receipt.
class PortableFont {
public:
    bool bind(RenderEngine& render,u32 texture,const PortableFontMetrics& metrics,std::span<const u8> coverage,
              std::string* error=nullptr){
        auto fail=[&](const char* why){if(error)*error=why;return false;};
        if(!texture||render.frame_in_progress()||render.model_pass_active())return fail("font binding requires a shared slot between frames");
        if(!metrics.ready()||coverage.size()!=metrics.atlas_bytes())return fail("font alpha atlas has the wrong byte count");
        const auto cap=render.capabilities().max_texture_size;
        if(cap&&(metrics.width()>cap||metrics.height()>cap))return fail("font atlas exceeds renderer texture bounds");
        const auto* replacement=metrics.find(kPortableFontReplacement);
        if(!replacement)return fail("font metrics are unavailable");
        bool visible=false;
        for(unsigned y=replacement->y;y<unsigned(replacement->y)+replacement->height&&!visible;++y){
            const auto first=coverage.subspan(std::size_t(y)*metrics.width()+replacement->x,replacement->width);
            visible=std::any_of(first.begin(),first.end(),[](u8 a){return a!=0;});
        }
        if(!visible)return fail("font replacement glyph has no alpha coverage");
        std::vector<u8> rgba(coverage.size()*4,255);
        for(std::size_t i=0;i<coverage.size();++i)rgba[i*4+3]=coverage[i];
        const bool prior_upload_error=render.shared_texture_upload_failed();
        if(!render.upload_texture_shared(texture,{metrics.width(),metrics.height(),rgba.data(),0,0,false})){
            // Optional UI fallback owns its error; don't poison a later
            // legacy resource-bank validation or erase its prior failure.
            if(!prior_upload_error)render.clear_shared_texture_upload_error();
            return fail("font atlas shared upload was refused");
        }
        metrics_=metrics;texture_=render.shared_texture_receipt(texture);
        if(error)error->clear();
        return true;
    }
    bool available(const RenderEngine& render)const{return render.shared_texture_handle(texture_)!=0;}
    const PortableFontMetrics* metrics(const RenderEngine& render)const{return available(render)?&metrics_:nullptr;}
    void clear(){metrics_={};texture_={};}
private:
    friend class PortableTextRenderer;
    PortableFontMetrics metrics_;
    RenderSharedTextureReceipt texture_;
};

inline constexpr std::size_t kPortableTextMaxBytes=16u<<10,kPortableTextMaxUnits=4096;
struct PortableTextStyle {
    Tint tint;
    f32 scale=1;       // One design-to-output scale for both axes.
    f32 max_width=-1;  // Negative = unclipped; nonnegative = whole-glyph clip, per line.
};
struct PortableTextMeasure {
    f32 width=0,height=0;
    unsigned lines=0;
    bool clipped=false;
};

class PortableTextRenderer {
public:
    using Style=PortableTextStyle;
    PortableTextMeasure measure(const RenderEngine& render,const PortableFont* font,std::string_view text,
                                const Style& style={})const{
        if(!valid_style(style))return {};
        if(font&&font->available(render))return atlas_layout(font->metrics_,text,style,0,0,nullptr);
        return bitmap_layout(text,style,nullptr);
    }
    unsigned draw(RenderEngine& render,const PortableFont* font,std::string_view text,f32 x,f32 y,
                  const Style& style={})const{
        if(text.empty()||!render.frame_in_progress()||render.model_pass_active()||!valid_style(style)||!std::isfinite(x)||!std::isfinite(y)||
           std::abs(x)>1e7f||std::abs(y)>1e7f)return 0;
        const auto texture=font?render.shared_texture_handle(font->texture_):0;
        const auto saved=render.state();
        render.state()={};
        render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;
        render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;
        unsigned draws=0;
        if(texture){
            vertices_.clear();
            atlas_layout(font->metrics_,text,style,x,y,&vertices_);
            if(!vertices_.empty()){
                render.state().texture=texture;
                render.state().position_matrix=kIdentityPositionMatrix;
                const auto first=render.append_vertices(vertices_.data(),vertices_.size());
                draws=render.draw(Primitive::Triangles,first,u32(vertices_.size()))?1:0;
            }
        }else{
            fallback_.clear();
            bitmap_layout(text,style,&fallback_);
            draws=legacy_.draw_impl(render,fallback_,s32(x),s32(y),bitmap_style(style),false);
        }
        render.state()=saved;return draws;
    }
private:
    // Limits apply to input even if a page has zero visible triangles.
    class Scalars {
    public:
        explicit Scalars(std::string_view text):text_(text){}
        bool next(u32* out){
            if(text_.empty()||bytes_==kPortableTextMaxBytes||units_==kPortableTextMaxUnits)return false;
            const auto scalar=utf8_front(text_);
            if(scalar.bytes>kPortableTextMaxBytes-bytes_)return false;
            const auto used=std::max(1u,unsigned(scalar.bytes));
            *out=scalar.bytes&&(scalar.value=='\n'||unicode_display_scalar(scalar.value))?scalar.value:kPortableFontReplacement;
            text_.remove_prefix(used);bytes_+=used;++units_;return true;
        }
        bool remaining()const{return !text_.empty();}
    private:
        std::string_view text_;
        std::size_t bytes_=0,units_=0;
    };

    static bool valid_style(const Style& s){
        return std::isfinite(s.scale)&&s.scale>0&&s.scale<=16&&std::isfinite(s.max_width);
    }
    static PortableTextMeasure atlas_layout(const PortableFontMetrics& font,std::string_view text,const Style& style,
                                          f32 x,f32 y,std::vector<Vertex>* vertices){
        if(text.empty())return {};
        PortableTextMeasure out;out.lines=1;
        Scalars scalars(text);
        const auto atlas_w=f32(font.width()),atlas_h=f32(font.height()),scale=style.scale;
        const auto* replacement=font.find(kPortableFontReplacement);
        s32 pen=0,left=0,right=0;
        bool skip_line=false;std::size_t line_begin=0;
        auto end_line=[&](){
            out.width=std::max(out.width,f32(right-left)*scale);
            if(vertices){
                if(left)for(auto i=line_begin;i<vertices->size();++i)(*vertices)[i].x-=f32(left)*scale;
                line_begin=vertices->size();
            }
            pen=left=right=0;skip_line=false;
        };
        u32 code=0;
        while(scalars.next(&code)){
            if(code=='\n'){end_line();++out.lines;continue;}
            if(skip_line)continue;
            const auto* glyph=font.find(code);
            if(!glyph)glyph=replacement;
            const auto gx=pen+glyph->bearing_x;
            const auto next_pen=pen+glyph->advance;
            const auto next_left=glyph->ink()?std::min(left,gx):left;
            const auto next_right=std::max({right,next_pen,glyph->ink()?gx+glyph->width:0});
            if(style.max_width>=0&&f32(next_right-next_left)*scale>style.max_width){
                out.clipped=skip_line=true;continue;
            }
            if(vertices&&glyph->ink()){
                const auto gy=s32(out.lines-1)*font.line_height()+font.ascent()-glyph->bearing_y;
                ScreenDraw::textured_quad(*vertices,x+f32(gx)*scale,y+f32(gy)*scale,
                    x+f32(gx+glyph->width)*scale,y+f32(gy+glyph->height)*scale,0,
                    f32(glyph->x)/atlas_w,f32(glyph->y)/atlas_h,
                    f32(glyph->x+glyph->width)/atlas_w,f32(glyph->y+glyph->height)/atlas_h,style.tint);
            }
            pen=next_pen;left=next_left;right=next_right;
        }
        end_line();
        out.height=f32(out.lines)*font.line_height()*scale;out.clipped|=scalars.remaining();return out;
    }
    static TextRenderer::Style bitmap_style(const Style& style){
        const auto& t=style.tint;
        return {t.r,t.g,t.b,t.a,std::max(1,s32(std::round(style.scale)))};
    }
    static PortableTextMeasure bitmap_layout(std::string_view text,const Style& style,std::string* ascii){
        if(text.empty())return {};
        const auto bitmap=bitmap_style(style);
        constexpr auto max_ink=TextRenderer::kMaxVertices/(TextRenderer::kVerticesPerQuad*TextRenderer::kGlyphColumns*TextRenderer::kGlyphRows);
        std::size_t ink=0;
        PortableTextMeasure out;out.lines=1;
        unsigned glyphs=0;
        bool skip_line=false;
        Scalars scalars(text);u32 code=0;
        while(scalars.next(&code)){
            if(code=='\n'){
                if(ascii)ascii->push_back('\n');
                ++out.lines;glyphs=0;skip_line=false;continue;
            }
            if(skip_line)continue;
            const auto width=f32(glyphs*unsigned(TextRenderer::kGlyphColumns+bitmap.letter_spacing)+TextRenderer::kGlyphColumns)*bitmap.scale;
            if(style.max_width>=0&&width>style.max_width){out.clipped=skip_line=true;continue;}
            char c=code<0x80?char(code):'?';
            if(c>='a'&&c<='z')c=char(c-'a'+'A');
            if(c=='_')c='-';
            if(!TextRenderer::covers(c))c='?';
            if(c!=' '){
                if(ink==max_ink){out.clipped=true;break;}
                ++ink;
            }
            if(ascii)ascii->push_back(c);
            ++glyphs;out.width=std::max(out.width,width);
        }
        out.height=f32(out.lines)*f32((TextRenderer::kGlyphRows+bitmap.line_spacing)*bitmap.scale);
        out.clipped|=scalars.remaining();return out;
    }
    mutable std::vector<Vertex> vertices_;
    mutable std::string fallback_;
    TextRenderer legacy_;
};
}
