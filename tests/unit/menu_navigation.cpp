#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/menu_layout.hpp"
#include "engine/assets/menu_navigation.hpp"
#include "engine/gameplay/menu_world.hpp"
#include "engine/input/input_router.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"menu navigation: %s\n",why);std::abort();}}
gf::MenuNavigationConfig fixture(){
    gf::MenuNavigationConfig c;c.repeat_limit=3;c.repeat_restart=1;
    for(unsigned i=0;i<7;++i)c.neighbor[i]={gf::u8((i+1)%7),gf::u8((i+6)%7),gf::u8((i+2)%7),gf::u8((i+5)%7)};
    return c;
}
gf::GamePadState direction(unsigned column){gf::GamePadState p;
    if(column==0)p.dpad_up=true;
    if(column==1)p.dpad_down=true;
    if(column==2)p.dpad_left=true;
    if(column==3)p.dpad_right=true;
    return p;
}
void tap(gf::MainMenu& menu,gf::GamePadState pad){menu.update(pad);menu.update({});}
std::vector<gf::u8> content(const gf::MenuNavigationConfig& c){
    std::vector<gf::u8> b(52);unsigned at=0;
    for(gf::u32 n:{0x47464e56u,1u,123u,7u,gf::u32(c.repeat_limit),gf::u32(c.repeat_restart)}){gf::store_be32(b.data()+at,n);at+=4;}
    for(const auto& row:c.neighbor)for(gf::u8 n:row)b[at++]=n;
    return b;
}
}
int main(){
    const auto config=fixture();check(gf::menu_navigation_valid(config),"owned graph valid");
    const auto bytes=content(config);gf::MenuNavigationConfig parsed;std::string error;
    check(gf::menu_navigation_load({bytes.data(),bytes.size()},123,&parsed,&error),"versioned content loads");
    check(parsed.neighbor==config.neighbor&&parsed.repeat_limit==3,"graph/repeat parameters retained");
    auto reject=[&](std::vector<gf::u8> b){check(!gf::menu_navigation_load({b.data(),b.size()},123,&parsed,&error)&&!error.empty()&&!gf::menu_navigation_valid(parsed),"bad content refuses without publishing graph");};
    {auto b=bytes;b.resize(51);reject(b);}
    {auto b=bytes;b.push_back(0);reject(b);}
    {auto b=bytes;b[11]^=1;reject(b);}
    {auto b=bytes;b[0]=0;reject(b);}
    {auto b=bytes;b[24]=7;reject(b);}
    {auto b=bytes;b[19]=255;reject(b);}
    {auto b=bytes;b[23]=4;reject(b);}
    {auto b=bytes;for(unsigned i=24;i<52;++i)b[i]=0;reject(b);}
    for(unsigned item=0;item<7;++item)for(unsigned column=0;column<4;++column){
        gf::MainMenu m;check(m.configure_navigation(config)&&m.enter_main_menu(item),"enter graph node");
        tap(m,direction(column));check(m.selection()==config.neighbor[item][column],"all owned graph directions use explicit adjacency");
    }
    {
        gf::MainMenu m;check(m.configure_navigation(config)&&m.enter_main_menu(0),"priority fixture");
        auto both=direction(0);both.dpad_down=both.dpad_left=both.dpad_right=true;
        tap(m,both);check(m.selection()==config.neighbor[0][0],"Up wins contradictory directions");
        m.enter_main_menu(0);both={};both.dpad_down=both.dpad_left=both.dpad_right=true;
        tap(m,both);check(m.selection()==config.neighbor[0][1],"Down beats Left/Right");
        m.enter_main_menu(0);both={};both.dpad_left=both.dpad_right=true;
        tap(m,both);check(m.selection()==config.neighbor[0][2],"Left beats Right");
        const auto revision=m.revision();gf::GamePadState back;back.b=true;tap(m,back);
        check(m.screen()==gf::MenuScreen::MainMenu&&m.revision()==revision,"B does not leave original main-menu idle path");
        m.enter_main_menu(0);auto go=direction(3);go.a=true;go.b=true;
        m.update(go);check(m.selection()==config.neighbor[0][3]&&m.screen()==gf::MainMenu::screen_for(m.selection()),"navigate before confirmation; B does not suppress main-menu A");
        const auto selected=m.selection();m.update({});tap(m,back);
        check(m.screen()==gf::MenuScreen::MainMenu&&m.selection()==selected,"NOT PORTED native return keeps selection");
    }
    {
        gf::MainMenu m;m.configure_navigation(config);m.enter_main_menu(0);const auto held=direction(3);
        m.update(held);const unsigned first=m.selection();
        for(unsigned tick=0;tick<3;++tick)m.update(held);
        check(m.selection()==first,"held direction waits beyond repeat limit");
        m.update(held);check(m.selection()==config.neighbor[first][3],"first repeat pulse");
        const auto second=m.selection();m.update(held);m.update(held);check(m.selection()==second,"repeat restart interval");
        m.update(held);check(m.selection()==config.neighbor[second][3],"next repeat pulse");
        m.update({});m.update(held);check(m.selection()==config.neighbor[config.neighbor[second][3]][3],"release/repress is immediate");
        m.enter_main_menu(0);gf::GamePadState a;a.a=true;m.update(a);const auto target=m.screen();
        for(unsigned i=0;i<100;++i)m.update(a);
        check(m.screen()==target,"held confirm is never repeated");
        m.enter_main_menu(0);m.update(held);const auto before=m.selection();
        check(m.configure_navigation(config),"reconfiguration accepts same graph");m.update(held);
        check(m.selection()==before,"reconfiguration cannot turn held input into new press");
    }
    {
        for(unsigned port=0;port<4;++port){
            gf::MainMenu m;m.configure_navigation(config);m.enter_main_menu(0);
            std::array<gf::GamePadState,4> pads{};pads[port]=direction(2);m.update_ports(pads);
            check(m.selection()==config.neighbor[0][2],"each independent controller navigates");
            pads={};m.update_ports(pads);pads[port].start=true;m.update_ports(pads);
            check(m.screen()==gf::MainMenu::screen_for(config.neighbor[0][2]),"each controller can confirm with Start");
        }
        gf::MainMenu m;m.configure_navigation(config);m.enter_main_menu(0);
        std::array<gf::GamePadState,4> pads{};pads[0]=direction(1);pads[3]=direction(0);m.update_ports(pads);
        check(m.selection()==config.neighbor[0][0],"combined ports retain source directional priority");
        gf::InputRouter router;gf::InputSnapshot devices;devices.port_count=4;
        devices.pad(2).buttons=gf::default_input_map().mask_dpad_right;
        for(unsigned port=0;port<4;++port)pads[port]=router.route(devices,port);
        m.enter_main_menu(0);m.update_ports(pads);check(m.selection()==config.neighbor[0][3],"physical port 2 reaches spatial menu through default router");
        for(unsigned port=0;port<4;++port)check(pads[port].dpad_right==(port==2),"default routing does not clone pad 0");
    }
    {
        gf::MainMenu m;m.configure_navigation(config);m.enter_main_menu(0);
        gf::GamePadState stick;stick.stick_x=100;tap(m,stick);check(m.selection()==config.neighbor[0][3],"horizontal stick navigates");
        m.enter_main_menu(0);stick={};stick.stick_y=100;tap(m,stick);check(m.selection()==config.neighbor[0][0],"vertical stick navigates");
        m.enter_main_menu(0);stick={};stick.cstick_x=127;tap(m,stick);check(m.selection()==0,"C-stick does not navigate main menu");
        stick={};stick.stick_x=20;tap(m,stick);check(m.selection()==0,"small stick deflection remains neutral");
        auto invalid=config;invalid.neighbor[0][0]=7;
        check(!m.configure_navigation(invalid)&&m.spatial_navigation(),"invalid replacement cannot discard configured graph");
        check(!m.enter_main_menu(7)&&m.selection()==0,"invalid entry cannot invent selection");
    }
    {
        gf::MenuWorld world;check(world.configure_navigation(config)&&world.enter_main_menu(0),"world delegates graph/startup entry");
        world.update(direction(2));check(world.selection()==config.neighbor[0][2],"world uses same spatial state machine");
    }
    bool real=false;
    std::ifstream nf("local/content/main-menu.gfmn",std::ios::binary),af("local/analysis/arc/stff_mdl.arc",std::ios::binary);
    if(nf&&af){
        std::vector<gf::u8> n((std::istreambuf_iterator<char>(nf)),{}),a((std::istreambuf_iterator<char>(af)),{});
        gf::MenuNavigationConfig actual;check(gf::menu_navigation_load({n.data(),n.size()},gf::menu_asset_hash({a.data(),a.size()}),&actual,&error),"real archive-bound graph loads");
        gf::MainMenu m;m.configure_navigation(actual);m.enter_main_menu(5);
        const unsigned columns[]={3,0,2,2,2,1,3},targets[]={4,0,3,2,1,6,5};
        for(unsigned i=0;i<7;++i){tap(m,direction(columns[i]));check(m.selection()==targets[i],"emulator-observed spatial cycle reproduced");}
        m.enter_main_menu(0);const auto revision=m.revision();tap(m,direction(0));
        check(m.selection()==0&&m.revision()==revision,"real upper boundary self-edge does not wrap or revise");real=true;
    }
    std::printf("{\"menu_navigation_checks\":%u,\"real_content_present\":%s,\"all_passed\":true}\n",checks,real?"true":"false");
}
