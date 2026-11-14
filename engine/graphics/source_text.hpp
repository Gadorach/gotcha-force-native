#pragma once
#include "engine/assets/source_font.hpp"
#include "engine/graphics/source_text_metrics.hpp"
#include "engine/graphics/transform.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/pointer_triangles.hpp"
namespace gf {
class SourceTextRenderer {
public:
    bool load(const SourceFont& font,RenderEngine& render,u32 texture){
        font_=font;texture_=0;metrics_={};if(!font.ready()||!texture)return false;const auto& image=font_.image();
        if(!render.upload_texture_shared(texture,{image.width,image.height,image.rgba.data(),0,0,true}))return false;
        texture_=texture;return true;
    }
    bool load_measured(const SourceFont& font,const SourceTextMetrics& metrics,RenderEngine& render,u32 texture){
        if(!metrics.for_font(font)){font_={};texture_=0;metrics_={};return false;}
        if(!load(font,render,texture))return false;
        metrics_=metrics;return true;
    }
    // Source801C53DC draws XY quads;801C5600 uses XZ. Callers supply source
    // placement, full quad extent and the already-computed half-step advance.
    bool draw(RenderEngine& render,const Camera& camera,const Mat4& world,std::string_view encoded,
               f32 extent,f32 advance,Tint tint,bool xz=false,u32 low=0,u32 high=0xffffffffu,PointerTriangles* submitted=nullptr){
        if(render.model_pass()&&render.model_pass()!=2)return true;
        if(!std::isfinite(advance)||std::abs(advance)>10000)return false;
        return draw_world(render,camera,world,encoded,extent,[&](u16){return advance;},tint,xz,low,high,submitted);
    }
    bool draw_measured(RenderEngine& render,const Camera& camera,const Mat4& world,std::string_view encoded,f32 extent,Tint tint,bool xz=false,u32 low=0,u32 high=0xffffffffu){
        if(!metrics_.ready())return false;
        if(render.model_pass()&&render.model_pass()!=2)return true;
        return draw_world(render,camera,world,encoded,extent,[&](u16 code){return metrics_.advance(code,extent);},tint,xz,low,high);
    }
private:
    template<class Advance>bool draw_world(RenderEngine& render,const Camera& camera,const Mat4& world,std::string_view encoded,f32 extent,Advance advance,Tint tint,bool xz,u32 low,u32 high,PointerTriangles* submitted=nullptr){
        if(!texture_||encoded.size()>1024||encoded.size()%2||!std::isfinite(extent)||extent<=0||extent>10000)return false;
        for(float v:world.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
        if(world.m[12]!=0||world.m[13]!=0||world.m[14]!=0||world.m[15]!=1)return false;
        std::vector<Vertex> vertices;vertices.reserve(encoded.size()*3);f32 pen=0;
        for(std::size_t at=0;at<encoded.size();at+=2){const auto code=u16(u16(u8(encoded[at]))<<8|u8(encoded[at+1]));if(code==font_.blank()){pen+=advance(code);continue;}const auto glyph=font_.glyph(code);if(glyph<0)return false;
            const auto uv=glyph_uv(unsigned(glyph));const auto u=uv[0],v=uv[1],du=uv[2],dv=uv[3];const f32 half=extent*.5f;
            const std::array<std::array<f32,2>,4> corners={{{pen-half,half},{pen+half,half},{pen+half,-half},{pen-half,-half}}};
            std::array<Vertex,4> quad;bool visible=true;
            for(unsigned i=0;i<4;++i){float local[3]={corners[i][0],xz?0:corners[i][1],xz?-corners[i][1]:0},point[3];mat4_transform_point(world,local,point);float sx=0,sy=0,depth=0;if(!camera.project(point,&sx,&sy,&depth)){visible=false;break;}quad[i].x=sx;quad[i].y=sy;quad[i].z=depth;quad[i].r=tint.r;quad[i].g=tint.g;quad[i].b=tint.b;quad[i].a=tint.a;quad[i].u=u+((i==1||i==2)?du:0);quad[i].v=v+(i>=2?dv:0);}
            if(visible)for(unsigned index:{0u,1u,2u,0u,2u,3u})vertices.push_back(quad[index]);
            pen+=advance(code);
        }
        if(vertices.empty())return true;
        f32 depth=0;if(render.model_pass()){Camera::Basis basis;if(!camera.basis(&basis))return false;for(unsigned i=0;i<3;++i)depth+=(world.m[i*4+3]-basis.eye[i])*basis.forward[i];}
        const bool ok=submit(render,vertices,low,high,true,depth);if(ok&&submitted)submitted->append(vertices);return ok;
    }
public:
    //80097AC4 screen glyphs: top-left anchored24*scale squares. Reference pixel
    //scales are explicit, without a fitted world-camera substitute.
    bool draw_screen(RenderEngine& render,std::string_view encoded,f32 x,f32 y,f32 extent,f32 advance,f32 sx,f32 sy,u32 low,u32 high){
        if(!std::isfinite(advance)||std::abs(advance)>1e7f)return false;
        return draw_screen_impl(render,encoded,x,y,extent,[&](u16){return advance;},sx,sy,low,high);
    }
    bool draw_screen_measured(RenderEngine& render,std::string_view encoded,f32 x,f32 y,f32 extent,f32 sx,f32 sy,u32 low,u32 high){
        return metrics_.ready()&&draw_screen_impl(render,encoded,x,y,extent,[&](u16 code){return metrics_.advance(code,extent);},sx,sy,low,high);
    }
    bool measure(std::string_view encoded,f32 extent,SourceTextMeasure* out,bool outline=true)const{return metrics_.measure(encoded,extent,out,outline);}
    const SourceFont& font()const{return font_;}
private:
    template<class Advance>bool draw_screen_impl(RenderEngine& render,std::string_view encoded,f32 x,f32 y,f32 extent,Advance advance,f32 sx,f32 sy,u32 low,u32 high){
        if(!texture_||render.model_pass()||encoded.size()>1024||encoded.size()%2)return false;
        for(float v:{x,y,extent,sx,sy})if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
        if(extent<=0||extent>10000||sx<=0||sy<=0)return false;
        std::vector<Vertex> vertices;f32 pen=0;
        for(std::size_t at=0;at<encoded.size();at+=2){const auto code=u16(u16(u8(encoded[at]))<<8|u8(encoded[at+1]));const f32 step=advance(code);if(!std::isfinite(step)||std::abs(step)>1e7f)return false;if(code==font_.blank()){pen+=step;continue;}const int glyph=font_.glyph(code);if(glyph<0)return false;const auto uv=glyph_uv(unsigned(glyph));ScreenDraw::textured_quad(vertices,(x+pen)*sx,y*sy,(x+pen+extent)*sx,(y+extent)*sy,0,uv[0],uv[1],uv[0]+uv[2],uv[1]+uv[3],{255,255,255,255});pen+=step;}
        return vertices.empty()||submit(render,vertices,low,high,false,0);
    }
    std::array<f32,4> glyph_uv(unsigned glyph)const{const auto cell=font_.cell();return {f32(glyph%font_.columns()*cell)/font_.image().width,f32(glyph/font_.columns()*cell)/font_.image().height,f32(cell)/font_.image().width,f32(cell)/font_.image().height};}
    bool submit(RenderEngine& render,const std::vector<Vertex>& vertices,u32 low,u32 high,bool depth_test,f32 depth){
        const auto saved=render.state();render.state()={};render.state().texture=texture_;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().texture_colour_lerp=true;render.state().texture_low_colour=low;render.state().texture_high_colour=high;render.state().depth_test=depth_test;render.state().depth_write=false;render.state().cull_backface=false;Mat4 identity;render.upload_matrix(0,identity.m);render.state().position_matrix=0;const auto first=render.append_vertices(vertices.data(),vertices.size());const bool ok=render.model_pass()?render.draw_model(Primitive::Triangles,first,u32(vertices.size()),depth):render.draw(Primitive::Triangles,first,u32(vertices.size()));render.state()=saved;return ok;
    }
    SourceFont font_;SourceTextMetrics metrics_;u32 texture_=0;
};
} // namespace gf
