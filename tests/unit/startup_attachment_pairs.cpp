#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/startup_attachment_pairs.hpp"
#include "tests/unit/startup_attachments_fixture.hpp"
#include "tests/unit/attachment_alternate_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"attachment pairs: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
class Recorder final:public gf::IRenderBackend {
public:
    std::vector<gf::DrawCall> calls;std::vector<gf::Vertex> vertices;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex* v,std::size_t n)override{vertices.assign(v,v+n);return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();vertices.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;std::string error;StartupRenderSelectionInput select;StartupRenderSelection chosen;
    check(startup_render_selection(select,255,&chosen,&error)&&chosen.alternate_ae==255&&!chosen.written,"skipped registration retains byteAE without synthesizing normal style");
    select.parent_callback_called=true;select.viewer_slot=-1;select.timer720=1;select.scene_counter=0;
    for(unsigned clock=0;clock<8;++clock){select.scene_counter=clock;check(startup_render_selection(select,1,&chosen,&error)&&chosen.written&&chosen.alternate_ae==bool(clock&2),"source blink tests scene counter bit1, not motion frame or every-other-frame parity");}
    select.scene_counter=0xffffffff;select.stored_actor_index=1;check(startup_render_selection(select,0,&chosen,&error)&&chosen.alternate_ae==0,"source wrapping counter+stored actor slot");
    StartupRenderViewerObservation viewer{0,1,0,0};select={};select.parent_callback_called=true;select.viewer_slot=0;select.mask270=1;select.viewers={&viewer,1};
    check(startup_render_selection(select,0,&chosen,&error)&&chosen.alternate_ae==1,"resolved eligible viewer +270 bit selects alternate");
    viewer.behaviour43d=130;check(startup_render_selection(select,0,&chosen,&error)&&chosen.alternate_ae==0&&startup_render_viewer_query(viewer)==1,"viewer query masks high behaviour bit before2..9 classification");
    viewer.mode=2;viewer.behaviour43d=0;check(startup_render_viewer_query(viewer)==0x80000000u,"viewer mode2 high-bit query disqualifies selection");
    select.force59a=255;check(startup_render_selection(select,0,&chosen,&error)&&chosen.alternate_ae==1,"any nonzero59A overrides disqualified viewer result");
    select.viewers={};check(!startup_render_selection(select,0,&chosen,&error),"force flag does not erase earlier required viewer lookup");
    select.state4a0=2;check(startup_render_selection(select,1,&chosen,&error)&&chosen.alternate_ae==1&&!chosen.written,"4A0early return retainsAE and does not read unavailable viewer");
    startup_attachments_fixture::Fixture fixture;check(fixture.load(&error),"owned native parent with two distinct attachment records");store_be16(fixture.frame.program.data()+92+30,24); // declared owned nonterminal wait, not source-game timing
    auto style=attachment_alternate_fixture::profile();StartupActorFrame parent;StartupActorFrameOutput output;StartupAttachmentPairs pairs;
    const ByteView bank{fixture.frame.motion.model.data(),fixture.frame.motion.model.size()},profile{fixture.attachments.data(),fixture.attachments.size()},fade{fixture.frame.fade.data(),fixture.frame.fade.size()},alternate{style.data(),style.size()};
    auto fresh=[&](){return parent.load(fixture.frame.assets(),StartupActorFrameMode::IsolatedRequirements,0,&output,&error)&&pairs.bind(parent,bank,profile,fade,alternate,StartupAttachmentPairMode::DeclaredSceneObservations,&error);};
    check(fresh(),"pair owner creates four source-style model instances before first parent frame");
    check(pairs.normal(0)->instance_binding()!=pairs.alternate(0)->instance_binding()&&pairs.normal(0)->instance_binding()->root==pairs.alternate(0)->instance_binding()->root,"normal/alternate copies share record identity but not native generation");
    Recorder backend;RenderEngine render(backend);render.initialize({640,480,false,true,"owned pairs"},nullptr,&error);pairs.upload_textures(render);Camera camera;camera.eye[2]=200;camera.target[2]=0;camera.far_plane=1000;camera.fov_y_degrees=90;
    StartupActorFrameInput actor_input;actor_input.isolated_external_mutations=actor_input.placement_declared=true;actor_input.position={3,4,5};StartupAttachmentPairInput input;input.scene_fields_declared=input.unmasked_child_tick_declared=true;input.registration.scene_counter=2;input.registration.timer720=1000;input.registration.stored_actor_index=5;
    auto step=[&](){return parent.step(actor_input,&output,&error)&&pairs.step(parent,input,&error);};
    auto draw=[&](){render.begin_frame({});render.reset_arena();const auto n=pairs.draw(parent,render,camera);render.end_frame();return n;};
    check(parent.reset(&output,&error)&&parent.request(0,&output,&error)&&step()&&draw()==1,"normal first child source stage selected under declared scene observations");
    check(pairs.selection()->alternate_ae==0&&pairs.normal(0)->ready()&&!pairs.alternate(0)->ready()&&!backend.calls[0].blend,"caller timer/slot are ignored; actual parent timer0 prevents fake blink");
    input.registration.force59a=1;check(step()&&draw()==1&&pairs.selection()->alternate_ae==1&&!pairs.normal(0)->ready()&&pairs.alternate(0)->ready()&&backend.calls[0].blend&&!backend.calls[0].depth_write,"source59A select switches only registered copy, not geometry/motion/time substitution");
    check(!backend.vertices.empty()&&backend.vertices[0].a==102,"selected alternate packet retains source material alpha.4");
    input.registration.force59a=0;input.registration.state4a0=2;check(step()&&draw()==1&&pairs.selection()->alternate_ae==1&&!pairs.selection()->written,"skipped body selector retains alternate for child render callback");
    input.registration.state4a0=0;input.parent_active0=0;check(step()&&draw()==1&&pairs.selection()->alternate_ae==1&&!pairs.selection()->written,"inactive parent callback retainsAE without fabricating a retirement; eligible child nodes still register");input.parent_active0=1;
    input.registration.state4a0=0;parent.set_render_enabled(false);check(step()&&draw()==0&&pairs.selection()->alternate_ae==1&&!pairs.selection()->written&&(*pairs.states())[0].phase==1&&(*pairs.states())[0].render_group==65,"startup actor-list gate suppresses child render callbacks, not child group/lifetime");
    parent.set_render_enabled(true);input.scene34=1;check(step()&&draw()==0&&pairs.selection()->alternate_ae==1&&!pairs.selection()->written,"scene34 display gate retainsAE and suppresses paired rendering");
    input.scene34=0;check(step()&&draw()==1&&pairs.selection()->alternate_ae==0&&pairs.normal(0)->ready()&&!pairs.alternate(0)->ready(),"registration resumes source selector, no stale alternate publication");
    const auto old_normal=pairs.normal(0)->instance_binding(),old_alternate=pairs.alternate(0)->instance_binding();input.retire662=1;
    check(step()&&draw()==0&&(*pairs.updates())[0].unregister&&(*pairs.states())[0].phase==3&&pairs.normal(0)->instance_binding()==old_normal,"retirement unregisters before table2 advances to3; copies retained until release tick");
    check(step()&&draw()==0&&(*pairs.updates())[0].released&&!(*pairs.states())[0].allocated&&!pairs.normal(0)->instance_binding()&&!pairs.alternate(0)->instance_binding(),"next release tick destroys both owned CPU instances/generations even if old identity tokens remain alive");
    check(step()&&!(*pairs.updates())[0].released&&!(*pairs.updates())[0].unregister,"no repeated release or automatic re-creation");(void)old_alternate;
    check(!pairs.bind(parent,bank,profile,fade,alternate,StartupAttachmentPairMode::DeclaredSceneObservations,&error),"no mid-life reconstruction of child history");
    check(fresh()&&parent.request(0,&output,&error)&&step(),"fresh parent generation starts separate paired history");auto moved=std::move(pairs);check(!pairs.ready()&&moved.ready()&&moved.bound(parent),"move-safe paired owner");
    const auto frames=moved.frames();check(!moved.step(parent,input,&error)&&moved.frames()==frames&&!moved.ready()&&moved.draw(parent,render,camera)==0,"duplicate child update rejects and invalidates all four publications");
    pairs=std::move(moved);input={};input.scene_fields_declared=input.unmasked_child_tick_declared=true;
    check(fresh()&&parent.request(0,&output,&error)&&parent.step(actor_input,&output,&error),"late selection-refusal setup");input.registration.viewer_slot=0;
    check(!pairs.step(parent,input,&error)&&pairs.frames()==0&&(*pairs.states())[0].phase==0&&parent.state()->frames==1&&draw()==0,"missing viewer fails AFTER staged child updates; child/AE counter rollback, separately accepted parent not rolled back");
    auto tiny=fixture.attachments;store_be_f32(tiny.data()+32,1e-8f);check(parent.load(fixture.frame.assets(),StartupActorFrameMode::IsolatedRequirements,0,&output,&error)&&pairs.bind(parent,bank,{tiny.data(),tiny.size()},fade,alternate,StartupAttachmentPairMode::DeclaredSceneObservations,&error)&&parent.request(0,&output,&error)&&parent.step(actor_input,&output,&error),"owned tiny growth stages valid child state but invalid render normals");input.registration.viewer_slot=-1;input.registration.force59a=1;
    check(!pairs.step(parent,input,&error)&&pairs.frames()==0&&(*pairs.states())[0].draw_scale==0&&!pairs.normal(0)->ready()&&!pairs.alternate(0)->ready()&&parent.state()->frames==1,"late alternate skin failure discards both children/selection/publications, not parent frame");
    input={};input.scene_fields_declared=input.unmasked_child_tick_declared=true;check(fresh()&&parent.request(0,&output,&error)&&step(),"scope-retention setup");const auto old_parent=parent.model()->instance_binding();check(parent.load(fixture.frame.assets(),StartupActorFrameMode::IsolatedRequirements,0,&output,&error)&&!pairs.bound(parent)&&draw()==0,"parent reload rejects scope even with old identity alive");(void)old_parent;
    check(fresh()&&parent.request(0,&output,&error)&&parent.step(actor_input,&output,&error)&&!pairs.step(parent,{},&error)&&!pairs.ready(),"missing scene/tick declaration refuses, no implicit dispatcher adoption");
    const unsigned owned=checks;unsigned actors=0,source_frames=0,source_draws=0;
    const auto classes=read("local/content/startup-actor-class/startup.gfac"),root=read("local/content/startup-root/root.gfrs"),source_fade=read("local/content/actor-fade/startup.gfaf"),source_alternate=read("local/content/attachment-alternate/style.gfaa");
    for(const char* name:{"g-red","sasuke"}){auto model=read(std::string("local/content/startup-characters/")+name+"/model.hsd");if(model.empty())continue;
        auto prefix=read(std::string("local/content/startup-characters/")+name+"/model-prefix.bin"),pose=read(std::string("local/content/character-pose-bindings/")+name+".gfcp"),program=read(std::string("local/content/character-program-bindings/")+name+".gfmp"),setup=read(std::string("local/content/startup-actor-setup/")+name+".gfcs"),runtime=read(std::string("local/content/startup-actor-runtime/")+name+".gfbr"),attachment=read(std::string("local/content/startup-attachments/")+name+".gfat");
        HsdModelBones bones;check(hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bones,&error),"source body bone bindings");std::vector<CharacterMotionClipBinding> clips;
        for(unsigned i:{0,1,3,4,7,8,9}){auto raw=read(std::string("local/content/startup-motion-programs/")+name+"/motions/motion-0"+std::to_string(i)+".hsd");CharacterMotionClipBinding c;c.index=u8(i);check(hsd_bone_motion_load({raw.data(),raw.size()},bones,&c.clip,&error),"source concrete motion clip");clips.push_back(std::move(c));}
        StartupActorFrameAssets a={{model.data(),model.size()},{prefix.data(),prefix.size()},{pose.data(),pose.size()},{program.data(),program.size()},{setup.data(),setup.size()},{runtime.data(),runtime.size()},{classes.data(),classes.size()},{root.data(),root.size()},{source_fade.data(),source_fade.size()},clips};const bool red=std::string(name)=="g-red";
        check(parent.load(a,StartupActorFrameMode::IsolatedRequirements,red?0:1,&output,&error)&&pairs.bind(parent,{model.data(),model.size()},{attachment.data(),attachment.size()},{source_fade.data(),source_fade.size()},{source_alternate.data(),source_alternate.size()},StartupAttachmentPairMode::DeclaredSceneObservations,&error),"four source model copies owned before first source parent frame");pairs.upload_textures(render);check(parent.reset(&output,&error)&&parent.request(7,&output,&error),"source request7 remains unchanged");
        for(unsigned frame=0;frame<64;++frame){input={};input.scene_fields_declared=input.unmasked_child_tick_declared=true;input.registration.scene_counter=frame;input.registration.force59a=frame%4==1;input.registration.state4a0=frame%4==2?2:0;input.scene34=frame%8==7;input.selector3ec=!red&&frame%4==3?4:0;
            check(parent.step(actor_input,&output,&error)&&pairs.step(parent,input,&error),"source parent/paired update under DECLARED scene selection observations");Camera view;view.eye[2]=1500;view.target[2]=0;view.far_plane=4000;view.fov_y_degrees=100;render.begin_frame({});render.reset_arena();const auto n=pairs.draw(parent,render,view);render.end_frame();check(n==(red||input.scene34||!parent.state()->dispatch.state().render_group?0u:4u),"source GRedmode0 stays hidden; Sasuke child6 uses selected complete copy and actor-list display gate");
            if(n){const bool style_selected=pairs.selection()->alternate_ae||input.selector3ec==4;check(backend.calls[0].blend==style_selected&&!backend.vertices.empty()&&backend.vertices[0].a==(style_selected?102:255),"source paired packet carries AE/3EC-selected material state and alpha, not an opacity-only shortcut");}
            source_frames++;source_draws+=n;
        }
        if(red)check(parent.state()->vm.state().lanes[0].motion.frame==10&&parent.state()->vm.state().lanes[0].threshold==24,"GRedhold not forced by paired rendering");
        actors++;
    }
    check(actors==0||actors==2,"optional source actor coverage cannot be partial");
    std::printf("{\"attachment_pair_checks\":%u,\"owned_fixture_checks\":%u,\"source_actors\":%u,\"declared_source_frames\":%u,\"source_mesh_draws\":%u,\"actual_scene_fields_or_full_startup_admitted\":false,\"all_passed\":true}\n",checks,owned,actors,source_frames,source_draws);
}
