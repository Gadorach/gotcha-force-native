#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/character_motion_driver_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"character motion driver: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    using namespace character_motion_driver_fixture;std::string error;Fixture f;check(f.load(&error),"owned model/rig/profile/clip fixture");gf::CharacterMotionDriver driver;
    check(f.driver(driver,&error),"bounded source-bound driver library loads");gf::CharacterProgramMotion a,b;
    check(driver.select(0,f.profile.descriptors[0],false,{},0,1,&a)&&a.frame==0&&a.elapsed==0&&a.end_frame==4&&!a.finished,"source instant select samples FIRST_PLAY at zero without advancing");
    check(driver.snapshot(a)&&driver.snapshot(a)->sample().bones[0].value[5]==0&&driver.snapshot(a)->sample().bones[1].present==0,"only source lane-cut bones sampled, not entire archive tree");
    check(driver.advance(0,a,.5f,&b)&&b.frame==.5f&&b.elapsed==.5f&&driver.snapshot(b)->sample().bones[0].value[5]==.5f,"retained fractional AObj/FObj step");
    check(a.frame==0&&driver.snapshot(a)->sample().bones[0].value[5]==0,"pure advance does not mutate input snapshot");a=b;
    for(unsigned i=0;i<7;++i){check(driver.advance(0,a,.5f,&b),"nonloop half-time advance to end");a=b;}
    check(a.frame==4&&a.elapsed==4&&a.finished&&driver.snapshot(a)->trace()[0].stopped&&driver.snapshot(a)->trace()[0].tracks[0].state==0,"source nonloop end stops AObj and clears FObj state");
    check(driver.advance(0,a,1,&b)&&b.elapsed==5&&b.frame==4&&driver.snapshot(b)->sample().bones[0].value[5]==4,"stopped AObj frame overwrites incremented lane frame; elapsed continues");
    check(driver.snapshot(b)->previous_elapsed()==4&&driver.snapshot(b)->previous_frame()==4,"source previous elapsed/frame preserved for future actor event timing");
    check(driver.select(0,f.profile.descriptors[0],false,b,30,1,&a)&&a.frame==30&&a.finished&&driver.snapshot(a)->trace()[0].frame==30,"source request30 samples before stop, no end4 clamp");
    check(driver.advance(0,a,1,&b)&&b.frame==30&&b.elapsed==1,"stopped beyond-end AObj remains30 on next tick, not31 or4");
    auto forged=b;forged.frame=3;check(!driver.advance(0,forged,1,&a),"scalar observation cannot be forged separately from opaque immutable state");
    check(!driver.advance(1,b,1,&a),"snapshot cannot silently move between lanes");gf::CharacterMotionDriver other;check(f.driver(other,&error)&&!other.advance(0,b,1,&a),"snapshot from another driver library refuses");
    check(!driver.advance(0,b,NAN,&a)&&!driver.advance(0,b,17,&a),"nonfinite/overbudget motion delta refuses");
    auto d=f.profile.descriptors[0];d.bytes[4]=0x80;check(!driver.select(0,d,false,{},0,1,&a),"unbound/common bank descriptor refuses");
    auto loop=f;loop.profile.descriptors[0].bytes[6]|=4;check(loop.driver(other,&error)&&!other.select(0,loop.profile.descriptors[0],false,{},0,1,&a),"source loop flag is not flattened into nonloop playback");
    auto fractional=f;fractional.clips[0].clip.bones[0].tracks[0].startframe=.5f;check(!fractional.driver(other,&error),"FObj runtime signed16 start domain: unqualified fractional descriptor refuses");
    auto longwait=f;auto& payload=longwait.clips[0].clip.bones[0].tracks[0].data;payload[5]=128;payload.insert(payload.begin()+6,{128,4});check(!longwait.driver(other,&error),"wait beyond source u16 storage domain refuses instead of guessing wrap semantics");
    check(driver.select(0,f.profile.descriptors[0],false,{},0,.1f,&a),"decimal-rate initial source request");float decimal=0;
    for(unsigned i=0;i<10;++i){decimal+=.1f;check(driver.advance(0,a,.1f,&b)&&b.frame==decimal&&driver.snapshot(b)->trace()[0].tracks[0].time==double(decimal),"retained FObj clock rounds at source float32 stores rather than accumulating hidden double time");a=b;}
    auto null=f;null.rig.lane_mask=null.profile.lane_mask=1;null.rig.cuts={0,2,2,2,2};null.clips[0].clip.bones[1].has_aobj=false;null.clips[0].clip.bones[1].tracks.clear();
    check(null.driver(other,&error)&&other.select(0,null.profile.descriptors[0],false,{},30,1,&a)&&a.frame==30&&a.end_frame==4,"existing public with null AObj does NOT reset prior lane frame/end");
    null.clips[0].clip.public_bone_count=1;
    check(null.driver(other,&error)&&other.select(0,null.profile.descriptors[0],false,{},30,1,&a)&&a.frame==0&&a.end_frame==0&&a.finished&&other.snapshot(a)->trace()[0].frame==30,"missing public resets lane frame/end after prior AObj sample; not universal end clamping");
    check(other.snapshot(a)->previous_frame()==30,"initial previous frame retains requested30 despite later missing-public reset to0");
    gf::CharacterProgramVm vm;gf::CharacterProgramActor actor;actor.default_group=5;std::vector<gf::CharacterProgramOperation> operations;
    check(vm.load(null.profile,actor,{},&error)&&vm.request(5,0,1,30,1,other,&operations,&error)&&vm.state().lanes[0].motion.frame==0,"VM accepts source bone-ordered frame overwrite rather than requiring fabricated return==request");
    auto allnull=f;for(auto& bone:allnull.clips[1].clip.bones){bone.has_aobj=false;bone.tracks.clear();}allnull.clips[1].clip.end_frame=0;
    check(allnull.driver(other,&error)&&other.select(0,allnull.profile.descriptors[0],false,{},0,1,&a)&&other.select(0,allnull.profile.descriptors[1],false,a,7,1,&b)&&b.frame==7&&b.end_frame==4&&!b.finished,"all-null-AObj selection retains prior end but does not invent completion from frame>=end");
    check(other.advance(0,b,1,&a)&&a.frame==8&&a.elapsed==1&&!a.finished,"all-null-AObj lane advances ordinary frame without fabricated finished");
    auto key=f;key.clips[0].clip.end_frame=9;gf::HsdAnimTrack track;track.channel=5;track.data={0x26};
    for(unsigned i=0;i<3;++i){little(track.data,float(i+1));track.data.push_back(i?10:0);}key.clips[0].clip.bones[0].tracks={track};
    check(key.driver(other,&error)&&other.select(0,key.profile.descriptors[0],false,{},0,1,&a)&&other.snapshot(a)->sample().bones[0].value[5]==2,"owned KEY pack source initial callbacks");
    for(unsigned i=0;i<9;++i){check(other.advance(0,a,1,&b),"KEY lifecycle advance");a=b;}
    const auto trace=other.snapshot(a)->trace()[0];
    check(a.frame==9&&trace.stopped&&trace.tracks[0].time==10&&trace.tracks[0].callbacks==3&&other.snapshot(a)->sample().bones[0].value[5]==3,"stop flush advances KEY FObj by framerate once WITHOUT advancing AObj frame");
    auto ambiguous=f;ambiguous.clips[0].clip.bones[0].tracks[0].data={1};little(ambiguous.clips[0].clip.bones[0].tracks[0].data,7);
    check(ambiguous.driver(other,&error)&&!other.select(0,ambiguous.profile.descriptors[0],false,{},0,1,&a),"initial no-callback stream refuses instead of inventing uninitialized SRT value");
    check(f.driver(driver,&error)&&driver.select(0,f.profile.descriptors[0],false,{},0,1,&a),"restore ordinary owned state");f.model.clear();f.clips.clear();f.profile={};f.rig={};
    check(driver.advance(0,a,1,&b)&&driver.snapshot(b)->sample().bones[0].value[5]==1,"driver/context own immutable clips/rig after caller inputs expire");
    unsigned profiles=0,requests=0,updates=0,poses=0,held=0;
    for(const char* name:{"g-red","sasuke"}){
        auto model=read(std::string("local/content/startup-characters/")+name+"/model.hsd"),prefix=read(std::string("local/content/startup-characters/")+name+"/model-prefix.bin");
        auto encoded=read(std::string("local/content/character-program-bindings/")+name+".gfmp"),layout=read(std::string("local/content/character-pose-bindings/")+name+".gfcp");if(model.empty()||encoded.empty()||layout.empty())continue;
        gf::HsdModelBones bones;gf::CharacterPoseRig rig;gf::CharacterProgramAsset profile;
        check(gf::hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bones,&error)&&gf::character_pose_rig_load({layout.data(),layout.size()},bones,&rig,&error)&&gf::character_program_load({encoded.data(),encoded.size()},gf::character_program_model_hash({model.data(),model.size()}),&profile,&error),"actual source-bound rig/profile");
        std::vector<gf::CharacterMotionClipBinding> clips;
        for(unsigned index:{0,1,3,4,7,8,9}){auto bytes=read(std::string("local/content/startup-motion-programs/")+name+"/motions/motion-0"+std::to_string(index)+".hsd");gf::CharacterMotionClipBinding c;c.index=gf::u8(index);
            check(gf::hsd_bone_motion_load({bytes.data(),bytes.size()},bones,&c.clip,&error),"actual clip including null-AObj/public distinction");clips.push_back(std::move(c));}
        check(driver.load({model.data(),model.size()},rig,profile,clips,&error),"actual concrete driver library");
        for(unsigned choice:{0,1,3,4,6,7}){
            check(vm.load(profile,actor,{},&error),"fresh actual VM state");const bool selected=vm.request(5,gf::u8(choice),3,-1,1,driver,&operations,&error);if(!selected)std::fprintf(stderr,"%s request%u: %s\n",name,choice,error.c_str());check(selected,"actual initial request uses real retained AObj/FObj sampling, no test-clock driver");
            bool done=false;
            for(unsigned tick=0;tick<512;++tick){
                std::array<gf::CharacterPoseLane,4> lanes;for(unsigned lane=0;lane<2;++lane){const auto* state=driver.snapshot(vm.state().lanes[lane].motion);check(state!=nullptr,"accepted immutable native lane snapshot");lanes[lane].current=&state->sample();lanes[lane].mirrored=state->mirrored();}
                gf::CharacterLocalPose local;gf::CharacterWorldPose world;gf::CharacterPoseFrame frame;
                check(gf::character_pose_sample(rig,lanes,&local,&error)&&gf::character_pose_initial_frame(rig,gf::Mat4{},&frame,&error)&&gf::character_pose_direct_world(rig,local,frame,&world,&error),"concrete driver samples feed existing declared pose subset with supplied identity anchor; not actor world integration");++poses;
                check(vm.update(3,1,driver,&operations,&error),"actual VM step consumes source nonloop lifecycle observations");++updates;
                if(vm.state().lanes[0].done&&vm.state().lanes[1].done){done=true;break;}
            }
            if(std::string(name)=="g-red"&&choice==7){
                const auto& lane=vm.state().lanes[0];
                check(!done&&lane.motion.frame==10&&lane.motion.end_frame==10&&lane.threshold==24&&lane.wait==36&&lane.descriptor_choice==7,"isolated G-Red request7 holds at frame24 wait after nonloop end10; never fake advancement to descriptor8");
                check(driver.snapshot(lane.motion)->trace()[rig.order[rig.cuts[0]]].stopped,"held program has a genuinely stopped AObj, not a sampler/VM error");++held;
            }else{
                if(!done)std::fprintf(stderr,"unexpected hold %s request%u frame=%g end=%d wait=%u threshold=%g\n",name,choice,vm.state().lanes[0].motion.frame,int(vm.state().lanes[0].motion.end_frame),vm.state().lanes[0].wait,vm.state().lanes[0].threshold);
                check(done,"other source requests reach terminal under retained nonloop driver");
                if(choice==7)check(driver.snapshot(vm.state().lanes[0].motion)->index()==8&&driver.snapshot(vm.state().lanes[1].motion)->index()==8,"Sasuke request7 uses original descriptor8 continuation");
            }
            ++requests;
        }
        ++profiles;
    }
    std::printf("{\"character_motion_driver_checks\":%u,\"source_profiles\":%u,\"source_requests\":%u,\"source_driver_updates\":%u,\"pose_subset_frames\":%u,\"bounded_held_programs\":%u,\"actor_root_events_materials_attachments_startup_admitted\":false,\"all_passed\":true}\n",checks,profiles,requests,updates,poses,held);
}
