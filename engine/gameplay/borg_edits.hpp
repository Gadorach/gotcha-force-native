#pragma once
#include <array>
#include "engine/core/types.hpp"
namespace gf {
enum class BorgEditField {Variant,Cost,Level,HP,DEF,SHT,ATK,SPD,BAmmo,XAmmo};
// Explicit native extension, never stored in retail reserved bytes. -1 means
// authored/default value. Ratings are the displayed0..10 domain; ammunition is
// a native override for a future battle consumer, not a claimed source default.
struct BorgEdits {
    bool modified=false;std::array<s16,8> values={-1,-1,-1,-1,-1,-1,-1,-1}; // cost,hp,DEF,SHT,ATK,SPD,B,X
    bool operator==(const BorgEdits&)const=default;
    bool valid()const{
        for(unsigned i=0;i<values.size();++i){const auto v=values[i];if(v< -1||(i==1&&v==0)||(i>=2&&i<=5&&v>10))return false;if(!modified&&v!=-1)return false;}
        return true;
    }
};
} // namespace gf
