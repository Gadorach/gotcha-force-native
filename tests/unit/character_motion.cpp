#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/character_motion.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"character motion: %s\n",why);std::abort();}}
void word(std::vector<gf::u8>& b,std::size_t at,gf::u32 v){gf::store_be32(b.data()+at,v);}
void scalar(std::vector<gf::u8>& b,std::size_t at,float v){gf::store_be_f32(b.data()+at,v);}
void unrelocate(std::vector<gf::u8>& b,gf::u32 field){
    const auto at=32+gf::load_be32(b.data()+4),count=gf::load_be32(b.data()+8);
    for(unsigned i=0;i<count;++i)if(gf::load_be32(b.data()+at+i*4)==field){word(b,at+i*4,0x1e0);return;}
    std::abort();
}
void le(std::vector<gf::u8>& b,float v){const auto bits=gf::f32_to_bits(v);for(unsigned i=0;i<4;++i)b.push_back(gf::u8(bits>>(i*8)));}
std::vector<gf::u8> clip_bytes(float constant=7){
    std::vector<gf::u8> b(0x200,0);std::vector<gf::u32> fields;
    auto ptr=[&](unsigned at,unsigned value){word(b,at,value);fields.push_back(at);};
    // Children/siblings are deliberately not a plausible traversal order. The
    // source attaches each public's AObj separately; NEVER map this tree again.
    word(b,0x20,0xdeadbeef);word(b,0x24,0x20);ptr(0x28,0x80);ptr(0x48,0xa0);word(b,0x50,1);
    scalar(b,0x84,10);scalar(b,0xa4,10);ptr(0x88,0xc0);ptr(0xa8,0xe0);
    std::vector<gf::u8> lin,con;
    for(float v:{0.0f,10.0f,10.0f}){lin.push_back(2);le(lin,v);lin.push_back(10);}
    for(unsigned i=0;i<2;++i){con.push_back(1);le(con,constant);con.push_back(i?10:0);}
    word(b,0xc4,lin.size());word(b,0xe4,con.size());b[0xcc]=5;b[0xec]=1;ptr(0xd0,0x140);ptr(0xf0,0x160);
    std::copy(lin.begin(),lin.end(),b.begin()+0x140);std::copy(con.begin(),con.end(),b.begin()+0x160);
    std::vector<gf::u8> out(32,0);out.insert(out.end(),b.begin(),b.end());
    for(auto field:fields){auto at=out.size();out.resize(at+4);word(out,at,field);}
    auto at=out.size();out.resize(at+16);word(out,at,0x20);word(out,at+8,0x40);word(out,at+12,6);
    const char names[]="boneZ\0boneA";out.insert(out.end(),names,names+sizeof(names));
    word(out,0,out.size());word(out,4,b.size());word(out,8,fields.size());word(out,12,2);return out;
}
gf::HsdModelBones binding(){gf::HsdModelBones b;b.loaded=true;b.root_offset=0x200;b.preorder_slots={1,0,2};b.bones.resize(3);
    for(unsigned i=0;i<3;++i)b.bones[i].joint_offset=0x100+i*0x100;
    return b;
}
std::vector<gf::u8> bank_bytes(){
    auto clip=clip_bytes();auto other=clip_bytes(9);std::vector<gf::u8> b(0x100,0);word(b,0,0x40);word(b,8,0x40);
    word(b,0x40,0x100);word(b,0x44,0);word(b,0x48,0x100+clip.size());word(b,0x4c,0xffffffff);
    // Pad to four-byte boundaries for the unrelocated table's offset domain.
    while(clip.size()%4)clip.push_back(0);
    word(b,0x48,0x100+clip.size());b.insert(b.end(),clip.begin(),clip.end());b.insert(b.end(),other.begin(),other.end());return b;
}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    auto b=bank_bytes(),c=clip_bytes();gf::CharacterMotionBank bank;gf::CharacterMotionSelection selected;gf::HsdBoneMotionClip clip;gf::HsdBoneMotionSample sample;std::string error;
    auto load_bank=[&](const auto& x){return gf::character_motion_bank_load({x.data(),x.size()},&bank,&error);};
    auto load_clip=[&](const auto& x,const auto& bones){return gf::hsd_bone_motion_load({x.data(),x.size()},bones,&clip,&error);};
    check(load_bank(b)&&bank.archives.size()==2&&bank.groups[0].size()==3,"sixteen source tables, null entry and distinct/aliased archives parsed");
    check(gf::character_motion_select(bank,0,0,&selected,&error)&&!selected.fallback,"ordinary selection keeps explicit default origin");
    check(load_clip(std::vector<gf::u8>(selected.archive.data,selected.archive.data+selected.archive.size),binding()),"selected owned archive binds public INDEX, not tree or sorted name");
    check(clip.bones.size()==3&&clip.public_bone_count==2&&clip.bones[1].classical_scale&&!clip.bones[2].has_aobj,"AnimJoint flags/public extent retained; missing public is distinct from null AObj");
    check(gf::hsd_bone_motion_sample(clip,0,&sample,&error)&&sample.bones[0].joint_offset==0x100&&sample.bones[1].joint_offset==0x200&&sample.bones[0].value[5]==0&&sample.bones[1].value[1]==7,"requested time0 includes FIRST_PLAY initial values in logical slots");
    check(sample.bones[0].present==(1u<<5)&&sample.bones[1].present==(1u<<1)&&sample.bones[2].present==0,"absent channels are not fabricated defaults or inverse binds");
    check(gf::hsd_bone_motion_sample(clip,5,&sample,&error)&&sample.bones[0].value[5]==5,"requested integer-time linear interpolation");
    check(gf::hsd_bone_motion_sample(clip,10,&sample,&error)&&sample.bones[0].value[5]==10,"declared end sample accepted");
    check(!gf::hsd_bone_motion_sample(clip,11,&sample,&error)&&sample.bones.empty(),"beyond-end time refuses, no automatic loop/wall clock invention");
    check(gf::hsd_bone_motion_sample_requested(clip,30,&sample,&error)&&sample.bones[0].value[5]==10,"explicit source request beyond clip end samples without changing the strict legacy contract");
    // Drop the owned duplicate hold key; the raw FObj extrapolates, proving
    // explicit request30 was not silently clamped to end10.
    auto tail=clip;tail.bones[0].tracks[0].data.resize(12);
    check(gf::hsd_bone_motion_sample_requested(tail,30,&sample,&error)&&sample.bones[0].value[5]==30,"source requested clock retained beyond end, no endpoint substitution");
    check(!gf::hsd_bone_motion_sample_requested(clip,8193,&sample,&error)&&sample.bones.empty(),"explicit beyond-end path retains bounded evaluation cap");
    tail.bones[0].tracks[0].data.back()=0x80;
    check(!gf::hsd_bone_motion_sample_requested(tail,30,&sample,&error)&&sample.bones.empty(),"beyond-end malformed wait refuses atomically");
    check(gf::character_motion_select(bank,0,1,&selected,&error)&&selected.fallback&&selected.index==0,"null entry uses group0/entry0 fallback");
    check(gf::character_motion_select(bank,1,99,&selected,&error)&&selected.fallback,"absent group falls back exactly like source");
    check(gf::character_motion_select(bank,2,2,&selected,&error)&&!selected.fallback&&selected.index==1,"exact table alias and nondefault clip selection");
    check(gf::hsd_bone_motion_load(selected.archive,binding(),&clip,&error)&&gf::hsd_bone_motion_sample(clip,0,&sample,&error)&&sample.bones[1].value[1]==9,"selected archive actually changes the sampled motion");
    b.clear();c.clear();check(gf::character_motion_select(bank,0,0,&selected,&error),"bank owns archive bytes after encoded input lifetime");
    bank={};check(gf::hsd_bone_motion_sample(clip,5,&sample,&error)&&sample.bones[1].value[1]==9,"clip owns track bytes after bank lifetime");b=bank_bytes();c=clip_bytes();
    auto bank_bad=[&](auto x,const char* why){check(!load_bank(x)&&!bank.loaded&&bank.archives.empty(),why);};
    auto x=b;word(x,0,4);bank_bad(x,"group table cannot start inside header");
    x=b;word(x,0,0xfffffffcu);bank_bad(x,"table offset overflow refuses");
    x=b;word(x,0x40,0);bank_bad(x,"source default must exist");
    x=b;word(x,0x40,0x101);bank_bad(x,"live/odd tagged pointer is not an unrelocated archive offset");
    x=b;word(x,0x40,0xfffffffcu);bank_bad(x,"archive offset overflow refuses");
    x=b;word(x,0x100,0xffffffffu);bank_bad(x,"archive size overflow refuses");
    x=b;word(x,0x100+12,4097);bank_bad(x,"embedded archive symbol budget refuses");
    x=b;word(x,0x48,0x104);bank_bad(x,"overlapping archives refuse");
    x=b;word(x,8,0x44);bank_bad(x,"partially overlapping table alias refuses");
    x=b;word(x,0x4c,0);bank_bad(x,"unterminated table cannot run through following zero padding into archives");
    x=b;word(x,0x40,0x40);bank_bad(x,"table cannot masquerade as an archive");
    check(load_bank(b),"reload owned bank");
    check(!gf::character_motion_select(bank,0,3,&selected,&error)&&!selected.archive.data,"out-of-range index isn't silently substituted for null");
    check(!gf::character_motion_select(bank,0x80,0,&selected,&error),"common-bank flag cannot silently select actor bank");
    check(!gf::character_motion_select(bank,0,255,&selected,&error),"negative signed choice refuses");
    auto clip_bad=[&](auto y,const char* why){check(!load_clip(y,binding())&&!clip.loaded&&clip.bones.empty(),why);};
    x=c;word(x,32+0x2c,0x40);clip_bad(x,"non-null constraint cannot be ignored");
    x=c;word(x,32+0x30,2);clip_bad(x,"unknown AnimJoint flags refuse");
    x=c;word(x,32+0x80,1);clip_bad(x,"AObj loop/lifecycle flags unqualified");
    x=c;word(x,32+0x80,0x20000000);clip_bad(x,"gameplay path still refuses authored loop lifecycle");
    auto inspect_clip=[&](const auto& y){return gf::hsd_bone_motion_load({y.data(),y.size()},binding(),&clip,&error,
                                                                        gf::HsdBoneMotionLoadMode::InspectAuthoredLoop);};
    check(inspect_clip(x)&&clip.bones[0].source_loop&&!clip.bones[1].source_loop&&
          gf::hsd_bone_motion_sample(clip,5,&sample,&error)&&sample.bones[0].value[5]==5,
          "inspection preserves per-bone loop flag and samples existing SRT");
    check(!gf::hsd_bone_motion_sample(clip,11,&sample,&error),"loop metadata does not change integer inspection sampling into wrapping");
    word(x,32+0x80,0x20000001);
    check(!inspect_clip(x)&&!clip.loaded,"inspection still refuses unknown bits alongside the loop flag");
    auto native_idle=[&](const auto& y){return gf::hsd_bone_motion_load({y.data(),y.size()},binding(),&clip,&error,
                                                                       gf::HsdBoneMotionLoadMode::NativeIdleLoop);};
    x=c;word(x,32+0x80,0x20000000);
    check(!native_idle(x)&&!clip.loaded,"native idle refuses mixed authored loop bits");
    word(x,32+0xa0,0x20000000);
    check(native_idle(x)&&clip.bones[0].source_loop&&clip.bones[1].source_loop&&
          gf::hsd_bone_motion_sample(clip,5,&sample,&error),"explicit idle accepts the uniformly flagged sampled tracks");
    auto action_window=[&](const auto& y){return gf::hsd_bone_motion_load({y.data(),y.size()},binding(),&clip,&error,
                                                                          gf::HsdBoneMotionLoadMode::NativeActionLoopWindow);};
    check(action_window(x)&&gf::hsd_bone_motion_sample(clip,10,&sample,&error)&&
          !gf::hsd_bone_motion_sample(clip,11,&sample,&error),"action window accepts the flag without wrapping");
    auto mixed=x;word(mixed,32+0xa0,0);
    check(!action_window(mixed)&&!clip.loaded,"action window refuses mixed loop flags");
    word(x,32+0x2c,0x40);
    check(!native_idle(x)&&!clip.loaded,"idle loop does not admit an AnimJoint constraint");
    check(!action_window(x)&&!clip.loaded,"action loop window does not admit an AnimJoint constraint");
    x=c;scalar(x,32+0x84,NAN);clip_bad(x,"nonfinite clip end refuses");
    x=c;scalar(x,32+0x84,8193);clip_bad(x,"clip end exceeds native work budget");
    x=c;scalar(x,32+0xa4,9);clip_bad(x,"mixed bone end times require separate lifecycle qualification");
    x=c;x[32+0xcc]=4;clip_bad(x,"spline path channel isn't guessed as scalar SRT");
    x=c;x[32+0xcc]=12;clip_bad(x,"branch visibility callback requires separate hierarchy effects");
    x=c;word(x,32+0xc0,0xc0);clip_bad(x,"cyclic track chain refuses");
    x=c;word(x,32+0xc0,0xe0);x[32+0xec]=5;clip_bad(x,"duplicate per-bone channel refuses instead of inventing callback order");
    x=c;word(x,32+0xc4,0);clip_bad(x,"empty packed payload refuses");
    x=c;x[32+0x140]=0xf;clip_bad(x,"unsupported packed opcode refused during preflight");
    x=c;x[32+0xcd]=0xe0;clip_bad(x,"unsupported value encoding refused during preflight");
    x=c;scalar(x,32+0xc8,NAN);clip_bad(x,"nonfinite start frame refuses");
    x=c;scalar(x,32+0xc8,-1);clip_bad(x,"negative start outside supported subset refuses");
    x=c;word(x,32+0xd0,0xfffffffcu);clip_bad(x,"track payload overflow refuses");
    x=c;const auto sy=32+gf::load_be32(x.data()+4)+gf::load_be32(x.data()+8)*4;word(x,sy+12,0);clip_bad(x,"duplicate public names would alter source's index-to-name lookup");
    auto bones=binding();bones.bones[1].joint_offset=bones.bones[0].joint_offset;
    check(!load_clip(c,bones),"ambiguous logical bone identities refuse");
    bones=binding();bones.bones[0].joint_offset=0;bones.root_offset=0;
    check(!load_clip(c,bones),"missing bone identity still refuses");
    bones.root_at_block_zero=true;
    check(load_clip(c,bones)&&gf::hsd_bone_motion_sample(clip,0,&sample,&error)&&sample.bones[0].joint_offset==0,
          "validated block-zero root can be bound and sampled");
    bones.bones[1].joint_offset=0;
    check(!load_clip(c,bones),"block-zero root does not allow other missing identities");
    x=c;word(x,32+0x28,0);clip_bad(x,"relocated zero AObj isn't silently treated as absent animation");
    unrelocate(x,0x28);check(load_clip(x,binding())&&gf::hsd_bone_motion_sample(clip,0,&sample,&error)&&sample.bones[0].present==0,"unrelocated null AObj is distinct from malformed data, no fabricated channels");
    x=c;std::copy_n(x.begin()+32+0x140,18,x.begin()+32);word(x,32+0xd0,0);
    check(load_clip(x,binding())&&gf::hsd_bone_motion_sample(clip,5,&sample,&error)&&sample.bones[0].value[5]==5,"relocated zero payload resolves to block0, not null");
    const auto reloc_at=32+gf::load_be32(x.data()+4),relocs=gf::load_be32(x.data()+8);
    for(unsigned i=0;i<relocs;++i)if(gf::load_be32(x.data()+reloc_at+i*4)==0xd0)word(x,reloc_at+i*4,0x1e0);
    clip_bad(x,"unrelocated zero payload is NOT accepted as block0");
    check(load_clip(c,binding()),"restore owned clip");clip.bones[0].tracks[0].data[0]=15;
    check(!gf::hsd_bone_motion_sample(clip,0,&sample,&error)&&sample.bones.empty(),"mutable parsed asset still refuses bad sample atomically");
    bool truncated=true;for(std::size_t n=0;n<c.size();++n)truncated=truncated&&!gf::hsd_bone_motion_load({c.data(),n},binding(),&clip,&error);
    check(truncated,"every truncated clip view refuses");
    truncated=true;for(std::size_t n=0;n<64;++n)truncated=truncated&&!gf::character_motion_bank_load({b.data(),n},&bank,&error);
    check(truncated,"every truncated bank header view refuses");
    unsigned seed=0x71c0deu;
    for(unsigned i=0;i<512;++i){
        seed=seed*1664525u+1013904223u;x=i&1?c:b;const auto at=seed%x.size();seed=seed*1664525u+1013904223u;x[at]^=gf::u8((seed>>24)|1);
        if(i&1){const bool ok=load_clip(x,binding());if(!ok&&(clip.loaded||!clip.bones.empty()))std::abort();}
        else {const bool ok=load_bank(x);if(!ok&&(bank.loaded||!bank.archives.empty()))std::abort();}
    }
    check(true,"512 deterministic malformed bank/clip probes preserve atomic failure");
    unsigned real=0,samples=0;
    for(const char* name:{"g-red","sasuke"}){
        auto prefix=read(std::string("local/content/startup-characters/")+name+"/model-prefix.bin");auto model=read(std::string("local/content/startup-characters/")+name+"/model.hsd");
        auto encoded=read(std::string("local/recomp-evidence/native-character-motion-20261109/resources-v1/")+name+"-motion-bank.bin");if(prefix.empty()||model.empty()||encoded.empty())continue;
        gf::HsdModelBones bound;check(gf::hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bound,&error),"original wrapper logical bone binding");
        check(load_bank(encoded),"original PZZ-equal named motion bank parses without runtime PZZ/AFS");
        for(unsigned choice:{0,1,3,4,7,9}){
            check(gf::character_motion_select(bank,0,choice,&selected,&error)&&!selected.fallback,"source-selected startup motion bank0 choice");
            const bool ok=gf::hsd_bone_motion_load(selected.archive,bound,&clip,&error);if(!ok)std::fprintf(stderr,"%s motion%u: %s\n",name,choice,error.c_str());
            check(ok,"actual public bone tracks load without tree-binding heuristic");
            const unsigned request=choice==9?6:choice;
            auto loose=read(std::string("local/content/startup-motion-clips/")+name+"/startup-choice-"+(request<10?"0":"")+std::to_string(request)+".hsd");
            if(!loose.empty())check(loose.size()==selected.archive.size&&std::equal(loose.begin(),loose.end(),selected.archive.data),"loose logical clip equals bank-selected source bytes");
            bool moving=false;gf::HsdBoneMotionSample first;
            check(gf::hsd_bone_motion_sample(clip,0,&first,&error),"actual requested FIRST_PLAY time0");
            for(unsigned time=0;time<=unsigned(clip.end_frame);++time){
                check(gf::hsd_bone_motion_sample(clip,time,&sample,&error),"all actual declared integer samples stay bounded");++samples;
                for(unsigned bone=0;bone<sample.bones.size();++bone)for(unsigned channel=1;channel<=10;++channel)
                    if(sample.bones[bone].present&(1u<<channel))moving=moving||std::abs(first.bones[bone].value[channel]-sample.bones[bone].value[channel])>1e-5f;
            }
            check(moving,"actual clip has changing original channels (not supplied diagnostic matrix motion)");
        }
        ++real;
    }
    std::printf("{\"character_motion_checks\":%u,\"real_models\":%u,\"real_integer_pose_channel_samples\":%u,\"assembled_pose_or_startup_admission\":false,\"all_passed\":true}\n",checks,real,samples);
}
