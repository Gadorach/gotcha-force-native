#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_motion_driver_fixture.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"motion transition edges: %s\n",why);std::abort();}}bool near(float a,float b){return std::abs(a-b)<.00002f;}
CharacterLocalPose seed(const CharacterPoseRig& rig){CharacterLocalPose p;for(const auto& b:rig.binding.bones){CharacterBonePose v;v.joint_offset=b.joint_offset;std::copy_n(b.translation,3,v.translation);p.bones.push_back(v);}p.bones[0].translation[0]=90;return p;}}
int main(){using namespace gf;using namespace character_motion_driver_fixture;Fixture f;std::string error;check(f.load(&error),"fixture");auto initial=seed(f.rig);CharacterMotionDriver d;auto load=[&](){return d.load_transitions({f.model.data(),f.model.size()},f.rig,f.profile,f.clips,initial,&error);};CharacterProgramMotion a,b;
    check(load()&&d.select_transition(0,f.profile.descriptors[0],false,{},0,0,2,2,&a)&&d.snapshot(a)->previous_weight()==1&&d.snapshot(a)->local_pose()->bones[0].translation[0]==90,"zero-rate first blend keeps supplied pose");
    for(unsigned i=0;i<3;++i){check(d.advance(0,a,0,&b)&&b.frame==0&&b.elapsed==0&&d.snapshot(b)->previous_weight()==1,"zero-rate update does not spend blend");a=b;}
    check(d.select_transition(0,f.profile.descriptors[0],false,{},0,.5f,2,2,&a),"fractional blend start");for(unsigned i=0;i<5;++i){check(d.advance(0,a,.5f,&b),"fractional timer");a=b;}check(a.frame==2&&a.elapsed==2.5f&&!d.snapshot(a)->blending(),"completion re-request under half rates");
    check(d.select_transition(0,f.profile.descriptors[1],false,{},2,1,2,1,&a)&&!a.finished,"source suppresses finish while blending even stopped clip");check(d.advance(0,a,1,&b)&&!b.finished&&!d.snapshot(b)->blending()&&b.frame==2,"re-request sees old blend mode before clear");check(d.advance(0,b,0,&a)&&a.finished&&a.frame==2,"ordinary next callback publishes finished");
    auto rotation=linear(1.5707963267948966f,1.5707963267948966f,2);rotation.channel=3;f.clips[1].clip.bones[0].tracks.push_back(rotation);f.profile.descriptors[1].bytes[6]=2;
    check(load()&&d.select_transition(0,f.profile.descriptors[1],true,{},0,1,2,1,&a),"mirrored quaternion blend");const auto& p=d.snapshot(a)->local_pose()->bones[0];check(near(p.translation[0],-5)&&near(p.quaternion[2],-std::sin(3.14159265358979323846f/8))&&near(p.quaternion[3],std::cos(3.14159265358979323846f/8)),"mirror before quaternion blend");
    check(d.select_transition(0,f.profile.descriptors[0],false,a,0,1,2,1,&b)&&near(d.snapshot(b)->local_pose()->bones[0].quaternion[2],-std::sin(3.14159265358979323846f/16)),"interrupted frozen quaternion preserved without Euler conversion");
    std::weak_ptr<const CharacterProgramMotionContext> prior=a.context;a={};check(prior.expired(),"new snapshot owns frozen values without parent chain");
    check(d.select_transition(0,f.profile.descriptors[0],false,b,0,1,2,1,&b)&&d.snapshot(b),"aliased input/output staged safely");
    // KEY at the old loop endpoint is flushed BEFORE reset/zero-rate sample.
    f.profile.descriptors[0].bytes[6]=5;f.profile.descriptors[0].bytes[7]=1;f.clips[0].clip.end_frame=9;HsdAnimTrack key;key.channel=5;key.data={0x26};for(unsigned i=0;i<3;++i){little(key.data,float(i+1));key.data.push_back(i?10:0);}f.clips[0].clip.bones[0].tracks={key};
    check(load()&&d.select(0,f.profile.descriptors[0],false,{},0,1,&a),"loop KEY start");for(unsigned i=0;i<9;++i){check(d.advance(0,a,1,&b),"loop KEY callback");a=b;}
    const auto trace=d.snapshot(a)->trace()[0];check(a.frame==1&&a.rewound&&!a.finished&&trace.tracks[0].callbacks==4&&trace.tracks[0].time==1&&d.snapshot(a)->sample().bones[0].value[5]==2,"loop flush is not nonloop sample-plus-second-flush");
    f.profile.descriptors[0].bytes[7]=0;f.clips[0].clip.end_frame=4;f.clips[0].clip.bones[0].tracks={linear(0,4,4)};check(load()&&d.select(0,f.profile.descriptors[0],false,{},0,1,&a)&&d.advance(0,a,4,&b)&&b.frame==0&&b.rewound&&!b.finished,"exact loop endpoint samples start, not endpoint");
    auto saved=b;check(!d.advance(0,b,17,&a)&&d.snapshot(saved)&&saved.frame==0,"late refusal retains accepted input");
    std::printf("character motion transition edges: %u owned checks PASS\n",checks);
}
