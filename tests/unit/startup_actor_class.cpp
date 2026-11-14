#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_actor_class_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup actor class: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;using namespace startup_actor_class_fixture;
    auto bytes=profile();StartupActorClassProfile p;std::string error;
    check(startup_actor_class_load({bytes.data(),bytes.size()},&p,&error)&&p.angle_retention==bits_to_f32(0x3f666666),"owned source operand profile");
    StartupActorClassState s=state(0x615),out;std::vector<StartupActorClassEffect> effects;
    check(startup_actor_class_initialize(p,s,&out,&effects)&&out.callbacks_initialized&&out.field4a8==1&&out.field6f6==0&&effects.size()==4,"G-Red initializer parent writes and four explicit auxiliary attempts");
    check(effects[0].kind==StartupActorClassEffectKind::AuxiliaryC74ECRequired&&effects[0].id==40&&effects[1].id==41&&effects[2].kind==StartupActorClassEffectKind::Auxiliary185F6CRequired&&effects[2].id==24&&effects[3].id==25,"G-Red source order and distinct allocator paths");
    s=out;s.angle18e2=-11;check(startup_actor_class_pre(p,s,&out)&&out.angle18e2==-9&&out.field6ee==0&&out.field1dfc==0,"float32 signed-angle damping truncates toward zero, not floor/unsigned");
    s.flags5e0=1;check(startup_actor_class_pre(p,s,&out)&&fields(s)==fields(out),"G-Red pre flag guards all writes");
    for(unsigned flag=0;flag<2;++flag)for(unsigned encoded=0;encoded<65536;++encoded){s.flags5e0=flag;s.angle18e2=std::bit_cast<s16>(u16(encoded));s.field6ee=23;s.field1dfc=-12345;
        const auto expected=flag?s.angle18e2:s16(int(f32(double(s.angle18e2)*double(p.angle_retention))));check(startup_actor_class_pre(p,s,&out)&&out.angle18e2==expected&&out.field6ee==(flag?23:0)&&out.field1dfc==(flag?-12345:0),"all signed16 inputs with guarded/unconditional source pre callback");}
    for(unsigned mode=0;mode<256;++mode){s.field6e9=u8(mode);s.fieldaf=59;check(startup_actor_class_post(p,s,&out,&effects)&&out.field6e9==0&&out.fieldaf==(mode==1?1:mode==2?255:59)&&effects.size()==(mode==1?1u:0u),"G-Red signed post selector is consumed once");
        if(mode==1)check(effects[0].kind==StartupActorClassEffectKind::Effect1AB6CRequired&&effects[0].id==0x32,"source effect32 requirement, not fake rendered effect");
        s=out;check(startup_actor_class_post(p,s,&out,&effects)&&effects.empty(),"consumed G-Red flag does not replay effect");}
    s=state(10);check(startup_actor_class_initialize(p,s,&out,&effects)&&out.field4a8==17&&out.field6f6==0xffff&&effects.size()==2&&effects[0].kind==StartupActorClassEffectKind::Attachment4D6DCRequired&&effects[0].id==6&&effects[1].id==7,"Sasuke initializer does not invent G-Red parent writes or defaults");s=out;
    for(unsigned flags:{0u,1u,0x1000u,0x1001u}){s.flags5e0=flags;s.field58e=255;s.field272=0xffff;s.field582=31;s.field6ee=23;s.field1dfc=-12345;
        check(startup_actor_class_pre(p,s,&out)&&out.field582==0&&out.field58e==0xdf&&out.field272==((flags&0x1000)?0xffff:0xfffb)&&out.field6ee==((flags&1)?23:0)&&out.field1dfc==((flags&1)?-12345:0),"Sasuke pre flags/status updates, no model3 vector branch admitted");}
    for(unsigned mode=0;mode<256;++mode)for(unsigned count=0;count<256;++count){s.field6e9=u8(mode);s.field6ec=u8(count);s.flags5e0=(count&1)?0x02000000:0;s.field7c8=43;
        check(startup_actor_class_post(p,s,&out,&effects)&&out.field6e9==mode&&out.field6ec==((mode>0&&mode<128)?((count+1)&3):count)&&out.field7c8==((count&1)?0:43)&&effects.empty(),"all Sasuke signed selectors and byte counters; persistent selector differs from G-Red");}
    s.field6e9=1;s.field6ec=0;for(unsigned i=0;i<5;++i){check(startup_actor_class_post(p,s,&s,&effects)&&s.field6ec==((i+1)&3)&&s.field6e9==1,"Sasuke persistent post selector advances on each callback");}
    s=state(3);out=state(10);auto old=fields(out);check(!startup_actor_class_initialize(p,s,&out,&effects)&&fields(out)==old&&effects.empty(),"other same-family model IDs refuse atomically, not broadened startup coverage");
    s=state(10);check(!startup_actor_class_pre(p,s,&out)&&!startup_actor_class_post(p,s,&out,&effects)&&fields(out)==old,"pre/post require explicit class initializer invocation");
    check(!startup_actor_class_initialize(p,s,nullptr,&effects)&&!startup_actor_class_initialize(p,s,&out,nullptr),"missing outputs refuse");
    for(unsigned at:{0u,4u,6u,8u,12u,16u,18u}){auto bad=bytes;bad[at]^=1;StartupActorClassProfile rejected=p;check(!startup_actor_class_load({bad.data(),bad.size()},&rejected,&error)&&!rejected.loaded,"bad headers/identity scope invalidate profile");}
    for(u32 invalid:{0x7fc00000u,0xbf800000u,0x40000000u}){auto bad=bytes;store_be32(bad.data()+20,invalid);check(!startup_actor_class_load({bad.data(),bad.size()},&p,&error),"nonfinite/negative/overflow damping domain refuses");}
    check(startup_actor_class_load({bytes.data(),bytes.size()},&p,&error),"restore owned profile");
    unsigned source_profiles=0;std::ifstream file("local/content/startup-actor-class/startup.gfac",std::ios::binary);std::vector<u8> actual{std::istreambuf_iterator<char>(file),{}};
    if(!actual.empty()){check(actual.size()==bytes.size()&&std::equal(actual.begin(),actual.end(),bytes.begin())&&startup_actor_class_load({actual.data(),actual.size()},&p,&error),"actual imported profile equals source operands and reviewed identities");source_profiles=1;}
    std::printf("{\"startup_actor_class_checks\":%u,\"source_profiles\":%u,\"signed16_pre_inputs_checked\":131072,\"Sasuke_post_byte_pairs\":65536,\"actual_construction_placement_effect_objects_or_visual_startup_admitted\":false,\"all_passed\":true}\n",checks,source_profiles);
}
