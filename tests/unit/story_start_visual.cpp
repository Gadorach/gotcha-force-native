#include <cstdio>
#include <cstdlib>
#include "tests/unit/story_start_visual_fixture.hpp"
int main(){using namespace gf;unsigned checks=0;auto check=[&](bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"Story start visual: %s\n",why);std::abort();}};
    const auto font=story_start_visual_fixture::font();auto art=story_start_visual_fixture::art();StoryStartVisual visual;std::string error;
    check(art.valid(font)&&visual.load(art,font,&error),"owned art/font load");StoryStartState state;check(story_start_open_declared({2,0,true,false},&state),"owned controller");u64 serial=0;
    auto tick=[&](StoryStartInput input=StoryStartInput{},StoryStartDependencies deps=StoryStartDependencies{},bool major=true){StoryStartPlan plan;check(story_start_step(state,input,deps,&plan,&error),"controller update");state=plan.state;check(visual.step(plan,major,++serial,&error),"one post-controller source callback");return plan;};
    tick();check(visual.widgets().empty()&&!visual.footer_visible(),"no widgets before resource completion");tick({}, {true,false});tick();
    check(visual.widgets().size()==7&&visual.footer_visible(),"source extended menu construction");
    check(visual.widgets()[0].frame==0&&visual.widgets()[0].counter==1,"background FIRST_PLAY0 sampled before increment");
    check(!visual.widgets()[1].visible&&visual.widgets()[1].counter==4&&visual.widgets()[2].angle_z==0xff00,"delay decrements and spinner turns immediately");
    for(unsigned i=0;i<4;++i)tick();
    check(visual.widgets()[1].visible&&visual.widgets()[1].source.position[1]==8,"source cursor context offset");
    check(visual.widgets()[3].visible&&visual.widgets()[3].frame==0&&visual.widgets()[4].frame==2,"selected/unselected original label selectors");
    while(state.phase!=2)tick();
    tick({0,0x40004});
    check(visual.selection()==1&&visual.widgets()[3].frame==2&&visual.widgets()[4].frame==0,"same-callback cursor and label update");
    tick({0x100,0});check(visual.widgets()[4].phase==3,"confirm enters retained blink state");
    while(state.page!=StoryStartPage::PlayNumber)tick();
    tick();
    check(visual.widgets().size()==12&&visual.footer_visible(),"play panel appends; underlying menu/footer retained");
    for(unsigned i=0;i<4;++i)tick();
    const auto poses=visual.poses();check(!poses.empty()&&poses==visual.poses()&&serial==visual.serial(),"repeated read/draw preparation never advances visual clocks");
    bool has_up=false,has_down=false;for(const auto& p:poses){has_up|=p.id==10;has_down|=p.id==11;}
    check(has_up&&!has_down,"highest initial play shows only available arrow");
    tick({0x200,0});check(state.page==StoryStartPage::Menu&&visual.widgets().size()==12,"cancel marks overlay closing without early release");
    tick();check(visual.widgets().size()==12,"next callback hides registered overlay");
    tick();check(visual.widgets().size()==7,"following callback releases overlay");
    tick({}, {},false);check(visual.footer_visible(),"source major change still allows final active callback");
    tick({}, {},false);check(!visual.footer_visible()&&visual.poses().empty(),"next hidden callback suppresses whole source group");
    tick({}, {},false);check(visual.widgets().empty(),"then unregister source instances");
    StoryStartPlan plan;plan.state=state;check(!visual.step(plan,true,serial,&error)&&!visual.ready()&&visual.poses().empty(),"duplicate callback refuses and invalidates publication");
    art.aspect=NAN;check(!art.valid(font),"nonfinite camera aspect refuses");
    std::printf("Story start retained visual clocks: %u owned checks PASS; no GPU/storage/audio ACK\n",checks);
}
