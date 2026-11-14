#pragma once
// 8893C/88AA0 list insertion ->53928/89CD0/89D28 ->89EB4/8A0A8.
// Native successful-allocation observations, NOT original pool addresses/capacity.
#include <algorithm>
#include <array>
#include <span>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
struct StartupRegistrationItem {u32 model=0,birth=0;u8 group=0,render_group=0,part=0,mask=0;bool visible=false,tail=false;};
struct StartupRegisteredModel {u32 model=0;u8 mask=0;};
inline bool startup_registration_plan(std::span<const StartupRegistrationItem> items,const std::array<bool,2>& parents_visible,bool menu_mode,std::vector<StartupRegisteredModel>* out){
    if(out)out->clear();
    if(!out||items.size()>128)return false;
    const std::array<u8,6> bases={0x28,0x20,0,0x10,8,0x18};
    std::vector<u32> ids;for(const auto& i:items){bool known=i.group==0x38;for(u8 base:bases)known|=i.group==base||i.group==base+1||i.group==base+2||i.group==base+7;if(i.model<3||!known||std::find(ids.begin(),ids.end(),i.model)!=ids.end()||(i.render_group!=0&&i.render_group!=1&&i.render_group!=0x41&&i.render_group!=0x42))return false;ids.push_back(i.model);}
    std::vector<StartupRegisteredModel> global;std::array<std::vector<StartupRegisteredModel>,2> actor;
    if(!menu_mode)for(unsigned i=0;i<2;++i)if(parents_visible[i])actor[i].push_back({i+1,0x47});
    auto group=[&](u8 which){std::vector<const StartupRegistrationItem*> list;for(const auto& i:items)if(i.group==which)list.push_back(&i);
        std::stable_sort(list.begin(),list.end(),[](const auto* a,const auto* b){if(a->tail!=b->tail)return !a->tail;if(a->birth!=b->birth)return a->tail?a->birth<b->birth:a->birth>b->birth;return a->part<b->part;});
        for(const auto* i:list)if(i->visible&&i->render_group){if(i->render_group==1)global.push_back({i->model,i->mask});else if(!menu_mode&&parents_visible[i->render_group-0x41])actor[i->render_group-0x41].push_back({i->model,i->mask});}
    };
    if(menu_mode){group(0x28);group(0x38);}else for(u8 base:bases){for(unsigned i=0;i<2;++i)group(u8(base+1+i));group(base);group(u8(base+7));}
    *out=std::move(global);if(!menu_mode)for(const auto& a:actor)out->insert(out->end(),a.begin(),a.end());return true;
}
} // namespace gf
