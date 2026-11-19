#include <cstdio>
#include <cstdlib>
#include "engine/assets/borg_catalog_profile.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg catalogue details: %s\n",why);std::abort();}
}
std::string replace(std::string text,std::string_view old,std::string_view next){
    const auto at=text.find(old);check(at!=std::string::npos,"replacement fixture");
    text.replace(at,old.size(),next);
    return text;
}
}
int main(){
    using namespace gf;
    const std::string base="description=A training unit.\ntribe=example:machine\ndefault-variant=standard\n"
        "variant=standard|Standard|240|placeholder\n";
    const auto full="GFBORG-CATALOG 2\n"+base+
        "collection-number=33\nsource-reference-code=00FA\nobserved-speed-note=No measured units documented\n"
        "combat-role=all-round\nrarity-grade=S\ncrystals=fusion-only\nalternate=no\n"
        "level-schedule=normal\nexperience=no-xp\ndisplay-jump=air:unresolved\n"
        "hp=200|unresolved\nratings=3|0|unresolved|5\n"
        "moves=actions/moves.txt\ntraits=unresolved\nforms=none\nfusions=relationships/fusion.txt\n"
        "acquisition=unresolved\neffects=none\nvariant-overrides=unresolved\n";
    BorgCatalogProfile profile;
    check(borg_catalog_read_profile("GFBORG-CATALOG 1\n"+base,&profile)&&!profile.details,"version one remains readable");
    check(borg_catalog_read_profile(full,&profile)&&profile.details&&profile.variants[0].id=="standard"&&
        profile.variants[0].force_cost==240,"version two extends, not replaces, variant rows");
    const auto& d=*profile.details;
    check(d.collection_number==33&&d.source_reference_code=="00FA"&&!d.observed_speed_note.empty()&&
        d.role==BorgCombatRole::AllRound&&d.rarity_grade==BorgRarityGrade::S&&
        d.alternate==BorgAlternateAvailability::No,"number, source reference code, combat role and grade are separate");
    check(d.level_schedule==BorgLevelSchedule::Normal&&d.experience==BorgExperienceAvailability::NoXP&&
        d.crystal_rule==BorgCrystalRule::FusionOnly&&!d.crystal_count,"progression schedule, XP and fusion-only acquisition stay separate");
    check(d.display_jump==BorgDisplayJump::Air&&!d.display_air_jumps&&d.hp[0]==200&&!d.hp[1]&&
        d.ratings[0]==3&&d.ratings[1]==0&&!d.ratings[2]&&d.ratings[3]==5,"partial values do not fill unknowns or infer a curve");
    const auto facet=[&](BorgDefinitionFacet f)->const auto&{return d.references[unsigned(f)];};
    check(facet(BorgDefinitionFacet::Moves).kind==BorgDefinitionReferenceKind::LocalFile&&
        facet(BorgDefinitionFacet::Moves).file=="actions/moves.txt"&&
        facet(BorgDefinitionFacet::Traits).kind==BorgDefinitionReferenceKind::Unresolved&&
        facet(BorgDefinitionFacet::Forms).kind==BorgDefinitionReferenceKind::None&&
        facet(BorgDefinitionFacet::Spawn).kind==BorgDefinitionReferenceKind::Unresolved,
        "facet files, unresolved and deliberate absence stay distinct");
    for(const auto& bad:{
        replace(full,"GFBORG-CATALOG 2","GFBORG-CATALOG 1"),full+"hp=100|100\n",
        replace(full,"collection-number=33","collection-number=0"),
        replace(full,"source-reference-code=00FA","source-reference-code=not-a-code"),
        replace(full,"combat-role=all-round","combat-role=tribe"),replace(full,"rarity-grade=S","rarity-grade=clear"),
        replace(full,"crystals=fusion-only","crystals=0"),replace(full,"experience=no-xp\n",""),
        replace(full,"display-jump=air:unresolved","display-jump=none:3"),
        replace(full,"hp=200|unresolved","hp=200"),
        replace(full,"ratings=3|0|unresolved|5","ratings=11|0|unresolved|5"),
        replace(full,"moves=actions/moves.txt","moves=../moves.txt"),replace(full,"forms=none\n","")
    }){
        check(!borg_catalog_read_profile(bad,&profile)&&profile.details->collection_number==33,
            "malformed, duplicate and incomplete profile cannot replace the previous result");
    }
    const auto known=replace(replace(replace(full,"collection-number=33","collection-number=unresolved"),
        "display-jump=air:unresolved","display-jump=air:3"),"crystals=fusion-only","crystals=4");
    check(borg_catalog_read_profile(known,&profile)&&!profile.details->collection_number&&profile.details->display_air_jumps==3&&
        profile.details->crystal_count==4&&profile.details->crystal_rule==BorgCrystalRule::Count,
        "known counts and intentionally unresolved collection number round-trip");
    check(borg_catalog_read_profile(known+"spawn=gameplay/spawn.txt\n",&profile)&&
        profile.details->references[unsigned(BorgDefinitionFacet::Spawn)].file=="gameplay/spawn.txt",
        "an additive spawn sidecar does not change older catalogue requirements");
    std::puts("Borg catalogue details: versioned identity, variants, stats and unresolved facets PASS");
}
