#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/graphics/convex_text_layout.hpp"
#include "engine/graphics/source_fixed_text_metrics.hpp"
#include "tests/unit/story_start_visual_fixture.hpp"
int main(){using namespace gf;unsigned checks=0;auto check=[&](bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"text-region layout: %s\n",why);std::abort();}};
    const std::array<TextRegionPoint,8> points{{{0,10},{10,0},{90,0},{100,10},{100,30},{90,40},{10,40},{0,30}}};ConvexTextRegion region;
    check(region.load(points),"owned rounded/chamfered polygon");
    for(int x=-1;x<=101;++x)for(int y=-1;y<=41;++y)for(unsigned margin=0;margin<3;++margin){
        const double diagonal=margin*std::sqrt(2.);const bool expected=x>=int(margin)&&x<=100-int(margin)&&y>=int(margin)&&y<=40-int(margin)&&x+y>=10+diagonal&&x-y>=-30+diagonal&&x-y<=90-diagonal&&x+y<=130-diagonal;
        check(region.contains({float(x),float(y),float(x),float(y)},float(margin))==expected,"all polygon half-planes/inset, not rectangular clipping");
    }
    std::array<SourceTextMeasure,2> lines{{{0,0,40,10,40,true},{0,0,20,10,20,true}}};std::vector<CenteredTextLine> placed;
    check(region.center(lines,12,1,1,2,&placed)&&placed.size()==2,"two-line group fits");
    check(placed[0].x==30&&placed[1].x==40&&placed[0].y==9&&placed[1].y==21,"horizontal line + vertical group centering");
    check((placed[0].ink.top+placed[1].ink.bottom)*.5f==20,"whole group, not independent vertical centers");
    lines[1]={};check(region.center(lines,12,1,1,2,&placed)&&placed.size()==1&&placed[0].y==15,"empty second line consumes no vertical space");
    lines[0]={5,2,45,12,40,true};check(region.center(lines,12,1.5f,2,2,&placed)&&placed[0].x==12.5f&&placed[0].y==6,"ink bearings and unequal pixel scale");
    lines={{{0,0,96,10,96,true},{0,0,96,10,96,true}}};check(!region.center(lines,12,1,1,2,&placed)&&placed.empty(),"rounded corners reject rectangle-only fit");
    check(!region.contains({1,1,9,9},0),"rounded corner excludes bounding-box-only region");
    check(!region.center(lines,12,1,1,NAN,&placed)&&placed.empty(),"nonfinite inset refuses");
    check(!region.load(std::array<TextRegionPoint,3>{{{0,0},{1,1},{2,2}}}),"degenerate silhouette refuses");
    const auto font=story_start_visual_fixture::font();SourceFixedTextMetrics metrics;check(metrics.load(font)&&metrics.for_font(font),"original fixed-advance font binding");SourceTextMeasure measure;
    check(metrics.measure(std::string("\x82\x60\x81\x40",4),8,10,&measure)&&measure.ink&&measure.left==0&&measure.right==8&&measure.advance==20,"actual alpha ink, not advance width or spaces");
    check(metrics.measure(std::string("\x81\x40",2),8,10,&measure)&&!measure.ink&&measure.advance==10,"space-only measurement");
    check(!metrics.measure(std::string("\x82\x62",2),8,10,&measure)&&!measure.ink,"unknown glyph no partial measure");
    check(!metrics.measure(std::string("\x82",1),8,10,&measure),"truncated character refuses");
    check(!metrics.measure({},8,0,&measure),"zero advance refuses");
    check(!metrics.measure({},std::numeric_limits<f32>::infinity(),10,&measure),"nonfinite extent refuses");
    std::printf("Convex/rounded textbox + fixed-font ink layout: %u checks PASS; native owner-requested centering, not original placement claim\n",checks);
}
