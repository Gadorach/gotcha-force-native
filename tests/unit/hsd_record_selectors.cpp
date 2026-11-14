#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {unsigned checks=0;void check(bool value,const char* text){++checks;if(!value){std::fprintf(stderr,"record selectors: %s\n",text);std::abort();}}}
int main(){
    using namespace gf;hsd_record_animation_fixture::Fixture f;auto bytes=f.archive();std::string error;HsdRecordAnimationBank bank;HsdRecordAnimationBinding binding;
    check(bank.load({bytes.data(),bytes.size()},&error)&&bank.select(0,3,0,0,&binding,&error),"owned joint/material binding");
    for(f32 jt:{0.f,.5f,1.f,3.f})for(f32 mt:{0.f,.5f,1.f,2.f}){
        HsdRecordAnimationPlayer joint,material,mixed;
        check(joint.bind(binding,&error)&&joint.request(jt,0,&error)&&material.bind(binding,&error)&&material.request(mt,0,&error)&&mixed.bind(binding,&error)&&mixed.request_selectors(jt,mt,&error),"independent requested selectors reuse same FObj kernel");
        const auto* p=mixed.pose();check(p&&p->joints.size()==joint.pose()->joints.size()&&p->materials.size()==material.pose()->materials.size(),"same immutable topology");
        for(unsigned i=0;i<p->joints.size();++i){const auto& a=p->joints[i];const auto& b=joint.pose()->joints[i];check(a.hidden==b.hidden,"joint visibility uses joint selector");for(unsigned k=0;k<3;++k)check(a.translation[k]==b.translation[k]&&a.rotation[k]==b.rotation[k]&&a.scale[k]==b.scale[k],"joint scalar uses joint selector");}
        for(unsigned i=0;i<p->materials.size();++i){const auto& a=p->materials[i];const auto& b=material.pose()->materials[i];check(a.alpha==b.alpha&&a.present==b.present,"material alpha selector");for(unsigned k=0;k<3;++k)check(a.ambient[k]==b.ambient[k]&&a.diffuse[k]==b.diffuse[k]&&a.specular[k]==b.specular[k],"material channels use material selector");}
        const auto trace=mixed.trace();for(unsigned i=0;i<trace.size();++i)check(trace[i].frame==(binding.objects()->at(i).kind==hsd_record_detail::Object::Joint?jt:mt),"each object's retained clock is explicit, no mislabeled common time");
        check(mixed.request_again_selectors(mt,jt,&error)&&mixed.pose()->joints[0].translation[0]==material.pose()->joints[0].translation[0],"fresh FIRST_PLAY re-request, not accumulated channels");
    }
    for(f32 bad:{-1.f,std::numeric_limits<f32>::infinity(),std::numeric_limits<f32>::quiet_NaN(),f32(hsd_anim_detail::kMaxFrame)+1}){
        HsdRecordAnimationPlayer p;check(p.bind(binding,&error)&&!p.request_selectors(0,bad,&error)&&!p.pose()&&!p.request(0,0,&error),"invalid material selector fails closed");
        check(p.bind(binding,&error)&&!p.request_selectors(bad,0,&error)&&!p.pose(),"invalid joint selector fails closed");
    }
    AnimatedModelScene model;headless::Render backend;RenderEngine render(backend);RenderConfig config;
    check(render.initialize(config,nullptr,&error)&&model.load(bank,0,3,0,0,&error)&&model.request_selectors(1,0,&error),"same scene path publishes separate selectors");
    model.set_enabled(true);Camera camera;camera.eye[0]=camera.eye[1]=0;camera.eye[2]=20;camera.target[0]=camera.target[1]=camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=100;
    render.begin_frame({});check(model.draw(render,camera)==3,"all owned scene meshes through shared renderer");
    check(model.request_again_selectors(2,1,&error)&&model.pose()->joints[0].translation[0]==2,"scene explicit selectors update source pose");
    auto moved=std::move(model);check(!model.ready()&&moved.ready()&&!model.request_again_selectors(0,0,&error),"move transfers draw owner");
    check(!moved.request_again_selectors(0,NAN,&error)&&!moved.pose()&&!moved.draw(render,camera),"failed selector never replays draw");
    std::printf("{\"record_selector_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
