// Analytical indexed geometry, pose/material/view invalidation and clipping.
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/menu_scene.hpp"
namespace {
using namespace gf;
void check(bool ok){if(!ok)std::abort();}
struct Sink final:IRenderBackend {
 std::vector<Vertex> vertices;std::vector<DrawCall> calls;
 bool initialize(const RenderConfig&,IWindowBackend*,std::string*)override{return true;}void shutdown()override{}RenderCapabilities capabilities()const override{return {};}
 bool upload_vertices(const Vertex* v,std::size_t n)override{vertices.assign(v,v+n);return true;}bool upload_texture(u32,const TextureDesc&)override{return true;}bool upload_matrix(u32,const f32*)override{return true;}
 void begin_frame(const FrameDesc&)override{calls.clear();vertices.clear();}void submit(const DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
bool near(float a,float b){return std::abs(a-b)<.0001f;}
}
int main(){
 using namespace gf;HsdScene asset;asset.joints.resize(1);asset.mesh_joint={0};HsdMesh mesh;mesh.vertices.resize(3);mesh.indices={0,1,2,0,1,2};mesh.cull_backface=false;
 mesh.vertices[0].position[0]=-1;mesh.vertices[0].position[1]=-1;mesh.vertices[1].position[0]=1;mesh.vertices[1].position[1]=-1;mesh.vertices[2].position[1]=1;for(auto& v:mesh.vertices){v.position[2]=-4;v.normal[2]=1;}asset.meshes.push_back(mesh);
 MenuScene scene;scene.set_interaction(false);std::string error;check(scene.load_scene(asset,&error));Sink sink;RenderEngine render(sink);Camera camera;camera.fov_y_degrees=90;camera.viewport_width=camera.viewport_height=128;
 auto draw=[&](){render.begin_frame({});render.reset_arena();auto n=scene.draw(render,camera);check(render.end_frame());return n;};
 check(draw()==1&&sink.vertices.size()==6);for(unsigned i=0;i<3;++i){check(near(sink.vertices[i].x,sink.vertices[i+3].x)&&near(sink.vertices[i].y,sink.vertices[i+3].y));check(near(sink.vertices[i].inv_w,.25f));}check(near(sink.vertices[0].x,48)&&near(sink.vertices[0].y,80));check(near(sink.vertices[2].x,64)&&near(sink.vertices[2].y,48));
 check(draw()==1&&near(sink.vertices[0].x,48));
 HsdAnimPose pose;pose.joints.resize(1);pose.joints[0].translation[0]=1;pose.materials.resize(1);pose.materials[0].present=true;pose.materials[0].alpha=.5f;pose.materials[0].diffuse[0]=.2f;pose.materials[0].diffuse[1]=.4f;pose.materials[0].diffuse[2]=.6f;check(scene.set_pose(&pose));check(draw()==1);check(near(sink.vertices[0].x,64)&&sink.vertices[0].r==51&&sink.vertices[0].g==102&&sink.vertices[0].b==153&&sink.vertices[0].a==128);
 camera.eye[0]=camera.target[0]=1;check(draw()==1&&near(sink.vertices[0].x,48));
 pose.joints[0].hidden=true;check(scene.set_pose(&pose));check(draw()==0&&sink.calls.empty());pose.joints[0].hidden=false;pose.materials[0].alpha=1;check(scene.set_pose(&pose));check(draw()==1&&sink.vertices[0].a==255);
 camera.target[2]=0;check(draw()==0&&sink.calls.empty());camera.target[2]=-1;check(draw()==1);
 // Moving the near plane past the mesh must invalidate cached projection.
  camera.near_plane=4.5f;check(draw()==0);camera.near_plane=.5f;check(scene.set_pose(nullptr));check(draw()==1);
  // A source projection may change aspect without changing viewport dimensions.
  camera.eye[0]=camera.target[0]=0;check(draw()==1&&near(sink.vertices[0].x,48));
  camera.projection_aspect=2;check(draw()==1&&near(sink.vertices[0].x,56)&&near(sink.vertices[0].y,80));
  camera.viewport_x=64;camera.viewport_y=224;check(draw()==1&&near(sink.vertices[0].x,120)&&near(sink.vertices[0].y,304));
  camera.projection_aspect=0;camera.viewport_x=camera.viewport_y=0;check(draw()==1&&near(sink.vertices[0].x,48)&&near(sink.vertices[0].y,80));
 // Off-screen geometry must not bypass the raw reflection-normal refusal.
 HsdScene invalid=asset;auto bad=mesh;bad.generated_uv_required=bad.reflection_uv_supported=true;bad.texture_coordinate_mode=1;bad.texture=0;for(auto& v:bad.vertices){v.position[0]+=1000;v.normal[2]=0;}invalid.meshes.insert(invalid.meshes.begin(),bad);invalid.mesh_joint.push_back(0);MenuScene guarded;guarded.set_interaction(false);check(guarded.load_scene(invalid,&error));render.begin_frame({});render.reset_arena();check(guarded.draw(render,camera)==0&&!guarded.error().empty());check(render.end_frame());
 std::puts("indexed geometry/cache: analytical positions, repeated indices, material/pose/view/visibility invalidation + offscreen generator refusal PASS");
}
