#pragma once
#include "engine/assets/battle_preparation_update.hpp"
#include <optional>
#include <vector>
namespace gf {
enum class BattlePreparationCommandKind:u8 {StoreByte,ObserveBusy,ReadByte,ObserveFade,FadeRequest,Call,IncrementWord,ReturnLatched};
struct BattlePreparationCommand {BattlePreparationCommandKind kind=BattlePreparationCommandKind::Call;u32 key=0;u16 offset=0;u32 value=0;u8 arguments=0;std::array<u32,3> args{};bool operator==(const BattlePreparationCommand&)const=default;};
struct BattlePreparationWaitInputs {u8 substate01=0;std::optional<u8> resource_busy,widgets1653;std::optional<s16> fade_remaining;};
struct BattlePreparationWaitPlan {u8 scene45=0,next_substate01=0;bool latched_ready=false,consumer_owners_pending=true;u32 handler_key=0,resume286d8=0;std::vector<BattlePreparationCommand> commands;};
//DECLARED scalar/call-order seam, never a frame/dispatch receipt. Counter
//increments are ordered operations, not guessed post-callee values. Consumer
//calls remain obligations; no public boolean can acknowledge/skip them here.
inline bool battle_preparation_wait_plan(const BattlePreparationUpdateProfile& profile,const BattlePreparationWaitInputs& in,BattlePreparationWaitPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle preparation wait plan: profile/substate or reached observation unknown";return false;};
    if(!out||!profile.ready()||in.substate01>2){return fail();}BattlePreparationWaitPlan p;p.scene45=1;p.next_substate01=in.substate01;p.handler_key=std::array<u32,3>{0x80029648,0x80029688,0x800296e0}[in.substate01];
    using K=BattlePreparationCommandKind;auto put=[&](K kind,u32 key,u16 offset,u32 value,u8 argc=0,std::array<u32,3> args={}){p.commands.push_back({kind,key,offset,value,argc,args});};auto fade=[&](u32 type){put(K::FadeRequest,0x80027aac,0,0,3,{0,profile.fade_updates(),type});};
    put(K::StoreByte,0,0x45,1);
    if(in.substate01==0){p.next_substate01=1;put(K::StoreByte,0,1,1);fade(0);}
    else if(in.substate01==1){if(!in.resource_busy)return fail();put(K::ObserveBusy,0x8000a3c4,0,*in.resource_busy);if(*in.resource_busy==0){if(!in.widgets1653)return fail();put(K::ReadByte,0,0x1653,*in.widgets1653);if(*in.widgets1653==0){p.next_substate01=2;put(K::StoreByte,0,1,2);fade(1);}}}
    else{if(!in.fade_remaining)return fail();put(K::ObserveFade,0x80027adc,0,u32(s32(*in.fade_remaining)));p.latched_ready=*in.fade_remaining==0;}
    for(unsigned i=0;i<19;++i){const auto& call=*profile.call(i);put(K::Call,call.key,0,0,call.arguments,{call.argument,0,0});if(i==3)put(K::IncrementWord,0,0xb0,1);}
    put(K::IncrementWord,0,0xac,1);put(K::ReturnLatched,0,0,unsigned(p.latched_ready));p.resume286d8=p.latched_ready?0x800286fc:0x800288ec;
    *out=std::move(p);if(error)error->clear();return true;
}
} // namespace gf
