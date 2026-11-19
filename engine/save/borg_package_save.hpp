#pragma once
#include "engine/gameplay/borg_package_roster.hpp"
#include "engine/save/account_save_format.hpp"

namespace gf {
enum class PackageBorgLoad {Loaded,Missing,Unavailable,Invalid};

class PackageBorgSaveStore {
public:
    explicit PackageBorgSaveStore(ISaveBackend& backend):backend_(backend),save_(backend){}
    PackageBorgLoad load(PackageBorgRoster* out){
        writable_=false;loaded_record_.clear();
        if(!out)return PackageBorgLoad::Invalid;
        std::vector<u8> bytes;
        const auto status=backend_.read_slot(kBorgPackageSaveSlot,bytes);
        if(status==SaveReadStatus::Missing){writable_=true;return PackageBorgLoad::Missing;}
        if(status!=SaveReadStatus::Loaded)return PackageBorgLoad::Unavailable;
        std::vector<u8> empty;SaveReader reader(empty);
        if(!save_.read_record(bytes,reader)||reader.u32_field()!=magic_)return PackageBorgLoad::Invalid;
        const auto version=reader.u32_field();
        if(version!=1&&version!=2)return PackageBorgLoad::Invalid;
        const auto owned=reader.u32_field(),placements=reader.u32_field();
        if(!reader.ok()||owned>kBoxBorgs+kWarehouseBorgs||placements>kForceCount*kForceSlots)return PackageBorgLoad::Invalid;
        PackageBorgRoster next;next.owned.reserve(owned);next.placements.reserve(placements);
        bool has_variants=false;
        for(u32 i=0;i<owned;++i){
            OwnedPackageBorg borg;borg.instance=reader.u64_field();const auto flags=reader.u32_field();
            const auto id=reader.blob(),name=reader.blob();
            if(!reader.ok()||flags>1||id.empty()||id.size()>96||name.empty()||name.size()>96)return PackageBorgLoad::Invalid;
            borg.disabled=flags!=0;
            borg.identity.id.assign(id.begin(),id.end());borg.identity.name.assign(name.begin(),name.end());
            if(version==2){
                const auto present=reader.u32_field();
                if(!reader.ok()||present>1)return PackageBorgLoad::Invalid;
                if(present){
                    const auto key=reader.blob(),label=reader.blob();
                    if(!reader.ok()||key.empty()||key.size()>32||label.empty()||label.size()>64)return PackageBorgLoad::Invalid;
                    borg.variant=SavedPackageVariant{std::string(key.begin(),key.end()),std::string(label.begin(),label.end())};
                    has_variants=true;
                }
            }
            next.owned.push_back(std::move(borg));
        }
        for(u32 i=0;i<placements;++i){
            const auto instance=reader.u64_field();const auto force=reader.u32_field(),slot=reader.u32_field();
            if(!reader.ok()||force>=kForceCount||slot>=kForceSlots)return PackageBorgLoad::Invalid;
            next.placements.push_back({instance,u8(force),u8(slot)});
        }
        if(!reader.consumed()||(version==2&&!has_variants)||!package_borg_roster_valid(next))return PackageBorgLoad::Invalid;
        *out=std::move(next);loaded_record_=std::move(bytes);writable_=true;return PackageBorgLoad::Loaded;
    }
    bool write(const PackageBorgRoster& roster){
        if(!writable_||!package_borg_roster_valid(roster))return false;
        std::vector<u8> current;const auto status=backend_.read_slot(kBorgPackageSaveSlot,current);
        if(loaded_record_.empty()?status!=SaveReadStatus::Missing:
            (status!=SaveReadStatus::Loaded||current!=loaded_record_)){writable_=false;return false;}
        const bool has_variants=std::any_of(roster.owned.begin(),roster.owned.end(),[](const auto& borg){return borg.variant.has_value();});
        SaveWriter writer;writer.u32_field(magic_);writer.u32_field(has_variants?2:1);
        writer.u32_field(u32(roster.owned.size()));writer.u32_field(u32(roster.placements.size()));
        for(const auto& borg:roster.owned){
            writer.u64_field(borg.instance);writer.u32_field(u32(borg.disabled));
            for(const auto* value:{&borg.identity.id,&borg.identity.name})
                writer.blob(reinterpret_cast<const u8*>(value->data()),value->size());
            if(has_variants){
                writer.u32_field(u32(borg.variant.has_value()));
                if(borg.variant)for(const auto* value:{&borg.variant->id,&borg.variant->label})
                    writer.blob(reinterpret_cast<const u8*>(value->data()),value->size());
            }
        }
        for(const auto& place:roster.placements){writer.u64_field(place.instance);writer.u32_field(place.force);writer.u32_field(place.slot);}
        account_save_detail::Framing next;SaveEngine framing(next);
        if(!framing.write(0,writer)||!backend_.store(kBorgPackageSaveSlot,next.bytes.data(),next.bytes.size())){
            writable_=false;return false;
        }
        loaded_record_=std::move(next.bytes);
        return true;
    }
private:
    static constexpr u32 magic_=0x4e425247u; // NBRG: native package roster
    ISaveBackend& backend_;SaveEngine save_;bool writable_=false;
    std::vector<u8> loaded_record_;
};
}
