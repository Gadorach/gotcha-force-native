#pragma once
// Small documented INI subset for editable OWNED battle-test tuning. No game
// constants inferred here. Distribution manifests remain standard JSON offline.
#include <charconv>
#include <string>
#include <string_view>
#include "engine/gameplay/battle_test.hpp"
namespace gf {
inline bool battle_test_config_load(ByteView file,BattleTestConfig* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle test config: ")+why;return false;};
    if(!out||!file.data||!file.size||file.size>8192)return fail("size/destination");
    const std::string_view text{reinterpret_cast<const char*>(file.data),file.size};
    for(char c:text)if((c<32&&c!='\n'&&c!='\r'&&c!='\t')||static_cast<unsigned char>(c)>126)return fail("non-ASCII/control input");
    auto trim=[](std::string_view s){while(!s.empty()&&(s.front()==' '||s.front()=='\t'||s.front()=='\r'))s.remove_prefix(1);
        while(!s.empty()&&(s.back()==' '||s.back()=='\t'||s.back()=='\r'))s.remove_suffix(1);
        return s;};
    constexpr std::array<std::string_view,27> names={"tick_rate","shot_interval","projectile_life","target_health","arena_half","actor_radius","target_radius","projectile_radius","move_per_tick","shot_per_tick","spawn_x","spawn_z","target0_x","target0_z","target1_x","target1_z","target2_x","target2_z","format","diagnostic_rules","proxy_art","camera_distance","camera_height","camera_fov","target3_x","target3_z","ally_mask"};
    std::array<bool,names.size()> seen{};BattleTestConfig next;bool section=false;
    for(std::size_t at=0;at<text.size();){const auto end=text.find('\n',at);auto line=trim(text.substr(at,end==text.npos?text.size()-at:end-at));at=end==text.npos?text.size():end+1;
        if(line.empty()||line.front()=='#'||line.front()==';')continue;
        if(line=="[battle_test]"){if(section)return fail("duplicate section");section=true;continue;}
        if(!section)return fail("missing/unsupported section");
        const auto equal=line.find('=');if(equal==line.npos)return fail("expected key=value");
        const auto key=trim(line.substr(0,equal)),value=trim(line.substr(equal+1));unsigned id=unsigned(names.size());
        for(unsigned i=0;i<names.size();++i)if(key==names[i])id=i;
        if(id==names.size()||seen[id]||value.empty())return fail("unknown/duplicate/empty field");
        seen[id]=true;
        if(id<4||(id>=18&&id<=20)||id==26){unsigned v=0;const auto result=std::from_chars(value.data(),value.data()+value.size(),v);
            if(result.ec!=std::errc{}||result.ptr!=value.data()+value.size())return fail("invalid integer");
            if(id==0)next.tick_rate=v;else if(id==1)next.shot_interval=v;else if(id==2)next.projectile_life=v;else if(id==3)next.target_health=v;
            else if(id==26)next.ally_mask=v;
            else if(v!=(id==18?2u:1u))return fail("unsupported format/diagnostic/proxy acknowledgement");
        }else{f32 v=0;const auto result=std::from_chars(value.data(),value.data()+value.size(),v);
            if(result.ec!=std::errc{}||result.ptr!=value.data()+value.size()||!std::isfinite(v))return fail("invalid finite scalar");
            switch(id){case 4:next.arena_half=v;break;case 5:next.actor_radius=v;break;case 6:next.target_radius=v;break;case 7:next.projectile_radius=v;break;
                case 8:next.move_per_tick=v;break;case 9:next.shot_per_tick=v;break;case 10:next.spawn.x=v;break;case 11:next.spawn.z=v;break;
                case 21:next.camera_distance=v;break;case 22:next.camera_height=v;break;case 23:next.camera_fov=v;break;
                case 24:next.targets[3].x=v;break;case 25:next.targets[3].z=v;break;
                default:{const unsigned target=(id-12)/2;if(id%2==0)next.targets[target].x=v;else next.targets[target].z=v;break;}}
        }
    }
    for(bool field:seen)if(!field)return fail("missing required field");
    if(!battle_test_config_valid(next))return fail("invalid diagnostic domain/placement");
    *out=next;return true;
}
} // namespace gf
