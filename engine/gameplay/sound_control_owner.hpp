#pragma once
// ECF60/EF9D4/10D9AC initialized SFX controls + existing native OPTION values.
// One serialized portable owner, no globals/device or mutable provider callback.
// Empty native effect slots are ACTUAL owner state, not original lifetime-absence
// proof. An unimplemented effect request invalidates output instead of dry sum.
#include <limits>
#include <memory>
#include <optional>
#include <algorithm>
#include <span>
#include <vector>
#include "engine/gameplay/option_settings.hpp"
#include "engine/gameplay/startup_sound_commands.hpp"
#include "engine/gameplay/sound_voice_parameters.hpp"
namespace gf {
namespace sound_control_owner_detail {
struct Scope {mutable bool music_claimed=false;};
struct BattleFlags {u8 field3b36=0,field3b37=0,field3e3c=0;bool operator==(const BattleFlags&)const=default;};
struct Data {
    OptionSettings settings;u64 revision=1;bool disabled=false,muted=false;
    std::vector<u8> banks;
    std::optional<BattleFlags> battle_flags;
};
}
struct SoundControlFrameInput {
    SoundVoiceLifecycleEventObservation event_mode;
    SoundVoiceControlObservation controls;
    std::vector<SoundVoiceBankGainObservation> banks;
    u64 revision=0;
};
class SoundControlSnapshot {
public:
    bool live()const{return data_&&!scope_.expired();}
    u64 revision()const{return data_?data_->revision:0;}
    bool same_owner(const SoundControlSnapshot& other)const{
        const auto scope=scope_.lock();return scope&&scope==other.scope_.lock();
    }
    const sound_control_owner_detail::BattleFlags* battle_flags()const{return live()&&data_->battle_flags?&*data_->battle_flags:nullptr;}
    std::optional<u8> bgm_index()const{return live()?std::optional<u8>{data_->settings.volume[0]}:std::nullopt;}
    bool commands(StartupSoundCommandObservation* out,std::string* error)const{
        if(out)*out={};
        if(!out||!live())return fail(error,"live control owner/output required");
        *out={true,true,u8(data_->disabled),s8(data_->settings.volume[1])};if(error)error->clear();return true;
    }
    // Pure stamped DATA lookup; this does NOT advance callback/event clocks.
    // The existing CPU owner checks callback succession and actual acquisitions.
    bool frame(u64 callback,SoundControlFrameInput* out,std::string* error)const{
        if(out)*out={};
        if(!out||!live()||!callback)return fail(error,"live snapshot/nonzero callback/output required");
        SoundControlFrameInput result;result.revision=data_->revision;
        // Native preference selects source mono0/stereo1. Default stereo remains
        // native policy, NOT a console audio-mode query/hardware-parity claim.
        result.event_mode={true,u8(data_->settings.mono?0:1)};
        result.controls.callback=callback;result.controls.declared=true;
        result.controls.volume=result.controls.master_volume=127;
        result.controls.mute=u8(data_->muted);result.controls.aux_flags=0;
        for(auto& aux:result.controls.aux)aux={true,false,0};
        for(u8 id:data_->banks){SoundVoiceBankGainObservation bank;bank.callback=callback;bank.bank=id;bank.volume=127;bank.declared=true;bank.programs.fill(127);result.banks.push_back(bank);}
        // OPTION SFX weight belongs to command lowering ONCE. It is deliberately
        // not multiplied into these global gains or into an extra SFX host sink.
        *out=std::move(result);if(error)error->clear();return true;
    }
private:
    friend class SoundControlOwner;
    friend class MusicRequestOwner;
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("sound control snapshot: ")+why;return false;}
    std::weak_ptr<const sound_control_owner_detail::Scope> scope_;
    std::shared_ptr<const sound_control_owner_detail::Data> data_;
};
class SoundControlOwner {
public:
    SoundControlOwner()=default;
    SoundControlOwner(const SoundControlOwner&)=delete;
    SoundControlOwner& operator=(const SoundControlOwner&)=delete;
    SoundControlOwner(SoundControlOwner&&)=default;
    SoundControlOwner& operator=(SoundControlOwner&&)=default;
    bool initialize(std::span<const u8> explicit_banks,const OptionSettings& settings,std::string* error){
        if(data_||scope_||failed_)return fail(error,"fresh/reset owner required");
        if(!option_settings_valid(settings)||explicit_banks.empty()||explicit_banks.size()>3)return fail(error,"native settings/explicit normal bank domain");
        std::array<bool,3> seen{};for(u8 bank:explicit_banks){if(bank>2||seen[bank])return fail(error,"unknown/duplicate bank; no bank0 alias");seen[bank]=true;}
        auto next=std::make_shared<sound_control_owner_detail::Data>();next->settings=settings;next->banks.assign(explicit_banks.begin(),explicit_banks.end());
        scope_=std::make_shared<const sound_control_owner_detail::Scope>();data_=std::move(next);if(error)error->clear();return true;
    }
    bool apply_preferences(const OptionSettings& settings,std::string* error){
        if(!ready()||!option_settings_valid(settings))return fail(error,"ready owner/validated native settings required");
        if(data_->settings==settings){if(error)error->clear();return true;}
        auto next=*data_;next.settings=settings;return publish(std::move(next),error);
    }
    bool set_disabled(bool value,std::string* error){
        if(!ready())return fail(error,"ready owner required");
        if(data_->disabled==value){if(error)error->clear();return true;}
        auto next=*data_;next.disabled=value;return publish(std::move(next),error);
    }
    bool set_muted(bool value,std::string* error){
        if(!ready())return fail(error,"ready owner required");
        if(data_->muted==value){if(error)error->clear();return true;}
        auto next=*data_;next.muted=value;return publish(std::move(next),error);
    }
    std::optional<u8> initial_program_gain(u8 bank,u8 program)const{
        if(!ready()||program>=128||std::find(data_->banks.begin(),data_->banks.end(),bank)==data_->banks.end())return std::nullopt;
        return u8(127);
    }
    //52990→F0920→F0F64→ED5E0(0,1,127), thenF0A34/F0A60.
    //This owner initializes all program gains to127 and exposes no mutation of
    //them; ED5E0's equal-value branch performs no dirty-word store. Do not use
    //this narrow operation to acknowledge an arbitrary gain/effect backend.
    bool initialize_battle_controls(std::string* error){
        if(initial_program_gain(0,1)!=127)return fail(error,"current bank0 initial program gains required");
        auto next=*data_;next.battle_flags=sound_control_owner_detail::BattleFlags{};
        return publish(std::move(next),error);
    }
    // This is an output-effect slot0/1, NOT graphics auxiliary IDs40/41. No new
    // unsupported effect is silently accepted with a fabricated empty getter.
    bool require_auxiliary_effect(u8 slot,std::string* error){
        if(!ready()||slot>=2)return fail(error,"ready native auxiliary slot required");
        failed_=true;scope_.reset();return fail(error,"auxiliary effect/output consumer required; no dry substitute");
    }
    bool ready()const{return data_&&scope_&&!failed_;}
    bool failed()const{return failed_;}
    SoundControlSnapshot snapshot()const{SoundControlSnapshot result;result.scope_=scope_;result.data_=data_;return result;}
    void reset(){scope_.reset();data_.reset();failed_=false;}
private:
    bool publish(sound_control_owner_detail::Data next,std::string* error){
        if(next.revision==std::numeric_limits<u64>::max())return fail(error,"revision overflow");
        ++next.revision;data_=std::make_shared<const sound_control_owner_detail::Data>(std::move(next));if(error)error->clear();return true;
    }
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("sound control owner: ")+why;return false;}
    std::shared_ptr<const sound_control_owner_detail::Scope> scope_;
    std::shared_ptr<const sound_control_owner_detail::Data> data_;
    bool failed_=false;
};
} // namespace gf
