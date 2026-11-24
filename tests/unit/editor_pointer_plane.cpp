#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/editor_pointer_plane.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"editor pointer plane: %s\n",why);std::abort();}}}
int main(){using namespace gf;Camera camera;camera.eye[0]=0;camera.eye[1]=10;camera.eye[2]=10;camera.target[0]=camera.target[1]=camera.target[2]=0;camera.up[0]=camera.up[2]=0;camera.up[1]=1;camera.fov_y_degrees=45;camera.near_plane=1;camera.far_plane=100;camera.viewport_width=640;camera.viewport_height=480;
    for(f32 x:{0.f,80.f,320.f,560.f,639.f})for(f32 y:{0.f,80.f,240.f,400.f,479.f}){std::array<f32,3> world;f32 sx=0,sy=0;check(editor_pointer_plane(camera,x,y,&world)&&world[1]==0,"existing editor ray intersects Y=0 without cursor/hand offsets");check(camera.project(world.data(),&sx,&sy,nullptr)&&std::abs(sx-x)<.0002f&&std::abs(sy-y)<.0002f,"world root projects directly to raw pointer");}
    camera.viewport_x=32;camera.viewport_y=224;camera.viewport_height=224;camera.projection_aspect=bits_to_f32(0x402aaaab);
    for(f32 x:{112.f,352.f,592.f})for(f32 y:{240.f,336.f,432.f}){std::array<f32,3> world;f32 sx=0,sy=0;check(editor_pointer_plane(camera,x,y,&world)&&camera.project(world.data(),&sx,&sy,nullptr)&&std::abs(sx-x)<.0002f&&std::abs(sy-y)<.0002f,"explicit source aspect and offset retain shared pointer/project round trip");}
    camera.viewport_x=camera.viewport_y=0;camera.viewport_height=480;camera.projection_aspect=0;
    std::array<f32,3> world={1,2,3};check(!editor_pointer_plane(camera,std::numeric_limits<f32>::quiet_NaN(),0,&world)&&world==std::array<f32,3>{},"invalid pointer clears output");check(!editor_pointer_plane(camera,0,0,nullptr),"null output refuses");camera.target[1]=camera.eye[1];world={1,2,3};check(!editor_pointer_plane(camera,320,240,&world)&&world==std::array<f32,3>{},"parallel ray refuses rather than inventing position");camera.target[1]=0;camera.far_plane=2;check(!editor_pointer_plane(camera,320,240,&world),"unchanged far-bound guard");std::printf("editor shared ray/hand anchor: %u checks PASS\n",checks);return 0;}
