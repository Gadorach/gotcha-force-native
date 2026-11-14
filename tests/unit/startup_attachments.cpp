#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_attachments_fixture.hpp"
#include "engine/core/headless_backends.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup attachments: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
class Recorder final:public gf::IRenderBackend {
public:
    std::vector<gf::DrawCall> calls;bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;std::string error;auto raw=startup_attachments_fixture::profile(123);StartupAttachmentProfile red,sasuke;
    check(startup_attachments_load({raw.data(),raw.size()},123,&red,&error),"owned model-bound two-child profile");auto sraw=startup_attachments_fixture::profile(123,false);check(startup_attachments_load({sraw.data(),sraw.size()},123,&sasuke,&error),"owned three-mode Sasuke profile");
    StartupAttachmentParent parent;parent.model_id=0x615;parent.mode6e8=1;StartupAttachmentState state;StartupAttachmentUpdate result;
    for(unsigned frame=0;frame<12;++frame){check(startup_attachment_state_step(red,0,0,parent,state,&result,&error)&&result.state.phase==1&&result.state.render_group==65&&result.visible,"GRedinitializer includes immediate first growth update");state=result.state;}
    check(state.draw_scale==1,"growth clamps at one");parent.mode6e8=0;
    for(unsigned frame=0;frame<12;++frame){check(startup_attachment_state_step(red,0,0,parent,state,&result,&error),"GRedshrink update");state=result.state;}
    check(state.draw_scale==0&&!result.visible,"zero draw scale hides whole source child, no singular render fallback");
    parent.retire662=1;check(startup_attachment_state_step(red,0,0,parent,state,&result,&error)&&result.unregister&&result.state.phase==3&&result.state.render_group==0&&result.state.allocated,"660E8retire2 then table2 advances3 in SAMEcallback");state=result.state;
    check(startup_attachment_state_step(red,0,0,parent,state,&result,&error)&&result.released&&!result.state.allocated,"next update performs free-stage release");state=result.state;check(startup_attachment_state_step(red,0,0,parent,state,&result,&error)&&!result.released&&!result.unregister,"no duplicate native release/unregister");
    parent={};parent.model_id=10;parent.reload784=1;
    for(unsigned mode=0;mode<3;++mode){parent.mode6e8=u8(mode|0x80);for(unsigned i=0;i<2;++i){check(startup_attachment_state_step(sasuke,i,1,parent,{},&result,&error)&&result.state.mode==mode&&result.visible==(i==0||mode==2),"Sasuke6E8low7 mode, even child always/odd mode2 reload gate");}}
    parent.mode6e8=2;parent.reload784=-1;check(startup_attachment_state_step(sasuke,1,1,parent,{},&result,&error)&&!result.visible,"reload784 is SIGNED, not unsigned positivity");parent.reload784=1;parent.selector3ec=4;check(startup_attachment_state_step(sasuke,1,1,parent,{},&result,&error)&&result.alternate,"alternate requirement preserved instead of alpha-only rendering");
    parent.mode6e8=3;check(!startup_attachment_state_step(sasuke,0,1,parent,{},&result,&error),"unimported mode refuses atomically");parent.mode6e8=0;parent.model_id=999;check(startup_attachment_state_step(sasuke,0,1,parent,{},&result,&error)&&result.unregister&&result.state.phase==3,"model mismatch retirement before initialization");
    for(unsigned at:{0u,4u,6u,8u,12u,16u,18u,19u,20u,24u,28u,32u,36u,37u,40u,41u,44u,45u,64u}){auto bad=raw;bad[at]^=0x80;StartupAttachmentProfile rejected;check(!startup_attachments_load({bad.data(),bad.size()},123,&rejected,&error)&&!rejected.loaded,"malformed/foreign profile fields refuse");}
    startup_attachments_fixture::Fixture fixture;check(fixture.load(&error),"owned bank/frame with two separate rigid child records");StartupActorFrame actor;StartupActorFrameOutput output;StartupActorBankAttachments owner;
    check(actor.load(fixture.frame.assets(),StartupActorFrameMode::IsolatedRequirements,0,&output,&error),"owned parent construction components");
    const auto profile=ByteView{fixture.attachments.data(),fixture.attachments.size()},fade=ByteView{fixture.frame.fade.data(),fixture.frame.fade.size()},bank=ByteView{fixture.frame.motion.model.data(),fixture.frame.motion.model.size()};
    check(owner.bind(actor,bank,profile,fade,StartupAttachmentOwnerMode::DeclaredNormalParent,&error)&&owner.frames()==0,"normal companion binds fresh native parent/model/material generation, no alternate/aux consumer claim");
    Recorder backend;RenderEngine render(backend);render.initialize({640,480,false,true,"owned child owner"},nullptr,&error);owner.upload_textures(render);Camera camera;camera.eye[2]=200;camera.target[2]=0;camera.far_plane=1000;camera.fov_y_degrees=90;
    StartupActorFrameInput input;input.isolated_external_mutations=input.placement_declared=true;input.position={3,4,5};
    check(owner.draw(actor,render,camera)==0&&actor.reset(&output,&error)&&actor.request(0,&output,&error)&&actor.step(input,&output,&error)&&owner.update(actor,&error),"accepted parent world/material drives first source child update");
    check((*owner.states())[0].draw_scale==.1f&&(*owner.states())[1].draw_scale==0,"mode comes from6E8control0[2], not opaque model selection");
    render.begin_frame({});render.reset_arena();check(owner.draw(actor,render,camera)==1,"only positive-scale GRedchild draws");render.end_frame();
    const auto rgba=actor.state()->material.state().material_rgba;check(backend.calls.size()==1&&backend.calls[0].post_rgb_add==std::array<u8,3>{u8(rgba>>24),u8(rgba>>16),u8(rgba>>8)},"child shares actual bound parent material sample, no free RGB or independent clock");
    AttachmentBoneTransform expected;check(attachment_bone_transform(actor.state()->world.matrices,(*owner.states())[0].transform,&expected,&error),"accepted parent-to-child transform scope");
    const auto& original=owner.model(0)->asset().scene.meshes[0].vertices[0];const auto& actual=owner.model(0)->world_geometry().world_vertices[0][0];const float scaled[3]={original.position[0]*.1f,original.position[1]*.1f,original.position[2]*.1f};float expected_position[3];mat4_transform_point(expected.world,scaled,expected_position);
    for(unsigned k=0;k<3;++k)check(std::abs(actual.position[k]-expected_position[k])<1e-4f,"source render-only144 scale follows parent-bone chain without authored root999 transform");
    actor.set_render_enabled(false);render.begin_frame({});render.reset_arena();check(owner.draw(actor,render,camera)==1,"child render group independent of parent's hidden group, no invented inherited hide");render.end_frame();
    auto moved=std::move(owner);check(!owner.ready()&&moved.ready()&&moved.bound(actor),"companion move retains heap models and logical parent scope");
    const auto before=(*moved.states())[0].draw_scale;check(!moved.update(actor,&error)&&!moved.ready()&&(*moved.states())[0].draw_scale==before&&moved.frames()==1&&moved.draw(actor,render,camera)==0,"duplicate parent frame refuses, child bookkeeping rolls back and old draw invalidates");
    check(!moved.bind(actor,bank,profile,fade,StartupAttachmentOwnerMode::DeclaredNormalParent,&error),"cannot reconstruct old child history by binding mid-parent-life");
    StartupActorFrame neighbour;check(neighbour.load(fixture.frame.assets(),StartupActorFrameMode::IsolatedRequirements,0,&output,&error)&&moved.bind(neighbour,bank,profile,fade,StartupAttachmentOwnerMode::DeclaredNormalParent,&error)&&!moved.bound(actor),"same-bank neighbouring actor has distinct generation");
    const auto retained=neighbour.model()->instance_binding();check(neighbour.load(fixture.frame.assets(),StartupActorFrameMode::IsolatedRequirements,0,&output,&error)&&!moved.bound(neighbour)&&moved.draw(neighbour,render,camera)==0,"same actor reload rejects old scope even when old identity kept alive");(void)retained;
    check(moved.bind(neighbour,bank,profile,fade,StartupAttachmentOwnerMode::DeclaredNormalParent,&error)&&neighbour.step(input,&output,&error)&&neighbour.step(input,&output,&error)&&!moved.update(neighbour,&error)&&moved.frames()==0,"skipped parent updates do not synthesize catch-up child ticks");
    check(!moved.bind(neighbour,{nullptr,1},profile,fade,StartupAttachmentOwnerMode::DeclaredNormalParent,&error),"invalid bank refuses before hash access");
    auto bad_profile=fixture.attachments;bad_profile[76]=31;
    check(neighbour.load(fixture.frame.assets(),StartupActorFrameMode::IsolatedRequirements,0,&output,&error)&&moved.bind(neighbour,bank,{bad_profile.data(),bad_profile.size()},fade,StartupAttachmentOwnerMode::DeclaredNormalParent,&error)&&neighbour.request(0,&output,&error)&&neighbour.step(input,&output,&error),"owned second child selects unrepresented parent bone");
    check(!moved.update(neighbour,&error)&&moved.frames()==0&&(*moved.states())[0].phase==0&&(*moved.states())[0].draw_scale==0&&neighbour.state()->frames==1&&moved.draw(neighbour,render,camera)==0,"later child failure discards both numeric steps and publications, separately accepted parent is NOT rolled back");
    unsigned profiles=0,frames=0,draws=0;const auto classes=read("local/content/startup-actor-class/startup.gfac"),root=read("local/content/startup-root/root.gfrs"),fade_raw=read("local/content/actor-fade/startup.gfaf");
    for(const char* name:{"g-red","sasuke"}){auto model=read(std::string("local/content/startup-characters/")+name+"/model.hsd");if(model.empty())continue;
        auto prefix=read(std::string("local/content/startup-characters/")+name+"/model-prefix.bin"),pose=read(std::string("local/content/character-pose-bindings/")+name+".gfcp"),program=read(std::string("local/content/character-program-bindings/")+name+".gfmp"),setup=read(std::string("local/content/startup-actor-setup/")+name+".gfcs"),runtime=read(std::string("local/content/startup-actor-runtime/")+name+".gfbr"),attach=read(std::string("local/content/startup-attachments/")+name+".gfat");
        HsdModelBones bones;check(hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bones,&error),"source body binding");std::vector<CharacterMotionClipBinding> clips;
        for(unsigned i:{0,1,3,4,7,8,9}){auto raw_clip=read(std::string("local/content/startup-motion-programs/")+name+"/motions/motion-0"+std::to_string(i)+".hsd");CharacterMotionClipBinding c;c.index=u8(i);check(hsd_bone_motion_load({raw_clip.data(),raw_clip.size()},bones,&c.clip,&error),"source motion library");clips.push_back(std::move(c));}
        StartupActorFrameAssets assets={{model.data(),model.size()},{prefix.data(),prefix.size()},{pose.data(),pose.size()},{program.data(),program.size()},{setup.data(),setup.size()},{runtime.data(),runtime.size()},{classes.data(),classes.size()},{root.data(),root.size()},{fade_raw.data(),fade_raw.size()},clips};const bool red_actor=std::string(name)=="g-red";
        check(actor.load(assets,StartupActorFrameMode::IsolatedRequirements,red_actor?0:1,&output,&error)&&owner.bind(actor,{model.data(),model.size()},{attach.data(),attach.size()},{fade_raw.data(),fade_raw.size()},StartupAttachmentOwnerMode::DeclaredNormalParent,&error),"source normal child companion/profile bound before first parent update");owner.upload_textures(render);check(actor.reset(&output,&error)&&actor.request(7,&output,&error),"source request7 not replaced/forced");
        Camera view;view.eye[2]=1500;view.target[2]=0;view.far_plane=4000;view.fov_y_degrees=100;
        for(unsigned frame=0;frame<128;++frame){check(actor.step(input,&output,&error)&&owner.update(actor,&error),"source sequential parent/child update under declared normal unrepresented fields");render.begin_frame({});render.reset_arena();const unsigned count=owner.draw(actor,render,view);render.end_frame();check(red_actor?count==0:count==4,"source GRedmode0 child scale stayszero/Sasuke6 visible while7 stays hidden");frames++;draws+=count;if(actor.state()->dispatch.state().substate==2)break;}
        if(red_actor)check(actor.state()->vm.state().lanes[0].motion.frame==10&&actor.state()->vm.state().lanes[0].threshold==24,"GRedrequest7hold remains unforced by companion");
        profiles++;
    }
    check(profiles==0||profiles==2,"optional source coverage requires both actors");
    std::printf("{\"startup_attachment_checks\":%u,\"source_profiles\":%u,\"declared_parent_child_frames\":%u,\"source_mesh_draws\":%u,\"normal_parent_zero_fields_declared\":true,\"full_constructor_or_visible_startup\":false,\"all_passed\":true}\n",checks,profiles,frames,draws);
}
