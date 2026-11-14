#pragma once
#include <vector>
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
struct CreditsGlyph {u32 model=0,column=0;};
struct CreditsRow {u32 kind=0,delay=0,first=0,count=0;f32 spawn_y=0;};
struct CreditsConfig {
    u32 model_count=0,max_active=0,max_updates=0,entry_wait=0,final_hold=0;
    f32 speed=0,spawn_y=0,delete_y=0,hold_y=0,x_base=0,z_base=0,x_step=0,z_step=0,grid_factor=0;
    f32 image_x=0,image_z=0,rotation_y=0,scale=1;
    std::vector<CreditsRow> rows;std::vector<CreditsGlyph> glyphs;
};
inline bool credits_config_valid(const CreditsConfig& c){
    if(c.model_count<2||c.model_count>128||c.rows.empty()||c.rows.size()>512||c.glyphs.empty()||c.glyphs.size()>8192||
       c.max_active<2||c.max_active>4096||!c.max_updates||c.max_updates>32768||c.entry_wait>600||!c.final_hold||c.final_hold>600)return false;
    for(f32 v:{c.speed,c.spawn_y,c.delete_y,c.hold_y,c.x_base,c.z_base,c.x_step,c.z_step,c.grid_factor,c.image_x,c.image_z,c.rotation_y,c.scale})
        if(!std::isfinite(v)||std::fabs(v)>1e6f)return false;
    if(c.speed<=0||c.speed>64||c.spawn_y>=c.delete_y||c.hold_y<c.spawn_y||c.hold_y>=c.delete_y||
       (c.delete_y-c.spawn_y)/c.speed>4096||c.scale<=0||c.scale>8||std::fabs(c.rotation_y)>7||
       std::fabs(c.x_step)>1000||std::fabs(c.z_step)>1000||c.grid_factor<=0||c.grid_factor>4)return false;
    std::size_t consumed=0;
    for(std::size_t i=0;i<c.rows.size();++i){const auto& row=c.rows[i];
        if(row.kind<1||row.kind>9||row.first!=consumed||row.count>50||row.count>c.glyphs.size()-consumed||
           !std::isfinite(row.spawn_y)||std::fabs(row.spawn_y)>1e6f||row.spawn_y>=c.delete_y||
           (c.delete_y-row.spawn_y)/c.speed>4096)return false;
        if(row.kind==9){if(i+1!=c.rows.size()||row.count||row.delay)return false;}
        else if(i+1==c.rows.size()||!row.delay||row.delay>1000)return false;
        if(row.kind<=3){if(!row.count||row.spawn_y!=c.spawn_y)return false;}
        else if(row.kind==4){if(row.count)return false;}
        else if(row.kind<9&&row.count!=1)return false;
        for(u32 g=0;g<row.count;++g){const auto& glyph=c.glyphs[consumed+g];
            if(!glyph.model||glyph.model>=c.model_count||glyph.column>49||(row.kind>4&&glyph.column))return false;}
        consumed+=row.count;
    }
    return consumed==c.glyphs.size()&&c.rows.front().count>0;
}
} // namespace gf
