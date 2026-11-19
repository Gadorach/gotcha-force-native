#pragma once
#include <variant>
#include "engine/assets/borg_practice_profile.hpp"
#include "engine/save/borg_package_save.hpp"
#include "engine/save/scoped_retail_force_record.hpp"
#include "platform/pc/borg_selected_save.hpp"

namespace gf::pc {
struct BorgPracticeFiles {
    BorgIdentity identity;
    std::string variant;
    std::array<std::string,NativeGRedPracticeAssets::Count> bytes;
    NativeGRedPracticeAssets views()const{
        NativeGRedPracticeAssets assets;
        for(unsigned i=0;i<bytes.size();++i)assets.bytes[i]={
            reinterpret_cast<const u8*>(bytes[i].data()),bytes[i].size()};
        return assets;
    }
};

// Capture only the exact requested normal base variant. Do not walk the
// preview/icon paths, inherit any override, or fall back to loose Borg files.
inline bool load_borg_practice_files(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                     std::string_view id,std::string_view variant,BorgPracticeFiles* out,std::string* error){
    using Assets=NativeGRedPracticeAssets;
    auto fail=[&](std::string why){if(error)*error=std::move(why);return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!package||!out)return fail("a uniquely discovered package is required");
    if(variant!="normal")return fail("practice requires the explicitly selected normal variant");
    if(!package->manifest.animation_map.empty())return fail("a separate animation map is not supported in practice");
    std::filesystem::path folder;BorgCatalogProfile catalogue;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error))return false;
    if(catalogue.default_variant!=variant||!catalogue.find(variant))return fail("the requested normal variant must be declared as the base");
    if(catalogue.details&&catalogue.details->references[unsigned(BorgDefinitionFacet::VariantOverrides)].kind!=
       BorgDefinitionReferenceKind::None)return fail("practice requires no variant overrides");
    BorgPracticeProfile profile;std::string rules;
    using borg_package_scan_detail::read_local;
    if(!read_local(folder,package->manifest.rules,kBorgPracticeProfileBytes,&rules)||
       !borg_read_normal_gred_practice_profile(rules,&profile)||profile.variant!=variant)
        return fail("the package does not declare qualified normal G-Red practice rules");
    profile.paths[Assets::Body]=package->manifest.model;profile.paths[Assets::Bank]=package->manifest.animations;
    BorgPracticeFiles next;next.identity=package->manifest.identity;next.variant=variant;
    for(unsigned i=0;i<Assets::Count;++i)
        if(!read_local(folder,profile.paths[i],Assets::sizes[i],&next.bytes[i])||next.bytes[i].size()!=Assets::sizes[i])
            return fail("unavailable or wrong-sized practice asset: "+std::string(Assets::roles[i]));
    if(!next.views().qualified())return fail("practice assets do not match the qualified normal G-Red content");
    *out=std::move(next);if(error)error->clear();return true;
}

struct BorgOwnedPracticeRequest {
    std::string root;
    AccountSaveKey key;
    u64 instance=0;
    bool operator==(const BorgOwnedPracticeRequest&)const=default;
};
struct BorgForcePracticeRequest {
    std::string root;
    AccountSaveKey key;
    unsigned force=0,slot=0; // Zero-based only after host argument validation.
    bool operator==(const BorgForcePracticeRequest&)const=default;
};
class BorgOwnedPractice {
public:
    const AccountSaveKey& save_key()const{return selected_->entry.key;}
    const OwnedPackageBorg& saved()const{return saved_;}
    const BorgPracticeFiles& files()const{return files_;}
    bool matches(const BorgOwnedPracticeRequest& request)const{
        const auto* bound=std::get_if<BorgOwnedPracticeRequest>(&request_);
        return bound&&*bound==request&&save_key()==request.key&&saved_.instance==request.instance;
    }
    bool matches(const BorgForcePracticeRequest& request)const{
        const auto* bound=std::get_if<BorgForcePracticeRequest>(&request_);
        return bound&&*bound==request&&save_key()==request.key&&retail_record_.has_value();
    }
    bool current()const{
        return record_.current()&&(!retail_record_||scoped_retail_force_record_current(*selected_->backend,*retail_record_));
    }

