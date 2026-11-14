#pragma once
#include "engine/assets/force_catalog.hpp"
namespace gf {
struct ChallengeAiTeamRow {u32 budget=0;u8 member_cap=0;std::array<u8,4> categories{};};
struct ChallengeAiWave {std::array<ChallengeAiTeamRow,2> teams;};
struct ChallengeAiPool {u8 category=0;std::vector<u16> models;};
//801962C4/80196EB8: catalog-bound authored tables, exported offline. These are
//not inventory Borgs, actor spawns, wave clocks, rewards or battle-ready rosters.
class ChallengeAiForceProfile {
public:
    bool assign(const ForceCatalog& catalog,std::array<std::vector<ChallengeAiWave>,3> waves,std::vector<ChallengeAiPool> pools,std::string* error){
        *this={};auto fail=[&](const char* why){if(error)*error=std::string("Challenge AI: ")+why;return false;};
        if(!catalog.ready()||!catalog.find(8)||pools.empty()||pools.size()>128)return fail("catalog/empty-pool fallback/pool domain");
        std::sort(pools.begin(),pools.end(),[](const auto& a,const auto& b){return a.category<b.category;});
        for(unsigned i=0;i<pools.size();++i){const auto& p=pools[i];if(p.category>=128||p.models.size()>256||(i&&p.category==pools[i-1].category))return fail("pool category/count/duplicate");for(auto model:p.models)if(!catalog.find(model))return fail("unknown pool model");}
        for(const auto& difficulty:waves){
            if(difficulty.empty()||difficulty.size()>15)return fail("wave domain");
            for(const auto& wave:difficulty)for(const auto& team:wave.teams){
                if(team.member_cap>30||team.budget>0x7fffffffu)return fail("member cap/budget domain");
                for(auto category:team.categories)if(std::none_of(pools.begin(),pools.end(),[&](const auto& p){return p.category==category;}))return fail("missing category pool");
            }
        }
        waves_=std::move(waves);pools_=std::move(pools);catalog_hash_=catalog.fingerprint();ready_=true;if(error)error->clear();return true;
    }
    bool load(ByteView b,const ForceCatalog& catalog,std::string* error){
        *this={};auto fail=[&](){if(error)*error="Challenge AI: profile extent/domain";return false;};
        if(!b.data||b.size<32||b.size>(128u<<10)||b.be32(0)!=0x47464349||b.be32(4)!=1||b.be32(8)!=catalog.fingerprint()||b.be16(12)!=3||!b.be16(14)||b.be16(14)>128||b.be16(22)||b.be32(24)!=b.size-32||b.be32(28))return fail();
        std::array<std::vector<ChallengeAiWave>,3> waves;std::size_t at=32;
        for(unsigned d=0;d<3;++d){const unsigned count=b.be16(16+2*d);if(!count||count>15)return fail();waves[d].resize(count);
            for(auto& wave:waves[d])for(auto& team:wave.teams){if(!b.contains(at,12)||b.data[at+9]||b.data[at+10]||b.data[at+11])return fail();team.budget=b.be32(at);team.member_cap=b.data[at+4];std::copy_n(b.data+at+5,4,team.categories.begin());at+=12;}
        }
        std::vector<ChallengeAiPool> pools;
        for(unsigned i=0;i<b.be16(14);++i){if(!b.contains(at,4)||b.data[at+1])return fail();ChallengeAiPool p;p.category=b.data[at];const unsigned count=b.be16(at+2);at+=4;if(count>256||!b.contains(at,2*count))return fail();for(unsigned j=0;j<count;++j)p.models.push_back(b.be16(at+2*j));at+=2*count;pools.push_back(std::move(p));}
        if(at!=b.size)return fail();
        return assign(catalog,std::move(waves),std::move(pools),error);
    }
    bool for_catalog(const ForceCatalog& catalog)const{return ready_&&catalog.ready()&&catalog_hash_==catalog.fingerprint();}
    bool ready()const{return ready_;}
    const ChallengeAiWave* wave(unsigned difficulty,unsigned index)const{return ready_&&difficulty<3&&index<waves_[difficulty].size()?&waves_[difficulty][index]:nullptr;}
    const ChallengeAiPool* pool(u8 category)const{if(!ready_)return nullptr;const auto p=std::lower_bound(pools_.begin(),pools_.end(),category,[](const auto& row,u8 c){return row.category<c;});return p!=pools_.end()&&p->category==category?&*p:nullptr;}
    unsigned waves(unsigned difficulty)const{return ready_&&difficulty<3?unsigned(waves_[difficulty].size()):0;}
private:
    std::array<std::vector<ChallengeAiWave>,3> waves_;std::vector<ChallengeAiPool> pools_;u32 catalog_hash_=0;bool ready_=false;
};
} // namespace gf
