#pragma once
#include "engine/assets/borg_fusion_profile.hpp"
#include "engine/assets/borg_form_profile.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgFusionInspection {
    BorgIdentity identity;
    BorgFusionProfile profile;
    unsigned unresolved_parties=0;
};

inline bool load_borg_fusion_inspection(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                         std::string_view id,BorgFusionInspection* out,std::string* error){
    auto fail=[&](std::string why){if(error)*error=std::move(why);return false;};
    const auto* owner=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!owner)return fail("a uniquely discovered Borg ID is required");
    BorgCatalogProfile catalogue;
    std::filesystem::path folder;
    if(!borg_catalog_detail::read_profile(root,*owner,&folder,&catalogue,error))return false;
    if(!catalogue.details)return fail("a version 2 catalogue profile is required for battle fusion");
    const auto& reference=catalogue.details->references[unsigned(BorgDefinitionFacet::Fusions)];
    if(reference.kind==BorgDefinitionReferenceKind::None)return fail("package declares no battle fusions");
    if(reference.kind!=BorgDefinitionReferenceKind::LocalFile)return fail("battle fusions are unresolved");
    BorgFusionInspection next;next.identity=owner->manifest.identity;
    std::string bytes;
    using borg_package_scan_detail::read_local;
    if(!read_local(folder,reference.file,kBorgFusionProfileBytes,&bytes)||!borg_read_fusion_profile(bytes,&next.profile))
        return fail("missing or invalid battle fusion definition file");
    for(const auto& rule:next.profile.fusions){
        if(rule.first.package!=id&&rule.second.package!=id&&rule.result.package!=id)
            return fail("declaring Borg is not a participant or result of "+rule.id);
        for(const auto* role:{&rule.first,&rule.second,&rule.result}){
            if(role->package=="unresolved"){++next.unresolved_parties;continue;}
            const auto* participant=scanned.find(role->package);
            if(!participant)return fail("missing or conflicting fusion package "+role->package);
            std::filesystem::path role_folder;std::string detail;
            const bool has_form=borg_fusion_detail::metadata_id(role->form);
            const bool has_variant=borg_fusion_detail::metadata_id(role->variant);
            if(!has_form&&!has_variant){
                if(!borg_catalog_detail::current_folder(root,*participant,&role_folder,&detail))
                    return fail("fusion package "+role->package+": "+detail);
                continue;
            }
            BorgCatalogProfile role_catalogue;
            if(!borg_catalog_detail::read_profile(root,*participant,&role_folder,&role_catalogue,&detail))
                return fail("fusion package "+role->package+": "+detail);
            if(has_variant&&!role_catalogue.find(role->variant))
                return fail("undeclared fusion variant "+role->variant+" on "+role->package);
            if(has_form){
                if(!role_catalogue.details)return fail("fusion form profile not declared on "+role->package);
                const auto& forms_file=role_catalogue.details->references[unsigned(BorgDefinitionFacet::Forms)];
                BorgFormProfile forms;
                if(forms_file.kind!=BorgDefinitionReferenceKind::LocalFile||
                    !read_local(role_folder,forms_file.file,kBorgFormProfileBytes,&bytes)||!borg_read_form_profile(bytes,&forms))
                    return fail("fusion form profile unavailable on "+role->package);
                if(!forms.find(role->form))return fail("undeclared fusion form "+role->form+" on "+role->package);
            }
        }
    }
    *out=std::move(next);if(error)error->clear();return true;
}
}
