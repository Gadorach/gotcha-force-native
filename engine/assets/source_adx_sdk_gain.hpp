#pragma once
#include <array>
#include <span>
#include <bit>
#include "engine/core/types.hpp"
namespace gf {
//Exact initialized MIX volume/pan tables, not a fitted logarithmic curve.
class SourceAdxSdkGainProfile {
public:
    bool load(ByteView b){
        *this={};if(!b.data||b.size!=2462||b.be32(0)!=0x47464147||b.be32(4)!=1||b.be16(8)!=963||b.be16(10)!=128||std::bit_cast<s32>(b.be32(12))!=-903||b.be32(16)!=60||b.be16(20)!=0xff64||b.be16(22))return false;
        SourceAdxSdkGainProfile p;for(unsigned i=0;i<963;++i){p.volume_[i]=b.be16(24+i*2);if(!p.volume_[i]||p.volume_[i]>0xff64||(i&&p.volume_[i]<p.volume_[i-1]))return false;}for(unsigned i=0;i<128;++i){p.pan_[i]=std::bit_cast<s32>(b.be32(24+963*2+i*4));if(p.pan_[i]>0||p.pan_[i]<-999||(i&&p.pan_[i]>p.pan_[i-1]))return false;}if(p.volume_[0]!=1||p.volume_[903]!=32767||p.pan_[0]!=0||p.pan_[63]!=-30||p.pan_[64]!=-30||p.pan_[127]!=-904)return false;p.tag_=2166136261;for(std::size_t i=0;i<b.size;++i)p.tag_=(p.tag_^b.data[i])*16777619u;if(!p.tag_)return false;*this=p;return true;
    }
    bool ready()const{return tag_!=0;}u32 tag()const{return tag_;}
    bool lookup(s32 attenuation,u16* out)const{if(!out||!ready())return false;*out=attenuation<-903?0:attenuation>=60?0xff64:volume_[unsigned(attenuation+903)];return true;}
    bool pan(unsigned index,s32* out)const{if(!out||!ready()||index>=128)return false;*out=pan_[index];return true;}
private:
    std::array<u16,963> volume_{};std::array<s32,128> pan_{};u32 tag_=0;
};
} //namespace gf
