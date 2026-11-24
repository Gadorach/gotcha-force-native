#pragma once
#include "engine/assets/startup_actor_setup.hpp"
#include "engine/assets/challenge_finalization.hpp"
#include "engine/assets/battle_target_query.hpp"
namespace gf {
//2532C's six zero selectors for the two normal classes, its eight-entry delay
//table, and20FD4's selector/level table. Nonzero selector policies stay closed.
class BattleActorInitialTargetProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="initial target profile: source extent/class/policy";return false;};
        if(!b.data||b.size!=4688||b.be32(0)!=0x47464e54||(b.be32(4)!=1&&b.be32(4)!=2)||b.be32(8)!=b.size||!b.be32(12)||!b.be32(16)||!battle_model_in_domain(BattleModelDomain(b.be32(4)),b.be16(24))||b.be16(26)!=6||b.be32(48)!=0x4dbebc20||b.be32(52)!=6||b.be32(56)||b.be32(60))return fail();
        for(unsigned i=28;i<48;++i)if(b.data[i])return fail();
        const auto model=b.be16(24);if(b.be32(20)!=(model==10?0x802d3ee0u:model==0x615?0x80365ac8u:model==0x70b?0x80373920u:0x8036fc50u))return fail();
        for(unsigned i=0;i<8;++i)delay_[i]=b.be16(64+2*i);
        for(unsigned i=0;i<suffix_.size();++i)suffix_[i]=b.be16(80+2*i);
        domain_=BattleModelDomain(b.be32(4));model_=model;hash_=b.be32(12);config_hash_=b.be32(16);config_key_=b.be32(20);ready_=true;if(error)error->clear();return true;
    }
    bool matches(const StartupActorSetupProfile& setup,u32 key)const{return ready_&&setup.loaded&&setup.domain==domain_&&setup.model_id==model_&&setup.model_hash==hash_&&key==config_key_&&challenge_payload_hash(startup_actor_setup_detail::config(setup))==config_hash_;}
    u16 delay(u8 draw)const{return delay_[draw&7];}
    bool suffix(u8 selector,u8 level,u8 draw,u16* out)const{if(!ready_||!out||selector>=18||level>=16)return false;*out=suffix_[(unsigned(selector)*16+level)*8+(draw&7)];return true;}
    BattleTargetQueryProfile query()const{return {{0,0x4dbebc20u,6,0x1e00},ready_};}
private:std::array<u16,8> delay_{};std::array<u16,18*16*8> suffix_{};u16 model_=0;u32 hash_=0,config_hash_=0,config_key_=0;bool ready_=false;BattleModelDomain domain_=BattleModelDomain::NormalPair;
};
} // namespace gf
