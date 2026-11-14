#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/animated_model_scene.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
#include "tests/unit/attachment_alternate_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* message){++checks;if(!ok){std::fprintf(stderr,"translucent record: %s\n",message);std::abort();}}}
int main(){
    using namespace gf;std::string error;auto profile_bytes=attachment_alternate_fixture::profile();AttachmentAlternateProfile style;
    check(attachment_alternate_load({profile_bytes.data(),profile_bytes.size()},&style,&error),"source-style fixture");
    hsd_record_animation_fixture::Fixture fixture;const auto bytes=fixture.archive();HsdRecordAnimationBank bank;
    check(bank.load({bytes.data(),bytes.size()},&error),"owned bank");
    HsdRecordAnimationBinding normal,translucent,alternate;
    check(bank.select_compact_textures(0,0,0,0,&normal,&error)&&bank.select_translucent_textures(0,0,0,0,style,&translucent,&error,true)&&bank.select_alternate_textures(0,0,0,0,style,&alternate,&error,true),"separate normal/flags-only/flags-plus-alpha entries");
    for(unsigned i=0;i<normal.scene()->meshes.size();++i){const auto& n=normal.scene()->meshes[i];const auto& t=translucent.scene()->meshes[i];const auto& a=alternate.scene()->meshes[i];
        check(t.material_alpha==n.material_alpha&&a.material_alpha==.4f,"flags-only keeps original alpha, old alternate still replaces it");
        check(t.render_mode==a.render_mode&&t.depth_write==a.depth_write&&t.blend==a.blend&&t.blend_src==a.blend_src&&t.blend_dst==a.blend_dst&&t.alpha_compare0==a.alpha_compare0,"same shared source flag/PE kernel");
        check(t.vertices.size()==n.vertices.size()&&t.indices==n.indices,"same complete original geometry");
    }
    AnimatedModelScene owner;check(owner.load_compact_translucent_textures(bank,0,3,0,0,style,&error)&&owner.request_selectors(2,1,&error),"new scene style can combine independent clocks");
    check(owner.replace_unanimated_alpha(.25f,&error)==false&&!owner.ready(),"live material alpha callback still owns alpha, no guard bypass");
    check(owner.load_compact_translucent_textures(bank,0,0,0,0,style,&error)&&owner.request_selectors(0,0,&error)&&owner.replace_static_alpha(.25f,&error),"static source alpha explicitly replaceable");
    for(const auto& material:owner.pose()->materials)check(material.alpha==63/255.f,"existing draw-alpha conversion once");
    auto invalid=style;invalid.words[6]=0;check(!bank.select_translucent_textures(0,0,0,0,invalid,&translucent,&error)&&!translucent.scene(),"unreviewed style refused and old binding cleared");
    std::printf("{\"translucent_record_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
