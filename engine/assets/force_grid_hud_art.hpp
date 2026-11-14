#pragma once
#include "engine/assets/force_list_art.hpp"
namespace gf {
// Original grid HUD layout operands, not screen-fitted text/rectangle positions.
struct ForceGridHudArt {
    u32 bank_hash=0;std::array<ForceListWidget,3> header,membership;
    std::array<ForceListWidget,7> blade;std::array<std::array<ForceListWidget,8>,3> cost;
    std::array<std::array<f32,3>,3> cost_origin;std::array<f32,3> name_origin{},highlight_origin{},new_origin{},membership_origin{};
    std::array<f32,3> held_offset{};std::array<u32,2> name_colours{};
    f32 name_extent=0,name_advance=0,header_one=0,blade_one=0,membership_one=0;
    f32 unlimited_y=0,fold_cost_x=0,hand_y=0,held_lift=0,held_scale=0,angle_unit=0;
    u16 hand=0,held_hand=0,highlight=0,fresh=0,pulse_step=0,period=0;
    std::array<ForceListWidget,3> grid;f32 grid_mask_z=0;
    std::array<ForceListWidget,1> footer_background;
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="grid HUD: GFGH extent/domain";return false;};
        if(!b.data||b.size<12||b.size>4096||b.be32(0)!=0x47464748u||b.be32(4)!=3)return fail();
        bank_hash=b.be32(8);std::size_t at=12;bool valid=true;
        auto word=[&](){if(!b.contains(at,4)){valid=false;return u32(0);}const auto v=b.be32(at);at+=4;return v;};
        auto half=[&](){if(!b.contains(at,2)){valid=false;return u16(0);}const auto v=b.be16(at);at+=2;return v;};
        auto real=[&](){const auto v=bits_to_f32(word());if(!std::isfinite(v)||std::abs(v)>1e6f)valid=false;return v;};
        auto widgets=[&](auto& group){for(auto& w:group){w.record=std::bit_cast<s16>(half());w.pass=half();w.scale=real();for(auto& p:w.position)p=real();if(w.record<-1||w.record>=4096||w.scale<=0||(w.pass!=1&&w.pass!=0x44&&w.pass!=0x45))valid=false;}};
        widgets(header);widgets(blade);for(auto& c:cost)widgets(c);widgets(membership);
        auto point=[&](auto& p){for(auto& v:p)v=real();};for(auto& p:cost_origin)point(p);point(name_origin);point(highlight_origin);point(new_origin);point(membership_origin);point(held_offset);
        name_extent=real();name_advance=real();header_one=real();blade_one=real();membership_one=real();unlimited_y=real();fold_cost_x=real();hand_y=real();held_lift=real();held_scale=real();angle_unit=real();
        for(auto& c:name_colours)c=word();
        hand=half();held_hand=half();highlight=half();fresh=half();pulse_step=half();period=half();
        widgets(grid);grid_mask_z=real();
        widgets(footer_background);
        if(!valid||at!=b.size||name_extent<=0||name_advance<=0||held_scale<=0||angle_unit<=0||hand>=4096||held_hand>=4096||highlight>=4096||fresh>=4096||!period||!pulse_step)return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
