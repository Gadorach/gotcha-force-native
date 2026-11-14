#pragma once
#include "engine/save/save_engine.hpp"
namespace gf {
// Owner-requested native tools. Story effects are deliberately represented by
// disabled placeholder rows until their actual gameplay owners exist.
struct CheatSettings {bool warehouse_selector=false,borg_editor=false;bool operator==(const CheatSettings&)const=default;};
enum class CheatSettingsLoad {Loaded,Missing,Invalid,Unavailable};
class CheatSettingsStore {
public:
    explicit CheatSettingsStore(ISaveBackend& backend):backend_(backend),save_(backend){}
    CheatSettingsLoad load(CheatSettings* out){
        writable_=false;if(!out)return CheatSettingsLoad::Invalid;
        *out={};std::vector<u8> record;const auto status=backend_.read_slot(2,record);
        if(status==SaveReadStatus::Missing){writable_=true;return CheatSettingsLoad::Missing;}
        if(status!=SaveReadStatus::Loaded)return CheatSettingsLoad::Unavailable;
        std::vector<u8> empty;SaveReader reader(empty);if(!save_.read_record(record,reader)||reader.u32_field()!=0x47464348u||reader.u32_field()!=1)return CheatSettingsLoad::Invalid;
        const auto flags=reader.u32_field();if(flags>3||!reader.consumed())return CheatSettingsLoad::Invalid;
        *out={bool(flags&1),bool(flags&2)};writable_=true;return CheatSettingsLoad::Loaded;
    }
    bool write(CheatSettings settings){if(!writable_)return false;SaveWriter writer;writer.u32_field(0x47464348u);writer.u32_field(1);writer.u32_field(unsigned(settings.warehouse_selector)|(unsigned(settings.borg_editor)<<1));return save_.write(2,writer);}
private:
    ISaveBackend& backend_;SaveEngine save_;bool writable_=false;
};
} // namespace gf
