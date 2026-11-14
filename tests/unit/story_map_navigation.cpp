#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/gameplay/story_map_navigation.hpp"
namespace {
using namespace gf;unsigned checks=0;
void check(bool yes){++checks;if(!yes)std::abort();}
std::vector<u8> fixture(){
    std::vector<u8> b(152,255);store_be32(b.data(),0x4746534e);store_be16(b.data()+4,1);store_be16(b.data()+6,152);store_be32(b.data()+8,123);
    store_be16(b.data()+12,15);b[14]=4;b[15]=0;
    for(unsigned i=0;i<4;++i)store_be32(b.data()+16+i*4,std::array<u32,4>{0x40004,0x80008,0x20002,0x10001}[i]);
    //Owned node0: primary1,2,3,4; alternative5,6,7,8.
    for(unsigned i=0;i<8;++i)b[32+i]=u8(i+1);
    return b;
}
}
int main(){using namespace gf;auto bytes=fixture();StoryMapNavigationProfile profile;std::string error;
    check(story_map_navigation_load({bytes.data(),bytes.size()},123,&profile,&error));StoryMapNavigationPlan p;
    check(story_map_navigation_plan(profile,{0,9},0x7fff,0xf000f,&p)&&p.state.current==1&&p.state.previous==0&&p.sound==119);
    check(story_map_navigation_plan(profile,{0,6},0,0x80008,&p)&&p.state.current==6);
    check(story_map_navigation_plan(profile,{0,9},1u<<5,0x40004,&p)&&p.state.current==5);
    check(story_map_navigation_plan(profile,{0,9},1u<<2,0xc000c,&p)&&p.state.current==2);
    check(story_map_navigation_plan(profile,{0,9},0,0xf000f,&p)&&!p.changed&&p.state.current==0&&!p.sound);
    check(story_map_navigation_plan(profile,{1,9},0x7fff,0xf000f,&p)&&!p.changed);
    for(unsigned mask=0;mask<32768;++mask){
        check(story_map_navigation_plan(profile,{0,9},mask,0x40004,&p));
        const unsigned chosen=(mask&2)?1:(mask&32)?5:0;
        check(p.state.current==chosen&&p.changed==(chosen!=0));
    }
    auto bad=profile;bad.neighbors[0][0]=-2;check(!story_map_navigation_plan(bad,{0,9},0,0,&p));
    bad=profile;bad.directions[0]=1;check(!story_map_navigation_plan(bad,{0,9},0,0,&p));
    check(!story_map_navigation_plan(profile,{15,0},0,0,&p));
    check(!story_map_navigation_plan(profile,{0,15},0,0,&p));
    check(!story_map_navigation_plan(profile,{0,9},0x8000,0,&p));
    for(unsigned at:{0u,4u,6u,8u,12u,14u,15u,16u,32u}){
        auto b=bytes;b[at]^=128;StoryMapNavigationProfile rejected;
        check(!story_map_navigation_load({b.data(),b.size()},123,&rejected,&error)&&!rejected.loaded);
    }
    check(!story_map_navigation_load({bytes.data(),bytes.size()-1},123,&profile));
    std::printf("Story map navigation: %u checks PASS; metadata/declared availability only, not 3D map or mission admission\n",checks);
}
