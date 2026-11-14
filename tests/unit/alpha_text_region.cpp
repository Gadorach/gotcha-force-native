#include <cstdio>
#include <cstdlib>
#include "engine/graphics/alpha_text_region.hpp"
int main(){using namespace gf;unsigned checks=0;auto check=[&](bool x){++checks;if(!x)std::abort();};
    std::array<Vertex,6> vertices{};const float xy[6][2]={{0,0},{20,0},{20,10},{0,0},{20,10},{0,10}};
    for(unsigned i=0;i<6;++i){vertices[i].x=xy[i][0];vertices[i].y=xy[i][1];vertices[i].u=xy[i][0]/20;vertices[i].v=xy[i][1]/10;vertices[i].inv_w=1;}
    std::vector<u8> rgba(20*10*4,255);for(unsigned y=0;y<10;++y)for(unsigned x=0;x<20;++x){const double cx=x<5?5:x>=15?15:x+.5,dx=x+.5-cx,dy=y+.5-5;if(dx*dx+dy*dy>25)rgba[(y*20+x)*4+3]=0;}
    TextureDesc texture{20,10,rgba.data(),0,0,false};AlphaTextRegion region;
    check(region.load(vertices,texture,{rgba.data(),rgba.size()}));check(region.hull().hull().size()>4);
    check(!region.contains({1,0,3,2},0));check(!region.contains({0,0,20,10},0));check(region.contains({7,2,13,8},1));
    std::array<SourceTextMeasure,1> lines{{{0,0,8,2,8,true}}};std::vector<CenteredTextLine> placed;
    check(region.center(lines,2,1,1,1,&placed)&&placed.size()==1);check(region.contains(placed[0].ink,1));
    const auto original=rgba;rgba[(5*20+10)*4+3]=0;
    check(!region.load(vertices,texture,{rgba.data(),rgba.size()})); //hole/non-row-convex region is not filled by its hull
    rgba=original;texture.pixels=rgba.data();check(!region.load(vertices,texture,{rgba.data(),rgba.size()-1}));
    texture.linear=true;check(region.load(vertices,texture,{rgba.data(),rgba.size()}));check(!region.contains({1,0,3,2},0));
    texture.wrap_s=2;check(!region.load(vertices,texture,{rgba.data(),rgba.size()}));
    std::printf("Texture-alpha rounded layout region: %u owned checks PASS; no rectangle or hole-filling fallback\n",checks);
}
