#pragma once
#include <limits>
#include <vector>
#include "engine/assets/source_adx_sdk_gain.hpp"
#include "engine/audio/pcm_q15.hpp"
namespace gf {
enum class SourceAdxSdkOutput:u8 {Mono,Stereo};
struct SourceAdxSdkGainPacket {u32 profile_tag=0;u64 control_serial=0;u16 envelope_start=0; s16 envelope_delta=0;u16 main_left=0,main_right=0,target=0;bool envelope_update=false;};
//Numeric control-state owner only. Callbacks are explicit source MIX updates,
//not game/render ticks or a real stream/device/backend-readiness observation.
//Copying this data for a staged PCM block is not a new playback capability.
class SourceAdxSdkGainControl {
public:
    bool configure(const SourceAdxSdkGainProfile& profile,SourceAdxSdkOutput mode){
        if(ready_||!profile.ready()||unsigned(mode)>1)return false;
        u16 left=0,right=0; s32 front=0,l=0,r=0;if(!profile.pan(0,&front)||!profile.pan(64,&l)||!profile.pan(63,&r)||!profile.lookup(front+(mode==SourceAdxSdkOutput::Mono?0:l),&left)||!profile.lookup(front+(mode==SourceAdxSdkOutput::Mono?0:r),&right))return false;profile_=profile;left_=left;right_=right;base_=target_=32767;ready_=true;return true;
    }
    bool request_volume(s32 sdk_units){if(!ready_)return false;const s32 clamped=sdk_units>0?0:sdk_units<-999?-999:sdk_units;if(clamped!=requested_){requested_=clamped;dirty_=true;}return true;}
    bool update_control(SourceAdxSdkGainPacket* out){
        if(!out||!ready_||serial_==std::numeric_limits<u64>::max())return false;
        bool update=false;if(settling_){base_=target_;settling_=false;update=true;}if(dirty_){if(!profile_.lookup(requested_,&target_))return false;dirty_=false;settling_=true;update=true;}const s16 delta=update?s16((s32(target_)-base_)/160):0;++serial_;*out={profile_.tag(),serial_,base_,delta,left_,right_,target_,update};return true;
    }
    bool ready()const{return ready_;}s32 requested_volume()const{return requested_;}bool dirty()const{return dirty_;}bool settling()const{return settling_;}
    u64 control_serial()const{return serial_;}
private:
    SourceAdxSdkGainProfile profile_;u16 base_=0,target_=0,left_=0,right_=0;u64 serial_=0;s32 requested_=0;bool ready_=false,dirty_=false,settling_=false;
};
//Source AX arithmetic for an explicit active160-sample mono input frame. The
//control packet is data, not proof that the actual SDK/queue clock ran. Native
//SRC/stream/EOF/generation/queue admission are separate producer responsibilities.
inline bool source_adx_sdk_gain_render(const SourceAdxSdkGainPacket& packet,std::span<const s16> mono,std::vector<s64>* stereo,u16* envelope_after=nullptr){
    if(!stereo||mono.size()!=160||!packet.profile_tag||!packet.control_serial||packet.envelope_start>32767||packet.main_left>32767||packet.main_right>32767)return false;
    const s32 end=s32(packet.envelope_start)+s32(packet.envelope_delta)*160;if(end<0||end>32767)return false;std::vector<s64> result(320);for(unsigned i=0;i<160;++i){const s32 volume=s32(packet.envelope_start)+s32(packet.envelope_delta)*s32(i);if(volume<0||volume>32767)return false;const s16 envelope=pcm_scale_q15_clipped(mono[i],volume);result[i*2]=s64(pcm_scale_q15_clipped(envelope,packet.main_left))*65536;result[i*2+1]=s64(pcm_scale_q15_clipped(envelope,packet.main_right))*65536;}*stereo=std::move(result);if(envelope_after)*envelope_after=u16(end);return true;
}
} //namespace gf
