#pragma once
#include "engine/assets/force_list_art.hpp"
namespace gf {
enum class ForcePopupKind:u8 {Operations,BorgSort,PartSort,Warehouse,Part,Tribe,Erase};
struct ForcePopupLayout {std::array<ForceListWidget,5> widgets;std::array<f32,3> label_origin{};std::vector<u16> labels;};
struct ForcePopupArt {
    u32 bank_hash=0;std::array<ForcePopupLayout,6> menus;std::array<ForceListWidget,6> confirmation;
    f32 menu_z=0,confirm_z=0,open_limit=0,visible_after=0,row_step=0,angle_unit=0,disabled_frame=0;
    bool load(ByteView bytes,std::string* error){
        *this={};auto fail=[&](){if(error)*error="force popup art: GFPU extent/domain";return false;};
        if(!bytes.data||bytes.size<12||bytes.size>8192||bytes.be32(0)!=0x47465055u||bytes.be32(4)!=1)return fail();
        bank_hash=bytes.be32(8);std::size_t at=12;bool valid=true;
        auto word=[&](){if(!bytes.contains(at,4)){valid=false;return u32(0);}auto v=bytes.be32(at);at+=4;return v;};auto half=[&](){if(!bytes.contains(at,2)){valid=false;return u16(0);}auto v=bytes.be16(at);at+=2;return v;};auto real=[&](){auto v=bits_to_f32(word());if(!std::isfinite(v)||std::abs(v)>1e6)valid=false;return v;};
        auto widget=[&](auto& group){for(auto& w:group){const auto id=word();if(id>=4096)valid=false;w.record=s16(id);w.pass=0x44;for(auto& p:w.position)p=real();}};
        for(auto& m:menus)widget(m.widgets);
        for(auto& m:menus){for(auto& p:m.label_origin)p=real();const auto count=half();if(half()||!count||count>32)return fail();for(unsigned i=0;i<count;++i){const auto id=half();if(id>=4096)valid=false;m.labels.push_back(id);}}
        widget(confirmation);menu_z=real();confirm_z=real();open_limit=real();visible_after=real();row_step=real();angle_unit=real();disabled_frame=real();
        if(!valid||at!=bytes.size||open_limit!=25||visible_after!=8||row_step>=0||angle_unit<=0||disabled_frame<0)return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
