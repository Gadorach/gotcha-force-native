#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_package_force_view.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg Force view: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    const PackageBorgRoster roster{{
        {1,{"lab:ant","Clockwork Ant"},false,SavedPackageVariant{"silver","Silver Ant"}},
        {2,{"lab:ant","Older Ant"}},
        {3,{"lab:ant","Bronze Ant"},false,SavedPackageVariant{"bronze","Bronze Ant"}},
        {4,{"lab:ant","Disabled Ant"},true,SavedPackageVariant{"silver","Silver Ant"}},
        {5,{"lab:ghost","Ghost"},false,SavedPackageVariant{"normal","Normal"}},
        {6,{"lab:crane","Crane"},false,SavedPackageVariant{"normal","Normal"}},
        {7,{"lab:bee","Bee"},false,SavedPackageVariant{"normal","Normal"}},
        {8,{"lab:other","Other"},false,SavedPackageVariant{"normal","Normal"}},
        {9,{"lab:ant","Another Ant"},false,SavedPackageVariant{"normal","Normal"}},
    },{{1,0,0},{2,0,1},{3,0,2},{4,0,3},{5,0,4},{6,0,5},{7,0,6},{8,0,7},{1,1,0},{9,1,1}}};
    BorgPackageCatalog catalog;catalog.status=BorgPackageScanStatus::Complete;
    catalog.discovered={{"ant",{{"lab:ant","Installed Ant"},{},{},{},{}}},
        {"other",{{"lab:other","Other"},{},{},{},{}}}};
    catalog.issues={{"incomplete","lab:crane",BorgPackageIssueKind::MissingAsset},
        {"duplicate","lab:bee",BorgPackageIssueKind::DuplicateId}};
    BorgCatalogSnapshot ant;ant.package=catalog.discovered[0];
    check(borg_catalog_read_profile("GFBORG-CATALOG 1\ndescription=Test.\ntribe=none\ndefault-variant=normal\n"
        "variant=normal|Normal|200|unresolved\nvariant=silver|Current Silver|225|unresolved\n",&ant.profile),
        "metadata only, art unavailable");
    const std::span<const BorgCatalogSnapshot> metadata(&ant,1);
    PackageForceView view;
    using Status=PackageForceSlotStatus;
    check(package_borg_force_view(roster,catalog,metadata,0,&view)&&view.slots.size()==8&&view.selectable==1&&
        !view.declared_package_cost,"blocked positions cannot produce a misleading total");
    check(view.slots[0].status==Status::MetadataReady&&view.slots[0].declared_cost==225&&
        view.slots[0].saved.variant->label=="Silver Ant"&&view.slots[0].saved.identity.name=="Clockwork Ant",
        "ready metadata uses the precise variant cost and retains saved labels");
    check(view.slots[1].status==Status::VariantUnspecified&&view.slots[2].status==Status::MissingVariant&&
        view.slots[3].status==Status::MetadataReady&&view.slots[3].saved.disabled&&!view.slots[3].selectable_metadata(),
        "legacy unspecified, missing and disabled exact variants are distinct");
    check(view.slots[4].status==Status::MissingPackage&&view.slots[5].status==Status::IncompletePackage&&
        view.slots[6].status==Status::Conflict&&view.slots[7].status==Status::CatalogueUnavailable,
        "package absence, incompleteness and unverified metadata stay distinct");
    check(package_borg_force_view(roster,catalog,metadata,1,&view)&&view.slots.size()==2&&view.selectable==2&&
        view.declared_package_cost==425&&view.slots[0].saved.instance==1&&view.slots[1].saved.instance==9,
        "complete package projection totals exact variants without deduplicating owned copies");
    auto failed=catalog;failed.status=BorgPackageScanStatus::Unavailable;
    check(package_borg_force_view(roster,failed,metadata,1,&view)&&view.selectable==0&&!view.declared_package_cost&&
        view.slots[0].status==Status::ScanUnavailable,"failed scan does not masquerade as lost ownership");
    check(!package_borg_force_view(roster,catalog,metadata,kForceCount,&view)&&view.force==1,
        "invalid Force index leaves previous view intact");
    std::puts("Borg Force view: exact saved variants, blocked slots and complete package-only cost PASS");
}
