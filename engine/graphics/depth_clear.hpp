#pragma once
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
//8002A888: colour/alpha writes off, ALWAYS depth-write fullscreen quad. Native
// near/depth coordinates use0 for far; the backend and prior colour stay intact.
inline bool draw_depth_clear(RenderEngine& render,f32 width,f32 height){
    if(render.model_pass()||!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0||width>16384||height>16384)return false;
    std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,0,0,width,height,0,0,0,1,1,{0,0,0,0});
    const auto saved=render.state();render.state()={};render.state().colour_write=false;render.state().alpha_write=false;render.state().depth_test=true;render.state().depth_write=true;render.state().depth_compare=7;render.state().cull_backface=false;
    Mat4 identity;render.upload_matrix(0,identity.m);render.state().position_matrix=0;const auto first=render.append_vertices(vertices.data(),vertices.size());const bool ok=render.draw(Primitive::Triangles,first,u32(vertices.size()));render.state()=saved;return ok;
}
} // namespace gf
