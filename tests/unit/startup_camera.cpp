#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_camera_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup camera: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    startup_camera_fixture::Fixture main,special(true);auto a=main.archive(),b=special.archive();std::string error;gf::StartupCameraSet set;
    std::array<gf::StartupCameraInput,2> inputs={gf::StartupCameraInput{255,{a.data(),a.size()}},gf::StartupCameraInput{12,{b.data(),b.size()}}};
    check(set.load(inputs,&error),"owned explicit source camera binding");gf::StartupCameraState state,next;check(set.initialize(&state)&&state.camera.eye[2]==10&&state.camera.fov_y_degrees==60,"main static descriptor initializes global CObj state");
    check(set.sample(255,0,state,&next,&error)&&next.camera.eye[0]==3&&next.camera.fov_y_degrees==35,"requested zero performs FIRST_PLAY callback, not legacy authored/unstepped state");state=next;
    check(set.sample(12,0,state,&next,&error)&&next.camera.eye[0]==5&&next.camera.eye[2]==10&&next.camera.target[0]==0&&next.camera.near_plane==1&&next.camera.far_plane==1000&&next.source_aspect==4.f/3&&next.camera.fov_y_degrees==30,"rebind animation only: missing channels retain global main state, special static descriptor/aspect ignored");
    check(set.clip(12)->aspect==1.18f&&set.clip(255)->base_end_frame==20,"descriptor provenance retained separately from runtime global projection");state=next;
    check(set.sample(12,5,state,&next,&error)&&next.camera.eye[0]==10&&next.camera.fov_y_degrees==35,"explicit requested-time interpolation");state=next;
    check(set.sample(12,30,state,&next,&error)&&next.frame==30&&next.camera.eye[0]==15,"explicit source request is not globally clamped to clip end");
    gf::HsdCameraClip legacy;check(gf::hsd_camera_clip_load({a.data(),a.size()},0,&legacy,&error),"legacy camera reader still works");gf::Camera camera;
    check(gf::hsd_camera_sample(legacy,0,&camera,&error)&&camera.eye[0]==0&&camera.fov_y_degrees==60,"legacy zero-update camera contract remains authored");
    auto badstate=state;badstate.main_binding^=1;check(!set.sample(12,0,badstate,&next,&error),"cross-main state identity refuses");badstate=state;badstate.camera.up[1]=-1;check(!set.sample(12,0,badstate,&next,&error),"unreviewed global up-vector override refuses");
    badstate=state;badstate.camera.eye[2]=NAN;check(!set.sample(12,0,badstate,&next,&error),"nonfinite retained camera coordinate cannot leak through missing source channels");
    next.frame=1234;next.camera.eye[0]=321;check(!set.sample(11,0,state,&next,&error)&&next.frame==1234&&next.camera.eye[0]==321,"failed sample leaves destination untouched, not a partial pose or implicit replay");
    check(!set.sample(11,0,state,&next,&error)&&!set.sample(12,100001,state,&next,&error),"unbound scene and work-domain overflow refuse");
    gf::StartupDirectorObservations observations;observations.fade_busy=observations.resources_busy=true;check(set.observations(&observations)&&observations.camera_end[32]==20&&observations.camera_end[12]==20&&!observations.camera_end_valid[11]&&observations.fade_busy&&observations.resources_busy,"source base-end observations populate known scenes without faking fade/resource readiness");
    auto duplicate=inputs;duplicate[1].scene=255;check(!set.load(duplicate,&error)&&!set.initialize(&next),"duplicate source invalidates loaded library atomically");
    check(set.load(inputs,&error)&&set.initialize(&state),"restore owned set");auto malformed=special;malformed.real(0x104,19);auto bytes=malformed.archive();inputs[1].archive={bytes.data(),bytes.size()};check(!set.load(inputs,&error),"unqualified mixed AObj end times refuse instead of guessing base lifecycle");inputs[1].archive={b.data(),b.size()};
    malformed=special;malformed.ptr(0x110,0x1a0);malformed.word(0x1a4,18);malformed.block[0x1ac]=10;malformed.ptr(0x1b0,0x1c0);malformed.stream(0x1c0,40,50);bytes=malformed.archive();inputs[1].archive={bytes.data(),bytes.size()};check(!set.load(inputs,&error),"duplicate callback channels refuse instead of guessing order");inputs[1].archive={b.data(),b.size()};
    check(!set.load(std::span<const gf::StartupCameraInput>(inputs).subspan(1),&error),"special camera descriptor cannot stand in for missing global main");
    gf::HsdAnimTrack key;key.channel=10;key.data={0x26};for(unsigned i=0;i<3;++i){const auto value=gf::f32_to_bits(float(35+i*10));for(unsigned k=0;k<4;++k)key.data.push_back(gf::u8(value>>(k*8)));key.data.push_back(i?10:0);}float value=0;
    check(gf::startup_camera_detail::value(key,8,9,&value,&error)&&value==45&&gf::startup_camera_detail::value(key,9,9,&value,&error)&&value==55,"source default-rate KEY stop flush occurs at AObj end, unlike a bare requested scalar sample");
    check(set.load(inputs,&error)&&set.initialize(&state),"owned lifetime setup");a.clear();b.clear();check(set.sample(12,5,state,&next,&error)&&next.camera.eye[0]==10,"camera set owns track/path/static data after source buffer lifetime");
    unsigned files=0,poses=0;std::vector<std::vector<gf::u8>> owned;std::vector<gf::StartupCameraInput> actual;
    for(unsigned scene:{255u,12u,13u,14u,15u,16u,17u,18u,19u,20u,21u}){auto data=read(std::string("local/content/startup-cameras/")+(scene==255?"main.hsd":"scene-"+std::to_string(scene)+".hsd"));if(data.empty())continue;owned.push_back(std::move(data));actual.push_back({gf::u8(scene),{owned.back().data(),owned.back().size()}});}
    check(actual.empty()||actual.size()==11,"partial optional camera bundle is not silently counted as a full source check");
    if(actual.size()==11){const bool loaded=set.load(actual,&error);if(!loaded)std::fprintf(stderr,"actual load: %s\n",error.c_str());check(loaded&&set.initialize(&state),"all actual bound main/special cameras load without runtime AFS/PZZ/DOL");
        check(set.observations(&observations)&&observations.camera_end[32]==1999&&observations.camera_end[12]==39&&observations.camera_end[21]==29,"real source ends replace prior synthetic96/128 values");
        for(const auto& source:actual){const auto* clip=set.clip(source.scene);check(clip!=nullptr,"actual explicit source");
            for(unsigned frame=0;frame<=unsigned(clip->end_frame);++frame){const bool ok=set.sample(source.scene,frame,state,&next,&error);if(!ok)std::fprintf(stderr,"scene%u frame%u: %s\n",unsigned(source.scene),frame,error.c_str());check(ok&&next.source_aspect==set.initial().source_aspect&&next.roll==0,"every original camera frame including requested0 and endpoint samples while retaining main aspect");state=next;++poses;}
            ++files;
        }
        auto director_bytes=read("local/content/startup-director/startup.gfdr");gf::StartupDirectorProfile profile;gf::StartupDirector director;const std::array<gf::u8,6> indices={7,29,0,0,0,0};
        check(gf::startup_director_load({director_bytes.data(),director_bytes.size()},&profile,&error)&&director.load(profile,indices,&error)&&set.initialize(&state),"bind source director to concrete camera set");
        unsigned samples=0,requests=0,ticks=0;gf::u8 selected=255;bool unused14=true;
        for(;ticks<8192&&!director.state().terminal;++ticks){observations.fade_busy=ticks<3;observations.resources_busy=ticks<180;std::vector<gf::StartupEffect> effects;check(director.step(observations,&effects,&error),"director advances with source ends and declared fade/resource readiness");
            for(const auto& effect:effects){if(effect.kind==gf::StartupEffectKind::Command&&effect.opcode==6)selected=effect.operands[0];
                if(effect.kind==gf::StartupEffectKind::Command&&effect.opcode==13&&effect.operands[0]==0)++requests;
                if((effect.kind==gf::StartupEffectKind::Command&&effect.opcode==7)||effect.kind==gf::StartupEffectKind::CameraSample){check(effect.value>=0&&std::floor(effect.value)==effect.value&&set.sample(selected,gf::u32(effect.value),state,&next,&error),"ordered concrete camera rebind/sample");state=next;++samples;unused14&=selected!=14;}}
        }
        check(director.state().terminal&&ticks==1283&&samples==421&&requests==12&&unused14,"bounded original-byte script reaches terminal with actual camera ends; scene14 imported but unused");
    }
    std::printf("{\"startup_camera_checks\":%u,\"source_camera_files\":%u,\"source_requested_poses\":%u,\"actor_placement_effects_or_visible_startup_admitted\":false,\"all_passed\":true}\n",checks,files,poses);
}
