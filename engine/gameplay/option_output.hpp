#pragma once
#include "engine/assets/option_output.hpp"
#include "engine/gameplay/option_settings.hpp"
#include "engine/audio/pcm_mixer.hpp"
#include "engine/input/rumble_policy.hpp"
namespace gf {
// Native PCM-category policy, not the CRI/MusyX synthesis/stream library. The
// exported common level curve supplies native linear PCM multipliers; original
// logarithmic quantization/fades and game sound-ID/category routing are separate.
class OptionOutput {
public:
    explicit OptionOutput(IHaptics& backend):rumble_(backend){}
    bool configure(const OptionOutputProfile& profile){
        for(unsigned i=0;i<5;++i)if(!std::isfinite(profile.level_gain[i])||profile.level_gain[i]<0||profile.level_gain[i]>1||
            (i&&profile.level_gain[i]<profile.level_gain[i-1]))return false;
        if(!rumble_.configure(profile.preview_updates))return false;
        profile_=profile;configured_=true;return true;
    }
    bool apply(const OptionSettings& settings,const std::array<unsigned,4>& devices,bool preview_changes){
        if(!configured_||!option_settings_valid(settings))return false;
        for(unsigned device:devices)if(device>=4)return false;
        std::array<f32,3> gains{};for(unsigned i=0;i<3;++i)gains[i]=profile_.level_gain[settings.volume[i]];
        if(!pcm_.set_policy(settings.mono!=0,gains)||!rumble_.set_policy(settings.rumble_mask,devices))return false;
        if(preview_changes&&have_settings_)for(unsigned port=0;port<4;++port)
            if((settings.rumble_mask&~settings_.rumble_mask)&(1u<<port))(void)rumble_.preview(port);
        settings_=settings;have_settings_=true;return true;
    }
    PcmMixer& pcm(){return pcm_;}
    f32 bgm_gain()const{return configured_&&have_settings_?profile_.level_gain[settings_.volume[0]]:1.0f;}
    void update(){rumble_.update();}
    void stop(){rumble_.stop_all();}
private:
    bool configured_=false,have_settings_=false;OptionOutputProfile profile_{};OptionSettings settings_{};
    PcmMixer pcm_;RumblePolicy rumble_;
};
} // namespace gf
