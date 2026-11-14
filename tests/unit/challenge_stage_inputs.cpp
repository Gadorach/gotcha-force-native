#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/challenge_stage_inputs.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"stage inputs: %s\n",why);std::abort();}}
void p32(std::vector<gf::u8>& b,unsigned at,gf::u32 x){for(unsigned i=0;i<4;++i)b[at+i]=gf::u8(x>>(24-8*i));}
std::vector<gf::u8> grid(bool floor){
    using namespace gf;std::vector<u8> b(108);p32(b,0,0x53544948);p32(b,4,108);p32(b,8,500);p32(b,12,500);p32(b,16,1);p32(b,20,1);p32(b,32,32);p32(b,36,44);p32(b,40,36);p32(b,44,floor?0:0xffffffffu);p32(b,48,0xffffffffu);p32(b,52,42);
    const f32 values[13]={0,10,0,0,10,500,500,10,0,0,1,0,-10};for(unsigned i=0;i<13;++i)p32(b,56+i*4,f32_to_bits(values[i]));return b;
}
}
int main(){
    using namespace gf;auto floor=grid(true),ceil=grid(false);std::vector<u8> model(32);p32(model,0,32);ChallengeStageInputRow row;row.stage=7;row.model_bytes=32;row.ground={-7500,7500,500,0,-.5f,5000,-100,true};std::array<ByteView,4> slots{{{model.data(),model.size()},{floor.data(),floor.size()},{ceil.data(),ceil.size()},{ceil.data(),ceil.size()}}};ChallengeStageInputs stage;std::string error;StageGroundActor actor;actor.fields_declared=true;actor.current24=actor.previous30=20;f32 y=99;
    check(!stage.ready()&&!stage.grid(0)&&!stage.model_input().data&&!stage.ground(actor,{100,20,100},1,&y,&error)&&y==0,"unready inputs do not replay a height");
    check(stage.load_declared(row,slots,&error)&&stage.ready()&&!stage.native_data_bound()&&stage.model_input().size==32&&stage.grid(0)&&!stage.grid(3),"owned declared slot binding, not native receipt/model render admission");
    check(stage.ground(actor,{100,20,100},1,&y,&error)&&y==10,"same shared static ground query");
    check(stage.ground(actor,{7500,123,100},1,&y,&error)&&y==123,"source strict outer bounds retained");
    actor.attached1db=1;check(!stage.ground(actor,{100,20,100},1,&y,&error)&&y==0,"moving-platform branch still refuses");actor.attached1db=0;actor.fields_declared=false;check(!stage.ground(actor,{100,20,100},1,&y,&error)&&y==0,"no invented actor fields");actor.fields_declared=true;
    model[0]=0xff;check(stage.model_input().be32(0)==32,"declared model input is owned, not a borrowed buffer");model[0]=0;
    const auto copy=stage;auto bad=slots;bad[2]={};check(!stage.load_declared(row,bad,&error)&&!stage.ready()&&!stage.grid(0)&&!stage.model_input().data,"late grid failure clears all old inputs");check(copy.ground(actor,{100,20,100},1,&y,&error)&&y==10,"separate immutable copy retains its own input lifetime");
    auto badrow=row;badrow.model_bytes=33;check(!stage.load_declared(badrow,slots,&error),"no guessed model extent");badrow=row;badrow.ground.declared=false;check(!stage.load_declared(badrow,slots,&error),"ground parameter declaration required");
    std::vector<u8> bytes(80);p32(bytes,0,0x4746434d);p32(bytes,4,1);bytes[9]=1;bytes[11]=48;p32(bytes,12,48);p32(bytes,32,999);p32(bytes,36,0x12345678);p32(bytes,40,7);p32(bytes,44,32);const f32 params[7]={-7500,7500,500,0,-.5f,5000,-100};for(unsigned i=0;i<7;++i)p32(bytes,48+4*i,f32_to_bits(params[i]));ChallengeStageInputsProfile profile;
    check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready()&&profile.rows().size()==1&&profile.find(999)&&!profile.find(1000),"owned portable parameter profile");
    for(unsigned at:{0u,4u,9u,11u,15u,16u,31u,32u,40u,60u,76u}){auto b=bytes;b[at]^=1;check(!profile.load({b.data(),b.size()},&error)&&!profile.ready(),"profile header/reserved/domain checks clear old data");}
    auto short_model=bytes;p32(short_model,44,31);check(!profile.load({short_model.data(),short_model.size()},&error),"source model minimum extent");
    auto duplicate=bytes;duplicate.insert(duplicate.end(),bytes.begin()+32,bytes.end());duplicate[9]=2;p32(duplicate,12,96);check(!profile.load({duplicate.data(),duplicate.size()},&error),"duplicate main resource ID");
    check(!profile.load({nullptr,80},&error)&&!profile.load({bytes.data(),79},&error),"null/short profile");
    check(profile.load({bytes.data(),bytes.size()},&error)&&!stage.load(std::make_shared<const ChallengeNativeStageData>(),profile,&error)&&!stage.ready()&&!stage.native_data_bound(),"empty native cache cannot become bound input readiness");
    std::printf("stage inputs: %u owned checks PASS\n",checks);
}
