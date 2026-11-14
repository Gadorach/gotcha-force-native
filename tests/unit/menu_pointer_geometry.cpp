#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/menu_scene.hpp"
#include "engine/gameplay/frame_digest.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"menu pointer geometry: %s\n",why);std::abort();}}
struct Backend final:IRenderBackend {
    std::vector<Vertex> vertices;std::vector<DrawCall> calls;
    bool initialize(const RenderConfig&,IWindowBackend*,std::string*)override{return true;}void shutdown()override{}
    bool upload_vertices(const Vertex* p,std::size_t n)override{vertices.assign(p,p+n);return true;}
    bool upload_texture(u32,const TextureDesc&)override{return true;}bool upload_matrix(u32,const f32*)override{return true;}
    void begin_frame(const FrameDesc&)override{calls.clear();}void submit(const DrawCall& call)override{calls.push_back(call);}void end_frame()override{}void present()override{}
};
HsdScene fixture(){HsdScene s;s.menu_motion.present=true;s.menu_motion.angle_unit=6.28318530718f/65536;s.menu_motion.selector_joint=900;
    auto add=[&](u32 id,f32 x,f32 y,u8 colour){HsdJoint joint;joint.archive_offset=id;joint.translation[0]=x;joint.translation[1]=y;joint.translation[2]=-100;s.joints.push_back(joint);HsdMesh mesh;mesh.cull_backface=false;mesh.material_colour[0]=colour;for(const auto& p:std::array<std::array<f32,2>,3>{{{-2,-2},{2,-2},{0,2}}}){HsdVertex v;v.position[0]=p[0];v.position[1]=p[1];v.normal[2]=1;mesh.vertices.push_back(v);}mesh.indices={0,1,2};s.meshes.push_back(mesh);s.mesh_joint.push_back(u32(s.joints.size()-1));};
    for(unsigned i=0;i<7;++i){const u32 id=100+i*3;s.menu_motion.bindings.push_back({i,id,id+1,id+2,512});add(id,f32(int(i)*8-24),0,u8(30+i));add(id+1,f32(int(i)*8-24),12,u8(60+i));add(id+2,f32(int(i)*8-24),20,u8(90+i));for(auto& v:s.meshes.back().vertices){v.position[2]=v.position[1];v.position[1]=0;v.normal[1]=1;v.normal[2]=0;}}
    add(900,0,0,120);return s;
}
std::array<f32,2> centre(const Backend& b,u8 red){for(const auto& call:b.calls){const auto* p=b.vertices.data()+call.vertex_first;if(p[0].r==red)return {(p[0].x+p[1].x+p[2].x)/3,(p[0].y+p[1].y+p[2].y)/3};}std::abort();}
}
int main(){using namespace gf;unsigned item=99;MenuPointerGeometry geometry;std::array<Vertex,3> triangle;triangle[0].x=10;triangle[0].y=10;triangle[1].x=110;triangle[1].y=10;triangle[2].x=10;triangle[2].y=110;for(auto& v:triangle)v.z=.2f;
    check(!geometry.hit(20,20,&item)&&item==7,"no unsubmitted or missing frame hit");geometry.begin(640,480);check(geometry.append(triangle,1,true,false,false),"rendered CW-front convention");geometry.finish(true);check(geometry.hit(20,20,&item)&&item==1,"triangle interior");check(!geometry.hit(100,100,&item)&&item==7,"triangle not bounding rectangle");check(!geometry.hit(-1,20,&item)&&!geometry.hit(std::numeric_limits<f32>::quiet_NaN(),20,&item),"out-of-window/nonfinite mouse refuses");
    geometry.begin(640,480);check(geometry.append(triangle,0,true,false,false),"first depth candidate");for(auto& v:triangle)v.z=.8f;check(geometry.append(triangle,2,true,false,false),"nearer reverse-depth candidate");geometry.finish(true);check(geometry.hit(20,20,&item)&&item==2,"nearest source GEQUAL depth wins");
    geometry.begin(640,480);check(geometry.append(triangle,0,true,false,false),"background candidate");for(auto& v:triangle)v.z=.1f;check(geometry.append(triangle,3,false,false,true),"foreground caption");geometry.finish(true);check(geometry.hit(20,20,&item)&&item==3,"actual foreground caption beats depth geometry");
    geometry.begin(640,480);std::swap(triangle[1],triangle[2]);check(geometry.append(triangle,4,true,false,false),"back-face submission valid");geometry.finish(true);check(!geometry.hit(20,20,&item),"culled back face cannot select");
    geometry.begin(640,480);check(!geometry.append(triangle,7,false,false,false),"out-of-range item refused");geometry.finish(true);check(!geometry.hit(20,20,&item),"failed frame never replays old hits");
    MenuScene scene;std::string error;Backend backend;RenderEngine render(backend);Camera camera;camera.near_plane=1;camera.far_plane=1000;check(scene.load_scene(fixture(),&error),"owned seven-item motion scene");
    auto draw=[&](){render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)>0&&render.end_frame(),"actual shared menu submission");FrameDigest digest;for(const auto& call:backend.calls)digest.add_draw(call);digest.add_vertices(backend.vertices.data(),backend.vertices.size());return digest.value();};
    const auto old=draw();check(!scene.menu_pointer_hit(320,240,&item),"default old scene does not enable mouse capture");check(scene.enable_menu_pointer(true),"explicit capture opt-in");check(draw()==old,"capture does not change submitted state/geometry");
    for(unsigned i=0;i<7;++i){const auto point=centre(backend,u8(30+i));check(scene.menu_pointer_hit(point[0],point[1],&item)&&item==i,"hit derives from each gear's actually submitted triangle");}
    scene.set_menu_focus(0,false);check(!scene.menu_pointer_hit(320,240,&item)&&item==7,"camera/transition focus invalidates old MAIN footprint");draw();check(!scene.menu_pointer_hit(320,240,&item),"focused transition is not selectable MAIN");scene.set_menu_focus(7,true);draw();scene.enable_menu_pointer(false);check(!scene.menu_pointer_hit(320,240,&item),"inactive menu cannot use retained triangles");check(scene.load_scene(fixture(),&error)&&!scene.menu_pointer_hit(320,240,&item),"reload clears capability and old frame");
    std::printf("geometry-bound menu pointer: %u checks PASS; actual submitted triangles, no fitted targets\n",checks);return 0;
}
