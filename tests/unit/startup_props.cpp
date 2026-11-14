#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "tests/unit/startup_props_fixture.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"startup props: %s\n",why);std::abort();}}
struct Test {
    gf::HsdRecordAnimationBank bank;gf::StartupPropProfile profile;gf::StartupPropScene scene;std::vector<gf::StartupPropSound> cues;std::string error;
    Test(){auto b=startup_props_fixture::archive();check(bank.load({b.data(),b.size()},&error),error.c_str());auto p=startup_props_fixture::profile(bank);check(gf::startup_prop_load({p.data(),p.size()},&profile,&error),error.c_str());reload();}
    void reload(){check(scene.load(bank,profile,gf::StartupPropMode::DeclaredSceneObservations,&error),error.c_str());}
    bool step(unsigned tick,unsigned phase,unsigned mode=1,unsigned gate=0){gf::StartupDirectorState d;d.updates=tick+1;d.phase=gf::u8(phase);return scene.before_actors(d,gf::u8(mode),&error)&&scene.after_actors(tick+1,gf::u8(gate),&cues,&error);}
};
void transform_cases(){
    gf::AttachmentBoneTransformInput in;in.flags=15;in.local_position={2,3,4};in.angles={0,16384,0};gf::AttachmentBoneTransform out;std::string error;
    check(gf::attachment_identity_transform(in,&out,&error)&&out.valid&&out.position==in.local_position,"explicit identity branch");const float point[3]={1,0,0};float p[3];gf::mat4_transform_point(out.world,point,p);check(std::abs(p[0]-2)<1e-6f&&p[1]==3&&std::abs(p[2]-3)<1e-6f,"position before source positiveY rotation");
    for(unsigned n=0;n<64;++n){for(unsigned k=0;k<3;++k)in.angles[k]=gf::startup_prop_detail::wrap(int(n*1973+k*8191)-32768);check(gf::attachment_identity_transform(in,&out,&error),"mixed-angle identity transform");
        double a[3];for(unsigned k=0;k<3;++k)a[k]=gf::f32(gf::f32(in.angles[k])*gf::bits_to_f32(0x38c90fdb));const double x=std::cos(a[0]),X=std::sin(a[0]),y=std::cos(a[1]),Y=std::sin(a[1]),z=std::cos(a[2]),Z=std::sin(a[2]);
        const double m[9]={z*y,z*Y*X-Z*x,z*Y*x+Z*X,Z*y,Z*Y*X+z*x,Z*Y*x-z*X,-Y,y*X,y*x};for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)check(std::abs(out.world.at(r,c)-m[r*3+c])<1e-6,"independent closed-form RzRyRx, no fitted transforms");}
    std::array<gf::Mat4,1> identity;check(!gf::attachment_bone_transform(identity,in,&out,&error),"old copied-bone API still refuses unqualified branch");
    in.flags=0x4f;check(!gf::attachment_identity_transform(in,&out,&error)&&!out.valid,"no fake parent bone to identity branch");in.flags=15;in.bone=1;check(!gf::attachment_identity_transform(in,&out,&error),"identity branch explicit domain");in.bone=0;in.local_position[0]=NAN;check(!gf::attachment_identity_transform(in,&out,&error)&&!out.valid,"invalid transform clears output");
}
void timeline(){
    Test t;unsigned cue378=0,cue379=0;
    for(unsigned tick=0;tick<1350;++tick){const unsigned phase=tick<652?3:tick<712?9:tick<772?10:tick<922?11:12;check(t.step(tick,phase,tick<=1222?1:2),t.error.c_str());const auto& s=*t.scene.states();
        for(const auto& cue:t.cues){cue378+=cue.id==378;cue379+=cue.id==379;check(cue.id==378||cue.id==379,"only original prop sound requirements");}
        if(tick==0)check(!s[0].enabled&&s[0].counter==0&&s[1].counter==24&&s[4].angles[1]==-4096&&s[6].angles[1]==4096,"init callback updates once; delay not decremented before source phase; hidden transforms rotate");
        if(tick==651)check(s[1].counter==24&&s[4].angles[1]==gf::startup_prop_detail::wrap(-4096*652),"full hidden prefix retained, not fitted angular phase");
        if(tick==652)check(s[0].enabled&&s[0].motion==1&&s[0].y==1000&&s[1].counter==23,"delay0 enables without falling same callback");
        if(tick==671)check(s[5].enabled&&s[5].y==1000&&s[4].angles[1]==0,"delayed gear/cap enable and retained angular phase");
        if(tick==683)check(s[0].motion==2&&s[0].y==60&&s[4].motion==2&&s[4].y==0,"original impacts");
        if(tick==691)check(s[4].motion==3&&s[4].y==0&&s[4].spin==-4096,"settle retains angular increment");
        if(tick==712)check(s[0].substate==2&&s[0].counter==30&&s[0].velocity==-2&&s[0].spin_delta==4369&&s[0].y==60,"source screw-down initialization, signed integer division");
        if(tick>712&&tick<742){const unsigned n=tick-712;check(s[0].y==60-2*float(n)&&s[0].spin==gf::startup_prop_detail::wrap(-4369*int(n))&&s[0].angles[1]==gf::startup_prop_detail::wrap(-4369*int(n*(n+1)/2)),"accelerating screw angle, not constant spin or generic easing");}
        if(tick==742)check(s[0].motion==3&&s[0].y==0&&s[0].spin==0&&s[0].spin_delta==0&&s[0].angles[1]==0,"final exact Y and angle resets");
        if(tick==900)check(s[4].transforms==901&&s[4].angles[1]==gf::startup_prop_detail::wrap(-4096*901),"gear still rotates after final settle");
    }
    check(cue378==1&&cue379==2,"three unfulfilled source sound requirements, not replayed");
    check(t.scene.states()->at(4).angles[1]==gf::startup_prop_detail::wrap(-4096*1350)&&t.scene.states()->at(6).angles[1]==gf::startup_prop_detail::wrap(4096*1350),"rotation continues through source menu mode2 handoff");
    t.reload();check(t.step(0,12)&&t.scene.states()->at(0).motion==3&&t.scene.states()->at(0).enabled&&t.scene.states()->at(4).spin==-4096,"phase12 constructor settled branch still rotates gears");
    t.reload();check(t.step(0,1,2)&&t.scene.states()->at(0).motion==3&&t.scene.states()->at(0).y==0,"mode2 constructor settled branch");
    t.reload();check(t.step(0,1)&&t.step(1,1,2)&&t.scene.states()->at(0).motion==1&&t.scene.states()->at(0).y==1000,"later mode2 does not force already constructed prop to settled");
}
void guards(){
    Test t;auto bytes=startup_props_fixture::profile(t.bank);gf::StartupPropProfile p;
    for(unsigned n=0;n<352;++n)check(!gf::startup_prop_load({bytes.data(),n},&p,&t.error)&&!p.loaded,"bounded truncation");
    for(unsigned at:{0u,4u,6u,8u,18u,20u,24u,28u,30u,32u,36u,40u,44u,48u,52u,53u,54u,55u,56u,57u,58u,60u,66u}){auto bad=bytes;bad[at]^=1;check(!gf::startup_prop_load({bad.data(),bad.size()},&p,&t.error)&&!p.loaded,"header/operand/reserved guards");}
    {auto bad=bytes;gf::store_be32(bad.data()+80,gf::f32_to_bits(NAN));check(!gf::startup_prop_load({bad.data(),bad.size()},&p,&t.error),"nonfinite motion input");}
    p=t.profile;p.parts[1].root=p.parts[0].root;check(!t.scene.load(t.bank,p,gf::StartupPropMode::DeclaredSceneObservations,&t.error),"duplicate published root refuses");
    p=t.profile;p.bank_hash^=1;check(!t.scene.load(t.bank,p,gf::StartupPropMode::DeclaredSceneObservations,&t.error),"wrong resource hash refuses");t.reload();
    gf::StartupDirectorState d;d.updates=1;d.phase=3;check(t.scene.before_actors(d,1,&t.error)&&!t.scene.states(),"before-actor work not published");check(!t.scene.before_actors(d,1,&t.error)&&!t.scene.ready(),"duplicate stage refuses and latches");t.reload();
    check(!t.scene.after_actors(1,0,&t.cues,&t.error),"after without before");t.reload();check(t.scene.before_actors(d,1,&t.error)&&!t.scene.after_actors(2,0,&t.cues,&t.error),"mismatched post-actor frame");t.reload();
    check(!t.step(1,3)&&t.scene.frames()==0,"skipped source frame");t.reload();
    auto root=t.scene.model(0)->asset()->joints[0];gf::StartupPropState s,next;std::vector<gf::StartupPropSound> cues;
    root.rotation[0]=.25f;check(gf::startup_prop_initialize(t.profile,0,root,1,1,&s,&t.error)&&s.angles[0]==2607,"source float32 multiply then fctiwz angle conversion");
    root.rotation[0]=7.f;check(gf::startup_prop_initialize(t.profile,0,root,1,1,&s,&t.error)&&s.angles[0]==gf::startup_prop_detail::wrap(gf::s32(gf::f32(t.profile.radians_to_angles*7.f))),"signed16 low-word wrap");
    root.rotation[0]=1e7f;check(!gf::startup_prop_initialize(t.profile,0,root,1,1,&s,&t.error),"unqualified fctiwz overflow refuses");root.rotation[0]=0;check(gf::startup_prop_initialize(t.profile,0,root,1,1,&s,&t.error),"numeric fixture reload");
    s.counter=-32768;check(gf::startup_prop_update(t.profile,0,9,1,s,&next,&cues,&t.error)&&next.counter==32767&&next.motion==0,"counter signed store wraps before source comparison");
    s.motion=2;s.substate=1;s.counter=8;s.settle_limit=8;check(!gf::startup_prop_update(t.profile,4,9,1,s,&next,&cues,&t.error)&&!next.initialized&&cues.empty(),"settle out-of-range refuses without output");
    s.motion=1;s.y=s.velocity=s.acceleration=1e7f;check(!gf::startup_prop_update(t.profile,0,9,1,s,&next,&cues,&t.error)&&!next.initialized,"derived float budget refusal");
    auto altered=startup_props_fixture::archive();gf::store_be_f32(altered.data()+32+0x1700+48,1);gf::HsdRecordAnimationBank badbank;check(badbank.load({altered.data(),altered.size()},&t.error),"late transform fixture bank");
    p=t.profile;p.bank_hash=badbank.hash();p.parts[7].height=10000;check(t.scene.load(badbank,p,gf::StartupPropMode::DeclaredSceneObservations,&t.error),"late transform refusal fixture");
    check(t.scene.before_actors(d,1,&t.error),"late transform pre-stage");
    t.cues.push_back({378,0});check(!t.scene.after_actors(1,0,&t.cues,&t.error)&&t.scene.frames()==0&&t.cues.empty(),"later out-of-domain transform discards earlier candidates and frame/output");
    gf::headless::Render backend;gf::RenderEngine renderer(backend);check(renderer.initialize({},nullptr,&t.error),"headless renderer");check(!t.scene.draw(renderer,gf::Camera{}),"no rejected pose replay");t.reload();
    check(t.step(0,12,1,1)&&t.scene.states()->at(4).angles[1]==-4096&&!t.scene.draw(renderer,gf::Camera{}),"display gate keeps hidden transform active");
    auto moved=std::move(t.scene);check(!t.scene.ready()&&moved.ready(),"move transfers storage");moved.release();check(!moved.ready()&&!moved.states(),"explicit release");t.reload();
    for(unsigned tick=0;tick<8192;++tick)if(!t.step(tick,1))check(false,"bounded waiting prefix");
    check(!t.step(8192,1),"8192 work refusal, not forced completion");
}
}
int main(){transform_cases();timeline();guards();std::printf("{\"startup_prop_checks\":%u,\"owned_only\":true,\"all_passed\":true}\n",checks);}
