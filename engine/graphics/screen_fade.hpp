#pragma once
#include "engine/gameplay/screen_fade.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
//One shared screen-fade quad through the regular render seam, not a new backend.
//Source fade's per-callback draw flag is authoritative, including alpha0 draws.
inline bool draw_screen_fade(RenderEngine& render,const ScreenFade& fade,unsigned width,unsigned height){
    if(!width||!height||width>16384||height>16384)return false;
    if(!fade.draw())return true;
    const u32 colour=fade.colour();const Tint tint{u8(colour>>24),u8(colour>>16),u8(colour>>8),u8(colour)};
    const auto saved=render.state();render.state()={};render.state().blend=tint.a!=255;render.state().blend_src=5;render.state().blend_dst=6;
    render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;
    const auto identity=mat4_identity();if(!render.upload_matrix(0,identity.m)){render.state()=saved;return false;}
    std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,0,0,f32(width),f32(height),0,0,0,1,1,tint);
    const auto first=render.append_vertices(vertices.data(),vertices.size());const bool ok=render.draw(Primitive::Triangles,first,u32(vertices.size()));render.state()=saved;return ok;
}
} // namespace gf
