#include <cstdio>
#include <cstdlib>
#include "engine/assets/borg_variant_profile.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg variant profile: %s\n",why);std::abort();}
}
std::string replace(std::string text,std::string_view old,std::string_view next){
    const auto at=text.find(old);check(at!=std::string::npos,"test replacement field");
    text.replace(at,old.size(),next);return text;
}
}
int main(){
    using namespace gf;
    const std::string profile=R"(GFBORG-VARIANTS 1
variant=bronze
model=inherit
animations=inherit
preview=visual/bronze.preview
textures=none
materials=unresolved
stats=stats/bronze.txt
movement=inherit
moves=inherit
collision=unresolved
effects=none
acquisition=unresolved
hp-level-offset=-2
ammo-level-offset=+6
variant=plain
model=visual/plain.hsd
animations=unresolved
preview=none
textures=inherit
materials=inherit
stats=unresolved
movement=inherit
moves=inherit
collision=inherit
effects=inherit
acquisition=inherit
hp-level-offset=0
ammo-level-offset=unresolved
)";
    BorgVariantProfile parsed;
    check(borg_read_variant_profile(profile,&parsed)&&parsed.variants.size()==2,"distinct catalogue variant overrides");
    const auto* bronze=parsed.find("bronze");const auto* plain=parsed.find("plain");
    check(bronze&&plain&&bronze->asset(BorgVariantAssetRole::Model).kind==BorgVariantAssetKind::Inherit&&
        bronze->asset(BorgVariantAssetRole::Preview).file=="visual/bronze.preview"&&
        bronze->asset(BorgVariantAssetRole::Textures).kind==BorgVariantAssetKind::None&&
        bronze->asset(BorgVariantAssetRole::Materials).kind==BorgVariantAssetKind::Unresolved,
        "assets preserve explicit inheritance, absence and unknowns");
    check(bronze->hp.kind==BorgVariantLevelKind::Offset&&bronze->hp.value==-2&&bronze->ammo.value==6&&
        plain->hp.kind==BorgVariantLevelKind::Offset&&plain->hp.value==0&&plain->ammo.kind==BorgVariantLevelKind::Unresolved,
        "health and ammo changes are independent, including explicit zero versus unresolved");
    for(const auto& bad:{
        replace(profile,"variant=plain","variant=bronze"),
        replace(profile,"variant=plain","variant=inherit"),
        replace(profile,"preview=visual/bronze.preview","preview=../bronze.preview"),
        replace(profile,"model=inherit","model=inherit\nmodel=none"),
        replace(profile,"hp-level-offset=-2","hp-level-offset=-256"),
        replace(profile,"ammo-level-offset=+6","ammo-level-offset=none"),
        replace(profile,"ammo-level-offset=+6","ammo-level-offset=+-6"),
        replace(profile,"collision=unresolved\n",""),
        profile+std::string(kBorgVariantProfileBytes,'\n')
    }){
        check(!borg_read_variant_profile(bad,&parsed)&&parsed.variants.size()==2&&bronze->hp.value==-2,
            "bad IDs, unsafe paths, duplicate fields and invalid offsets refuse atomically");
    }
    check(borg_read_variant_profile(replace(profile,"hp-level-offset=-2","hp-level-offset=inherit"),&parsed)&&
        parsed.find("bronze")->hp.kind==BorgVariantLevelKind::Inherit,"level rules may explicitly inherit");
    std::puts("Borg variants: explicit appearance, preview and independent level offsets PASS");
}