    static std::unique_ptr<BorgOwnedPractice> open(const BorgOwnedPracticeRequest& request,
                                                   const std::filesystem::path& packages_root,
                                                   const BorgPackageCatalog& packages,std::string* error){
        auto fail=[&](const char* why)->std::unique_ptr<BorgOwnedPractice>{if(error)*error=why;return {};};
        if(!request.instance)return fail("an explicit owned instance ID is required");
        auto selected=open_borg_selected_save(request.root,request.key.account,request.key.save,error);
        if(!selected)return {};
        auto next=std::unique_ptr<BorgOwnedPractice>(new BorgOwnedPractice(request,std::move(selected)));
        PackageBorgRoster roster;
        if(!next->load_roster(&roster,false,error))return {};
        const auto* owned=roster.find(request.instance);
        if(!owned)return fail("the requested instance is not owned in this save");
        if(!next->qualify(*owned,packages_root,packages,error))return {};
        return next;
    }
    static std::unique_ptr<BorgOwnedPractice> open_force_slot(const BorgForcePracticeRequest& request,
                                                              const std::filesystem::path& packages_root,
                                                              const BorgPackageCatalog& packages,const ForceCatalog* catalogue,
                                                              std::string* error){
        auto fail=[&](const char* why)->std::unique_ptr<BorgOwnedPractice>{if(error)*error=why;return {};};
        if(request.force>=kForceCount||request.slot>=kForceSlots)return fail("invalid Force or slot");
        auto selected=open_borg_selected_save(request.root,request.key.account,request.key.save,error);
        if(!selected)return {};
        auto next=std::unique_ptr<BorgOwnedPractice>(new BorgOwnedPractice(request,std::move(selected)));
        PackageBorgRoster roster;
        if(!next->load_roster(&roster,true,error))return {};
        ForceInventory retail;
        if(!read_scoped_retail_force_record(*next->selected_->backend,catalogue,&next->retail_record_.emplace(),&retail))
            return fail("retail Force record is unverified; supply its matching --retail-catalog");
        const auto placement=std::find_if(roster.placements.begin(),roster.placements.end(),[&](const auto& row){
            return row.force==request.force&&row.slot==request.slot;
        });
        const bool packaged=placement!=roster.placements.end();
        const bool legacy=retail.ready()&&retail.state()->forces[request.force].slots[request.slot]>=0;
        if(legacy)return fail(packaged?"the requested Force slot overlaps retail and package claims":
            "the requested Force slot is retail-only; package practice cannot use it");
        if(!packaged)return fail("the requested Force slot is empty");
        // No admission of the whole Force or other slots. Qualify only the
        // instance selected by this exact position in the same saved roster.
        const auto* owned=roster.find(placement->instance);
        if(!owned)return fail("the requested Force slot has no owned package instance");
        if(!next->qualify(*owned,packages_root,packages,error))return {};
        return next;
    }
private:
    using Request=std::variant<BorgOwnedPracticeRequest,BorgForcePracticeRequest>;
    explicit BorgOwnedPractice(Request request,std::unique_ptr<BorgSelectedSave> selected)
        :request_(std::move(request)),selected_(std::move(selected)),record_(*selected_->backend){}
    bool load_roster(PackageBorgRoster* out,bool allow_missing,std::string* error){
        const auto result=record_.load(out);
        if(result==PackageBorgLoad::Loaded||(allow_missing&&result==PackageBorgLoad::Missing))return true;
        if(error)*error=result==PackageBorgLoad::Missing?"the selected save has no package roster":
            "the selected package roster is unavailable or invalid";
        return false;
    }
    bool qualify(const OwnedPackageBorg& owned,const std::filesystem::path& root,const BorgPackageCatalog& packages,std::string* error){
        auto fail=[&](const char* why){if(error)*error=why;return false;};
        if(owned.disabled)return fail("the requested owned copy is disabled");
        if(!owned.variant)return fail("the requested owned copy has no exact saved variant");
        if(!load_borg_practice_files(root,packages,owned.identity.id,owned.variant->id,&files_,error))return false;
        if(!current())return fail("the selected save changed during practice preparation; reopen it");
        saved_=owned;if(error)error->clear();return true;
    }
    // No write interface is exposed to the practice consumer. The exact
    // roster record is checked through the selected account/save lease.
    const Request request_;
    std::unique_ptr<BorgSelectedSave> selected_;
    mutable PackageBorgSaveStore record_;
    std::optional<std::vector<u8>> retail_record_; // Force launches additionally hold the exact checked slot-0 bytes.
    OwnedPackageBorg saved_;
    BorgPracticeFiles files_;
};
}
