#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/option_transition.hpp"
#include "engine/gameplay/menu_world.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* text){++checks;if(!ok){std::fprintf(stderr,"option transition: %s\n",text);std::abort();}}
}
int main(){
    gf::OptionTransition flow;check(!flow.begin(0),"unconfigured flow refuses");
    check(!flow.configure({0,5,5,1})&&!flow.configure({0,5,9,0}),"invalid timings refuse");
    check(flow.configure({2,10,14,1}),"owned timing config");
    check(flow.frame()==2&&!flow.active(),"main timeline initial");
    check(flow.begin(0)&&flow.frame()==10&&flow.phase()==gf::OptionTransitionPhase::Entering,"entry initializes exact source counter");
    check(!flow.begin(0),"active flow cannot be restarted");
    flow.update(1);check(flow.frame()==11,"entry advances and ignores B");
    flow.update(1);flow.update(1);flow.update(1);check(flow.frame()==14&&flow.phase()==gf::OptionTransitionPhase::UnportedUI,"entry clamps to endpoint without fake settings UI");
    flow.update(1);check(flow.phase()==gf::OptionTransitionPhase::UnportedUI,"B held through entry cannot trigger return");
    flow.update(3);check(flow.phase()==gf::OptionTransitionPhase::Leaving&&flow.frame()==14,"fresh B on second port initializes return");
    flow.update(0);check(flow.frame()==13,"return decrements");flow.update(0);flow.update(0);
    check(flow.update(0)&&flow.frame()==2&&!flow.active(),"return restores main frame exactly");
    check(flow.configure({1,5,10,3})&&flow.begin(0),"non-dividing bounded step accepted");flow.update(0);flow.update(0);
    check(flow.frame()==10,"entry does not overshoot");flow.update(1);flow.update(0);check(flow.update(0)&&flow.frame()==1,"return does not underflow");
    gf::MenuNavigationConfig nav;nav.repeat_limit=3;nav.repeat_restart=1;
    for(unsigned i=0;i<7;++i)nav.neighbor[i]={gf::u8((i+1)%7),gf::u8((i+6)%7),gf::u8((i+1)%7),gf::u8((i+6)%7)};
    gf::MenuWorld menu;check(menu.configure_navigation(nav)&&menu.enter_main_menu(0),"world owns actual input histories");
    std::array<gf::GamePadState,4> pads{};pads[2].a=true;menu.update_ports(pads);
    check(menu.screen()==gf::MenuScreen::Option,"any-port confirmation activates OPTION");
    for(unsigned i=0;i<20;++i)menu.observe_ports(pads);
    menu.resume_main_menu();menu.update_ports(pads);
    check(menu.screen()==gf::MenuScreen::MainMenu&&menu.selection()==0,"held confirmation during transition cannot reopen on return");
    pads={};menu.update_ports(pads);pads[2].a=true;menu.update_ports(pads);check(menu.screen()==gf::MenuScreen::Option,"fresh confirmation still works");
    std::printf("{\"option_transition_checks\":%u,\"all_passed\":true}\n",checks);
}
