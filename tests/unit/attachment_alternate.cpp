#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/attachment_alternate_scene.hpp"
#include "tests/unit/attachment_alternate_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"attachment alternate: %s\n",why);std::abort();}}
std::vector<gf::u8> bytes(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
class Recorder final:public gf::IRenderBackend {
public:
    std::vector<gf::DrawCall> calls;std::vector<gf::Vertex> vertices;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex* v,std::size_t n)override{vertices.assign(v,v+n);return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();vertices.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;auto raw=attachment_alternate_fixture::profile();AttachmentAlternateProfile p;std::string error;
    check(attachment_alternate_load({raw.data(),raw.size()},&p,&error),"owned source-style profile");
    for(unsigned at=0;at<raw.size();++at){auto bad=raw;bad[at]^=0x80;AttachmentAlternateProfile rejected;check(!attachment_alternate_load({bad.data(),bad.size()},&rejected,&error)&&!rejected.loaded,"all malformed/unreviewed operand bytes refuse");}
    AttachmentAlternateState input,style;input.joint_flags=0x10050180;input.dobj_flags=2;input.render_mode=0x400a;input.alpha=.25f;
    check(attachment_alternate_apply(p,input,&style,&error)&&style.joint_flags==0x20090180&&style.dobj_flags==4&&style.render_mode==0x60002001&&style.alpha==bits_to_f32(0x3ecccccd),"source JObj/DObj/MObj pass changes plus REPLACED alpha, not multiplied opacity");
    HsdMesh base,mesh;base.render_mode=input.render_mode;base.material_alpha=.25f;
    check(attachment_alternate_mesh(base,style,&mesh,&error)&&mesh.blend&&!mesh.depth_write&&mesh.depth_test&&mesh.alpha_compare0==7&&mesh.alpha_compare1==7&&!mesh.alpha_write,"no-PE fallback uses source XLU/no-Z-write/default alpha compare");
    input.has_pe=true;input.pe={0x33,128,17,0,0,4,5,15,3,4,0,6};
    check(attachment_alternate_apply(p,input,&style,&error)&&style.pe[0]==0x13&&style.pe[9]==7&&style.pe[11]==6&&style.pe[1]==128&&style.pe[2]==17,"explicit PE only clears write-Z and changes compare0, retains refs/compare1/operator");
    check(attachment_alternate_mesh(base,style,&mesh,&error)&&!mesh.blend&&!mesh.depth_write&&mesh.alpha_write&&mesh.alpha_compare1==6&&mesh.alpha_ref1==17,"PE NONE blending is NOT overridden by XLU render-mode bit");
    style.pe[0]|=4;check(!attachment_alternate_mesh(base,style,&mesh,&error),"destination-alpha PE requires unsupported renderer consumer");style.pe[0]&=~4;style.pe[0]|=64;check(!attachment_alternate_mesh(base,style,&mesh,&error),"dither PE not silently omitted");
    input.joint_flags|=0x1000;check(!attachment_alternate_apply(p,input,&style,&error),"instance branch refuses");
    Recorder backend;RenderEngine render(backend);render.initialize({640,480,false,true,"owned alternate"},nullptr,&error);Camera camera;camera.eye[2]=10;camera.target[2]=0;camera.far_plane=100;camera.fov_y_degrees=90;
    AttachmentBoneTransform transform;transform.valid=true;ActorFadeMaterial fade{{16,32,64},true};AttachmentAlternateModelScene scene;
    for(bool pe:{false,true}){auto bank=attachment_alternate_fixture::archive(pe);AttachmentModelScene normal;
        check(normal.load({bank.data(),bank.size()},0,&error)&&scene.load({bank.data(),bank.size()},0,{raw.data(),raw.size()},&error),"independent normal and alternate model copies load");
        check(normal.asset().scene.meshes[0].render_mode==0x400a&&normal.asset().scene.meshes[0].material_alpha==.25f&&scene.asset().scene.meshes[0].material_alpha==.25f,"source geometry/material immutable; alternate fields live in draw copy/state");
        check(scene.draw(render,camera)==0&&scene.set_world_fade(transform,fade,&error),"alternate requires explicit accepted transform/material");scene.upload_textures(render);
        render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)==1,"alternate rigid geometry reaches draw seam");render.end_frame();
        check(backend.calls.size()==1&&backend.calls[0].blend==!pe&&!backend.calls[0].depth_write&&backend.calls[0].post_rgb_add==std::array<u8,3>{16,32,64},"source PE/default render-state and fade addition reach packet");
        check(!backend.vertices.empty(),"alternate packet has geometry");for(const auto& v:backend.vertices)check(v.a==102&&v.r==255&&v.g==0&&v.b==0,"material-source red and alpha102 replace vertex color/alpha selection");
        check(!normal.set_world_fade(transform,fade,true,&error),"old normal bool-alternate refusal remains unchanged");
        const auto old=scene.instance_binding();check(scene.load({bank.data(),bank.size()},0,{raw.data(),raw.size()},&error)&&old!=scene.instance_binding()&&!scene.ready(),"reload replaces generation and invalidates publication");
        check(scene.set_world_fade(transform,fade,&error),"fresh alternate publication");auto moved=std::move(scene);check(!scene.ready()&&moved.ready(),"move does not permit moved-from draw");scene=std::move(moved);
        auto bad=transform;bad.world.at(1,1)=0;check(!scene.set_world_fade(bad,fade,&error)&&!scene.ready()&&scene.draw(render,camera)==0,"failed normal matrix cannot replay old alternate geometry");
        check(!scene.load({bank.data(),bank.size()},0,{nullptr,1},&error)&&!scene.instance_binding(),"invalid style load clears old generation");
    }
    const unsigned owned=checks;unsigned records=0,draws=0;
    for(const char* name:{"g-red","sasuke"}){auto bank=bytes(std::string("local/content/startup-characters/")+name+"/model.hsd");if(bank.empty())continue;const bool red=std::string(name)=="g-red";
        for(unsigned i=0;i<2;++i){const unsigned record=red?(i?9:10):(i?14:11),expected=red?2:(i?2:4);check(scene.load({bank.data(),bank.size()},u16(record),{raw.data(),raw.size()},&error)&&scene.material_states().size()==expected,"actual complete alternate copy of published attachment record");
            for(const auto& s:scene.material_states())check(!s.has_pe&&s.dobj_flags==4&&(s.render_mode&0x6000200bu)==0x60002001&&s.alpha==p.alpha,"actual material/pass source fields, not opacity-only approximation");
            check(scene.set_world_fade(transform,fade,&error),"actual alternate transform/fade");Camera view;view.eye[2]=1000;view.target[2]=0;view.far_plane=4000;view.fov_y_degrees=100;scene.upload_textures(render);render.begin_frame({});render.reset_arena();const auto n=scene.draw(render,view);render.end_frame();check(n==expected,"all original alternate meshes emit under declared camera, no actor AE claim");draws+=n;records++;
        }
    }
    check(records==0||records==4,"optional four-record coverage cannot be partial");
    std::printf("{\"attachment_alternate_checks\":%u,\"owned_fixture_checks\":%u,\"source_records\":%u,\"declared_source_mesh_draws\":%u,\"parent_AE_or_visible_startup_admitted\":false,\"all_passed\":true}\n",checks,owned,records,draws);
}
