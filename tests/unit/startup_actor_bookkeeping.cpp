#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_actor_bookkeeping_fixture.hpp"
#include "tests/unit/actor_fade_fixture.hpp"
#include "tests/unit/character_model_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup bookkeeping: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
class Recorder final:public gf::IRenderBackend {
public:
    std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;using namespace startup_actor_bookkeeping_fixture;
    StartupActorSetupProfile setup;StartupActorRuntimeProfile p;StartupActorClassProfile classes;StartupActorBookkeeping initial,s,next;std::string error;
    check(load(setup,p,classes,initial,&error),"owned source runtime constructor row");
    check(initial.motion_delta==1&&initial.timer_delta==1&&initial.rate768==1&&initial.reloads[0].current==5&&initial.reloads[1].remaining==0&&initial.reloads[2].remaining==180,"source clock initialization and reloadslots0/2, not allzero timer assumptions");
    s=initial;check(startup_actor_pre_update(p,s,{},&s,&error)&&s.reloads[2].current==-1&&s.reloads[2].previous==-1&&s.reloads[2].remaining==179,"G-Red zero capacity/mode1 sourceearlyvalue-1 retained, not forced to0");
    for(unsigned i=1;i<180;++i)check(startup_actor_pre_update(p,s,{},&s,&error),"bounded constructor reload progression");
    check(s.reloads[2].remaining==0&&s.reloads[2].current==0,"source timer reaches target without fake ammo clamp");
    s=initial;s.timer694=.5f;s.field73f=9;s.timers690.fill(.5f);s.byte_counters={1,2,128,255};check(startup_actor_pre_update(p,s,{},&next,&error)&&next.timer694==0&&next.field73f==0&&next.timers690[0]==0&&next.byte_counters==std::array<u8,4>{0,1,127,254},"positive timers/byte decrements preserve stores and bytewrap");
    s.klass.flags5e0=1;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.timer694==.5f&&next.field73f==9,"flags0/1 blocktimer694 branch");
    for(unsigned value=0;value<65536;++value){s=initial;s.counter6b8=std::bit_cast<s16>(u16(value));const auto expected=value?std::bit_cast<s16>(u16(value-1)):s16(0);check(startup_actor_pre_update(p,s,{},&next,&error)&&next.counter6b8==expected,"all signed16 countdown states including wrap");}
    s=initial;s.counter6b8=5;s.field71c=1;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.counter6b8==5,"holdcounter predicate");s.field4a1=1;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.counter6b8==0,"clearcounter takes priority overhold");
    s=initial;s.phase19=2;s.counter6ba=31;s.value1c6=200;s.flags8c4=4;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.counter6ba==30&&next.value1c6==199,"phase1/substate2 modulo30 source decrement");
    s.counter6ba=1;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.counter6ba==0&&next.value1c6==199&&next.flags8c4==0,"zero is source multiple30 and clearsflag, not preclamped before test");
    s=initial;s.value736=32767;s.limit736=32767;s.increment736=1;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.value736==-32768,"signed16 store wraps BEFORElimit comparison");
    s=initial;s.value736=0;s.counter73a=2;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.value736==0&&next.counter73a==1,"regeneration hold decrements without refill");s.cached87=1;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.counter73a==2,"cached87 predicate uses retained previouspostvalue");
    s=initial;s.field661=2;s.klass.flags5e0=0x80;s.field747=7;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.value736==s.value736&&next.field747==7,"specialresource mode80gate retains747");
    s=initial;s.timers684[0]=.5f;s.field6be=-17;s.field6c2=99;s.timers684[3]=.5f;s.field6c8=5;s.field6ca=1;s.timer720=.5f;s.klass.field272=0xffff;check(startup_actor_pre_update(p,s,{},&next,&error)&&next.field6c2==-17&&!next.field6c8&&!next.field6ca&&!next.klass.field272,"source expiry copies and clears scalarstatus");
    for(unsigned branch=0;branch<4;++branch){s=initial;if(branch<2)s.timers684[1+branch]=.5f;else if(branch==2){s.field708=s.field709=1;}else{s.field7d4=1;s.klass.flags5e0=0x02000000;}
        next=initial;const auto before=fields(next);check(!startup_actor_pre_update(p,s,{},&next,&error)&&fields(next)==before,"dependentactor/audio/recovery callback boundary refuses atomically, no unconsumed continuation");}
    s=initial;s.peers={11,22,33,0,0,0,0,0};s.peer_count=3;const std::array<StartupActorPeerObservation,3> observations={StartupActorPeerObservation{11,2},{22,2},{33,1}};
    check(startup_actor_pre_update(p,s,observations,&next,&error)&&next.peer_count==2&&next.peers[0]==22&&next.peers[1]==33,"source compaction incrementsindex and skips adjacentretiredentry thisupdate");s=next;check(startup_actor_pre_update(p,s,observations,&next,&error)&&next.peer_count==1&&next.peers[0]==33,"nextsourceupdate removes retainedadjacententry");
    check(!startup_actor_pre_update(p,s,{},&next,&error),"unknown peerphase refuses, not assumeddead/live");
    s=initial;s.reloads[0]={2,2,5,1,3,10};check(startup_actor_pre_update(p,s,{},&next,&error)&&next.reloads[0].current==1&&next.reloads[0].previous==1,"mode1 integerinterval calculation");
    s.reloads[0]={2,7,5,1,0,10};check(!startup_actor_pre_update(p,s,{},&next,&error),"positive remaining withzero interval refuses, no singular fallback");
    s=initial;s.field6fc=1;s.reloads[0]={2,2,5,1,3,5};s.reloads[2]={2,2,5,0,3,5};check(startup_actor_pre_update(p,s,{},&next,&error)&&next.reloads[0].remaining==-5&&next.reloads[2].remaining==0,"sourcehitstopmultiplier10;mode1negative remainder retained vs mode0clamp");
    s=initial;s.klass.flags5e0=0x20000040;s.field7c9=1;check(startup_actor_post_update(p,s,&next)&&next.cached87==1&&next.field276==1&&next.klass.field272==0xffff&&next.field1eb==0,"postflags/size gate sourcewrites");s.field8cc=255;check(startup_actor_post_update(p,s,&next)&&next.field1eb==1,"signed negative8CC mode branch");
    auto raw=runtime();for(unsigned at:{0u,4u,6u,8u,12u,16u,18u,20u,42u}){auto bad=raw;bad[at]^=1;StartupActorRuntimeProfile rejected=p;check(!startup_actor_runtime_load({bad.data(),bad.size()},1234,&rejected,&error)&&!rejected.loaded,"bad runtimeprofile header/model scope rejects");}
    s=initial;s.motion_delta=NAN;check(!startup_actor_pre_update(p,s,{},&next,&error)&&!startup_actor_post_update(p,s,&next),"nonfinite clock cannot reach generic ormaterial consumer");
    auto model=character_model_fixture::archive(),prefix=character_model_fixture::prefix(),pose=character_model_fixture::profile();CharacterModelScene scene,other;
    check(scene.load({model.data(),model.size()},{prefix.data(),prefix.size()},{pose.data(),pose.size()},&error)&&other.load({model.data(),model.size()},{prefix.data(),prefix.size()},{pose.data(),pose.size()},&error),"two separatelyconstructed nativeinstances ofsame model");
    auto bound_setup=setup;bound_setup.model_hash=scene.instance_binding()->model_hash;auto owned_fade=actor_fade_fixture::profile();ActorFadeProfile fade;check(actor_fade_load({owned_fade.data(),owned_fade.size()},&fade,&error),"owned fade library");StartupActorMaterial material;
    check(material.bind(scene,bound_setup,fade)&&material.bound(scene)&&!material.bound(other),"materialowner bound toexact modelinstance, notsamehash neighbor");
    HsdBoneMotionSample sample;sample.bones.resize(2);sample.bones[0].joint_offset=0x40;sample.bones[1].joint_offset=0x80;std::array<CharacterPoseLane,4> lanes;lanes[0].current=&sample;CharacterLocalPose local;CharacterPoseFrame frame;CharacterWorldPose world;
    check(character_pose_sample(scene.rig(),lanes,&local,&error)&&character_pose_initial_frame(scene.rig(),Mat4{},&frame,&error)&&character_pose_direct_world(scene.rig(),local,frame,&world,&error),"owned supplied pose for lifetime test, NOTplacementproof");
    s=initial;s.model_hash=bound_setup.model_hash;check(material.request(scene,14)&&material.tick(scene,s)&&material.state().channels[1].time==1,"model-boundmaterial uses initializedactor1DC8, no separate deltaargument");
    Recorder backend;RenderEngine render(backend);render.initialize({640,480,false,true,"owned material lifetime"},nullptr,&error);scene.upload_textures(render);Camera camera;camera.eye[2]=10;camera.target[2]=0;camera.far_plane=100;CharacterModelRenderState render_state;render_state.dynamic_material=true;
    check(material.publish(scene,world,render_state,&error),"owned material/geometry publication");render.begin_frame({});render.reset_arena();check(material.draw(scene,render,camera)==2,"both weighted andrigid meshes draw throughowner");render.end_frame();check(backend.calls.size()==2&&backend.calls[0].post_rgb_add==std::array<u8,3>{255,128,64},"derived materialcolor captured inactual modeldraw calls");
    check(!material.publish(other,world,render_state,&error)&&!material.ready()&&material.draw(scene,render,camera)==0,"foreignsame-modelinstance rejects andlatches no oldframe replay");
    check(material.bind(scene,bound_setup,fade)&&!scene.ready()&&material.draw(scene,render,camera)==0,"explicit rebind initializesnewcolor butdoesnot replay previouspublishedgeometry");
    const auto old_binding=scene.instance_binding();check(scene.load({model.data(),model.size()},{prefix.data(),prefix.size()},{pose.data(),pose.size()},&error)&&scene.instance_binding()!=old_binding&&!material.bound(scene)&&!material.tick(scene,s),"reload invalidates material binding evenif modelhash andoldidentity reference are retained");
    check(material.bind(scene,bound_setup,fade)&&material.request(scene,14)&&material.tick(scene,s)&&material.draw(scene,render,camera)==0,"ticked butunpublished frame cannot reuse oldpose");
    auto badworld=world;badworld.joint_offsets[0]=999;check(!material.publish(scene,badworld,render_state,&error)&&!material.ready()&&!scene.ready(),"failedpose invalidates both materialowner andrenderer");
    unsigned real=0;auto class_bytes=read("local/content/startup-actor-class/startup.gfac");StartupActorClassProfile source_classes;
    if(!class_bytes.empty())check(startup_actor_class_load({class_bytes.data(),class_bytes.size()},&source_classes,&error),"source class profile");
    for(const char* name:{"g-red","sasuke"}){auto a=read(std::string("local/content/startup-actor-setup/")+name+".gfcs"),b=read(std::string("local/content/startup-actor-runtime/")+name+".gfbr");if(a.empty()||b.empty())continue;const u32 hash=load_be32(a.data()+8);StartupActorClassState before,after;std::vector<StartupActorClassEffect> requests;
        check(startup_actor_setup_load({a.data(),a.size()},hash,&setup,&error)&&startup_actor_runtime_load({b.data(),b.size()},hash,&p,&error),"actualmodel-boundconfig+constructorrow");before.model_id=p.model_id;
        check(startup_actor_class_initialize(source_classes,before,&after,&requests)&&startup_actor_bookkeeping_initialize(setup,p,after,&s,&error),"actual constructor numericstate seedsgenericclocks/reload/resources");
        check(s.motion_delta==1&&s.timer_delta==1&&s.value736==10000&&s.increment736==2000,"source clock andresource-counter constructor values");
        for(unsigned tick=0;tick<256;++tick){check(startup_actor_pre_update(p,s,{},&s,&error)&&startup_actor_class_pre(source_classes,s.klass,&s.klass)&&startup_actor_class_post(source_classes,s.klass,&s.klass,&requests)&&startup_actor_post_update(p,s,&s),"source scalar/preclass/postclass/postscalar sequence underisolatedstate");}
        check(s.reloads[2].remaining==0&&s.field1eb==1,"source steady state reached withoutcallback invention");++real;
    }
    check(real==0||real==2,"partialoptionalruntimeinputs do not masquerade asfull sourcecoverage");
    std::printf("{\"startup_bookkeeping_checks\":%u,\"source_profiles\":%u,\"source_isolated_updates\":%u,\"material_instance_lifetime_guards\":true,\"full_actor_constructor_placement_or_visual_startup\":false,\"all_passed\":true}\n",checks,real,real*256);
}
