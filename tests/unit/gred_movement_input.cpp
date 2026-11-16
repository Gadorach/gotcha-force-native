#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>
#include "engine/gameplay/gred_movement_admission.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"G-Red movement input: %s (%s)\n",why,error.c_str());std::abort();}}
std::vector<gf::u8> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
gf::CharacterMovementInputProfile fixture(){
    using namespace gf;CharacterMovementInputProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.resource_policy=1;p.held_mask=0xf0100;
    p.low_threshold=20;p.high_threshold=52;p.axis_window=2;p.other_window=3;p.direction_window=20;p.release_window=16;
    const std::array<std::array<int,2>,8> vectors={{{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}}};
    for(unsigned i=0;i<8;++i){const auto x=vectors[i][0],y=vectors[i][1];p.octants[i]=gf::u8((x>0?2:x<0?1:0)|(y>0?8:y<0?4:0));}
    p.nibble_indices.fill(0);for(auto& c:p.commands){c.kind=1;c.pulse=1;c.masks={2,0};}
    p.commands[4].masks={0x100,0};p.commands[4].pulse=16;p.commands[5].masks={0x100,0};p.commands[5].flags=4;
    p.resource_limit=1000;p.recovery_increment=200;p.costs={15,10,45,5};return p;
}
}
int main(){
    using namespace gf;auto p=fixture();check(character_movement_input_profile_valid(p),"owned explicit input fixture");
    CharacterMovementInputState s;auto step=[&](s16 x,s16 y,u32 held,u32 flags=0){check(character_movement_input_step(p,s,{x,y,held,flags},&s,&error),"source movement-only input update");};
    for(auto [x,y]:{std::pair<s16,s16>{60,0},{-60,0},{0,60},{0,-60},{60,60},{-60,60},{-60,-60},{60,-60}}){
        u8 nibble=0;check(character_movement_stick_nibble(p,x,y,52,&nibble),"bounded heading quantization");
        check(nibble==u8((x>0?2:x<0?1:0)|(y>0?8:y<0?4:0)),"eight source stick octants retain axis order");
    }
    u8 nibble=0;check(character_movement_stick_nibble(p,51,0,52,&nibble)&&nibble==0&&character_movement_stick_nibble(p,52,0,52,&nibble)&&nibble==2,"radial high threshold inclusive");
    check(character_movement_stick_nibble(p,12,16,20,&nibble)&&nibble!=0,"radial low threshold, not per-axis threshold");
    step(60,0,2u<<16);check((s.directions5d8&0xf0)==0x20&&s.initial_window==19,"rapid deflection generates directional dash buffer with same-update countdown");
    GRedMovementAdmissionFields fields;fields.prior_handlers_declined=true;
    CharacterAirResourceState resource{p.resource_limit,0,0};GRedMovementAdmission result;
    check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.changed&&result.selector==12&&result.consume_direction&&result.direction58f==2,"ground directional admission");
    s.directions5d8=result.remaining_directions;step(60,0,2u<<16);
    check(!(s.directions5d8&0xf0),"consumed dash does not retrigger while held");
    step(0,0,0);check(!s.flick_flags&&(s.directions5d8&0xf000)==0x2000&&s.release_window==15,"release history and rearming are bounded");
    step(60,0,2u<<16);check((s.directions5d8&0xf0)==0x20,"fresh deflection rearms after neutral");
    s={};step(20,0,2u<<16);step(30,0,2u<<16);step(60,0,2u<<16);
    check(!(s.directions5d8&0xf0),"slow stick ramp does not become a flick dash");
    check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==1,"slow ramp admits walking");
    s={};step(0,0,0x100);
    check(s.commands5d4==1,"ground jump edge begins buffered jump command but not air command");
    for(unsigned tick=1;tick<16;++tick){step(0,0,0x100);check((s.commands5d4&1)!=0,"jump buffer persists for authored pulse");}
    step(0,0,0x100);check(!(s.commands5d4&1),"held jump does not continually reset edge pulse");
    step(0,0,0x100,0x40);check(!(s.commands5d4&2),"becoming airborne does not invent another button edge");
    step(0,0,0,0x40);step(0,0,0x100,0x40);check((s.commands5d4&3)==3,"new airborne edge generates separate air command");
    s={};step(60,0,(2u<<16)|0x100);
    check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==4&&result.reset_direction57c&&!result.consume_direction,"jump wins over simultaneous ground dash");
    fields.flags5e0=2;check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==12,"jump lock does not imply dash lock");
    fields.flags5e0=1;check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==4,"dash lock does not imply jump lock");
    fields.flags5e0=3;check(gred_movement_admit(p,s,resource,fields,&result,&error)&&!result.changed,"both source admission locks respected");
    fields.flags5e0=0x40;fields.selector=6;fields.air_ready588=1;
    s={};step(60,0,(2u<<16)|0x100,0x40);
    check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==8&&!result.consume_direction,"air relaunch wins over simultaneous air dash");
    s.commands5d4=0;check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==15,"air dash admission uses positive flight resource");
    auto empty=resource;empty.value736=0;
    check(gred_movement_admit(p,s,empty,fields,&result,&error)&&!result.changed&&result.contact_tail_required,"empty meter blocks air actions without claiming contact-tail execution");
    fields.selector=5;s.previous_held&=~0x100u;
    check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==6&&!result.consume_direction,"flight release precedes directional air dash");
    fields.selector=16;fields.air_ready588=0;s.commands5d4=2;s.previous_held|=0x100;
    check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==8,"active dash relaunch has distinct readiness predicate");
    fields.air_ready588=1;s.commands5d4=0;s.previous_held=0;
    check(gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==17,"active dash release admits recovery when source readiness permits");
    fields.selector=4;check(gred_movement_admit(p,s,resource,fields,&result,&error)&&!result.changed,"input kernel cannot replace source takeoff completion");
    fields.prior_handlers_declined=false;result.selector=99;
    check(!gred_movement_admit(p,s,resource,fields,&result,&error)&&result.selector==99,"unknown higher-priority handlers refuse atomically");
    auto foreign=p;foreign.model=10;fields.prior_handlers_declined=true;
    check(!gred_movement_admit(foreign,s,resource,fields,&result,&error),"Sasuke does not inherit G-Red meter admission");
    auto sentinel=s;sentinel.previous_held=1234;
    check(!character_movement_input_step(p,s,{0,0,0x200,0},&sentinel,&error)&&sentinel.previous_held==1234,"weapon input requires its producer, not fabricated movement-only continuation");
    check(!character_movement_input_step(p,s,{300,0,0,0},&sentinel,&error)&&sentinel.previous_held==1234,"out-of-domain axis refuses without output mutation");
    CharacterAirResourceState recovered;
    check(character_air_resource_recover({0,0,3},0,0x40,1,p.resource_limit,p.recovery_increment,&recovered)&&recovered.value736==0&&recovered.air_actions747==3,"previous airborne post-value suppresses refill and action reset");
    check(character_air_resource_recover({0,0,3},0,0,0,p.resource_limit,p.recovery_increment,&recovered)&&recovered.value736==200&&!recovered.air_actions747,"accepted grounded cache permits refill and reset");
    check(character_air_resource_recover({900,0,3},0,0,0,p.resource_limit,p.recovery_increment,&recovered)&&recovered.value736==1000,"recovery caps at source resource limit");
    check(character_air_resource_recover({100,2,3},0,0,0,p.resource_limit,p.recovery_increment,&recovered)&&recovered.value736==100&&recovered.delay73a==1,"source recovery delay ticks before refill");
    for(unsigned value=0;value<65536;++value){
        const s16 before=std::bit_cast<s16>(u16(value));const s16 expected=std::bit_cast<s16>(u16(value+1));
        check(character_air_resource_recover({before,0,1},0,0,0,32767,1,&recovered)&&recovered.value736==expected,"signed16 recovery store precedes cap for every input word");
    }
    check(character_air_resource_consume({100,0,0},15,2,.5f,&recovered)&&recovered.value736==92&&recovered.delay73a==2,"fractional resource spending uses source truncation");
    check(character_air_resource_consume({10,0,0},45,0,1,&recovered)&&!recovered.value736,"resource spending clamps negative stored result");
    check(!character_air_resource_consume({100,0,0},15,0,NAN,&recovered),"nonfinite active spending rate refuses");
    check(character_air_resource_consume({0,7,0},15,0,NAN,&recovered)&&recovered.delay73a==7,"empty resource does not read unused rate or reset delay");
    CharacterMovementInputState repeat_a,repeat_b;
    for(unsigned tick=0;tick<600;++tick){
        CharacterMovementInputSample sample{tick%40<20?s16(60):s16(0),0,tick%40<20?2u<<16:0u,tick%3?0u:0x40u};
        check(character_movement_input_step(p,repeat_a,sample,&repeat_a,&error)&&character_movement_input_step(p,repeat_b,sample,&repeat_b,&error)&&repeat_a==repeat_b,"fixed-source-input conditioning is deterministic");
    }
    auto encoded=read("local/recomp-evidence/gred-input-resource-v1/movement.gfmi");
    unsigned real=0;if(!encoded.empty()){
        CharacterMovementInputProfile original;
        check(character_movement_input_load({encoded.data(),encoded.size()},&original,&error),"qualified original input/resource profile");
        check(original.model==0x615&&original.movement_type==0&&original.resource_policy==1&&original.resource_limit==10000&&original.costs[2]==450&&original.costs[3]==50,"G-Red-specific recovered policy and dash resource values");
        for(unsigned size=0;size<encoded.size();++size)check(!character_movement_input_load({encoded.data(),size},&original,&error)&&!original.loaded,"all truncated source input profiles refuse");
        for(unsigned at=0;at<encoded.size();++at){auto changed=encoded;changed[at]^=1;check(!character_movement_input_load({changed.data(),changed.size()},&original,&error)&&!original.loaded,"edited source profile refuses atomically");}
        ++real;
    }
    std::printf("source movement inputs/resource/admission: %u checks PASS; original G-Red profiles=%u; no terrain/weapon/contact/physics or actor admission\n",checks,real);
}
