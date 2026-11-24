#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_library.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg library: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using State=BorgLibraryState;
    BorgPackageCatalog packages;
    packages.status=BorgPackageScanStatus::Complete;
    BorgPackageManifest live;
    check(borg_package_read_manifest("GFBORG 2\nid=lab:ant\nname=Current Ant\ncatalog=card.txt\n"
        "model=body.hsd\npreview=preview.txt\nanimations=body.motions\ngameplay=gameplay.txt\n",&live),"parse live package manifest");
    auto other=live;
    other.identity={"lab:bad","Bad Card"};
    auto missing=live;
    missing.identity={"lab:missing","Missing Card"};
    auto declared_only=live;
    declared_only.identity={"lab:preview","Preview Only"};declared_only.gameplay.clear();
    auto old=live;
    old.identity={"lab:old","Legacy Borg"};old.catalog.clear();
    packages.discovered={{"random folder",live},{"bad",other},{"missing",missing},{"old",old},{"preview",declared_only}};
    packages.issues={{"c1","lab:conflict",BorgPackageIssueKind::DuplicateId},{"c2","lab:conflict",BorgPackageIssueKind::DuplicateId},
        {"i","lab:incomplete",BorgPackageIssueKind::MissingAsset,"Incomplete Ant"},{"malformed",{},BorgPackageIssueKind::InvalidManifest}};
    BorgCatalogSnapshot ant;
    ant.package=packages.discovered[0];
    check(borg_catalog_read_profile("GFBORG-CATALOG 1\ndescription=Test.\ntribe=lab:machine\ndefault-variant=normal\n"
        "variant=silver|Silver New|19|unresolved\nvariant=normal|Base New|0|placeholder\n"
        "variant=bronze|Bronze New|21|missing.tpl\n",&ant.profile),"parse exact variants");
    ant.preview_bindings={{"normal","preview.txt","body.hsd","body.motions",""},{"silver","silver.txt","body.hsd","body.motions",""}};
    auto preview=ant;
    preview.package=packages.discovered[4];
    const BorgCatalogSnapshot snapshots[]{ant,preview};
    const BorgLibraryCatalogIssue issues[]{{"lab:bad",State::CatalogInvalid},{"lab:missing",State::CatalogMissing}};
    const BorgLibrary library(packages,snapshots,issues);
    check(library.rows().size()==11&&library.rows().front().key==BorgLibraryKey{"lab:ant","silver"},
        "one row per parsed variant and one per known diagnostic package");
    const auto* base=library.find("lab:ant","normal");
    const auto* silver=library.find("lab:ant","silver");
    const auto* bronze=library.find("lab:ant","bronze");
    check(base&&base->preview_declared&&base->gameplay_declared&&base->declared_cost==0&&base->tribe=="lab:machine"&&
        silver&&silver->preview_declared&&!silver->gameplay_declared&&silver->variant_label=="Silver New"&&silver->declared_cost==19&&
        bronze&&!bronze->preview_declared&&!bronze->gameplay_declared,"independent exact declarations and zero cost");
    check(library.find("lab:preview","normal")->preview_declared&&!library.find("lab:preview","normal")->gameplay_declared,
        "preview without gameplay declaration");
    check(!library.find("lab:ant","")&&!library.find("lab:ant","Base New")&&!library.find("lab:ant","Normal")&&
        !library.find("random folder","normal")&&!library.find("lab:bad","normal"),"no title, default, case, or directory substitution");
    const std::pair<std::string_view,State> expected[]{{"lab:bad",State::CatalogInvalid},{"lab:missing",State::CatalogMissing},
        {"lab:old",State::CatalogNotDeclared},{"lab:conflict",State::PackageConflict},{"lab:incomplete",State::PackageIncomplete}};
    for(const auto& [id,state]:expected){
        const auto row=std::find_if(library.rows().begin(),library.rows().end(),[&](const auto& r){return r.identity.id==id;});
        check(row!=library.rows().end()&&!row->key&&!row->declared_cost&&row->state==state,"keyless package diagnostics");
    }

    const PackageBorgRoster roster{{
        {1,{"lab:ant","Old Ant"},false,SavedPackageVariant{"silver","Silver Old"}},
        {2,{"lab:ant","Another Ant"},true,SavedPackageVariant{"normal","Base Old"}},
        {3,{"lab:ant","Removed Variant"},false,SavedPackageVariant{"gold","Gold Old"}},
        {4,{"lab:ant","Old Record"}},
        {5,{"lab:absent","Missing Ant"},true,SavedPackageVariant{"normal","Base Old"}},
        {6,{"lab:conflict","Conflicting Ant"},false,SavedPackageVariant{"normal","Base Old"}},
    },{}};
    const auto owned=library.project_owned(roster);
    check(owned&&owned->size()==roster.owned.size()&&(*owned)[0].saved==roster.owned[0]&&
        (*owned)[0].installed->key==BorgLibraryKey{"lab:ant","silver"}&&(*owned)[1].saved.disabled&&
        (*owned)[1].installed->key==BorgLibraryKey{"lab:ant","normal"},"owned instances preserve names, labels and disabled flags");
    check(!(*owned)[2].installed&&(*owned)[2].state==State::VariantMissing&&(*owned)[3].state==State::VariantUnspecified&&
        (*owned)[4].state==State::PackageMissing&&(*owned)[4].saved.disabled&&(*owned)[5].state==State::PackageConflict,
        "owned unavailable states never resolve the base variant");
    auto invalid=roster;invalid.owned[1].instance=1;
    check(!library.project_owned(invalid),"invalid ownership snapshot refused");
    auto malformed=ant;malformed.profile.variants.push_back(malformed.profile.variants.front());
    auto stale=ant;stale.package.folder="old folder";
    auto wrong=ant;wrong.package.manifest.identity.name="Old manifest";
    for(const auto* snapshot:{&malformed,&stale,&wrong})
        check(!BorgLibrary(packages,{snapshot,1}).find("lab:ant","normal"),"invalid or stale metadata cannot publish exact keys");
    const BorgCatalogSnapshot duplicate[]{ant,ant};
    check(!BorgLibrary(packages,duplicate).find("lab:ant","normal"),"duplicate catalogue inputs cannot publish an exact key");
    auto undeclared=ant;undeclared.package=packages.discovered[3];
    check(!BorgLibrary(packages,{&undeclared,1}).find("lab:old","normal"),"undeclared catalogue cannot provide an exact row");
    packages.issues.push_back({"extra","lab:ant",BorgPackageIssueKind::DuplicateId});
    check(!BorgLibrary(packages,snapshots).find("lab:ant","normal"),"catalogue data cannot hide a scan conflict");
    packages.status=BorgPackageScanStatus::EntryBudget;
    const BorgLibrary failed(packages,snapshots);
    const auto failed_owned=failed.project_owned(roster);
    check(failed.rows().empty()&&failed_owned&&failed_owned->front().state==State::ScanUnavailable&&
        failed_owned->front().saved==roster.owned.front(),"failed scan hides all partial installed keys but preserves ownership");
    std::puts("Borg library: exact keys, independent declarations, diagnostics and saved-name overlay PASS");
}
