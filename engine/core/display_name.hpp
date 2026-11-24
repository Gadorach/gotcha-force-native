#pragma once
#include "engine/core/utf8.hpp"

namespace gf {
inline bool display_name_valid(std::string_view name,std::size_t byte_limit){
    if(name.empty()||name.size()>byte_limit)return false;
    while(!name.empty()){
        const auto scalar=utf8_front(name);
        if(!scalar.bytes||!unicode_display_scalar(scalar.value))return false;
        name.remove_prefix(scalar.bytes);
    }return true;
}
}
