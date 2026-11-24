#pragma once
#include "engine/gameplay/battle_actor_scalar_effects.hpp"
#include "engine/gameplay/battle_actor_scalar_defaults.hpp"
namespace gf {
struct BattleActorDimensions {
    f32 field668=0,field66c=0,field670=0,field674=0,field770=0,offset6d0=0,size7fc=0;
    std::array<f32,3> values678{};std::array<f32,15> values814{};
    u8 cached87=0,input334=32;std::vector<BattleActorScalarDefaultWrite> writes;
};
//55DCC/4D08C/68004 common fresh operands and the human334 default. The separate
//AI gateway replaces334 from27A44's actual table. No ground/world frame grant.
inline bool battle_actor_fresh_dimensions(const StartupActorSetupProfile& setup,const BattleActorScalarEffects& effects,f32 scale,BattleActorDimensions* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle actor dimensions: fresh source operand domain";return false;};
    if(!out||!startup_actor_setup_detail::valid(setup)||!std::isfinite(scale)||scale<=0||scale>1e4f)return fail();
    StartupActorInitialSetup shared;if(!startup_actor_dimensions(setup,scale,&shared))return fail();
    BattleActorDimensions p;p.field668=shared.field668;p.field66c=shared.field66c;p.field670=shared.field670;p.field674=shared.field674;p.field770=shared.field770;
    const auto d=startup_actor_setup_detail::data(setup),c=startup_actor_setup_detail::config(setup);
    for(unsigned i=0;i<3;++i)p.values678[i]=d.bef32(0x74+4*i);
    p.values814=effects.values850;
    for(auto value:p.values814)if(!std::isfinite(value)||std::abs(value)>1e7f)return fail();
    //68004 deliberately uses unscaled class dimensions.
    p.size7fc=f32(std::max(c.bef32(0x4c),c.bef32(0x50))/setup.size_divisor);
    if(!std::isfinite(p.size7fc)||std::abs(p.size7fc)>1e7f)return fail();
    auto put=[&](u16 at,u8 extent,u32 value){p.writes.push_back({at,extent,value});};
    put(0x87,1,0);put(0x668,4,f32_to_bits(p.field668));put(0x66c,4,f32_to_bits(p.field66c));put(0x670,4,f32_to_bits(p.field670));put(0x674,4,f32_to_bits(p.field674));put(0x770,4,f32_to_bits(p.field770));
    for(unsigned i=0;i<3;++i)put(u16(0x678+4*i),4,f32_to_bits(p.values678[i]));
    for(unsigned i=0;i<15;++i)put(u16(0x814+4*i),4,f32_to_bits(p.values814[i]));
    put(0x6d0,4,0);put(0x7fc,4,f32_to_bits(p.size7fc));put(0x334,1,32);
    *out=std::move(p);if(error)error->clear();return true;
}
} // namespace gf
