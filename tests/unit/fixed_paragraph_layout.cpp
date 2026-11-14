#include <cstdio>
#include <cstdlib>
#include "engine/graphics/source_fixed_paragraph.hpp"
#include "tests/unit/story_start_visual_fixture.hpp"
int main(){using namespace gf;unsigned checks=0;auto check=[&](bool x){++checks;if(!x)std::abort();};
    std::array<Vertex,6> triangles{};const float xy[6][2]={{0,0},{80,0},{80,30},{0,0},{80,30},{0,30}};
    for(unsigned i=0;i<6;++i){triangles[i].x=xy[i][0];triangles[i].y=xy[i][1];triangles[i].u=xy[i][0]/80;triangles[i].v=xy[i][1]/30;triangles[i].inv_w=1;}
    std::vector<u8> rgba(80*30*4,255);for(unsigned y=0;y<30;++y)for(unsigned x=0;x<80;++x){const double cx=x<15?15:x>=65?65:x+.5,dx=x+.5-cx,dy=y+.5-15;if(dx*dx+dy*dy>225)rgba[(y*80+x)*4+3]=0;}
    AlphaTextRegion region;check(region.load(triangles,{80,30,rgba.data(),0,0,false},{rgba.data(),rgba.size()}));
    const auto font=story_start_visual_fixture::font();SourceFixedTextMetrics metrics;check(metrics.load(font));
    const std::string a("\x82\x60",2),space("\x81\x40",2);auto words=[&](unsigned n){std::string result;for(unsigned i=0;i<n;++i){if(i)result+=space;result+=a;}return result;};
    std::array<std::string,2> source{words(5),words(1)};FixedParagraphLayout result;
    check(layout_fixed_paragraph(source,font.blank(),metrics,region,8,10,8,1,1,1,&result));
    check(result.reflowed&&result.text.size()==2&&result.text[0]==words(3)&&result.text[1]==words(3));
    check(result.lines.size()==2);for(const auto& line:result.lines)check(region.contains(line.ink,1));
    source={words(1),""};check(layout_fixed_paragraph(source,font.blank(),metrics,region,8,10,8,1,1,1,&result)&&!result.reflowed&&result.lines.size()==1);
    source={"",""};check(layout_fixed_paragraph(source,font.blank(),metrics,region,8,10,8,1,1,1,&result)&&result.lines.empty());
    source={std::string{},std::string{}};for(unsigned i=0;i<16;++i)source[0]+=a;
    check(!layout_fixed_paragraph(source,font.blank(),metrics,region,8,10,8,1,1,1,&result)&&result.lines.empty()&&result.text.empty());
    check(!layout_fixed_paragraph(source,font.blank(),metrics,region,8,10,8,1,1,1,nullptr));
    std::printf("Fixed-font rounded paragraph layout: %u checks PASS; balanced word-preserving reflow, no shrink/clip\n",checks);
}
