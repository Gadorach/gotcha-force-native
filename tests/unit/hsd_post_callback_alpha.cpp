#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/animated_model_scene.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"post material alpha: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;hsd_record_animation_fixture::Fixture fixture;const auto bytes=fixture.archive();HsdRecordAnimationBank bank;std::string error;
    check(bank.load({bytes.data(),bytes.size()},&error),"owned bank");
    AnimatedModelScene ordinary;check(ordinary.load(bank,0,3,0,0,&error)&&ordinary.request_selectors(0,0,&error),"ordinary requested animated record");
    check(!ordinary.replace_unanimated_alpha(.5f,&error)&&!ordinary.ready(),"old live-alpha refusal remains");
    AnimatedModelScene scene;check(!scene.replace_after_material_callbacks_alpha(.5f,&error)&&!scene.ready(),"unloaded explicit owner refuses");
    check(scene.load(bank,0,3,0,0,&error),"reload fresh owner");check(!scene.replace_after_material_callbacks_alpha(.5f,&error),"unrequested explicit owner refuses");
    check(scene.load(bank,0,3,0,0,&error)&&scene.request_selectors(0,0,&error),"fresh actual callbacks");
    const auto clock=scene.animation().frame();
    check(scene.replace_after_material_callbacks_alpha(.5f,&error)&&scene.pose()&&!scene.pose()->materials.empty(),"explicit post-callback owner accepted");
    for(const auto& material:scene.pose()->materials)check(material.alpha==127/255.f,"post-callback normalized byte replaces each final material alpha");
    check(scene.animation().frame()==clock,"alpha publication does not tick callbacks");
    check(scene.request_again_selectors(1,1,&error)&&scene.replace_after_material_callbacks_alpha(.25f,&error),"underlying material tracks remain evaluable with later final override");
    for(const auto& material:scene.pose()->materials)check(material.alpha==63/255.f,"new final alpha after new material sample");
    AnimatedModelScene moved(std::move(scene));check(moved.ready()&&!scene.ready()&&!scene.replace_after_material_callbacks_alpha(.4f,&error),"move keeps sole live final-alpha owner");
    check(!moved.replace_after_material_callbacks_alpha(std::numeric_limits<f32>::quiet_NaN(),&error)&&!moved.ready(),"nonfinite late alpha invalidates publication");
    check(!moved.replace_after_material_callbacks_alpha(.5f,&error),"failed instance cannot replay prior alpha pose");
    check(moved.load(bank,0,3,0,0,&error)&&moved.request_selectors(0,0,&error)&&!moved.replace_static_alpha(.5f,&error),"old static guard still refuses animated record");
    std::printf("{\"post_callback_alpha_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
