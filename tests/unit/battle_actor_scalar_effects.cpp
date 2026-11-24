#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_actor_scalar_effects.hpp"
#include "tests/unit/startup_actor_setup_fixture.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"scalar effects: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;const auto bytes=startup_actor_setup_fixture::profile();StartupActorSetupProfile setup;
    check(startup_actor_setup_load({bytes.data(),bytes.size()},1234,&setup,&error),"owned setup");
    for(unsigned i=0xb8;i<0xbe;++i)setup.class_config[i]=255;
    setup.class_config[0xbb]=0;store_be16(setup.actor_data.data(),500);store_be16(setup.actor_data.data()+2,100);store_be16(setup.actor_data.data()+0xa8,0x8123);
    for(unsigned i=0;i<15;++i)store_be_f32(setup.actor_data.data()+(i<6?0x80+4*i:0x134+4*(i-6)),f32(i)+.25f);
    for(unsigned i=0;i<14;++i)store_be_f32(setup.actor_data.data()+0xb8+4*i,f32(i)-.5f);
    store_be_f32(setup.actor_data.data()+0x98,3.5f);
    BattleActorLevelRow level;level.model=0x615;level.model_hash=1234;level.level=0;level.values={1000,0,4,1,7,3,6,0,9};
    BattleActorScalarEffects p;check(battle_actor_scalar_effects(setup,level,&p,&error),"fresh numeric plan");
    check(p.field59c==0x8123&&p.field6be==500&&p.field6c2==500&&p.field6c0==100&&p.field7d6==100&&p.field204==3.5f&&p.field208==3.5f,"source unsigned/signed/scalar copies");
    for(unsigned i=0;i<15;++i)check(p.values850[i]==f32(i)+.25f,"all15 source vector values including unconditional nine-value tail");
    for(unsigned i=0;i<14;++i)check(p.values88c[i]==f32(i)-.5f,"source14-value tail");
    check(p.reloads[0].current==0&&p.reloads[0].previous==4&&p.reloads[0].target==4&&p.reloads[0].remaining==7&&p.reloads[2].current==3&&p.reloads[2].remaining==0&&!p.reloads[1].target,"shared reload kernel respects zero/nonzero channels and untouched middle");
    check(p.kinds8cd==std::array<u8,3>{5,0,2}&&p.field8d0==5&&p.field8d1==2,"source classifications use definitions even before child allocation");
    unsigned bars=0;for(const auto& op:p.operations)bars+=op.kind==BattleActorEffectOperationKind::BarChild;check(bars==1,"one required bar allocation operation");
    level.values={1000,0,0,0,7,0,0,0,9};setup.class_config[0xb9]=0;
    check(battle_actor_scalar_effects(setup,level,&p,&error)&&p.kinds8cd[0]==3&&p.field8d0==4,"combined classification branch");
    unsigned writes=0;for(const auto& op:p.operations)writes+=op.kind==BattleActorEffectOperationKind::Byte&&op.offset==0x8d0;check(writes==1,"no fabricated duplicate store on below-six branch");
    setup.class_config[0xbc]=0;check(battle_actor_scalar_effects(setup,level,&p,&error)&&p.field8d0==4,"combined classification wraps six-or-more by three");
    writes=0;for(const auto& op:p.operations)writes+=op.kind==BattleActorEffectOperationKind::Byte&&op.offset==0x8d0;check(writes==2,"actual intermediate and corrected stores retained");
    for(unsigned i=0xb8;i<0xbe;++i)setup.class_config[i]=255;
    setup.class_config[1]=2;check(!battle_actor_scalar_effects(setup,level,&p,&error)&&p.operations.empty(),"unowned program4F0 lookup refuses");
    setup.class_config[1]=0;setup.class_config[0xbb]=254;check(!battle_actor_scalar_effects(setup,level,&p,&error),"negative unknown effect selector refused");
    setup.class_config[0xbb]=255;store_be32(setup.actor_data.data()+0x80,0x7fc00000);check(!battle_actor_scalar_effects(setup,level,&p,&error),"nonfinite required operand refused");
    std::printf("battle scalar effects: %u owned checks PASS\n",checks);
}
