#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_cheat_picker.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"portable cheat picker: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using Result=BorgCheatPickerResult;
    BorgPackageCatalog packages;packages.status=BorgPackageScanStatus::Complete;
    const auto entry=[](std::string folder,std::string id,std::string name){
        BorgPackageEntry package;package.folder=std::move(folder);
        package.manifest.identity={std::move(id),std::move(name)};package.manifest.catalog="catalog.txt";
        return package;
    };
    packages.discovered={entry("ant","lab:ant","Ant"),entry("missing","lab:missing","Missing Catalogue")};
    BorgCatalogSnapshot ant;ant.package=packages.discovered[0];
    check(borg_catalog_read_profile("GFBORG-CATALOG 1\ndescription=Ant\ntribe=none\ndefault-variant=normal\n"
        "variant=normal|Normal|22|placeholder\nvariant=silver|Silver|0|unresolved\n",&ant.profile),"validated metadata");
    const BorgLibrary library(packages,{&ant,1});
    const BorgLibraryKey silver{"lab:ant","silver"};
    BorgCheatPicker none(library);
    check(none.library().rows().size()==3&&none.select(silver)==Result::PermissionDenied&&!none.offer(),
        "browsing a shared catalogue alone cannot authorize an offer");
    BorgCheatPickerAuthority authority;
    BorgCheatPicker before_authorization(library,authority.permit());
    authority.set_authorized(true);
    check(before_authorization.select(silver)==Result::PermissionDenied,"an old denied session does not acquire permission");
    BorgCheatPicker permitted(library,authority.permit());
    check(permitted.select({"lab:ant",{}})==Result::ExactVariantUnavailable&&
        permitted.select({"lab:ant","gold"})==Result::ExactVariantUnavailable&&
        permitted.select({"lab:missing","normal"})==Result::ExactVariantUnavailable&&!permitted.offer(),
        "no default, substitute, or synthetic selection for diagnostics");
    check(permitted.select(silver)==Result::OfferReady&&permitted.offer()->key==silver&&
        permitted.offer()->variant_label=="Silver"&&permitted.offer()->declared_cost==0&&!permitted.offer()->gameplay_declared,
        "an offer is an exact metadata row, including catalogue-only zero-cost variants, not admission or ownership");
    permitted.cancel();
    check(!permitted.offer()&&permitted.select(silver)==Result::OfferReady,"cancel clears the offer");
    authority.set_authorized(false);
    check(!permitted.offer()&&permitted.select(silver)==Result::PermissionDenied,"revocation invalidates an existing offer");
    authority.set_authorized(true);
    check(permitted.select(silver)==Result::PermissionDenied,"reenabling does not revive a previous permit");
    BorgCheatPicker new_session(library,authority.permit());
    check(new_session.select(silver)==Result::OfferReady&&new_session.select({"lab:missing","normal"})==Result::ExactVariantUnavailable&&
        !new_session.offer(),"an invalid selection never retains the previous selection as an offer");
    BorgCheatPickerAuthority::Permit ended;
    {
        BorgCheatPickerAuthority short_session;
        short_session.set_authorized(true);ended=short_session.permit();
    }
    BorgCheatPicker expired(library,ended);
    check(expired.select(silver)==Result::PermissionDenied,"permission does not outlive the trusted session owner");
    std::puts("Portable cheat picker: exact installed metadata and revocable offer-only permission PASS");
}
