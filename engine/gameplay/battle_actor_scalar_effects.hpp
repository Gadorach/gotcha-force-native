#pragma once
#include "engine/assets/battle_actor_effects.hpp"
#include "engine/assets/battle_actor_levels.hpp"
#include "engine/gameplay/startup_actor_bookkeeping.hpp"
namespace gf {
enum class BattleActorEffectOperationKind {Byte,Half,Word,FloatBits,BarChild,ResetColour};
struct BattleActorEffectOperation {BattleActorEffectOperationKind kind;u16 offset=0;u32 value=0;};
struct BattleActorScalarEffects {
    u8 field660=0,field661=0,field591=0,field589=0,field8cc=0,field7cd=0,field655=0;
    u16 field59c=0,field59e=0;
    std::array<f32,15> values850{};std::array<f32,14> values88c{};
    f32 rate5f4=0,rate768=0,field204=0,field208=0,field664=0;
    s16 field6be=0,field6c0=0,field6c2=0,field6c4=0,field6c6=0,field7d6=0,field7dc=0;
    std::array<StartupActorReload,3> reloads{};std::array<s16,3> initial_counts{};
    std::array<f32,3> fields63c{},fields648{};std::array<u8,3> kinds8cd{};
    u8 field8d0=0,field8d1=0;std::array<u32,2> colours7ec{};
    std::vector<BattleActorEffectOperation> operations;
};
//562B8 fresh normal numeric/branch plan. Allocation and06238's current resource
//context store are performed by the native constructor, never acknowledged here.
inline bool battle_actor_scalar_effects(const StartupActorSetupProfile& setup,const BattleActorLevelRow& level,BattleActorScalarEffects* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle scalar effects: ")+why;return false;};
    if(!out||!startup_actor_setup_detail::valid(setup)||level.model!=setup.model_id||level.model_hash!=setup.model_hash||level.variant||level.level>=10)return fail("same normal model/current level row required");
    const auto d=startup_actor_setup_detail::data(setup),c=startup_actor_setup_detail::config(setup);
    for(unsigned i=0xb8;i<0xbe;++i)if(std::bit_cast<s8>(c.data[i])< -1)return fail("unqualified negative effect selector");
    BattleActorScalarEffects p;using K=BattleActorEffectOperationKind;
    auto put=[&](K kind,unsigned at,u32 value){p.operations.push_back({kind,u16(at),value});};
    auto byte=[&](unsigned at,u8 value){put(K::Byte,at,value);};auto half=[&](unsigned at,u16 value){put(K::Half,at,value);};auto real=[&](unsigned at,f32 value){put(K::FloatBits,at,f32_to_bits(value));};
    auto number=[](f32 value){return std::isfinite(value)&&std::abs(value)<=1e7f;};
    p.field660=c.data[0];p.field661=c.data[1];p.field591=c.data[4];byte(0x660,p.field660);byte(0x661,p.field661);byte(0x591,p.field591);
    p.field59c=d.be16(0xa8);p.field59e=d.be16(0xaa);half(0x59c,p.field59c);half(0x59e,p.field59e);
    for(unsigned i=0;i<15;++i){p.values850[i]=d.bef32(i<6?0x80+4*i:0x134+4*(i-6));if(!number(p.values850[i]))return fail("source850 vector domain");real(0x850+4*i,p.values850[i]);}
    p.field589=d.data[0xb6];byte(0x589,p.field589);p.rate5f4=p.rate768=setup.one;real(0x5f4,p.rate5f4);real(0x768,p.rate768);
    p.field8cc=d.data[0x182];byte(0x8cc,p.field8cc);
    p.field6be=p.field6c2=std::bit_cast<s16>(d.be16(0));p.field6c0=100;p.field6c4=p.field6c6=p.field7d6=p.field7dc=std::bit_cast<s16>(d.be16(2));
    half(0x6be,u16(p.field6be));half(0x6c2,u16(p.field6c2));half(0x6c0,100);half(0x6c6,u16(p.field6c6));half(0x6c4,u16(p.field6c4));half(0x7dc,u16(p.field7dc));half(0x7d6,u16(p.field7d6));
    p.field204=p.field208=d.bef32(0x98);if(!number(p.field204))return fail("source204/208 scalar");real(0x208,p.field208);real(0x204,p.field204);p.field664=setup.one;real(0x664,p.field664);
    if(!startup_actor_reload_seed(level.values,&p.reloads))return fail("reload row");
    for(unsigned i=0;i<2;++i){const unsigned channel=i*2,at=1+i*4;const auto& r=p.reloads[channel];p.initial_counts[channel]=level.values[at];
        half(0x774+16*i,u16(r.current));half(0x776+16*i,u16(r.previous));put(K::Word,0x78c+16*i,(u32(u16(level.values[at]))<<16)|u16(level.values[at+1]));put(K::Word,0x790+16*i,(u32(u16(level.values[at+2]))<<16)|u16(level.values[at+3]));
        //Other zero-current modes retain the fresh clear instead of inventing a
        //store. Mode1 adds the interval to that actual zero.
        if(r.current||(r.mode>=0&&r.mode<=3))real(0x778+16*i,r.remaining);
    }
    p.field7cd=c.data[0x11]?(p.field661==2?2:1):0;byte(0x7cd,p.field7cd);
    for(unsigned i=0;i<3;++i){if(c.data[0xbb+i]!=255)put(K::BarChild,i,0);real(0x63c+4*i,0);real(0x648+4*i,0);}
    byte(0x655,0);
    for(unsigned i=0;i<3;++i){const auto& r=p.reloads[i];u8 kind=r.target?(r.mode==3?1:2):(c.data[0xb8+i]==255?0:1);byte(0x8cd+i,kind);if(c.data[0xbb+i]!=255){kind=u8(kind+3);byte(0x8cd+i,kind);}p.kinds8cd[i]=kind;}
    if(!p.kinds8cd[0]){p.field8d0=p.kinds8cd[1];byte(0x8d0,p.field8d0);}
    else if(p.kinds8cd[0]==3){p.field8d0=u8(p.kinds8cd[1]+3);byte(0x8d0,p.field8d0);if(p.field8d0>=6){p.field8d0=u8(p.field8d0-3);byte(0x8d0,p.field8d0);}}
    else{p.field8d0=p.kinds8cd[0];byte(0x8d0,p.field8d0);}
    if(!p.kinds8cd[2]&&(p.field661==2||p.field661==5))return fail("reached program4F0+41 lookup requires its current owner");
    p.field8d1=p.kinds8cd[2];byte(0x8d1,p.field8d1);
    for(unsigned i=0;i<14;++i){p.values88c[i]=d.bef32(0xb8+4*i);if(!number(p.values88c[i]))return fail("source88C vector domain");real(0x88c+4*i,p.values88c[i]);}
    put(K::ResetColour,0,0);put(K::Word,0x7ec,0);put(K::Word,0x7f0,0);
    *out=std::move(p);if(error)error->clear();return true;
}
} // namespace gf
