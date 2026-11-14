#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <utility>
#include "engine/gameplay/startup_menu_handoff.hpp"
#include "tests/unit/startup_camera_fixture.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"startup menu handoff: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
bool same(const gf::StartupCameraPoseState& a,const gf::StartupCameraPoseState& b){
    for(unsigned k=0;k<3;++k)if(a.camera.eye[k]!=b.camera.eye[k]||a.camera.target[k]!=b.camera.target[k]||a.camera.up[k]!=b.camera.up[k])return false;
    return a.camera.fov_y_degrees==b.camera.fov_y_degrees&&a.camera.near_plane==b.camera.near_plane&&a.camera.far_plane==b.camera.far_plane&&a.source_aspect==b.source_aspect&&a.roll==b.roll&&a.scene==b.scene&&a.main_binding==b.main_binding&&a.initialized==b.initialized;
}
std::vector<gf::u8> encode(const gf::StartupMenuHandoffProfile& p){
    std::vector<gf::u8> b(40);gf::store_be32(b.data(),0x47464842);gf::store_be16(b.data()+4,1);gf::store_be16(b.data()+6,5);gf::store_be32(b.data()+8,40);gf::store_be32(b.data()+12,p.main_camera_hash);
    unsigned at=20;for(float v:{p.initial_speed,p.first_frame,p.acceleration,p.speed_limit,p.goal}){gf::store_be_f32(b.data()+at,v);at+=4;}return b;
}
void dump(FILE* out,const gf::StartupMenuHandoff& owner,const gf::StartupMenuHandoffStep& effects){
    const auto& s=owner.state();const auto& c=effects.camera;
    std::fprintf(out,"{\"update\":%u,\"counter\":%u,\"speed\":%u,\"create\":%s,\"navigate\":%s,\"camera\":[",s.updates-1,gf::f32_to_bits(s.frame),gf::f32_to_bits(s.speed),effects.create_menu_objects?"true":"false",effects.navigation_enabled?"true":"false");
    bool first=true;for(float v:{c.camera.eye[0],c.camera.eye[1],c.camera.eye[2],c.camera.target[0],c.camera.target[1],c.camera.target[2],c.camera.fov_y_degrees,c.camera.near_plane,c.camera.far_plane,c.roll,c.source_aspect}){std::fprintf(out,"%s%u",first?"":",",gf::f32_to_bits(v));first=false;}std::fputs("]}\n",out);
}
}
int main(int argc,char** argv){
    check(argc==1||argc==5,"optional actual inputs require profile/main-camera/final-special-camera/new-journal");
    startup_camera_fixture::Fixture main,special(true);auto a=main.archive(),b=special.archive();std::string error;gf::StartupCameraSet cameras;
    const std::array<gf::StartupCameraInput,2> inputs={gf::StartupCameraInput{255,{a.data(),a.size()}},gf::StartupCameraInput{12,{b.data(),b.size()}}};
    check(cameras.load(inputs,&error),"owned camera library loads");gf::StartupCameraState base;check(cameras.initialize(&base),"owned camera baseline");gf::StartupCameraRequestedState requested;
    check(cameras.sample_requested(255,2.5f,base,&requested,&error)&&requested.frame==2.5f&&requested.camera.eye[0]==5.5f&&requested.camera.fov_y_degrees==37.5f,"fractional requested time is sampled rather than truncated");
    check(cameras.sample_requested(12,.25f,requested,&requested,&error)&&requested.frame==.25f&&requested.camera.eye[0]==5.25f&&requested.camera.fov_y_degrees==30.25f&&requested.camera.eye[2]==10&&requested.source_aspect==4.f/3,"new in-place fractional request retains live main channels/aspect");
    for(unsigned scene:{255u,12u})for(unsigned frame:{0u,1u,5u,20u,30u}){gf::StartupCameraState old;gf::StartupCameraRequestedState next;check(cameras.sample(gf::u8(scene),frame,base,&old,&error)&&cameras.sample_requested(gf::u8(scene),float(frame),base,&next,&error)&&same(old,next)&&old.frame==frame&&next.frame==float(frame),"integer and new entry share identical camera evaluation including beyond-end requests");}
    const auto before=requested;for(float invalid:{-1.f,NAN,INFINITY,100001.f})check(!cameras.sample_requested(255,invalid,base,&requested,&error)&&same(requested,before)&&requested.frame==before.frame,"invalid fractional time preserves output atomically");
    auto foreign=base;foreign.main_binding^=1;check(!cameras.sample_requested(255,1.5f,foreign,&requested,&error)&&same(requested,before),"foreign main state refuses");foreign=base;foreign.camera.up[1]=-1;check(!cameras.sample_requested(255,1.5f,foreign,&requested,&error),"orientation guard remains");
    check(!cameras.sample_requested(11,1.5f,base,&requested,&error)&&same(requested,before),"missing camera never replays a sample");
    gf::HsdAnimTrack key;key.channel=10;key.data={0x26};for(unsigned i=0;i<3;++i){const auto value=gf::f32_to_bits(float(35+i*10));for(unsigned k=0;k<4;++k)key.data.push_back(gf::u8(value>>(k*8)));key.data.push_back(i?10:0);}float scalar=0;
    check(gf::startup_camera_detail::value_requested(key,8.5f,9,&scalar,&error)&&scalar==45&&gf::startup_camera_detail::value_requested(key,9,9,&scalar,&error)&&scalar==55,"fractional sampler retains default-rate KEY flush at AObj end");
    gf::StartupMenuHandoffProfile profile;profile.loaded=true;profile.main_camera_hash=base.main_binding;profile.first_frame=2;profile.goal=5;profile.acceleration=.25f;profile.speed_limit=.75f;
    auto bytes=encode(profile);gf::StartupMenuHandoffProfile decoded;
    check(gf::startup_menu_handoff_load({bytes.data(),bytes.size()},&decoded,&error)&&decoded.first_frame==2&&decoded.acceleration==.25f,"owned GFHB field-encoded profile loads");
    for(unsigned offset:{0u,4u,6u,8u,12u,16u}){auto bad=bytes;if(offset==12)gf::store_be32(bad.data()+12,0);else bad[offset]^=0x80;check(!gf::startup_menu_handoff_load({bad.data(),bad.size()},&decoded,&error)&&!decoded.loaded,"malformed header/hash/reserved refuses");}
    for(unsigned length:{0u,39u})check(!gf::startup_menu_handoff_load({bytes.data(),length},&decoded,&error),"profile extent refuses");
    bytes.push_back(0);check(!gf::startup_menu_handoff_load({bytes.data(),bytes.size()},&decoded,&error),"profile trailing bytes refuse");
    for(unsigned field=0;field<5;++field){auto bad=profile;if(field==0)bad.initial_speed=NAN;if(field==1)bad.first_frame=-1;if(field==2)bad.acceleration=0;if(field==3)bad.speed_limit=17;if(field==4)bad.goal=1;check(!gf::startup_menu_handoff_profile_valid(bad),"invalid operand domains refuse");}
    gf::StartupMenuHandoff owner;gf::StartupMenuHandoffStep effects;
    check(!owner.begin(profile,cameras,base,false,&error)&&!owner.ready()&&!owner.camera(),"no terminal result: no entry or camera publication");
    auto bad_profile=profile;bad_profile.main_camera_hash^=1;check(!owner.begin(bad_profile,cameras,base,true,&error),"profile main binding refuses");bad_profile=profile;bad_profile.goal=21;check(!owner.begin(bad_profile,cameras,base,true,&error),"handoff end cannot exceed qualified main timeline");
    check(owner.begin(profile,cameras,base,true,&error)&&owner.ready()&&!owner.camera()&&!owner.state().entered,"bind/preflight does not publish the first camera or create menu objects");
    const std::array<float,6> frames={2,2.5f,3.25f,4,4.75f,5};
    for(unsigned i=0;i<frames.size();++i){check(owner.step(cameras,&effects,&error)&&effects.camera_sample&&effects.create_menu_objects==(i==0)&&effects.navigation_enabled==(i==5)&&owner.state().frame==frames[i]&&effects.camera.frame==frames[i]&&owner.camera()!=nullptr,"source entry/speed/camera/navigation ordering");check(owner.state().navigation_ready==(i==5),"navigation remains disabled before endpoint");}
    check(owner.step(cameras,&effects,&error)&&!effects.create_menu_objects&&!effects.camera_sample&&!effects.navigation_enabled&&owner.state().updates==6,"completed handoff emits no entry/camera/enable replay");
    auto moved=std::move(owner);check(moved.ready()&&moved.camera()&&moved.state().frame==5,"handoff stores values without borrowed library pointers");
    auto immediate=profile;immediate.goal=immediate.first_frame;check(owner.begin(immediate,cameras,base,true,&error)&&owner.step(cameras,&effects,&error)&&effects.create_menu_objects&&effects.navigation_enabled&&owner.state().updates==1,"equal source start/goal completes on first sample without invented wait");
    check(owner.begin(profile,cameras,base,true,&error),"foreign library guard setup");gf::StartupCameraSet empty;check(!owner.step(empty,&effects,&error)&&!owner.ready()&&!owner.camera()&&!effects.camera_sample&&owner.state().updates==0,"unloaded library invalidates publication and rolls back counters");check(!owner.step(cameras,&effects,&error)&&!owner.camera(),"failed owner never silently replays prior pose");
    startup_camera_fixture::Fixture invalid;invalid.stream(0x160,20,600);auto invalid_bytes=invalid.archive();gf::StartupCameraSet invalid_set;const std::array<gf::StartupCameraInput,1> invalid_input={gf::StartupCameraInput{255,{invalid_bytes.data(),invalid_bytes.size()}}};check(invalid_set.load(invalid_input,&error)&&invalid_set.initialize(&base),"late invalid camera fixture");
    auto late=profile;late.main_camera_hash=base.main_binding;late.first_frame=0;late.goal=10;late.acceleration=late.speed_limit=1;check(owner.begin(late,invalid_set,base,true,&error),"first camera preflight succeeds before later invalid FOV");
    for(unsigned i=0;i<3;++i)check(owner.step(invalid_set,&effects,&error),"valid prefix of late invalid camera");
    check(!owner.step(invalid_set,&effects,&error)&&!owner.camera()&&!effects.camera_sample&&owner.state().updates==3&&owner.state().frame==2,"late camera failure keeps accepted state but invalidates drawing/no partial effects");
    check(cameras.initialize(&base),"restore source baseline");auto slow=profile;slow.first_frame=0;slow.goal=19;slow.acceleration=slow.speed_limit=1e-10f;check(owner.begin(slow,cameras,base,true,&error),"bounded pathological schedule setup");for(unsigned i=0;i<8192;++i)check(owner.step(cameras,&effects,&error),"bounded prefix remains valid");check(!owner.step(cameras,&effects,&error)&&!owner.camera()&&!owner.state().navigation_ready,"native work budget refuses, never forces completion");
    unsigned source_samples=0;
    if(argc==5){auto actual_profile=read(argv[1]),actual_main=read(argv[2]),actual_special=read(argv[3]);gf::StartupMenuHandoffProfile actual;gf::StartupCameraSet actual_set;const std::array<gf::StartupCameraInput,2> source_inputs={gf::StartupCameraInput{255,{actual_main.data(),actual_main.size()}},gf::StartupCameraInput{17,{actual_special.data(),actual_special.size()}}};
        check(gf::startup_menu_handoff_load({actual_profile.data(),actual_profile.size()},&actual,&error)&&actual_set.load(source_inputs,&error)&&actual_set.initialize(&base)&&actual_set.sample(17,gf::u32(actual_set.clip(17)->base_end_frame),base,&base,&error)&&owner.begin(actual,actual_set,base,true,&error),"actual final-special endpoint feeds source handoff profile/main-camera bind");
        FILE* journal=std::fopen(argv[4],"wx");check(journal!=nullptr,"new actual sample journal only");
        while(!owner.state().navigation_ready){check(owner.step(actual_set,&effects,&error),"actual fractional source sample");dump(journal,owner,effects);++source_samples;}
        check(std::fclose(journal)==0&&source_samples==64&&owner.state().frame==449,"actual source reaches idle endpoint naturally in64 updates");
    }
    std::printf("{\"startup_menu_handoff_checks\":%u,\"source_samples\":%u,\"all_passed\":true,\"startup_scene_admitted\":false}\n",checks,source_samples);
}
