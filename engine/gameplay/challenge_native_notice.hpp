#pragma once
#include "engine/gameplay/challenge_box_load_driver.hpp"
#include "engine/graphics/text.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
//Deliberately native UI, NOT recovered original memory-card text/art/timing.
//Input decisions stay in the guarded native load driver; drawing is read-only.
inline bool draw_challenge_native_notice(RenderEngine& render,ChallengeNativeLoadNotice notice){
    if(notice==ChallengeNativeLoadNotice::None)return true;
    const char* body=nullptr;const char* action="A OR B: USE DEFAULT DATA";
    switch(notice){
    case ChallengeNativeLoadNotice::Loaded:body="NATIVE SAVE DATA VALIDATED";action="A: USE DATA   B: CANCEL LOAD";break;
    case ChallengeNativeLoadNotice::Missing:body="NO NATIVE SAVE FILE";break;
    case ChallengeNativeLoadNotice::Unavailable:body="COULD NOT READ NATIVE SAVE";break;
    case ChallengeNativeLoadNotice::Invalid:body="NATIVE SAVE IS INVALID";break;
    default:return false;
    }
    TextRenderer text;TextRenderer::Style style;style.scale=2;
    for(const char* line:{"NATIVE SAVE FILE",body,"FILE LEFT UNCHANGED",action})if(text.measure_width(line,style)>480)return false;
    const auto saved=render.state();render.state()={};render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;
    const auto identity=mat4_identity();if(!render.upload_matrix(0,identity.m)){render.state()=saved;return false;}
    std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,64,156,576,324,0,0,0,1,1,{24,32,48,244});ScreenDraw::textured_quad(vertices,64,156,576,160,0,0,0,1,1,{72,168,232,255});
    const auto base=render.append_vertices(vertices.data(),vertices.size());bool ok=render.draw(Primitive::Triangles,base,u32(vertices.size()));
    ok=ok&&text.draw_centered(render,"NATIVE SAVE FILE",320,178,style)==1&&text.draw_centered(render,body,320,216,style)==1&&text.draw_centered(render,"FILE LEFT UNCHANGED",320,250,style)==1&&text.draw_centered(render,action,320,286,style)==1;
    render.state()=saved;return ok;
}
} // namespace gf
