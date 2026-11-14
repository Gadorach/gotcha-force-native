#pragma once
#include <stdexcept>
#include "engine/gameplay/challenge_ai_force.hpp"
namespace challenge_ai_force_fixture {
inline gf::ForceCatalog catalog(bool zero=false,bool foreign=false){
    std::vector<gf::ForceBorgMetadata> entries;
    for(unsigned i=0;i<3;++i){gf::ForceBorgMetadata m;m.model=gf::u16(8+i);m.number=gf::u16(i);m.tribe=0;m.source_name="OWNED AI "+std::to_string(i);m.cost.fill(gf::s16(zero?0:i==0?100:i==1?251:1));if(foreign)++m.cost[0];entries.push_back(std::move(m));}
    gf::ForceCatalog out;std::string error;if(!out.assign(std::move(entries),{8},{},"OWNED FORCE","ABCDEFGHIJKLMNOPQRSTUA","OWNED",21,&error))throw std::runtime_error(error);return out;
}
inline std::array<std::vector<gf::ChallengeAiWave>,3> waves(gf::u8 cap=2,gf::u32 budget=334){
    std::array<std::vector<gf::ChallengeAiWave>,3> out;for(auto& d:out){d.resize(1);for(auto& team:d[0].teams)team={budget,cap,{42,42,42,42}};}return out;
}
inline gf::ChallengeAiForceProfile profile(const gf::ForceCatalog& c,gf::u8 cap=2,gf::u32 budget=334,std::vector<gf::u16> models={8,9}){
    gf::ChallengeAiForceProfile p;std::string error;if(!p.assign(c,waves(cap,budget),{{42,std::move(models)}},&error))throw std::runtime_error(error);return p;
}
} // namespace challenge_ai_force_fixture
