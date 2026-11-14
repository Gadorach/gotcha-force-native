#pragma once
#include "engine/assets/hsd_anim.hpp"
#include "engine/assets/source_font.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
//Visible coverage of the original tab word, not the transparent quad's centre.
//This is bounded layout inspection only; drawing stays in SourceTextRenderer.
struct ForceListTitleSlot {f32 x=0,y=0,z=0,height=0,width=0;};
inline bool force_list_title_slot(const HsdMesh& mesh,const HsdTexture& texture,const HsdAnimPose::Material& material,ForceListTitleSlot* out){
    if(out)*out={};
    if(!out||mesh.vertices.size()!=4||mesh.indices.size()!=6||mesh.deformation_required||!texture.width||!texture.height||texture.width>4096||texture.height>4096||texture.pixels().size()!=std::size_t(texture.width)*texture.height*4)return false;
    //Like MenuScene, static words use the decoded UVs when no live texture
    //callback is present. Absence of animation is not absence of a drawable word.
    const bool animated=material.present&&material.has_texture;
    const auto u_of=[&](const HsdVertex& vertex){return animated?vertex.uv[0]*material.uv_scale[0]+material.uv_offset[0]:vertex.uv[0];};
    const auto v_of=[&](const HsdVertex& vertex){return animated?vertex.uv[1]*material.uv_scale[1]+material.uv_offset[1]:vertex.uv[1];};
    f32 u0=1e6f,u1=-1e6f,v0=1e6f,v1=-1e6f,x0=1e6f,x1=-1e6f,y0=1e6f,y1=-1e6f;
    const f32 z=mesh.vertices[0].position[2];
    for(const auto& vertex:mesh.vertices){for(float p:vertex.position)if(!std::isfinite(p)||std::abs(p)>1e7f)return false;
        const f32 u=u_of(vertex),v=v_of(vertex);if(!std::isfinite(u)||!std::isfinite(v)||std::abs(vertex.position[2]-z)>.001f)return false;
        u0=std::min(u0,u);u1=std::max(u1,u);v0=std::min(v0,v);v1=std::max(v1,v);x0=std::min(x0,vertex.position[0]);x1=std::max(x1,vertex.position[0]);y0=std::min(y0,vertex.position[1]);y1=std::max(y1,vertex.position[1]);}
    if(u0<0||v0<0||u1>1||v1>1||u1<=u0||v1<=v0||x1<=x0||y1<=y0)return false;
    unsigned corners=0;for(const auto& vertex:mesh.vertices){const f32 u=u_of(vertex),v=v_of(vertex);
        const f32 x=x0+(u-u0)/(u1-u0)*(x1-x0),y=y1-(v-v0)/(v1-v0)*(y1-y0);if(std::abs(x-vertex.position[0])>.001f||std::abs(y-vertex.position[1])>.001f)return false;
        const bool right=std::abs(u-u1)<1e-6f,bottom=std::abs(v-v1)<1e-6f;if((!right&&std::abs(u-u0)>1e-6f)||(!bottom&&std::abs(v-v0)>1e-6f))return false;const unsigned bit=1u<<(unsigned(right)+2*unsigned(bottom));if(corners&bit)return false;corners|=bit;}
    std::array<unsigned,4> uses{};f32 area=0;for(unsigned i=0;i<6;i+=3){for(unsigned j=0;j<3;++j){if(mesh.indices[i+j]>=4)return false;++uses[mesh.indices[i+j]];}const auto& a=mesh.vertices[mesh.indices[i]],&b=mesh.vertices[mesh.indices[i+1]],&c=mesh.vertices[mesh.indices[i+2]];const f32 twice=(b.position[0]-a.position[0])*(c.position[1]-a.position[1])-(b.position[1]-a.position[1])*(c.position[0]-a.position[0]);if(twice==0)return false;area+=std::abs(twice)*.5f;}
    if(std::abs(area-(x1-x0)*(y1-y0))>.001f||std::any_of(uses.begin(),uses.end(),[](unsigned n){return !n||n>2;}))return false;
    f32 left=1e6f,right=-1e6f,top=1e6f,bottom=-1e6f;bool ink=false;const auto& pixels=texture.pixels();
    for(unsigned y=0;y<texture.height;++y)for(unsigned x=0;x<texture.width;++x){const auto at=(std::size_t(y)*texture.width+x)*4;if(!pixels[at+3]||!(pixels[at]||pixels[at+1]||pixels[at+2]))continue;
        const f32 a=std::max(u0,f32(x)/texture.width),b=std::min(u1,f32(x+1)/texture.width),c=std::max(v0,f32(y)/texture.height),d=std::min(v1,f32(y+1)/texture.height);if(a>=b||c>=d)continue;
        ink=true;left=std::min(left,a);right=std::max(right,b);top=std::min(top,c);bottom=std::max(bottom,d);}
    if(!ink)return false;
    out->x=x0+((left+right)*.5f-u0)/(u1-u0)*(x1-x0);out->y=y1-((top+bottom)*.5f-v0)/(v1-v0)*(y1-y0);out->z=z;out->height=(bottom-top)/(v1-v0)*(y1-y0);out->width=x1-x0;return true;
}
//The original encoded font keeps its ordinary fixed advance. Measure its actual
//ink without changing glyphs, atlas gutters, row spacing or any old draw API.
inline bool force_list_title_text(const SourceFont& font,std::string_view text,f32 extent,f32 advance,const ForceListTitleSlot& slot,const Mat4& world,std::array<f32,3>* position,f32* scale){
    if(position)*position={};
    if(scale)*scale=0;
    if(!position||!scale||!font.ready()||text.empty()||text.size()>1024||text.size()%2||!std::isfinite(extent)||extent<=0||extent>10000||!std::isfinite(advance)||advance<=0||advance>10000)return false;
    for(f32 v:{slot.x,slot.y,slot.z,slot.height,slot.width})if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    if(slot.height<=0||slot.width<=0)return false;
    //The list's title root is translation plus positive uniform XY scale only.
    for(f32 v:world.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    if(world.m[0]<=0||world.m[0]!=world.m[5]||world.m[1]!=0||world.m[2]!=0||world.m[4]!=0||world.m[6]!=0||world.m[8]!=0||world.m[9]!=0||world.m[10]!=1||world.m[12]!=0||world.m[13]!=0||world.m[14]!=0||world.m[15]!=1)return false;
    f32 left=1e6f,right=-1e6f,bottom=1e6f,top=-1e6f,pen=0;bool ink=false;
    for(std::size_t at=0;at<text.size();at+=2){const auto code=u16(u16(u8(text[at]))<<8|u8(text[at+1]));if(code!=font.blank()){const auto glyph=font.glyph(code);if(glyph<0)return false;
        for(unsigned y=0;y<font.cell();++y)for(unsigned x=0;x<font.cell();++x){const auto pixel=((std::size_t(unsigned(glyph)/font.columns())*font.cell()+y)*font.image().width+(unsigned(glyph)%font.columns())*font.cell()+x)*4;const auto& rgba=font.image().rgba;if(!rgba[pixel+3]||!(rgba[pixel]||rgba[pixel+1]||rgba[pixel+2]))continue;
            ink=true;left=std::min(left,pen-extent*.5f+f32(x)*extent/font.cell());right=std::max(right,pen-extent*.5f+f32(x+1)*extent/font.cell());bottom=std::min(bottom,extent*.5f-f32(y+1)*extent/font.cell());top=std::max(top,extent*.5f-f32(y)*extent/font.cell());}}
        pen+=advance;}
    if(!ink||top<=bottom)return false;
    const f32 size=slot.height/(top-bottom);if((right-left)*size>slot.width)return false;
    f32 centre[3];const f32 local[3]={slot.x,slot.y,slot.z};mat4_transform_point(world,local,centre);const f32 result=size*world.m[0];*scale=result;*position={centre[0]-(left+right)*.5f*result,centre[1]-(bottom+top)*.5f*result,centre[2]};return true;
}
} //namespace gf
