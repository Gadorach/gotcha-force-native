#pragma once
#include "engine/assets/character_program.hpp"
#include "engine/assets/startup_actor_setup.hpp"
#include "engine/assets/challenge_finalization.hpp"
namespace gf {
//Source-bound normal request(0,8), through its first unchanged wait. Metadata
//keys identify class tables, never executable pointers or continuation authority.
class BattleActorInitialProgramProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle initial program: bounded normal request/source binding";return false;};
        if(!b.contains(0,64)||b.size!=160||b.be32(0)!=0x47464950||(b.be32(4)!=1&&b.be32(4)!=2)||b.be32(8)!=b.size||b.be16(14)||!battle_model_in_domain(BattleModelDomain(b.be32(4)),b.be16(12))||!b.be32(16)||!b.be32(20)||b.be32(40)!=96||b.be32(44)!=0x0f000800||b.be32(48)!=0xbf800000||b.be32(52)!=b.be32(32)+28||b.be32(56)||b.be32(60))return fail();
        const auto model=b.be16(12);const bool sasuke=model==10,ai=b.be32(4)==2;
        const std::array<u32,4> keys=sasuke?std::array<u32,4>{0x802bde80,0x802bd780,0x802be01e,0x802bd900}:model==0x615?std::array<u32,4>{0x80366220,0x80367460,0x803663bc,0x803675a6}:model==0x70b?std::array<u32,4>{0x80372d40,0x80373420,0x80372e6a,0x80373516}:std::array<u32,4>{0x803700c0,0x803709c0,0x8037020e,0x80370aea};
        for(unsigned i=0;i<4;++i)if(b.be32(24+4*i)!=keys[i])return fail();
        CharacterProgramAsset asset;if(!character_program_load_request_prefix({b.data+64,96},b.be32(16),&asset,error))return false;
        if(asset.lane_mask!=3||asset.programs.size()!=1||asset.descriptors.size()!=1)return fail();
        const auto& p=asset.programs[0];const auto& d=asset.descriptors[0];
        if(p.group||p.choice!=8||p.bytes.size()!=28||d.group||d.choice!=5)return fail();
        const ByteView code{p.bytes.data(),p.bytes.size()};
        //Admit the reached select/control0/control1/wait operands, without
        //embedding or replacing the source program payload in the engine.
        if(code.data[0]!=1||code.data[1]||code.data[2]!=5||code.data[3]||code.be32(4)||code.data[8]!=2||code.data[9]!=255||code.be16(10)||code.be32(12)||code.data[16]!=3||code.data[17]||code.be32(18)||code.be16(22)!=0x200||code.data[24]||code.data[25]!=(sasuke?2:1)||code.be16(26)!=(sasuke?0xfff8:0xffff))return fail();
        if(d.bytes[0]!=(ai?1:255)||d.bytes[1]||d.bytes[2]||d.bytes[3])return fail();
        for(unsigned lane=0;lane<2;++lane){const auto at=4+8*lane;if(d.bytes[at]||d.bytes[at+1]!=9||d.bytes[at+2]!=(model==0x105?0:1)||d.bytes[at+3]||d.bytes[at+4]!=255)return fail();}
        if(d.bytes[9]!=((sasuke||ai)?255:2)||d.bytes[10]!=((sasuke||ai)?255:2)||d.bytes[11]||d.bytes[17]||d.bytes[18]!=5||d.bytes[19])return fail();
        domain_=BattleModelDomain(b.be32(4));model_=b.be16(12);hash_=b.be32(16);config_hash_=b.be32(20);program_root_=b.be32(24);descriptor_root_=b.be32(28);program_key_=b.be32(32);continuation_key_=b.be32(52);asset_=std::move(asset);ready_=true;if(error)error->clear();return true;
    }
    bool matches(const StartupActorSetupProfile& setup,u32 program_root,u32 descriptor_root)const{
        return ready_&&setup.loaded&&setup.domain==domain_&&setup.model_id==model_&&setup.model_hash==hash_&&program_root==program_root_&&descriptor_root==descriptor_root_&&challenge_payload_hash(startup_actor_setup_detail::config(setup))==config_hash_&&asset_.default_models==std::array<u8,2>{setup.class_config[0xbe],setup.class_config[0xbf]};
    }
    const CharacterProgramAsset& asset()const{return asset_;}
    u32 source_program_key()const{return program_key_;}
    u32 continuation_key()const{return continuation_key_;}
private:CharacterProgramAsset asset_;u16 model_=0;u32 hash_=0,config_hash_=0,program_root_=0,descriptor_root_=0,program_key_=0,continuation_key_=0;bool ready_=false;BattleModelDomain domain_=BattleModelDomain::NormalPair;
};
}
