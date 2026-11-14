#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/startup_sound_spatial.hpp" // retained inverse definition, not its test main
#include "tests/unit/startup_static_graphics_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const std::string& why){++checks;if(!ok){std::fprintf(stderr,"static graphics: %s\n",why.c_str());std::abort();}}
struct Test {std::vector<gf::u8> raw;gf::HsdRecordAnimationBank bank;gf::StartupStaticGraphicsProfile profile;gf::StartupStaticGraphicsScene scene;gf::StartupCameraPoseState camera;std::string error;
    Test(){raw=startup_static_graphics_fixture::archive();check(bank.load({raw.data(),raw.size()},&error),error);profile=startup_static_graphics_fixture::profile(bank);camera=startup_static_graphics_fixture::camera(profile);reload();}
    void reload(){check(scene.load(bank,profile,gf::StartupStaticGraphicsMode::DeclaredSceneObservations,&error),error);}
    bool step(unsigned tick,unsigned phase,unsigned mode=1,unsigned gate=0,bool resource=true){gf::StartupDirectorState d;d.updates=tick+1;d.phase=gf::u8(phase);return scene.before_actors(d,gf::u8(mode),resource,&error)&&scene.register_camera(tick+1,camera,gf::u8(gate),&error);}
};
void matrices(){
    for(unsigned n=0;n<128;++n){gf::Mat4 m;for(unsigned r=0;r<3;++r){m.at(r,r)=r==0&&n%2?-1.5f:1.2f+float(r);m.at(r,3)=float(int(n)-64)*float(r+1);}m.at(0,1)=.25f;m.at(1,2)=.5f;m.at(2,0)=-.125f;gf::Mat4 inverse,retained;check(gf::mat4_inverse_affine(m,&inverse)&&gf::startup_sound_spatial_detail::inverse(m,&retained),"bounded full affine inverse");
        for(unsigned k=0;k<16;++k)check(inverse.m[k]==retained.m[k],"established cofactor/f32-buffer arithmetic retained");
        const auto identity=gf::mat4_multiply(m,inverse);for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)check(std::abs(identity.at(r,c)-(r==c?1.f:0.f))<4e-5,"inverse product, including shear/reflection/T");}
    gf::Mat4 bad,out;out.m[3]=42;bad.m[0]=0;check(!gf::mat4_inverse_affine(bad,&out)&&out.m[3]==42,"singular no partial inverse");bad.m[0]=NAN;check(!gf::mat4_inverse_affine(bad,&out)&&out.m[3]==42,"nonfinite inverse refuses");
}
void lifecycle(){
    Test t;const unsigned copyright_slot=36-15,tm_slot=36-16;for(unsigned tick=0;tick<1250;++tick){const unsigned phase=tick<922?3:12,mode=tick<1223?1:2;check(t.step(tick,phase,mode),t.error);const auto& s=*t.scene.states();
        if(tick==0)check(s[0].visible&&!s[copyright_slot].visible&&!s[tm_slot].visible,"ordinary records show at construction; captions wait");
        if(tick==922){check(s[copyright_slot].visible&&s[copyright_slot].substate==1&&s[copyright_slot].alpha_writes==0,"phase12 activation does not overwrite source material");check(!t.scene.model(tm_slot)->pose()->joints[0].matrix_override,"TM keeps authored world, unlike copyright");}
        if(tick==1223)check(s[copyright_slot].alpha==1&&s[copyright_slot].alpha_writes==1&&s[copyright_slot].visible,"mode2 starts fade without same-callback decrement");
        if(tick==1238)check(s[copyright_slot].alpha>0&&s[copyright_slot].alpha<1e-6f&&s[copyright_slot].visible&&t.scene.model(copyright_slot)->pose()->materials[0].alpha==0,"tiny positive f32 alpha has byte0 but remains source-visible");
        if(tick==1239)check(s[copyright_slot].phase==2&&!s[copyright_slot].visible&&s[copyright_slot].alpha<0&&s[copyright_slot].material_alpha>0,"negative accumulator is not clamped or published");
        if(tick==1241)check(s[copyright_slot].phase==4&&!t.scene.model(copyright_slot)->asset()&&s[0].visible,"caption released, ordinary mode2 scene persists");
    }
    t.reload();t.camera.camera.eye[0]=7;t.camera.camera.eye[1]=3;t.camera.camera.eye[2]=12;t.camera.camera.target[0]=-2;t.camera.camera.target[1]=1;t.camera.camera.target[2]=0;
    check(t.step(0,12)&&t.step(1,12),t.error);gf::Mat4 view;check(gf::camera_view_matrix(t.camera.camera,&view),"current camera view");const auto product=gf::mat4_multiply(view,t.scene.states()->at(copyright_slot).world);const auto expected=gf::mat4_multiply(gf::mat4_translation(t.profile.screen_position.data()),gf::mat4_rotation_x(t.profile.screen_rotation_x));for(unsigned i=0;i<12;++i)check(std::abs(product.m[i]-expected.m[i])<2e-5,"view*inverse(view)*screen transform, no fitted camera");
    gf::headless::Render backend;gf::RenderEngine render(backend);check(render.initialize({},nullptr,&t.error),"headless");auto changed=t.camera.camera;changed.eye[0]+=1;check(!t.scene.draw(render,changed)&&!t.scene.ready(),"view-dependent pose cannot replay under another camera");
    t.reload();check(t.step(0,1,3)&&!t.scene.states()->at(0).visible&&t.scene.states()->at(0).phase==2,"ordinary immediate mode guard");
    t.reload();check(t.step(0,12,2)&&!t.scene.states()->at(copyright_slot).visible&&t.step(1,12,2)&&t.scene.states()->at(copyright_slot).visible,"late resource constructor keeps separate phase branch");
    t.reload();t.camera.initialized=false;check(t.step(0,3)&&t.step(1,3),"hidden copyright reads no camera state");check(!t.step(2,12)&&!t.scene.ready(),"missing required view invalidates all draws");
    t.reload();check(t.step(0,12,1,1)&&t.step(1,12,1,1),"display suppression reads no inverse view");
    t.camera=startup_static_graphics_fixture::camera(t.profile);t.reload();check(!t.step(1,3),"skipped source frame");t.reload();check(t.step(0,3)&&!t.step(0,3),"duplicate source frame");
    t.reload();check(t.step(0,3,1,0,false)&&t.scene.states()->at(0).phase==0&&t.step(1,3),"resource wait not forced");auto moved=std::move(t.scene);check(!t.scene.ready()&&moved.ready(),"move ownership");moved.release();check(!moved.ready(),"explicit release");
}
void alpha_guard(){
    Test t;gf::AnimatedModelScene model;check(model.load(t.bank,5,0,0,0,&t.error)&&model.request(0,1,&t.error)&&model.replace_static_alpha(.9f,&t.error)&&model.pose()->materials[0].alpha==229/255.f,"static source alpha truncation");auto moved=std::move(model);check(moved.pose()->materials[0].alpha==229/255.f&&!model.ready()&&moved.advance(1,&t.error),"alpha override moves/persists on static instance");check(!moved.replace_static_alpha(NAN,&t.error)&&!moved.ready(),"bad static alpha invalidates draw");
    hsd_record_animation_fixture::Fixture animated;auto b=animated.archive();gf::HsdRecordAnimationBank bank;check(bank.load({b.data(),b.size()},&t.error)&&model.load(bank,0,3,0,0,&t.error)&&model.request(0,1,&t.error)&&!model.replace_static_alpha(.4f,&t.error),"cannot override live authored callbacks with static API");
}
bool original(){std::ifstream f("local/recomp-evidence/native-startup-animation-resolution-20261109/recovery-v1/tl00_mdl.arc",std::ios::binary);if(!f)return false;std::vector<gf::u8> raw((std::istreambuf_iterator<char>(f)),{});gf::HsdRecordAnimationBank bank;std::string error;check(bank.load({raw.data(),raw.size()},&error),error);for(unsigned r=5;r<=36;++r){gf::HsdRecordAnimationBinding b;check(bank.select(gf::u16(r),3,0,0,&b,&error)&&b.objects()->empty(),error);gf::AnimatedModelScene model;check(model.load(bank,gf::u16(r),0,0,0,&error)&&model.request(0,1,&error),error);}return true;}
}
int main(){matrices();lifecycle();alpha_guard();const bool real=original();std::printf("{\"static_graphics_checks\":%u,\"original_present\":%s,\"all_passed\":true}\n",checks,real?"true":"false");}
