#pragma once
#include <array>
#include <charconv>
#include <initializer_list>
#include <optional>
#include "engine/assets/borg_package.hpp"

namespace gf {
enum class BorgCombatRole:u8 {Unresolved,ShortRange,LongRange,AllRound,Speed,Support};
enum class BorgRarityGrade:u8 {Unresolved,D,C,B,A,S};
enum class BorgCrystalRule:u8 {Unresolved,Count,FusionOnly,NotApplicable};
enum class BorgAlternateAvailability:u8 {Unresolved,No,Yes};
enum class BorgLevelSchedule:u8 {Unresolved,Fast,Normal,Slow};
enum class BorgExperienceAvailability:u8 {Unresolved,EarnsXP,NoXP};
enum class BorgDisplayJump:u8 {Unresolved,None,Boost,Air};
enum class BorgDefinitionFacet:u8 {Moves,Traits,Forms,Fusions,Acquisition,Effects,VariantOverrides,Spawn,Controllers,Count};
enum class BorgDefinitionReferenceKind:u8 {Unresolved,None,LocalFile};
struct BorgDefinitionReference {
    BorgDefinitionReferenceKind kind=BorgDefinitionReferenceKind::Unresolved;
    std::string file;
};

// A catalogue/presentation contract, not movement or combat admission. A
// collection number and source reference code do not identify runtime models.
struct BorgCatalogDetails {
    std::optional<u16> collection_number;
    std::string source_reference_code,observed_speed_note;
    BorgCombatRole role=BorgCombatRole::Unresolved;
    BorgRarityGrade rarity_grade=BorgRarityGrade::Unresolved;
    BorgCrystalRule crystal_rule=BorgCrystalRule::Unresolved;
    u8 crystal_count=0;
    BorgAlternateAvailability alternate=BorgAlternateAvailability::Unresolved;
    BorgLevelSchedule level_schedule=BorgLevelSchedule::Unresolved;
    BorgExperienceAvailability experience=BorgExperienceAvailability::Unresolved;
    BorgDisplayJump display_jump=BorgDisplayJump::Unresolved;
    std::optional<u8> display_air_jumps;
    std::array<std::optional<u16>,2> hp{}; // level 1, level 10; no curve implied
    std::array<std::optional<u8>,4> ratings{}; // defence, shot, attack, speed
    std::array<BorgDefinitionReference,unsigned(BorgDefinitionFacet::Count)> references;
};

namespace borg_catalog_detail {
inline bool unsigned_field(std::string_view text,u32 maximum,u32* out){
    if(text.empty()||text.size()>10)return false;
    const auto end=text.data()+text.size();
    const auto result=std::from_chars(text.data(),end,*out);
    return result.ec==std::errc{}&&result.ptr==end&&*out<=maximum;
}
template<class T> bool optional_number(std::string_view text,u32 maximum,u32 minimum,std::optional<T>* out){
    if(text=="unresolved"){out->reset();return true;}
    u32 value=0;
    if(!unsigned_field(text,maximum,&value)||value<minimum)return false;
    *out=T(value);return true;
}
template<class T,std::size_t N> bool number_list(std::string_view text,u32 maximum,u32 minimum,std::array<std::optional<T>,N>* out){
    for(unsigned i=0;i<N;++i){
        const auto pipe=text.find('|');
        if((pipe==std::string_view::npos)!=(i+1==N)||!optional_number(text.substr(0,pipe),maximum,minimum,&(*out)[i]))return false;
        if(pipe!=std::string_view::npos)text.remove_prefix(pipe+1);
    }
    return true;
}
template<class T> bool label(std::string_view text,std::initializer_list<std::string_view> choices,T* out){
    unsigned index=0;
    for(auto choice:choices){
        if(text==choice){*out=T(index);return true;}
        ++index;
    }
    return false;
}
inline bool source_code(std::string_view value){
    if(value.size()!=4)return false;
    for(char c:value)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;
    return true;
}
class DetailsReader {
public:
    bool add(std::string_view key,std::string_view value){
        if(key=="source-reference-code"){
            if(!details_.source_reference_code.empty()||!source_code(value))return false;
            details_.source_reference_code=value;return true;
        }
        if(key=="observed-speed-note"){
            if(!details_.observed_speed_note.empty()||!display_name_valid(value,96))return false;
            details_.observed_speed_note=value;return true;
        }
        const auto field=std::find(fields_.begin(),fields_.end(),key);
        if(field==fields_.end())return false;
        const auto i=unsigned(field-fields_.begin());
        if(seen_[i])return false;
        seen_[i]=true;
        switch(i){
        case 0:return optional_number(value,65535,1,&details_.collection_number);
        case 1:return label(value,{"unresolved","short-range","long-range","all-round","speed","support"},&details_.role);
        case 2:return label(value,{"unresolved","D","C","B","A","S"},&details_.rarity_grade);
        case 3:{
            if(value=="unresolved")return true;
            if(value=="fusion-only"){details_.crystal_rule=BorgCrystalRule::FusionOnly;return true;}
            if(value=="none"){details_.crystal_rule=BorgCrystalRule::NotApplicable;return true;}
            u32 number=0;
            if(!unsigned_field(value,255,&number)||!number)return false;
            details_.crystal_rule=BorgCrystalRule::Count;details_.crystal_count=u8(number);return true;
        }
        case 4:return label(value,{"unresolved","no","yes"},&details_.alternate);
        case 5:return label(value,{"unresolved","fast","normal","slow"},&details_.level_schedule);
        case 6:return label(value,{"unresolved","earns-xp","no-xp"},&details_.experience);
        case 7:
            if(value.starts_with("air:")){
                details_.display_jump=BorgDisplayJump::Air;
                return optional_number(value.substr(4),255,1,&details_.display_air_jumps);
            }
            return label(value,{"unresolved","none","boost"},&details_.display_jump);
        case 8:return number_list(value,65535,1,&details_.hp);
        case 9:return number_list(value,10,0,&details_.ratings);
        default:{
            auto& ref=details_.references[i-10];
            if(value=="unresolved")return true;
            if(value=="none"){ref.kind=BorgDefinitionReferenceKind::None;return true;}
            if(!borg_package_asset_path_valid(value))return false;
            ref.kind=BorgDefinitionReferenceKind::LocalFile;ref.file=value;return true;
        }
        }
    }
    bool finish(std::optional<BorgCatalogDetails>* out){
        // Spawn and controllers are additive declarations. Older version 2
        // profiles continue to load with missing optional facets unresolved.
        if(!std::all_of(seen_.begin(),seen_.begin()+10+unsigned(BorgDefinitionFacet::Spawn),[](bool seen){return seen;}))return false;
        *out=std::move(details_);return true;
    }
private:
    static constexpr std::array<std::string_view,10+unsigned(BorgDefinitionFacet::Count)> fields_{
        "collection-number","combat-role","rarity-grade","crystals","alternate","level-schedule","experience",
        "display-jump","hp","ratings","moves","traits","forms","fusions","acquisition","effects","variant-overrides","spawn","controllers"
    };
    std::array<bool,fields_.size()> seen_{};
    BorgCatalogDetails details_;
};
}
}
