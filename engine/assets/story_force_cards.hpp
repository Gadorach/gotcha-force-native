#pragma once
#include "engine/assets/source_force_cards.hpp"
#include "engine/graphics/camera.hpp"
namespace gf {
//Story F9124 root and F7F70 renderer0 projection, separate from Challenge's
//moving/hidden card positions. Profiles contain no pointers or object images.
class StoryForceCardArt {
public:
    bool load(ByteView cards,ByteView presentation,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="Story card art: profile/bank/camera domain";return false;};
        if(!source_force_cards_load(cards,&layout_,error))return fail();
        //The common algorithm also accepts Challenge's .5 grid scale; Story's
        //source operand is1. Do not admit a swapped peer profile just because
        //both modes bind the same vsel01 bank.
        if(layout_.grid_scale!=1)return fail();
        if(!presentation.data||presentation.size!=84||presentation.be32(0)!=0x4746534b||presentation.be32(4)!=1||presentation.be32(8)!=layout_.bank_hash||presentation.be16(82))return fail();
        std::size_t at=12;auto real=[&](){const auto f=presentation.bef32(at);at+=4;return f;};
        for(auto& f:camera_.eye)f=real();
        for(auto& f:camera_.target)f=real();
        for(auto& f:camera_.up)f=real();
        camera_.fov_y_degrees=real();camera_.near_plane=real();camera_.far_plane=real();aspect_=real();
        for(auto& f:origin_)f=real();
        angle_unit_=real();shell_=presentation.be16(at);
        for(unsigned i=0;i<3;++i)if(shell_==layout_.formats[i].record)return fail();
        constexpr std::array<f32,3> eye{0,0,0},target{0,0,-1},up{0,1,0};
        for(unsigned i=0;i<3;++i)if(camera_.eye[i]!=eye[i]||camera_.target[i]!=target[i]||camera_.up[i]!=up[i])return fail();
        if(!camera_.basis(nullptr)||camera_.fov_y_degrees!=41||camera_.near_plane!=10||camera_.far_plane!=40000||!std::isfinite(aspect_)||std::abs(aspect_-4.f/3)>1e-6f||angle_unit_!=bits_to_f32(0x38c90fdb)||shell_>=4096)return fail();
        for(auto f:origin_)if(!std::isfinite(f)||std::abs(f)>1e6f)return fail();
        tag_=2166136261u;for(auto bytes:{cards,presentation})for(std::size_t i=0;i<bytes.size;++i)tag_=(tag_^bytes.data[i])*16777619u;
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u32 bank_hash()const{return layout_.bank_hash;}u32 tag()const{return tag_;}
    const SourceForceCardsArt& layout()const{return layout_;}const Camera& camera()const{return camera_;}
    const std::array<f32,3>& origin()const{return origin_;}f32 angle_unit()const{return angle_unit_;}u16 shell()const{return shell_;}
private:
    SourceForceCardsArt layout_;Camera camera_;std::array<f32,3> origin_{};f32 aspect_=0,angle_unit_=0;u32 tag_=0;u16 shell_=0;bool ready_=false;
};
} //namespace gf
