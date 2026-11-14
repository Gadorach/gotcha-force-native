#pragma once
// Existing native menu art/motion/navigation, created once at bridge400 and
// input-enabled only at449. No save/audio/destination/reticle policy added.
#include "engine/assets/menu_layout.hpp"
#include "engine/assets/menu_motion.hpp"
#include "engine/assets/menu_navigation.hpp"
#include "engine/gameplay/menu_scene.hpp"
#include "engine/gameplay/menu.hpp"
namespace gf {
class StartupMenuScene {
public:
    bool load(ByteView bank,ByteView layout,ByteView motion,ByteView navigation,std::string* error){
        loaded_=created_=enabled_=false;creates_=enables_=0;menu_={};scene_={};HsdScene source,configured;Camera ignored;MenuNavigationConfig nav;
        if(!hsd_load(bank,&source,error)||!menu_layout_load(layout,bank,source,&configured,&ignored,error)||!menu_motion_load(motion,menu_asset_hash(bank),configured,&configured.menu_motion,error)||!menu_navigation_load(navigation,menu_asset_hash(bank),&nav,error)||!menu_.configure_navigation(nav)||!scene_.load_scene(std::move(configured),error))return false;
        loaded_=true;if(error)error->clear();return true;
    }
    bool step(bool create,bool enable,const std::array<GamePadState,4>& pads,std::string* error){
        return step_impl(create,enable,pads,nullptr,error);
    }
    bool step_with_sound(bool create,bool enable,const std::array<GamePadState,4>& pads,UiSoundBatch* sounds,std::string* error){
        if(sounds)*sounds={};
        return step_impl(create,enable,pads,sounds,error);
    }
private:
    bool step_impl(bool create,bool enable,const std::array<GamePadState,4>& pads,UiSoundBatch* sounds,std::string* error){
        if(!loaded_||(!created_&&enable&&!create)||(create&&created_)||(enable&&enabled_)){if(error)*error="startup menu: duplicate/out-of-order handoff";loaded_=false;return false;}
        if(create){created_=true;++creates_;if(!menu_.enter_main_menu(scene_.scene().menu_selection))return false;scene_.set_selection(menu_.selection());}
        else if(created_)scene_.advance_motion();
        if(!created_||!enabled_||enable)menu_.observe_ports(pads);else menu_.update_ports_with_sound(pads,sounds);
        if(enable){enabled_=true;++enables_;}scene_.set_selection(menu_.selection());if(error)error->clear();return true;
    }
public:
    unsigned upload_textures_shared(RenderEngine& r,u32 base){return loaded_?scene_.upload_textures_shared(r,base):0;}
    unsigned draw_world(unsigned item,unsigned part,RenderEngine& r,const Camera& c){return created_?scene_.draw_menu_world_model(r,c,item,part):0;}
    unsigned draw_selector(RenderEngine& r,const Camera& c){return created_?scene_.draw_menu_selector(r,c):0;}
    unsigned draw_caption(RenderEngine& r,const Camera& c){return created_?scene_.draw_menu_caption(r,c):0;}
    bool ready()const{return loaded_;}bool created()const{return created_;}bool enabled()const{return enabled_;}unsigned creates()const{return creates_;}unsigned enables()const{return enables_;}unsigned selection()const{return menu_.selection();}MenuScreen screen()const{return menu_.screen();}
private:
    MenuScene scene_;MainMenu menu_;bool loaded_=false,created_=false,enabled_=false;unsigned creates_=0,enables_=0;
};
} // namespace gf
