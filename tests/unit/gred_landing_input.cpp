#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/gred_movement_admission.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source landing input: %s (%s)\n",why,error.c_str());std::abort();}}
gf::CharacterMovementInputProfile fixture(){
    gf::CharacterMovementInputProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.held_mask=0xf0100;p.resource_policy=1;
    p.low_threshold=20;p.high_threshold=52;p.axis_window=2;p.other_window=3;p.direction_window=20;p.release_window=16;
    p.octants={2,10,8,9,1,5,4,6};p.resource_limit=10000;p.recovery_increment=2000;
    for(auto& c:p.commands){c.kind=1;c.pulse=1;c.masks={512,0};}
    p.commands[4].pulse=16;p.commands[4].masks[0]=256;p.commands[5].flags=4;p.commands[5].masks[0]=256;return p;
}
}
int main(){
    using namespace gf;const auto p=fixture();check(character_movement_input_profile_valid(p),"owned normal-G-Red input/resource fixture");
    GRedMovementAdmissionFields fields;fields.selector=7;fields.flags5e0=0x200;fields.prior_handlers_declined=true;
    CharacterAirResourceState resource;CharacterMovementInputState input;GRedMovementAdmission result;
    input.previous_held=0x100;input.commands5d4=1;input.pressed5bc=0;
    check(gred_movement_admit(p,input,resource,fields,&result,&error)&&result.changed&&result.selector==4&&result.reset_direction57c,
        "landing accepts buffered AND held ground jump without requiring a host press edge or positive meter");
    input.directions5d8=0x80;
    check(gred_movement_admit(p,input,resource,fields,&result,&error)&&result.selector==4&&!result.consume_direction&&result.remaining_directions==0x80,
        "jump has priority over a simultaneous buffered dash and does not invent buffer consumption");
    input.commands5d4=0;input.previous_held=8u<<16;
    check(gred_movement_admit(p,input,resource,fields,&result,&error)&&result.changed&&result.selector==12&&result.direction58f==8&&
        result.consume_direction&&result.remaining_directions==0,"landing ground dash consumes original direction nibble without an air-meter condition");
    input={};CharacterMovementInputState soft;
    check(character_movement_input_step(p,input,{0,32,8u<<16,0x200},&soft,&error)&&!(soft.directions5d8&0xf0),
        "native soft steering sample does not fabricate a source flick");
    check(gred_movement_admit(p,soft,resource,fields,&result,&error)&&!result.changed,"ordinary soft walk input does not cancel landing");
    input={};input.previous_held=0x100;
    check(gred_movement_admit(p,input,resource,fields,&result,&error)&&!result.changed,"expired held jump does not automatically repeat on landing");
    input.commands5d4=1;fields.flags5e0|=2;
    check(gred_movement_admit(p,input,resource,fields,&result,&error)&&!result.changed,"source jump lock remains in force");
    fields.flags5e0=0x201;input.commands5d4=0;input.directions5d8=0x80;
    check(gred_movement_admit(p,input,resource,fields,&result,&error)&&!result.changed,"source dash lock remains in force");
    CharacterAirResourceState recovered;
    check(character_air_resource_recover(resource,0,0x200,1,10000,2000,&recovered)&&recovered.value736==0,
        "new grounded/landing flags alone cannot replace prior cached87 history");
    check(character_air_resource_recover(resource,0,0x200,0,10000,2000,&recovered)&&recovered.value736==2000,
        "ground recovery uses shared source kernel before movement, not clip completion or an instant refill");
    std::printf("source landing jump/dash priority/buffers/resource history: %u checks PASS; numeric predicate subset, no actor/whole-frame equivalence\n",checks);
}
