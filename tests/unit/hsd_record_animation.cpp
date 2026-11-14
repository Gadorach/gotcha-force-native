// Owned safety/lifetime/callback cases; optional original TL00 record/track sweep.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"record animation: %s\n",why);std::abort();}}
gf::ByteView view(const std::vector<gf::u8>& bytes){return {bytes.data(),bytes.size()};}
bool load(const hsd_record_animation_fixture::Fixture& f,gf::HsdRecordAnimationBank& bank,std::string& error,int omit=-1){auto b=f.archive(omit);return bank.load(view(b),&error);}
void material_cases(){
    for(unsigned channel=1;channel<=10;++channel)for(float value:{-2.f,-.01f,0.f,.1f,.5f,.999f,1.f,1.01f,2.f}){
        gf::HsdAnimPose::Material m;check(gf::hsd_material_callback(channel,value,&m)&&m.present,"finite callback");
        if(channel==10){const float expected=std::min(1.f,std::max(0.f,float(1.f-value)));check(m.alpha==expected,"subtract before alpha clamp");}
        else {const auto byte=unsigned(255.f*std::min(1.f,std::max(0.f,value)));const float colour=(channel<=3?m.ambient[channel-1]:channel<=6?m.diffuse[channel-4]:m.specular[channel-7]);check(colour==float(byte)/255.f,"RGB clamp/multiply/truncate");}
    }
    gf::HsdAnimPose::Material sentinel;sentinel.alpha=.3f;
    for(float value:{NAN,INFINITY,-INFINITY,1e8f})check(!gf::hsd_material_callback(4,value,&sentinel)&&!sentinel.present&&sentinel.alpha==.3f,"bad callback no mutation");
    check(!gf::hsd_material_callback(0,0,&sentinel)&&!gf::hsd_material_callback(11,0,&sentinel)&&!gf::hsd_material_callback(4,0,nullptr),"channel/null guards");
    for(unsigned b=0;b<=255;++b){gf::u8 out=0;check(gf::hsd_material_draw_alpha(float(b)/255.f,&out)&&out==b,"alpha byte roundtrip");}
    gf::u8 out=42;check(!gf::hsd_material_draw_alpha(NAN,&out)&&out==42&&!gf::hsd_material_draw_alpha(1,nullptr),"bad draw alpha no mutation");
    check(!gf::hsd_material_draw_alpha(-.1f,&out)&&!gf::hsd_material_draw_alpha(1.1f,&out)&&out==42,"draw boundary requires normalized stored alpha; no invented clamp");
}
void owned_cases(){
    using hsd_record_animation_fixture::Fixture;Fixture f;gf::HsdRecordAnimationBank bank;gf::HsdRecordAnimationBinding binding;gf::HsdRecordAnimationPlayer player;std::string error;
    check(load(f,bank,error),error.c_str());check(bank.records()==1&&bank.hash(),"published bank");
    check(bank.select(0,3,0,0,&binding,&error),error.c_str());check(binding.scene()->joints.size()==3&&binding.scene()->meshes.size()==3&&binding.objects()->size()==5,"explicit bindings/geometry");
    check(player.bind(binding,&error)&&!player.pose()&&player.request(0,1,&error),"first requested sample");
    check(player.pose()->joints[0].translation[0]==0&&player.pose()->materials[0].diffuse[0]==1&&player.pose()->materials[1].alpha==1&&player.pose()->materials[2].specular[0]==127/255.f,"material callbacks with overshoot");
    auto copied=player;check(player.advance(.5f,&error)&&player.pose()->joints[0].translation[0]==.5f&&copied.pose()->joints[0].translation[0]==0,"staged copies independent clocks");
    check(player.advance(3.5f,&error)&&player.pose()->joints[0].translation[0]==4&&player.trace()[0].stopped,"per-AObj source endpoint");
    const auto stopped=player.trace();check(player.advance(1,&error)&&player.trace()[0].tracks[0].callbacks==stopped[0].tracks[0].callbacks&&player.trace()[0].frame==4,"stopped AObj no replay");
    check(!player.advance(NAN,&error)&&!player.pose()&&player.trace().empty(),"late refusal invalidates publication");
    check(!player.request(0,1,&error),"explicit rebind required after refusal");
    check(copied.advance(0,&error)&&copied.pose()->joints[0].translation[0]==0,"zero-rate update");
    auto moved=std::move(copied);check(moved.pose()&&!copied.pose()&&!copied.advance(1,&error),"moved player owns source/tracks");
    check(!bank.select(0,4,0,0,&binding,&error)&&!binding.scene(),"bad selection clears old binding");
    check(!bank.select(1,3,0,0,&binding,&error)&&!bank.select(0,0,1,0,&binding,&error)&&!bank.select(0,3,1,0,&binding,&error)&&!bank.select(0,3,0,0,nullptr,&error),"selection domain");
    check(bank.select(0,0,0,0,&binding,&error)&&binding.objects()->empty(),"static explicit channel mask");
    // Bank reload/input-buffer destruction cannot invalidate already bound players.
    check(!bank.load({},&error)&&!bank.records()&&moved.advance(1,&error),"retained immutable bank survives failed reload");
    auto reject=[&](Fixture bad,int omit=-1){gf::HsdRecordAnimationBank b;gf::HsdRecordAnimationBinding selected;const bool accepted=load(bad,b,error,omit)&&b.select(0,3,0,0,&selected,&error);check(!accepted&&!selected.scene(),"malformed/unsupported binding refuses atomically");};
    reject(f,0x34);reject(f,0x770);reject(f,0x904);reject(f,0xa30);
    {auto b=f;b.ptr(0x740,0x740);reject(b);}
    {auto b=f;b.ptr(0x600,0x600);reject(b);}
    {auto b=f;b.ptr(0x800,0x800);reject(b);}
    {auto b=f;b.ptr(0x900,0x900);reject(b);}
    {auto b=f;b.ptr(0x908,0x100);reject(b);}
    {auto b=f;b.word(0x700,1);reject(b);}
    {auto b=f;b.real(0x704,0);reject(b);}
    {auto b=f;b.real(0x704,INFINITY);reject(b);}
    {auto b=f;b.real(0x748,-1);reject(b);}
    {auto b=f;b.word(0x744,65537);reject(b);}
    {auto b=f;b.block[0x74c]=4;reject(b);}
    {auto b=f;b.block[0xa0c]=11;reject(b);}
    {auto b=f;b.block[0x76c]=5;reject(b);}
    {auto b=f;b.block[0x74f]=1;reject(b);}
    {auto b=f;b.ptr(0x70c,0x100);reject(b);}
    {auto b=f;b.ptr(0x3c,0x70);b.ptr(0x70,0x100);reject(b);}
    {auto b=f;b.ptr(0x10,0xffffffffu);reject(b);}
    {auto b=f;b.ptr(0x100,0x200);reject(b);}
    {auto b=f;b.ptr(0x13c,0x200);reject(b);}
    // NODE changes only the addressed node; BRANCH changes descendants as well.
    for(unsigned branch:{11u,12u}){auto b=f;b.block[0x76c]=gf::u8(branch);b.stream(0x7a0,0,0);check(load(b,bank,error)&&bank.select(0,3,0,0,&binding,&error)&&player.bind(binding,&error)&&player.request(0,1,&error),"visibility fixture");
        check(player.pose()->joints[0].hidden&&!player.pose()->joints[1].hidden&&player.pose()->joints[2].hidden==(branch==12),"node versus recursive branch; child callback order retained");}
    // Truncated/unknown track refuses only at interpretation, no old pose replay.
    {auto b=f;b.block[0xc40]=7;check(load(b,bank,error)&&bank.select(0,3,0,0,&binding,&error)&&player.bind(binding,&error),"late payload fixture binds");check(!player.request(0,1,&error)&&!player.pose(),"late material failure discards earlier joint callbacks");}
    {auto b=f;b.word(0xa04,1);check(load(b,bank,error)&&bank.select(0,3,0,0,&binding,&error)&&player.bind(binding,&error),"truncated payload binds metadata");check(!player.request(0,1,&error)&&!player.pose(),"truncated scalar refuses safely");}
    check(load(f,bank,error)&&bank.select(0,3,0,0,&binding,&error)&&player.bind(binding,&error)&&player.request(8,1,&error)&&player.trace()[0].frame==8&&player.trace()[0].stopped,"beyond-end FIRST_PLAY is not clamped");
    check(player.bind(binding,&error)&&!player.request(-1,1,&error),"start domain");
    check(player.bind(binding,&error)&&!player.request(0,17,&error),"rate domain");
    {auto b=f;b.real(0x704,1);b.stream(0x780,2,8,4,6);check(load(b,bank,error)&&bank.select(0,3,0,0,&binding,&error)&&player.bind(binding,&error)&&player.request(0,1,&error),"KEY stop fixture");
        check(player.advance(1,&error)&&player.trace()[0].stopped&&player.trace()[0].frame==1,"KEY flush does not advance AObj frame twice");
        const auto t=player.trace()[0].tracks[0];check(t.time==2&&t.state==0&&player.pose()->joints[0].translation[0]==2,"only current KEY interpolation flushes at requested rate");
        check(!player.trace()[1].stopped&&player.trace()[1].frame==1,"different AObjs retain their own endpoints");}
    {auto b=f;b.real(0x748,1);check(load(b,bank,error)&&bank.select(0,3,0,0,&binding,&error)&&player.bind(binding,&error)&&player.request(.5f,.5f,&error)&&player.pose()->joints[0].translation[0]==1.5f,"fractional FObj start plus requested frame");}
    {auto b=f;b.ptr(0x34,0);check(!load(b,bank,error),"relocated zero descriptor refuses, not null animation");}
    check(load(f,bank,error)&&bank.select(0,3,0,0,&binding,&error),"restore work-budget binding");
    check(player.bind(binding,&error)&&player.request(0,0,&error),"work budget fixture");
    for(unsigned i=0;i<8192;++i)if(!player.advance(0,&error))check(false,"bounded prefix");
    check(!player.advance(0,&error)&&!player.pose(),"work budget refuses instead of forcing completion");
}
void scene_cases(){
    hsd_record_animation_fixture::Fixture f;gf::HsdRecordAnimationBank bank;std::string error;check(load(f,bank,error),"scene bank");
    gf::AnimatedModelScene scene;check(scene.load(bank,0,3,0,0,&error)&&!scene.ready()&&!scene.pose(),"no scene publication before request");
    gf::headless::Render backend;gf::RenderEngine render(backend);gf::RenderConfig config;check(render.initialize(config,nullptr,&error),"owned headless renderer");
    gf::Camera camera;camera.eye[0]=camera.eye[1]=0;camera.eye[2]=20;camera.target[0]=camera.target[1]=camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=100;
    check(!scene.draw(render,camera),"unrequested scene draws nothing");check(scene.request(0,1,&error),error.c_str());
    scene.set_enabled(true);render.begin_frame({});check(scene.draw(render,camera)==3,"shared scene submits all rigid meshes");
    auto world=gf::Mat4{};world.m[3]=2;check(scene.set_root_world(world,&error)&&scene.pose()->joints[0].matrix_override&&scene.pose()->joints[0].matrix[3]==2,"source root matrix replaces authored root SRT");
    check(scene.advance(1,&error)&&scene.pose()->joints[0].translation[0]==1&&scene.pose()->joints[0].matrix[3]==2,"child hierarchy survives root override; no second root translation");
    auto moved=std::move(scene);check(!scene.ready()&&!scene.pose()&&!scene.draw(render,camera)&&moved.ready()&&moved.advance(1,&error),"moved scene owns accepted draw");
    check(!moved.advance(INFINITY,&error)&&!moved.ready()&&!moved.pose()&&!moved.draw(render,camera),"failed scene never replays prior valid pose");
    check(!moved.request(0,1,&error),"failed scene requires load");
    check(moved.load(bank,0,3,0,0,&error)&&moved.request(0,1,&error),"reload restores scene");
    world.m[0]=0;check(!moved.set_root_world(world,&error)&&!moved.pose(),"singular root refuses");
    moved.release();check(!moved.asset()&&!moved.ready()&&!moved.draw(render,camera),"explicit scene release");
    // Metadata may bind a zero-scale track, but raw singular geometry is not admitted.
    f.block[0x74c]=8;f.stream(0x780,0,0);check(load(f,bank,error)&&moved.load(bank,0,3,0,0,&error),"singular scale fixture binds");check(!moved.request(0,1,&error)&&!moved.pose(),"no singular-scale renderer weakening");
}
void journal(FILE* out,unsigned record,unsigned schedule,unsigned update,const gf::HsdRecordAnimationPlayer& player){
    if(!out)return;
    std::fprintf(out,"{\"record\":%u,\"schedule\":%u,\"update\":%u,\"frame\":%.9g,\"objects\":[",record,schedule,update,double(player.frame()));
    const auto objects=player.trace();for(unsigned i=0;i<objects.size();++i){const auto& o=objects[i];std::fprintf(out,"%s[%.9g,%u,[",i?",":"",double(o.frame),unsigned(o.stopped));
        for(unsigned k=0;k<o.tracks.size();++k){const auto& t=o.tracks[k];std::fprintf(out,"%s[%u,%u,%u,%u,%u,%zu,%.17g,%.17g,%llu]",k?",":"",t.channel,t.state,t.flags,t.op,t.interpolation,t.cursor,t.time,t.value,static_cast<unsigned long long>(t.callbacks));}std::fprintf(out,"]]");}
    std::fprintf(out,"],\"joints\":[");const auto* pose=player.pose();for(unsigned i=0;i<pose->joints.size();++i){const auto& j=pose->joints[i];std::fprintf(out,"%s[",i?",":"");for(unsigned k=0;k<3;++k)std::fprintf(out,"%s%.9g,%.9g,%.9g",k?",":"",double(j.rotation[k]),double(j.translation[k]),double(j.scale[k]));std::fprintf(out,",%u]",unsigned(j.hidden));}
    std::fprintf(out,"],\"materials\":[");for(unsigned i=0;i<pose->materials.size();++i){const auto& m=pose->materials[i];std::fprintf(out,"%s[",i?",":"");for(unsigned k=0;k<3;++k)std::fprintf(out,"%s%.9g,%.9g,%.9g",k?",":"",double(m.ambient[k]),double(m.diffuse[k]),double(m.specular[k]));std::fprintf(out,",%.9g,%u]",double(m.alpha),unsigned(m.present));}std::fprintf(out,"]}\n");
}
bool original_cases(FILE* out){
    std::ifstream file("local/recomp-evidence/native-startup-animation-resolution-20261109/recovery-v1/tl00_mdl.arc",std::ios::binary);if(!file)return false;
    std::vector<gf::u8> bytes((std::istreambuf_iterator<char>(file)),{});gf::HsdRecordAnimationBank bank;std::string error;check(bank.load(view(bytes),&error),error.c_str());check(bank.records()==37,"original published TL00 records");
    for(unsigned record=0;record<5;++record){gf::HsdRecordAnimationBinding binding;check(bank.select(gf::u16(record),3,0,0,&binding,&error),error.c_str());
        for(unsigned schedule=0;schedule<4;++schedule){const float starts[]={0,0,17.5f,2000},rates[]={1,.5f,1,1};gf::HsdRecordAnimationPlayer player;check(player.bind(binding,&error)&&player.request(starts[schedule],rates[schedule],&error),error.c_str());journal(out,record,schedule,0,player);
            for(unsigned update=1;update<=120;++update){check(player.advance(rates[schedule],&error),error.c_str());journal(out,record,schedule,update,player);}}
        gf::AnimatedModelScene scene;check(scene.load(bank,gf::u16(record),3,0,0,&error)&&scene.request(0,1,&error),error.c_str());
        for(unsigned update=1;update<=120;++update)check(scene.advance(1,&error),error.c_str());
    }
    return true;
}
} // namespace
int main(int argc,char** argv){check(argc<=2,"optional journal path only");material_cases();owned_cases();scene_cases();FILE* out=argc==2?std::fopen(argv[1],"wx"):nullptr;check(argc==1||out,"new journal file required");const bool original=original_cases(out);if(out)check(std::fclose(out)==0,"journal close");std::printf("{\"record_animation_checks\":%u,\"original_TL00_present\":%s,\"all_passed\":true}\n",checks,original?"true":"false");}
