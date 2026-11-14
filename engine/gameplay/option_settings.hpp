#pragma once
#include <array>
#include "engine/save/save_engine.hpp"
namespace gf {
// Native preferences, NOT a GameCube memory-card structure. Sound initialization
// is native policy (stereo); original initialization queries console settings.
struct OptionSettings {
    u8 mono=0;
    std::array<u8,3> volume={4,4,4};
    u8 rumble_mask=0;
    bool operator==(const OptionSettings&)const=default;
};
inline bool option_settings_valid(const OptionSettings& s){
    if(s.mono>1||s.rumble_mask>15)return false;
    for(u8 level:s.volume)if(level>4)return false;
    return true;
}
// Original 800FFA1C -> 8008BBD0 flags1/2: restore volumes and four rumble
// choices. It does NOT reset the sound-mode byte; unrelated pad mappings aren't
// represented by this slice and are not claimed to be restored.
inline void option_restore_values(OptionSettings& s){s.volume={4,4,4};s.rumble_mask=0;}
inline constexpr u32 kOptionSettingsSlot=1; // gameplay Session uses slot0
enum class OptionSettingsLoad { Loaded,Missing,Unavailable,Invalid };
class OptionSettingsStore {
public:
    explicit OptionSettingsStore(ISaveBackend& backend):backend_(backend),save_(backend){}
    OptionSettingsLoad load(OptionSettings* out){
        if(!out)return OptionSettingsLoad::Invalid;
        std::vector<u8> record;
        const auto status=backend_.read_slot(kOptionSettingsSlot,record);
        if(status==SaveReadStatus::Missing)return OptionSettingsLoad::Missing;
        if(status!=SaveReadStatus::Loaded)return OptionSettingsLoad::Unavailable;
        std::vector<u8> empty;SaveReader reader(empty);
        if(!save_.read_record(record,reader))return OptionSettingsLoad::Invalid;
        if(reader.u32_field()!=0x47465354u||reader.u32_field()!=1)return OptionSettingsLoad::Invalid;
        const u32 mono=reader.u32_field();std::array<u32,3> volume{};
        for(auto& v:volume)v=reader.u32_field();
        const u32 rumble=reader.u32_field();
        if(!reader.consumed()||mono>1||rumble>15)return OptionSettingsLoad::Invalid;
        OptionSettings candidate;candidate.mono=u8(mono);candidate.rumble_mask=u8(rumble);
        for(unsigned i=0;i<3;++i){if(volume[i]>4)return OptionSettingsLoad::Invalid;candidate.volume[i]=u8(volume[i]);}
        *out=candidate;return OptionSettingsLoad::Loaded;
    }
    bool write(const OptionSettings& s){
        if(!option_settings_valid(s))return false;
        SaveWriter writer;writer.u32_field(0x47465354u);writer.u32_field(1);writer.u32_field(s.mono);
        for(u8 level:s.volume)writer.u32_field(level);
        writer.u32_field(s.rumble_mask);return save_.write(kOptionSettingsSlot,writer);
    }
private:
    ISaveBackend& backend_;SaveEngine save_;
};
} // namespace gf
