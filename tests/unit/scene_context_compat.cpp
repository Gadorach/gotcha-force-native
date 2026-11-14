// NEW default-interface witness, before/current builds. No old main replay.
#include <cstdio>
#include "engine/gameplay/menu_scene.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "tests/unit/startup_actor_frame_fixture.hpp"
int main(){
    using namespace gf;std::string error;RecordingRenderBackend backend;RenderEngine render(backend);if(!render.initialize({320,240,false,true,"new default-context witness"},nullptr,&error))return 2;
    HsdScene data;data.joints.resize(1);HsdMesh mesh;mesh.render_mode=5;mesh.cull_backface=false;mesh.material_ambient[0]=100;mesh.material_ambient[1]=150;mesh.material_ambient[2]=200;mesh.indices={0,1,2};const float points[3][3]={{-1,-1,0},{0,1,0},{1,-1,0}};for(const auto& p:points){HsdVertex v;std::copy_n(p,3,v.position);v.normal[2]=1;mesh.vertices.push_back(v);}data.meshes.push_back(mesh);data.mesh_joint.push_back(0);data.lighting.declared=true;data.lighting.lights.resize(1);data.lighting.lights[0].flags=4;for(auto& c:data.lighting.lights[0].colour)c=128;
    MenuScene scene;scene.set_interaction(false);if(!scene.load_scene(data,&error))return 3;
    startup_actor_frame_fixture::Fixture f;if(!f.load(&error))return 3;StartupActorFrame actor;StartupActorFrameOutput out;if(!actor.load(f.assets(),StartupActorFrameMode::IsolatedRequirements,0,&out,&error)||!actor.reset(&out,&error)||!actor.request(0,&out,&error))return 3;actor.upload_textures(render,10);
    Camera camera;camera.eye[2]=100;camera.fov_y_degrees=100;camera.far_plane=1000;camera.viewport_width=320;camera.viewport_height=240;StartupActorFrameInput input;input.isolated_external_mutations=input.placement_declared=true;
    for(unsigned frame=0;frame<12;++frame){if(!actor.step(input,&out,&error))return 3;render.begin_frame({frame,0,0,0});render.reset_arena();scene.draw(render,camera);actor.draw(render,camera);render.end_frame();render.present();std::printf("%u %llu %u\n",frame,static_cast<unsigned long long>(backend.last_frame_digest()),backend.draws());}
}
