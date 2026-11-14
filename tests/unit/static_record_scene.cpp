#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/static_record_scene.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"static record: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;std::string error;hsd_record_animation_fixture::Fixture fixture;const auto bytes=fixture.archive();HsdRecordAnimationBank bank;
    check(bank.load({bytes.data(),bytes.size()},&error),"owned bank");
    RecordingRenderBackend backend;RenderEngine render(backend);std::map<u16,AnimatedModelScene> cache;
    Camera camera;camera.eye[2]=10;camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=100;Mat4 world;
    auto draw=[&](bool always,bool compact){render.begin_frame({});render.reset_arena();check(draw_static_record(render,bank,cache,0,world,camera,4096,&error,always,compact),"shared draw");check(render.end_frame(),"submission");render.present();return backend.last_frame_digest();};
    const auto ordinary=draw(false,true);check(cache.size()==1&&cache.at(0).ready(),"one requested instance");
    check(cache.at(0).animation().frame()==0&&cache.at(0).animation().trace().empty(),"static request only, no draw clock");
    check(draw(false,true)==ordinary,"warm draw identical");
    const auto forced=draw(true,true);check(forced!=ordinary,"explicit depth comparison observed");
    check(draw(false,true)==ordinary,"ordinary call clears depth override");
    world.m[3]=1;check(draw(false,true)!=ordinary,"caller world applied");world=Mat4{};
    cache.clear();check(draw(false,false)==ordinary,"noncompact compatibility path");
    check(!draw_static_record(render,bank,cache,1,world,camera,4096,&error),"absent record refuses");check(cache.size()==1,"failed load not cached");
    world.m[0]=NAN;check(!draw_static_record(render,bank,cache,0,world,camera,4096,&error),"nonfinite root refuses");check(!cache.at(0).ready(),"failed pose cannot draw old pose");
    world=Mat4{};check(!draw_static_record(render,bank,cache,0,world,camera,4096,&error),"failed cache requires explicit clear");
    cache.clear();check(draw(false,true)==ordinary,"fresh cache restores instance");
    std::printf("static record: %u checks PASS\n",checks);
}
