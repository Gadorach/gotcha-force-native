#pragma once
#include "engine/gameplay/model_pass_queue.hpp"
#include "engine/gameplay/startup_scene_lighting.hpp"
#include "engine/gameplay/menu_scene.hpp"
namespace scene_pass_fixture {
inline gf::StartupScenePassProfile profile(){
    gf::StartupScenePassProfile p;p.loaded=true;p.bank_hash=123;p.translucent_groups={0,1,2,-1,3,-1,-1,-1};p.passes={1,4,2,2,2,2};p.overlay_order={1,0,2,3};p.selector=p.scene33=p.sort_enabled=1;p.forward=p.backward=15;p.hold=90;p.target_colours={0x303030ff,0x404040ff,0x505050ff};p.fog_type=2;p.fog_start=1000000;p.fog_end=2000000;return p;
}
inline gf::SceneLighting rig(){gf::SceneLighting r;r.declared=true;r.lights.resize(3);for(unsigned i=0;i<3;++i){auto& l=r.lights[i];l.flags=4;for(unsigned k=0;k<3;++k)l.colour[k]=gf::u8(150+i*20);l.colour[3]=0;}return r;}
inline gf::HsdScene scene(gf::u32 mode,float z,gf::u8 red=255){
    gf::HsdScene s;s.joints.resize(1);s.joints[0].translation[2]=z;gf::HsdMesh m;m.render_mode=mode;m.material_alpha=.5f;m.blend=(mode&0x40000000u)!=0;m.depth_write=!m.blend;m.cull_backface=false;m.material_colour[0]=red;
    const float points[3][3]={{-1,-1,0},{0,1,0},{1,-1,0}};for(const auto& p:points){gf::HsdVertex v;std::copy_n(p,3,v.position);v.normal[2]=1;m.vertices.push_back(v);}m.indices={0,1,2};s.meshes.push_back(m);s.mesh_joint.push_back(0);return s;
}
} // namespace scene_pass_fixture
