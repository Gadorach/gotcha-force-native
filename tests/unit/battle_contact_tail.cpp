#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_contact_tail.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"contact tail: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::array<u8,112> bytes{};
    const std::array<u32,28> words{0x47464354,1,112,0,0x8002bb14,0x8002d35c,0x8002d58c,0x8002d198,0x8002e1c8,0x8002cb20,0x8002ce44,0x8002d7c4,0x8002df08,0x8002db58,0x8005a580,0x80036e24,0x8008972c,0x80089658,0x8000bda4,0x8002ce90,0x80029b58,0x41a00000,0x800,0x17,13,12,6,0};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleContactTailProfile profile;std::string error;check(profile.load({bytes.data(),bytes.size()},&error),"source profile");
    const std::array<f32,4> bounds{5000,-5000,5000,-5000};BattleContactTailActorInput actor;actor.position={0,10,0};actor.health=50;
    check(battle_contact_tail_actor_gates(profile,actor,bounds,&error),"unchanged initial actor returns without response");
    for(u8* flag:{&actor.contact.moved1dd,&actor.contact.attached1db,&actor.contact.current274,&actor.contact.field1da,&actor.contact.field1d9,&actor.contact.retained.timer1e8,&actor.forced663,&actor.peer_mask71e,&actor.peer_mask71f}){
        *flag=1;check(!battle_contact_tail_actor_gates(profile,actor,bounds,&error),"reached stateful response refuses");*flag=0;
    }
    actor.health=0;check(!battle_contact_tail_actor_gates(profile,actor,bounds,&error),"death/recovery branch refuses");actor.health=50;
    actor.contact.retained.displacement1cc[0]=1;check(!battle_contact_tail_actor_gates(profile,actor,bounds,&error),"collider correction cannot masquerade as a no-op");actor.contact.retained.displacement1cc={};
    actor.position[0]=-4980;check(battle_contact_tail_actor_gates(profile,actor,bounds,&error),"inclusive authored bound margin");
    actor.position[0]=-4981;check(!battle_contact_tail_actor_gates(profile,actor,bounds,&error),"source clamp continuation required");
    actor.disabled4a1=1;check(battle_contact_tail_actor_gates(profile,actor,bounds,&error),"source disabled4A1 early bound return");
    bool skip=false;check(battle_contact_static_pair_gate(0x800,0,22,&skip)&&skip,"descriptor flag skips solid pass");
    check(battle_contact_static_pair_gate(4,1,22,&skip)&&skip,"source freeze skips solid pass");
    check(battle_contact_static_pair_gate(4,0,23,&skip)&&skip,"source kind23 skips solid pass");
    check(battle_contact_static_pair_gate(4,0,22,&skip)&&!skip,"ordinary movable body requires pair consumers");
    bytes[87]^=1;check(!profile.load({bytes.data(),bytes.size()},&error),"changed source bound operand refuses");
    std::printf("initial contact tail gates: %u checks PASS\n",checks);
}
