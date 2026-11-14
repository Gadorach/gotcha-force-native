#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/gameplay/menu_scene.hpp"

namespace {
void check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"render repair: %s\n",text);std::abort();}}
struct Backend final:gf::IRenderBackend {
    std::vector<gf::Vertex> vertices;std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}
    bool upload_vertices(const gf::Vertex* v,std::size_t n)override{vertices.assign(v,v+n);return true;}
    bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}
    bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();}
    void submit(const gf::DrawCall& c)override{calls.push_back(c);}
    void end_frame()override{}void present()override{}
};
gf::HsdScene fixture(gf::u32 flags=0){
    gf::HsdScene s;s.joints.resize(1);auto& j=s.joints[0];j.flags=flags;
    j.translation[0]=10;j.translation[1]=20;j.translation[2]=-100;j.scale[0]=2;j.scale[1]=3;
    gf::HsdMesh m;m.render_mode=2;m.material_colour[0]=10;m.material_colour[1]=10;m.material_colour[2]=10;
    for(auto p:std::vector<std::pair<float,float>>{{-1,-1},{1,-1},{1,1},{-1,1}}){gf::HsdVertex v;v.position[0]=p.first;v.position[1]=p.second;v.colour[0]=200;v.colour[1]=100;v.colour[2]=50;m.vertices.push_back(v);}
    m.indices={0,1,2,0,2,3};s.meshes.push_back(m);s.mesh_joint={0};return s;
}
gf::Camera camera(){gf::Camera c;c.near_plane=1;c.far_plane=1000;return c;}
unsigned draw(gf::MenuScene& scene,Backend& backend,const gf::Camera& c=camera()){gf::RenderEngine r(backend);r.begin_frame({});r.reset_arena();const auto n=scene.draw(r,c);r.end_frame();return n;}
}
int main(){
    std::string error;unsigned cases=0;
    // This is the exact bug that left the seven tall effects visible.
    {gf::MenuScene s;Backend b;check(s.load_scene(fixture(0x10),&error),"hidden load");check(draw(s,b)==0,"HIDDEN omits own geometry");
     gf::HsdAnimPose pose;pose.joints.resize(1);pose.joints[0].translation[2]=-100;
     check(s.set_pose(&pose),"visibility override");check(draw(s,b)==1,"animation NODE shows node");
     pose.joints[0].hidden=true;check(draw(s,b)==1,"pose is copied, not borrowed");
     check(s.set_pose(&pose),"hide pose");check(draw(s,b)==0,"animated hidden omits geometry");++cases;}
    // Translation occurs once; inherited nonuniform scale is retained.
    {gf::MenuScene s;Backend b;check(s.load_scene(fixture(0x200),&error),"full billboard load");s.set_show_source_markers(true);check(draw(s,b)==1,"billboard draw");
     float lo[2]={1e9f,1e9f},hi[2]={-1e9f,-1e9f};for(auto& v:b.vertices){lo[0]=std::min(lo[0],v.x);hi[0]=std::max(hi[0],v.x);lo[1]=std::min(lo[1],v.y);hi[1]=std::max(hi[1],v.y);}
     const auto c=camera();float x=0,y=0,q=0;const float p[3]={10,20,-100};check(c.project(p,&x,&y,&q),"project origin");
     check(std::fabs((lo[0]+hi[0])/2-x)<0.01f && std::fabs((lo[1]+hi[1])/2-y)<0.01f,"no double translation");
     check(std::fabs(hi[0]-lo[0]-c.size_scale(4,q))<0.01f,"billboard X scale");
     check(std::fabs(hi[1]-lo[1]-c.size_scale(6,q))<0.01f,"billboard Y scale");++cases;}
    {gf::MenuScene s;Backend b;check(s.load_scene(fixture(0x100),&error),"TEXGEN load");check(draw(s,b)==1,"TEXGEN is not a billboard/source marker");++cases;}
    {auto f=fixture();f.meshes[0].cull_frontface=true;gf::MenuScene s;Backend b;check(s.load_scene(f,&error),"override load");s.set_depth_test(false);s.set_cull_backface(false);draw(s,b);
     check(!b.calls[0].depth_test&&!b.calls[0].depth_write&&!b.calls[0].cull_frontface&&!b.calls[0].cull_backface,"diagnostics override each mesh");
     check(b.vertices[0].r==200&&b.vertices[0].g==100,"vertex diffuse mode does not multiply material colour");++cases;}
    {const float scale[3]={2,1,1},normal[3]={1,1,0};float out[3];gf::mat4_transform_normal(gf::mat4_scale(scale),normal,out);
     check(std::fabs(out[0]-0.4472136f)<1e-5f&&std::fabs(out[1]-0.8944272f)<1e-5f,"inverse-transpose normals");++cases;}
    {auto c=camera();gf::Vertex v[3];v[0].x=-0.1f;v[0].z=-0.5f;v[1].x=0.5f;v[1].z=-2;v[2].y=0.5f;v[2].z=-2;std::vector<gf::Vertex> out;
     gf::project_triangle(c,v,out);check(out.size()>=3,"near crossing triangle is clipped not dropped");
     for(auto& p:out)check(p.z<=1.00001f&&p.inv_w>0&&p.inv_w<=1.00001f&&std::isfinite(p.x),"clipped reciprocal W valid");
     out.clear();for(auto& p:v)p.z=2;gf::project_triangle(c,v,out);check(out.empty(),"behind-camera triangle omitted");++cases;}
    {gf::u8 data[8]={0,0,0,4,0,0,0,0};gf::hsd_detail::Reader r;r.block={data,8};r.targets={4};gf::u32 value=0;
     check(!r.ptr(0,&value),"unrelocated field rejected even if target exists");r.fields={0};check(r.ptr(0,&value)&&value==4,"relocated field accepted");++cases;}
    {gf::MenuScene s;Backend b;auto f=fixture(0x10);auto child=f.joints[0];child.parent=0;child.flags=0;child.translation[0]=0;child.translation[1]=0;child.translation[2]=0;f.joints.push_back(child);f.meshes.push_back(f.meshes[0]);f.mesh_joint.push_back(1);
     check(s.load_scene(f,&error),"hidden parent child load");check(draw(s,b)==1,"node hidden does not hide children");++cases;}
    {gf::MenuScene s;Backend b;check(s.load_scene(fixture(),&error),"cache fixture");draw(s,b);const float before=b.vertices[0].x;draw(s,b);check(b.vertices[0].x==before,"cached draw is identical");
     auto c=camera();c.eye[0]=20;c.target[0]=20;check(draw(s,b,c)==1,"translated camera still sees mesh");check(std::fabs(b.vertices[0].x-before)>0.01f,"camera invalidates projected cache");
     gf::HsdAnimPose pose;pose.joints.resize(1);pose.joints[0].translation[0]=30;pose.joints[0].translation[2]=-100;
     check(s.set_pose(&pose),"cache pose");draw(s,b);check(std::fabs(b.vertices[0].x-before)>0.01f,"pose invalidates projected cache");++cases;}
    {auto f=fixture();f.lighting.declared=true;gf::SceneLight light;light.flags=13;light.position[2]=1;
     for(auto& c:light.colour)c=128;
     f.lighting.lights.push_back(light);auto& m=f.meshes[0];m.render_mode=12;
     for(auto& c:m.material_specular)c=128;
     for(auto& v:m.vertices)v.normal[2]=1;
     gf::MenuScene s;Backend b;check(s.load_scene(f,&error),"real-light seam fixture");check(draw(s,b)==1,"lit draw");
     check(b.vertices[0].light[0]==128&&b.vertices[0].specular[0]>0,"separate lighting channels reach render seam");
     check(!b.calls[0].texture_after_lighting,"diffuse texture lighting order");
     f.meshes[0].texture_after_lighting=true;check(s.load_scene(f,&error),"EXT fixture");draw(s,b);check(b.calls[0].texture_after_lighting,"EXT order captured per draw");
     f.lighting.lights[0].flags=14;check(!s.load_scene(f,&error),"unsupported light kind refuses scene rendering");++cases;}
    {auto f=fixture();f.meshes[0].colour_write=false;f.meshes[0].alpha_write=false;gf::MenuScene s;Backend b;
     check(s.load_scene(f,&error)&&draw(s,b)==1,"depth-only mesh still submits geometry");
     check(!b.calls[0].colour_write&&!b.calls[0].alpha_write&&b.calls[0].depth_write,"mesh PE masks reach captured draw state");++cases;}
    std::printf("{\"menu_render_repair_cases\":%u,\"all_passed\":true}\n",cases);
}
