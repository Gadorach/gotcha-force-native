#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/scene_light_colours.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"scene colours: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;SceneLighting rig;SceneLightColours colours;std::string error;
    check(!initialize_scene_light_colours(rig,&colours,&error)&&colours==SceneLightColours{},"undeclared light owner refuses");
    rig.declared=true;check(initialize_scene_light_colours(rig,&colours,&error)&&colours==SceneLightColours{},"empty reached chain clears full state");
    SceneLight a;a.colour[0]=0x12;a.colour[1]=0x34;a.colour[2]=0x56;a.colour[3]=0x78;
    SceneLight b;b.colour[0]=0x90;b.colour[1]=0xab;b.colour[2]=0xcd;b.colour[3]=0xef;
    rig.lights={a,b};rig.animated=true;check(initialize_scene_light_colours(rig,&colours,&error)&&colours.count==2&&colours.baseline[0]==0x12345678&&colours.baseline[1]==0x90abcdef,"authored order and RGBA including alpha retained; animation unrequested");
    for(const auto& channel:colours.channels){check(channel.controls==std::array<u8,72>{},"all channel control bytes zero");check(channel.current==colours.baseline,"all four channels seeded including unused zero slots");}
    rig.lights.resize(8,a);check(initialize_scene_light_colours(rig,&colours,&error)&&colours.count==8,"eight source slots supported");
    rig.lights.push_back(a);check(!initialize_scene_light_colours(rig,&colours,&error)&&colours==SceneLightColours{},"ninth colour cannot overlap count/control storage");
    rig.lights={a};rig.lights[0].flags=2;check(!initialize_scene_light_colours(rig,&colours,&error),"unsupported light type remains refused");
    std::printf("scene light colours: %u checks PASS\n",checks);
}
