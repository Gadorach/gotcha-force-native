#pragma once
#include "engine/gameplay/battle_test.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/graphics/projected_triangle.hpp"
namespace gf {
// Proxy art ONLY. No original stage/Borg models are substituted or claimed.
class BattleTestView {
public:
    static Camera camera_for(const BattleTest& world,s32 width,s32 height){
        const auto& cfg=world.config();const auto eye=world.camera_eye(),interest=world.camera_interest();
        Camera camera;camera.eye[0]=eye.x;camera.eye[1]=0.7f+cfg.camera_height;camera.eye[2]=eye.z;
        camera.target[0]=interest.x;camera.target[1]=0.8f;camera.target[2]=interest.z;camera.near_plane=0.1f;camera.far_plane=cfg.arena_half*12;
        camera.fov_y_degrees=cfg.camera_fov;camera.viewport_width=width;camera.viewport_height=height;return camera;
    }
    static bool pointer_aim(const BattleTest& world,s32 width,s32 height,s32 x,s32 y,BattlePoint* out){
        if(!out||x<0||y<0||x>=width||y>=height)return false;
        const auto& cfg=world.config();const auto camera=camera_for(world,width,height);Camera::Basis b;if(!camera.basis(&b))return false;
        const f32 nx=2*f32(x)/width-1,ny=1-2*f32(y)/height;
        f32 ray[3];for(unsigned i=0;i<3;++i)ray[i]=b.forward[i]+b.right[i]*nx*b.tan_half_fov*b.aspect+b.up[i]*ny*b.tan_half_fov;
        if(std::abs(ray[1])<1e-6f)return false;
        const f32 t=(0.7f-camera.eye[1])/ray[1];if(t<=0)return false;
        const BattlePoint p{camera.eye[0]+ray[0]*t,camera.eye[2]+ray[2]*t};
        if(!std::isfinite(p.x)||!std::isfinite(p.z)||std::abs(p.x)>cfg.arena_half||std::abs(p.z)>cfg.arena_half)return false;
        *out=p;return true;
    }
    unsigned draw(RenderEngine& render,const BattleTest& world,s32 width,s32 height){
        if(!world.configured()||width<=0||height<=0)return 0;
        const auto saved=render.state();auto& state=render.state();state={};state.depth_test=state.depth_write=true;state.depth_compare=6;
        // Render seam factors are one-based (source-alpha=5, inverse=6), NOT GX's raw codes.
        state.colour_write=state.alpha_write=true;state.cull_backface=false;state.position_matrix=0;state.blend_src=5;state.blend_dst=6;
        const f32 identity[12]={1,0,0,0,0,1,0,0,0,0,1,0};if(!render.upload_matrix(0,identity)){state=saved;return 0;}
        const auto& cfg=world.config();const auto camera=camera_for(world,width,height);geometry_.clear();
        auto box=[&](f32 x,f32 y,f32 z,f32 hx,f32 hy,f32 hz,Tint color){
            Vertex corners[8];for(unsigned i=0;i<8;++i){auto& v=corners[i];v.x=x+(i&1?hx:-hx);v.y=y+(i&2?hy:-hy);v.z=z+(i&4?hz:-hz);}
            constexpr unsigned faces[6][4]={{1,3,7,5},{0,4,6,2},{2,6,7,3},{0,1,5,4},{4,5,7,6},{0,2,3,1}};
            constexpr f32 lights[6]={1,.72f,.9f,.55f,.8f,.62f};
            for(unsigned f=0;f<6;++f){const auto tint=shade(color,lights[f]);for(unsigned tri=0;tri<2;++tri){
                Vertex points[3]={corners[faces[f][0]],corners[faces[f][tri?2:1]],corners[faces[f][tri?3:2]]};
                for(auto& v:points){v.r=tint.r;v.g=tint.g;v.b=tint.b;v.a=tint.a;}project_triangle(camera,points,geometry_);}}
        };
        box(0,-0.25f,0,cfg.arena_half,0.2f,cfg.arena_half,{47,65,75,255});
        for(int i=-4;i<=4;++i){const f32 p=f32(i)*cfg.arena_half/4;box(p,-0.025f,0,0.015f,0.01f,cfg.arena_half,{74,93,99,255});box(0,-0.025f,p,cfg.arena_half,0.01f,0.015f,{74,93,99,255});}
        const auto player=world.player(),facing=world.facing();box(player.x,0.7f,player.z,cfg.actor_radius,0.7f,cfg.actor_radius,{58,170,244,255});
        box(player.x+facing.x,0.15f,player.z+facing.z,0.15f,0.1f,0.15f,{255,245,120,255});
        for(const auto& target:world.targets())if(target.health){box(target.position.x,0.8f,target.position.z,cfg.target_radius,0.8f,cfg.target_radius,target.ally?Tint{89,206,127,255}:Tint{224,87,65,255});
            box(target.position.x,2.0f,target.position.z,cfg.target_radius*f32(target.health)/cfg.target_health,0.08f,0.08f,{90,240,110,255});}
        for(const auto& shot:world.projectiles())if(shot.active)box(shot.position.x,0.7f,shot.position.z,cfg.projectile_radius,cfg.projectile_radius,cfg.projectile_radius,{255,233,85,255});
        if(world.locked()){const auto target=world.targets()[world.selected_target()].position;const f32 point[]={target.x,0.8f,target.z};std::size_t first=0,count=0;
            (void)ScreenDraw::ring(camera,point,cfg.target_radius*1.5f,0.08f,24,{255,230,75,255},geometry_,&first,&count);}
        unsigned draws=0;if(!geometry_.empty()){const auto base=render.append_vertices(geometry_.data(),geometry_.size());draws+=render.draw(Primitive::Triangles,base,u32(geometry_.size()))?1:0;}
        TextRenderer::Style style;style.scale=width>=600?2:1;
        const auto line="NATIVE BATTLE TEST - PROXY ART / TEST RULES\nWASD MOVE  SPACE/A FIRE  Z LOCK  Q/Y NEXT\nR ALLY/ENEMY  MOUSE CLICK AIM/FIRE\nE/X RESET  ENTER/START PAUSE  ESC QUIT\nTICK "+std::to_string(world.tick())+"  SHOTS "+std::to_string(world.shots())+"  HITS "+std::to_string(world.hits())+
            "  DOWN "+std::to_string(world.destroyed())+"\n"+(world.paused()?"PAUSED":world.locked()?(world.targeting_allies()?"ALLY ":"ENEMY ")+std::to_string(world.selected_target()):"FREE TETHER CAMERA");
        draws+=text_.draw(render,line,12,12,style);state=saved;return draws;
    }
private:std::vector<Vertex> geometry_;TextRenderer text_;
};
} // namespace gf
