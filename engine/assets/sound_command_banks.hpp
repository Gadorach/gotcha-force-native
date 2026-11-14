#pragma once
// Explicit immutable normal bank0/1/2 tone resources. Source bank is cue/128,
// chain next-byte stays within THAT bank. FNV is a scope binding, not auth.
#include <memory>
#include <span>
#include "engine/assets/startup_sound_commands.hpp"
namespace gf {
struct SoundCommandBankInput {u8 id=0;ByteView tones;};
class SoundCommandBanks {
public:
    bool load(const StartupSoundCommandProfile& source,std::span<const SoundCommandBankInput> inputs,std::string* error){
        banks_={};profile_={};
        auto fail=[&](const char* why){if(error)*error=std::string("sound command banks: ")+why;return false;};
        if(!startup_sound_command_profile_valid(source)||inputs.empty()||inputs.size()>3)return fail("source gain profile/explicit normal-bank budget");
        std::array<std::shared_ptr<const StartupSoundToneBank>,3> next;
        for(const auto& input:inputs){if(input.id>2||next[input.id])return fail("unknown/duplicate bank");if(!input.tones.data||input.tones.size>2064)return fail("bounded owned tone input");
            auto profile=source;profile.tone_hash=startup_sound_commands_asset_detail::fnv(input.tones);
            if(input.id==0&&profile.tone_hash!=source.tone_hash)return fail("original bank0 profile binding");
            auto bank=std::make_shared<StartupSoundToneBank>();if(!startup_sound_tones_load(input.tones,profile,bank.get(),error))return false;next[input.id]=std::move(bank);
        }
        banks_=std::move(next);profile_=source;if(error)error->clear();return true;
    }
    bool ready()const{return startup_sound_command_profile_valid(profile_);}
    const StartupSoundCommandProfile& profile()const{return profile_;}
    std::shared_ptr<const StartupSoundToneBank> find(u8 id)const{return ready()&&id<3?banks_[id]:nullptr;}
private:
    StartupSoundCommandProfile profile_;std::array<std::shared_ptr<const StartupSoundToneBank>,3> banks_{};
};
} // namespace gf
