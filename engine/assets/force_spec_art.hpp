#pragma once
#include "engine/assets/force_list_art.hpp"
#include "engine/assets/source_font.hpp"
namespace gf {
struct ForceSpecWidget {u16 record=0;u8 type=0;s8 index=0;std::array<f32,3> position{};};
struct ForceSpecArt {
    u32 bank_hash=0,messages_hash=0;Camera camera;f32 aspect=0;std::vector<ForceSpecWidget> widgets;
    f32 floor_step=0,floor_low=0,floor_reset=0,number_step=0,value_step=0,part_shift=0,part_variant_x=0,angle_unit=0;
    std::array<std::array<u32,2>,3> colours{}; // source high,low; styles8,9,43
    std::array<std::string,20> tribes;std::array<std::string,5> jumps,features;
    bool load(ByteView b,const SourceFont& font,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="spec art: GFSA extent/domain";return false;};
        if(!b.data||b.size<72||b.size>16384||b.be32(0)!=0x47465341u||b.be32(4)!=1||!font.ready())return fail();
        bank_hash=b.be32(8);messages_hash=b.be32(12);std::array<f32,13> cam;for(unsigned i=0;i<13;++i)cam[i]=b.bef32(16+i*4);
        for(unsigned i=0;i<3;++i){camera.eye[i]=cam[i];camera.target[i]=cam[3+i];camera.up[i]=cam[6+i];}camera.fov_y_degrees=cam[9];camera.near_plane=cam[10];camera.far_plane=cam[11];aspect=cam[12];camera.viewport_width=640;camera.viewport_height=480;
        const auto count=b.be16(68);if(!count||count>128||b.be16(70)||!b.contains(72,std::size_t(count)*20+56)||!bank_hash||!messages_hash||aspect!=640.f/480||!camera.basis(nullptr))return fail();
        std::size_t at=72;for(unsigned i=0;i<count;++i){ForceSpecWidget w;const auto record=b.be32(at);const auto type=b.be16(at+4);const auto index=std::bit_cast<s16>(b.be16(at+6));if(record>4095||type>20||index<-1||index>5)return fail();w.record=u16(record);w.type=u8(type);w.index=s8(index);for(unsigned k=0;k<3;++k){w.position[k]=b.bef32(at+8+k*4);if(!std::isfinite(w.position[k])||std::abs(w.position[k])>1e6f)return fail();}widgets.push_back(w);at+=20;}
        f32* values[]={&floor_step,&floor_low,&floor_reset,&number_step,&value_step,&part_shift,&part_variant_x,&angle_unit};for(auto p:values){*p=b.bef32(at);at+=4;if(!std::isfinite(*p))return fail();}
        for(auto& pair:colours)for(auto& v:pair){v=b.be32(at);at+=4;}
        for(unsigned i=0;i<30;++i){if(!b.contains(at,2))return fail();const auto size=b.be16(at);at+=2;if(!size||size%2||size>128||!b.contains(at,size))return fail();for(unsigned j=0;j<size;j+=2){auto code=b.be16(at+j);if(code!=font.blank()&&font.glyph(code)<0)return fail();}auto& text=i<20?tribes[i]:i<25?jumps[i-20]:features[i-25];text.assign(reinterpret_cast<const char*>(b.data+at),size);at+=size;}
        // Exact source arithmetic domain permits bounded integer floor wrapping.
        if(at!=b.size||floor_step!=10||floor_low!=-6000||floor_reset!=2000||number_step!=-60||value_step!=-50||part_shift!=-29||part_variant_x!=275||f32_to_bits(angle_unit)!=0x38c90fdb)return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
