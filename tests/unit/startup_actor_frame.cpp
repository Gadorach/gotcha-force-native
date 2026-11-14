#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_actor_frame_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup actor frame: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
class Recorder final:public gf::IRenderBackend {
public:
    std::vector<gf::DrawCall> calls;bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
std::vector<gf::s64> logical(const gf::StartupActorFrameState& s){auto v=startup_actor_bookkeeping_fixture::fields(s.scalars);const auto& a=s.vm.state().actor;for(auto x:{a.default_group,a.auxiliary,a.control0[0],a.control0[1],a.control0[2],a.control0[3]})v.push_back(x);v.push_back(a.behaviour);for(const auto& l:s.vm.state().lanes){v.push_back(l.program);v.push_back(l.cursor);v.push_back(l.wait);v.push_back(gf::f32_to_bits(l.motion.frame));v.push_back(gf::f32_to_bits(l.motion.elapsed));v.push_back(l.done);}const auto& d=s.dispatch.state();v.push_back(d.phase);v.push_back(d.substate);v.push_back(d.render_group);v.push_back(gf::f32_to_bits(s.dispatch.root_state().remaining));const auto fade=actor_fade_fixture::fields(s.material.state());v.insert(v.end(),fade.begin(),fade.end());v.push_back(s.frames);return v;}
}
int main(){
    using namespace gf;std::string error;startup_actor_frame_fixture::Fixture fixture;check(fixture.load(&error),"owned complete component inputs");StartupActorFrame actor;StartupActorFrameOutput out;
    check(actor.load(fixture.assets(),StartupActorFrameMode::IsolatedRequirements,0,&out,&error)&&out.requirements.size()==4&&actor.ready(),"isolated mode loads native model/driver/VM/class/scalars/root/material, recordsfourUNBOUNDclassrequests");
    check(actor.state()->scalars.motion_delta==1&&actor.state()->dispatch.root_state().actor_scale[0]>2&&actor.state()->dispatch.root_state().root_scale[0]==1&&!actor.state()->material.state().channels[1].active&&!actor.state()->vm.state().lanes[0].done,"constructor numeric root state isNOTsource reset/curve14/defaultunitglobal scale");
    Recorder backend;RenderEngine render(backend);render.initialize({640,480,false,true,"owned staged actor"},nullptr,&error);actor.upload_textures(render);Camera camera;camera.eye[2]=100;camera.target[2]=0;camera.far_plane=1000;camera.fov_y_degrees=80;
    StartupActorFrameInput input;input.isolated_external_mutations=input.placement_declared=true;input.position={3,4,5};
    check(actor.draw(render,camera)==0&&actor.step(input,&out,&error)&&out.stages.size()==10&&!actor.state()->vm.state().lanes[0].done&&actor.state()->frames==1,"first sourceinnerStart step no VMadvance, butpre/post/world/material stages run");
    for(unsigned i=0;i<out.stages.size();++i)check(unsigned(out.stages[i])==i,"source stage orderdimensions/pre/classpre/substate/classpost/post/events/world/material/publish");
    check(actor.state()->scalars.phase19==u8(actor.state()->dispatch.state().phase)&&startup_actor_class_fixture::fields(actor.state()->scalars.klass)==startup_actor_class_fixture::fields(actor.state()->classes.state()),"acceptedframe exposesonecoherentphase/class fieldview");
    check(actor.draw(render,camera)==0&&actor.state()->scalars.reloads[2].remaining==179,"constructor rendergroup0 hides WHOLEactor, no per-mesh hiding; sourcecounteradvanced");
    check(actor.step(input,&out,&error)&&actor.state()->vm.state().lanes[0].done&&actor.state()->vm.state().lanes[1].done&&!actor.state()->vm.state().lanes[0].motion.selected,"second sourceMotion step usesglobalnull gate without inventing clip");
    const auto initial_c0=actor.state()->dimensions.fieldc0;check(actor.reset(&out,&error)&&out.reset_fields&&actor.state()->dispatch.root_state().actor_scale[0]==1&&actor.state()->dimensions.fieldc0==initial_c0&&actor.state()->material.state().channels[1].time==0,"explicit reset changesonlysource fields andrequests14, notctorC0/C4 orimmediatesample");
    check(actor.request(0,&out,&error)&&!out.motion.empty()&&actor.draw(render,camera)==0,"requestprefix usesexplicitgroup5, invalidates oldpublication");
    check(actor.step(input,&out,&error)&&actor.state()->dimensions.field668==80&&actor.state()->material.state().channels[1].rgba==0xff8040ffu,"runtime dimensionrefresh usesresetB4; fadersamples14afterworld usingownedactorclock");
    render.begin_frame({});render.reset_arena();check(actor.draw(render,camera)==2,"owned weighted/rigid body draws throughstagedmaterial owner");render.end_frame();check(backend.calls.size()==2&&backend.calls[0].post_rgb_add==std::array<u8,3>{255,128,64},"final publishedmaterial RGB reaches drawpacket");
    const auto before=logical(*actor.state());const auto frames=actor.state()->frames;auto bad=input;bad.position[0]=1e8f;
    check(!actor.step(bad,&out,&error)&&!actor.ready()&&out.requirements.empty()&&out.motion.empty()&&out.stages.empty()&&logical(*actor.state())==before&&actor.state()->frames==frames,"lateWORLDfailure rollsback scalar/class/VM/root/fade/requirements andcounter, notpartial publication");
    check(actor.draw(render,camera)==0&&error.find("actor position")!=std::string::npos,"failedframe cannotreplay oldgeometry andretains actionable nestedrefusal");
    check(actor.request(0,&out,&error)&&actor.state()->scalars.phase19==0&&actor.draw(render,camera)==0&&actor.step(input,&out,&error),"onlyexplicitnewrequest recoversnumericready/coherentphase; nostalepose beforeacceptedframe");
    bool anchor=false;
    for(unsigned i=0;i<8&&!anchor;++i){const auto& old=actor.state()->world;std::array<float,3> previous{};for(unsigned k=0;k<3;++k)previous[k]=old.matrices[0].at(k,3);input.position[0]+=10;check(actor.step(input,&out,&error),"owned requestcontinuation");
        for(const auto& req:out.requirements)if(req.origin==StartupActorRequirementOrigin::Substate&&req.code==u8(StartupActorEffectKind::AnchorEffect2Required)){check(req.anchor_known&&req.anchor==previous&&req.id==2,"anchorrequest usesPREVIOUSsource world, notcurrentpose/fakeorigin");anchor=true;}}
    check(anchor,"owned program exercised anchored requirement");
    check(!actor.step({},&out,&error)&&actor.draw(render,camera)==0,"missingexplicitplacement/isolation contract refuses, doesnotguess origin");
    auto exit_fixture=fixture;exit_fixture.program[92+10]=1;StartupActorFrame exiting;check(exiting.load(exit_fixture.assets(),StartupActorFrameMode::IsolatedRequirements,0,&out,&error)&&exiting.reset(&out,&error)&&exiting.request(0,&out,&error),"owned earlyexit program");
    input.position={0,0,0};check(exiting.step(input,&out,&error)&&exiting.step(input,&out,&error)&&exiting.state()->dispatch.state().phase==StartupActorDispatchState::ScaleOut,"control1entersrootphase aftersourceStartdelay");
    for(unsigned i=0;i<5;++i)check(exiting.step(input,&out,&error),"source exitfirsthalf");
    check(exiting.state()->dispatch.root_state().remaining==5,"exitcounterfive");
    check(exiting.reset(&out,&error)&&exiting.state()->dispatch.root_state().remaining==5&&exiting.state()->material.state().channels[1].curve==14,"source reset retainsinterruptedactor1C, oldobservationreset seamnotreinterpreted");
    check(exiting.request(0,&out,&error)&&exiting.step(input,&out,&error)&&exiting.step(input,&out,&error),"re-enterowned exit");unsigned sounds=0;
    for(unsigned i=0;i<10;++i){check(exiting.step(input,&out,&error)&&exiting.state()->dispatch.root_state().remaining==9-float(i),"rootcounter fixedoneupdate inclfreshentryinit10");for(const auto& e:out.requirements)sounds+=e.origin==StartupActorRequirementOrigin::Substate&&e.code==u8(StartupActorEffectKind::Sound242Required)&&e.id==242;check(exiting.state()->dispatch.state().render_group==(i==9?0:65),"sourcegroup clears ontenthstep");}
    check(sounds==1&&exiting.state()->material.state().channels[1].curve==12&&exiting.state()->material.state().channels[1].time==2&&exiting.state()->material.state().channels[1].loops==2&&exiting.draw(render,camera)==0,"owned4-periodcurve12 loopswhile10rootsteps elapse; sound242 UNBOUND once/actorhidden");
    auto moved=std::move(exiting);check(!exiting.ready()&&moved.ready()&&moved.step(input,&out,&error),"movingcontroller preservesheapmodel/materialbinding andinvalidatesmovedfromuse");
    auto height=fixture;store_be16(height.program.data()+92+22,0x40);StartupActorFrame guarded;check(guarded.load(height.assets(),StartupActorFrameMode::IsolatedRequirements,0,&out,&error)&&guarded.request(0,&out,&error),"owned profile triggerspose-dependentheight predicate");
    const auto untouched=logical(*guarded.state());check(!guarded.step(input,&out,&error)&&logical(*guarded.state())==untouched&&error.find("8004D08C")!=std::string::npos,"unportedpose-dependent55DCC branch refuses beforeframecommit");
    auto invalid=fixture.assets();invalid.model={nullptr,1};check(!guarded.load(invalid,StartupActorFrameMode::IsolatedRequirements,0,&out,&error)&&!guarded.state(),"invalid modelview rejects beforehash/read andclears oldbinding");
    check(!guarded.load(fixture.assets(),static_cast<StartupActorFrameMode>(99),0,&out,&error)&&!guarded.load(fixture.assets(),StartupActorFrameMode::IsolatedRequirements,1,&out,&error),"unsupportedmode andwrongsource startupslot refuse");
    unsigned profiles=0,requests=0,source_frames=0,holds=0,exits=0;
    auto class_bytes=read("local/content/startup-actor-class/startup.gfac"),root_bytes=read("local/content/startup-root/root.gfrs"),fade_bytes=read("local/content/actor-fade/startup.gfaf");
    for(const char* name:{"g-red","sasuke"}){auto model=read(std::string("local/content/startup-characters/")+name+"/model.hsd");if(model.empty())continue;auto prefix=read(std::string("local/content/startup-characters/")+name+"/model-prefix.bin"),pose=read(std::string("local/content/character-pose-bindings/")+name+".gfcp"),program=read(std::string("local/content/character-program-bindings/")+name+".gfmp"),setup=read(std::string("local/content/startup-actor-setup/")+name+".gfcs"),runtime=read(std::string("local/content/startup-actor-runtime/")+name+".gfbr");
        HsdModelBones bones;check(hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bones,&error),"actual bonebinding");std::vector<CharacterMotionClipBinding> clips;for(unsigned index:{0,1,3,4,7,8,9}){auto raw=read(std::string("local/content/startup-motion-programs/")+name+"/motions/motion-0"+std::to_string(index)+".hsd");CharacterMotionClipBinding c;c.index=u8(index);check(hsd_bone_motion_load({raw.data(),raw.size()},bones,&c.clip,&error),"actualclip data");clips.push_back(std::move(c));}
        StartupActorFrameAssets assets={{model.data(),model.size()},{prefix.data(),prefix.size()},{pose.data(),pose.size()},{program.data(),program.size()},{setup.data(),setup.size()},{runtime.data(),runtime.size()},{class_bytes.data(),class_bytes.size()},{root_bytes.data(),root_bytes.size()},{fade_bytes.data(),fade_bytes.size()},clips};
        for(unsigned choice:{0,1,3,4,6,7}){StartupActorFrame source;const bool red=std::string(name)=="g-red";check(source.load(assets,StartupActorFrameMode::IsolatedRequirements,red?0:1,&out,&error),"actualsourcecomponents assembled inisolatedmode");
            input.position={7,13,-21};check(source.step(input,&out,&error)&&source.step(input,&out,&error)&&source.reset(&out,&error)&&source.request(u8(choice),&out,&error),"actualfresh/null/reset/request sequence, declaredplacement");++requests;
            for(unsigned tick=0;tick<384;++tick){check(source.step(input,&out,&error)&&out.stages.size()==10&&source.state()->scalars.phase19==u8(source.state()->dispatch.state().phase)&&startup_actor_class_fixture::fields(source.state()->scalars.klass)==startup_actor_class_fixture::fields(source.state()->classes.state())&&source.state()->scalars.klass.field582==source.state()->vm.state().actor.default_group&&source.state()->scalars.field709==source.state()->vm.state().actor.auxiliary,"actualatomic componentframe withcoherentaliases");++source_frames;
                if(source.state()->dispatch.state().substate==2){check(source.state()->dispatch.state().render_group==0&&source.state()->dispatch.root_state().root_scale[0]>0,"source numericexit/positivefinalscale andgroup hide");++exits;break;}
                if(source.state()->vm.state().lanes[0].done&&source.state()->vm.state().lanes[1].done)break;
                if(red&&choice==7&&tick==127){check(source.state()->vm.state().lanes[0].motion.frame==10&&source.state()->vm.state().lanes[0].threshold==24,"GRedholdnotforced byassembledpipeline");++holds;break;}
                check(tick<383,"boundedactualsubset");}
        }
        ++profiles;
    }
    check(profiles==0||profiles==2,"partialoptionalinputs cannotclaimtwoactorcoverage");
    std::printf("{\"startup_actor_frame_checks\":%u,\"source_profiles\":%u,\"source_requests\":%u,\"source_staged_frames\":%u,\"preserved_G_Red_hold\":%u,\"source_numeric_exits\":%u,\"isolated_requirements_and_declared_placement_only\":true,\"visible_startup_admitted\":false,\"all_passed\":true}\n",checks,profiles,requests,source_frames,holds,exits);
}
