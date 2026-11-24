#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/character_model_scene.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/character_model_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"character model scene: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
struct UvRender final:gf::IRenderBackend {
    std::vector<gf::Vertex> vertices;
    std::vector<gf::DrawCall> draws;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}
    gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex* v,std::size_t n)override{if(n)vertices.assign(v,v+n);else vertices.clear();return true;}
    bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{draws.clear();}
    void submit(const gf::DrawCall& call)override{draws.push_back(call);}
    void end_frame()override{}
    void present()override{}
};
}
int main(){
    auto data=character_model_fixture::archive(),prefix=character_model_fixture::prefix(),profile=character_model_fixture::profile();std::string error;
    gf::HsdSkinAsset raw;check(gf::hsd_skin_load({data.data(),data.size()},&raw,&error)&&raw.meshes.size()==2&&raw.meshes[0].kind==gf::HsdSkinKind::Envelope,"low flagbit1 envelope retained alongside valid rigid child");
    check(raw.scene.meshes[0].pobj_flags==0xa001&&raw.scene.meshes[0].texture_matrix_mask==1&&raw.scene.meshes[0].vertices[0].texture_matrix_indices[0]==30,"PNMTX/TEXMTX direct byte selectors stay aligned with source vertices");
    gf::MenuScene plain;check(!plain.load_scene(raw.scene,&error),"default MenuScene still refuses raw deformation/texgen");
    auto malformed=data;character_model_fixture::word(malformed,32+0x15c,0xa0020003);gf::HsdScene partial;
    check(!gf::hsd_load({malformed.data(),malformed.size()},&partial,&error),"published bad root DOBJ refuses instead of silently keeping only its rigid child");
    malformed=data;character_model_fixture::word(malformed,32+0x124,0x120);check(!gf::hsd_load({malformed.data(),malformed.size()},&partial,&error),"DOBJ self cycle refuses");
    malformed=data;character_model_fixture::word(malformed,32+0x154,0x150);check(!gf::hsd_load({malformed.data(),malformed.size()},&partial,&error),"POBJ self cycle refuses");
    auto zero_root=character_model_fixture::archive(false,true),zero_profile=profile;
    character_model_fixture::word(zero_profile,28,0);character_model_fixture::word(zero_profile,36,0);
    auto view=[](const auto& bytes){return gf::ByteView{bytes.data(),bytes.size()};};
    gf::HsdRecordAnimationBank zero_bank;gf::CharacterModelScene zero_scene;
    check(zero_bank.load(view(zero_root),&error)&&zero_scene.load_with_record_bank(zero_bank,view(prefix),view(zero_profile),&error)&&
          zero_scene.rig().binding.root_at_block_zero&&zero_scene.rig().binding.bones[0].joint_offset==0&&
          zero_scene.rig().binding.bones[1].parent==0&&zero_scene.rig().binding.bones[0].parent==gf::kHsdNoBone,
          "required relocated zero stays a real rig root; the optional null parent is distinct");
    const auto relocations=32+view(zero_root).be32(4);
    for(unsigned i=0;i<view(zero_root).be32(8);++i)
        if(view(zero_root).be32(relocations+i*4)==0x760)character_model_fixture::word(zero_root,relocations+i*4,0x7fc);
    check(!zero_bank.load(view(zero_root),&error),"unrelocated zero cannot stand in for the required root");
    auto separate=raw.scene;
    separate.model_roots.push_back(0x80);
    separate.joints[1].model_root=1;
    separate.meshes[1].pobj_flags|=0x1000;
    // Keep both first-record envelope influences local in this check.
    separate.mesh_joint[1]=gf::u32(separate.joints.size());
    separate.joints.push_back(separate.joints[1]);
    separate.joints.back().archive_offset=0x40;
    separate.joints[1].model_root=0;
    gf::hsd_detail::Reader scoped_reader;
    std::vector<gf::HsdSkinJoint> scoped_joints;std::vector<gf::HsdSkinMesh> scoped_meshes;
    check(gf::hsd_anim_detail::open_container(view(data),&scoped_reader,&error)&&
          gf::hsd_skin_metadata(separate,scoped_reader,&scoped_joints,&scoped_meshes,&error,0)&&
          scoped_meshes[0].kind==gf::HsdSkinKind::Envelope&&scoped_meshes[1].palettes.empty(),
          "body-only metadata does not evaluate an unsupported secondary shape");
    check(!gf::hsd_skin_metadata(separate,scoped_reader,&scoped_joints,&scoped_meshes,&error,1)&&
          error.find("shape/unknown POBJ")!=std::string::npos,"selecting the secondary shape still refuses");
    gf::CharacterModelScene scene;const bool loaded=scene.load({data.data(),data.size()},{prefix.data(),prefix.size()},{profile.data(),profile.size()},&error);if(!loaded)std::fprintf(stderr,"fixture refused: %s\n",error.c_str());
    check(loaded&&!scene.ready(),"explicit evaluator/material component loads but cannot draw before accepted pose");
    gf::HsdBoneMotionSample values;values.bones.resize(2);values.bones[0].joint_offset=0x40;values.bones[1].joint_offset=0x80;
    auto retarget=values;
    retarget.bones[1].present=0x22;retarget.bones[1].value[1]=.4f;
    retarget.bones[1].value[5]=40;retarget.bones[1].value[6]=60;
    check(gf::character_pose_scale_translations(retarget,50)&&retarget.bones[1].value[5]==20&&
          retarget.bones[1].value[6]==60&&retarget.bones[1].value[1]==.4f,
          "only keyed translation components are retargeted");
    std::array<gf::CharacterPoseLane,4> lanes;lanes[0].current=&values;gf::CharacterLocalPose local;gf::CharacterPoseFrame frame;gf::CharacterWorldPose world;
    check(gf::character_pose_sample(scene.rig(),lanes,&local,&error)&&gf::character_pose_initial_frame(scene.rig(),gf::Mat4{},&frame,&error)&&gf::character_pose_direct_world(scene.rig(),local,frame,&world,&error),"owned source-direct pose");
    gf::CharacterModelRenderState state;check(scene.set_world_pose(world,state,&error)&&scene.ready(),"complete weighted+rigid geometry evaluated before draw");
    check(std::fabs(scene.world_geometry().world_vertices[0][0].position[0]-(-.25f))<1e-5f,"weighted real matrix blend changes position, no rigid stand-in");
    gf::Camera camera;camera.eye[2]=10;camera.target[2]=0;camera.far_plane=100;float uv[2],normal[3]={1,0,0};
    check(gf::character_reflection_uv(normal,camera,uv)&&uv[0]==1&&uv[1]==.5f,"source normalized view-normal reflection coordinate");
    normal[0]=0;normal[1]=1;check(gf::character_reflection_uv(normal,camera,uv)&&uv[0]==.5f&&uv[1]==0,"reflection Y sign/post bias");
    normal[1]=0;check(!gf::character_reflection_uv(normal,camera,uv),"zero normal refuses instead of generating guessed coordinates");
    gf::Camera::Basis camera_basis;check(camera.basis(&camera_basis),"reflection test camera basis");
    gf::HsdMesh patch_mesh;
    patch_mesh.vertices.resize(3);patch_mesh.indices={0,1,2};
    patch_mesh.vertices[1].position[0]=1;patch_mesh.vertices[2].position[1]=1;
    patch_mesh.vertices[1].normal[2]=patch_mesh.vertices[2].normal[2]=1;
    auto patched=patch_mesh.vertices;
    check(gf::character_walk_reflection_uv(patch_mesh,patched,camera_basis)&&patched[0].normal[2]==1&&
          patched[0].uv[0]==.5f&&patched[0].uv[1]==.5f&&patch_mesh.vertices[0].normal[2]==0,
          "walk lab derives an authored zero normal on the private draw copy only");
    patched[1].normal[2]=0;
    check(!gf::character_walk_reflection_uv(patch_mesh,patched,camera_basis),
          "provisional lab still refuses a lost nonzero authored normal");
    gf::headless::Render renderer;gf::RenderEngine render(renderer);check(render.initialize({640,480,false,true,"owned"},nullptr,&error),"owned backend render init");scene.upload_textures(render);
    render.begin_frame({0,0,0,0});render.reset_arena();check(scene.draw(render,camera)==2,"all-bone mask overrides authored root HIDDEN and draws weighted+rigid child");render.end_frame();
    state.visible_bones=2;check(scene.set_world_pose(world,state,&error),"source visibility update");render.begin_frame({1,0,0,0});render.reset_arena();check(scene.draw(render,camera)==1,"HIDDEN is node-only: hiding envelope owner does not hide child");render.end_frame();
    state.visible_bones=0;check(scene.set_world_pose(world,state,&error),"all-hidden state is valid");render.begin_frame({2,0,0,0});render.reset_arena();check(scene.draw(render,camera)==0&&scene.error().empty(),"zero visible nodes isn't misreported as a failed pose");render.end_frame();
    state.visible_bones=0xffffffffu;state.dynamic_material=true;check(!scene.set_world_pose(world,state,&error)&&!scene.ready(),"unported dynamic material refuses without replaying old frame");state.dynamic_material=false;
    state.attachments=true;check(!scene.set_world_pose(world,state,&error),"requested attachments cannot be silently dropped");state.attachments=false;
    state.dynamic_material=true;gf::ActorFadeMaterial fade{{16,32,64},true};
    check(scene.set_world_pose_fade(world,state,fade,&error)&&scene.ready(),"explicit reviewed source fade-mobj binding evaluates weighted model");
    fade.source_class=false;check(!scene.set_world_pose_fade(world,state,fade,&error)&&!scene.ready(),"unknown material binding refuses, no old faded pose replay");
    fade.source_class=true;state.attachments=true;check(!scene.set_world_pose_fade(world,state,fade,&error),"fade material does not admit unported attachments");state.attachments=false;state.dynamic_material=false;
    check(!scene.set_world_pose_fade(world,state,fade,&error),"source fade requires explicit dynamic material declaration");
    auto badworld=world;badworld.joint_offsets[1]=999;check(!scene.set_world_pose(badworld,state,&error),"pose identity mismatch refuses");
    badworld=world;badworld.matrices[1].m[0]=NAN;check(!scene.set_world_pose(badworld,state,&error),"nonfinite pose refuses before publication");
    auto moved=world;moved.matrices[1].at(0,3)+=4;
    check(scene.set_world_pose(moved,state,&error)&&std::fabs(scene.world_geometry().world_vertices[0][0].position[0]-2.75f)<1e-5f,"changed supplied bone updates actual envelope geometry");
    check(scene.set_world_pose(moved,state,&error)&&std::fabs(scene.world_geometry().world_vertices[0][0].position[0]-2.75f)<1e-5f&&scene.asset().scene.meshes[0].vertices[0].position[0]==-1,"repeat pose does not accumulate or mutate original vertices");
    check(scene.asset().scene.meshes[0].deformation_required&&scene.asset().scene.meshes[0].generated_uv_required,"source requirements remain tagged; only evaluated draw copy is static");
    auto composite=character_model_fixture::archive(false,false,true);
    gf::HsdScene composite_raw;
    check(gf::hsd_load(view(composite),&composite_raw,&error),"fixture includes another published record");
    auto shaped=composite;
    const auto mark_shape=[&](auto& bytes,unsigned record){
        for(unsigned i=0;i<composite_raw.meshes.size();++i){
            if(composite_raw.joints[composite_raw.mesh_joint[i]].model_root!=record)continue;
            const auto& mesh=composite_raw.meshes[i];
            const auto at=32+mesh.pobj_offset+12;
            character_model_fixture::word(bytes,at,(view(bytes).be32(at)&~0x30000000u)|0x10000000u);
            return true;
        }
        return false;
    };
    gf::CharacterModelScene strict_selected;
    check(mark_shape(shaped,1)&&!strict_selected.load(view(shaped),view(prefix),view(profile),&error)&&
          error.find("shape/unknown POBJ")!=std::string::npos,"archive-wide binding still rejects a secondary shape");
    gf::HsdSkinAsset scoped_raw;
    check(gf::hsd_skin_load(view(shaped),&scoped_raw,&error,0)&&!scoped_raw.loaded,
          "unremapped record metadata is not a sampleable asset");
    check(strict_selected.load_record_zero(view(shaped),view(prefix),view(profile),&error)&&
          strict_selected.asset().meshes.size()==2,"explicit record-zero body excludes that secondary record");
    check(mark_shape(shaped,0)&&!strict_selected.load_record_zero(view(shaped),view(prefix),view(profile),&error)&&
          error.find("shape/unknown POBJ")!=std::string::npos&&!strict_selected.ready(),"a selected shape is still unsupported");
    gf::HsdRecordAnimationBank composite_bank;gf::CharacterModelScene mounted;
    const std::array mounts{gf::CharacterPreviewMount{1,1,{2,0,0}}};
    check(composite_bank.load(view(composite),&error,true)&&
          mounted.load_preview_with_record_bank(composite_bank,view(prefix),view(profile),&error,
                                               gf::CharacterModelScene::Inspection::Strict,mounts),
          "bounded selected static subtree loads");
    gf::CharacterModelScene assembled_body;
    check(assembled_body.load_assembled_record_zero(composite_bank,view(prefix),view(profile),mounts,&error)&&
          assembled_body.set_world_pose(world,state,&error)&&assembled_body.asset().scene.meshes.size()==3,
          "explicit battle body assembly binds the same exact static subtree");
    check(mounted.rig().binding.bones.size()==2&&mounted.asset().scene.joints.size()==4&&
          mounted.asset().meshes.size()==3&&mounted.set_world_pose(world,state,&error),
          "mounted joints do not become logical pose slots");
    const auto source_x=mounted.asset().scene.meshes.back().vertices[0].position[0];
    const auto expect=source_x+world.matrices[1].at(0,3)+5;
    check(std::abs(mounted.world_geometry().world_vertices.back()[0].position[0]-expect)<1e-5f&&
          mounted.set_world_pose(moved,state,&error)&&
          std::abs(mounted.world_geometry().world_vertices.back()[0].position[0]-expect-4)<1e-5f,
          "mount replaces root SRT, retains child SRT, and follows the next pose");
    state.visible_bones=1;check(mounted.set_world_pose(world,state,&error),"visibility follows mount bone");
    mounted.upload_textures(render,100);render.begin_frame({3,0,0,0});render.reset_arena();
    check(mounted.draw(render,camera)==1,"hidden mount bone hides all of its mounted geometry");render.end_frame();
    state.visible_bones=0xffffffffu;
    const std::array absent_mount{gf::CharacterPreviewMount{2,1,{}}};
    check(!mounted.load_preview_with_record_bank(composite_bank,view(prefix),view(profile),&error,
                                                gf::CharacterModelScene::Inspection::Strict,absent_mount)&&
          !mounted.ready(),"missing exact record refuses without reusing an old assembly");
    gf::CharacterModelScene painted;const auto paint=character_model_fixture::archive(true);
    auto zero_painted=paint;
    character_model_fixture::scalar(zero_painted,32+0x223+14,0);
    gf::CharacterModelScene zero_reflecting;
    check(zero_reflecting.load_record_zero(view(zero_painted),view(prefix),view(profile),&error)&&
          zero_reflecting.set_world_pose(world,state,&error),"zero reflection fixture has valid geometry");
    zero_reflecting.upload_textures(render);
    render.begin_frame({});render.reset_arena();
    check(zero_reflecting.draw(render,camera)==0&&zero_reflecting.error().find("invalid reflection")!=std::string::npos,
          "strict default does not hide or repair unsupported reflection normals");
    render.end_frame();
    for(const auto& wrong:{gf::BorgZeroReflection{0x154,1},gf::BorgZeroReflection{0x150,2},gf::BorgZeroReflection{0x420,1}}){
        check(zero_reflecting.load_record_zero(view(zero_painted),view(prefix),view(profile),&error)&&
              !zero_reflecting.bind_zero_reflections(std::span(&wrong,1),&error),"wrong PObj, zero count or material refuses");
    }
    const std::array exact_zeros{gf::BorgZeroReflection{0x150,1}};
    check(zero_reflecting.load_record_zero(view(zero_painted),view(prefix),view(profile),&error)&&
          zero_reflecting.bind_zero_reflections(exact_zeros,&error)&&zero_reflecting.set_world_pose(world,state,&error),
          "native reflection policy binds one exact mesh");
    zero_reflecting.upload_textures(render);
    render.begin_frame({});render.reset_arena();
    check(zero_reflecting.draw(render,camera)==2&&zero_reflecting.world_geometry().world_vertices[0][0].normal[0]==0&&
          zero_reflecting.asset().scene.meshes[0].vertices[0].normal[0]==0,"body renders without rewriting source or posed normals");
    render.end_frame();
    auto two_reflecting=data;
    character_model_fixture::scalar(two_reflecting,32+0x223+14,0);
    const std::array both_zeros{gf::BorgZeroReflection{0x150,1},gf::BorgZeroReflection{0x420,1}};
    check(zero_reflecting.load_record_zero(view(two_reflecting),view(prefix),view(profile),&error)&&
          zero_reflecting.bind_zero_reflections(both_zeros,&error)&&zero_reflecting.set_world_pose(world,state,&error),
          "two exact PObjs sharing display-list data bind independently");
    zero_reflecting.upload_textures(render);
    render.begin_frame({});render.reset_arena();
    check(zero_reflecting.draw(render,camera)==2,"both admitted meshes draw");
    render.end_frame();
    check(painted.load({paint.data(),paint.size()},{prefix.data(),prefix.size()},{profile.data(),profile.size()},&error),
          "fixture with independent reflected mesh and explicit TEX1 mesh");
    check(!painted.supports_texture_scroll(0,0)&&painted.supports_texture_scroll(1,0)&&!painted.supports_texture_scroll(1,1),
          "only explicit repeating coordinate is eligible (not reflection or clamp)");
    const std::array scroll{gf::CharacterTextureScroll{1,0,-.25f}};
    check(painted.set_world_pose(world,state,&error,scroll),"negative texture phase accepted");
    const auto& drawn=painted.world_geometry().world_vertices;
    const auto& authored=painted.asset().scene.meshes;
    check(drawn[1][0].uv1[0]==-.25f&&drawn[1][0].uv1[1]==.5f&&drawn[1][0].uv[0]==0&&authored[1].vertices[0].uv1[0]==0&&
          drawn[0][0].uv[0]==authored[0].vertices[0].uv[0]&&drawn[1][0].position[0]==0,
          "only selected coordinate shifts, without changing either mesh's pose or the source UV");
    check(painted.set_world_pose(world,state,&error,scroll)&&drawn[1][0].uv1[0]==-.25f,"scroll is not accumulated");
    check(painted.set_world_pose(world,state,&error)&&drawn[1][0].uv1[0]==0,"empty presentation restores original UV");
    const std::array clamped{gf::CharacterTextureScroll{1,1,.5f}};
    check(!painted.set_world_pose(world,state,&error,clamped)&&!painted.ready(),"clamped scroll refuses without replaying old pose");

    auto highlight_source=paint;
    character_model_fixture::word(highlight_source,32+0x540,0x30082);
    gf::HsdRecordAnimationBank highlight_bank;
    gf::CharacterModelScene highlight_scene;
    check(highlight_bank.load(view(highlight_source),&error,true,gf::HsdStaticMaterialPolicy::BodyInspection)&&
          !highlight_scene.load_with_record_bank(highlight_bank,view(prefix),view(profile),&error)&&
          highlight_scene.load_preview_with_record_bank(highlight_bank,view(prefix),view(profile),&error)&&
          highlight_scene.set_world_pose(world,state,&error),"mode 2 is admitted only for qualified body inspection");
    check(!strict_selected.load_record_zero(view(highlight_source),view(prefix),view(profile),&error),
          "record-zero battle scope does not enable inspection highlights");
    check(!assembled_body.load_assembled_record_zero(highlight_bank,view(prefix),view(profile),mounts,&error)&&
          error.find("unsupported generated coordinates")!=std::string::npos,
          "assembled battle body never borrows an inspection material policy");
    UvRender uv_device;gf::RenderEngine uv_render(uv_device);
    check(uv_render.initialize({640,480,false,true,"highlight fixture"},nullptr,&error),"highlight render init");
    check(highlight_scene.upload_textures(uv_render)>0,"highlight and explicit-UV textures upload");
    auto draw_highlight=[&](unsigned tick){
        uv_render.begin_frame({tick,0,0,0});uv_render.reset_arena();
        const auto count=highlight_scene.draw(uv_render,camera);uv_render.end_frame();return count;
    };
    auto highlight_uv_is=[&](float u,float v){
        for(const auto& d:uv_device.draws)if(d.texture_after_lighting&&d.texture_colour_mode==3){
            if(!d.texture||d.vertex_count!=3||d.vertex_first+d.vertex_count>uv_device.vertices.size())return false;
            for(unsigned i=d.vertex_first;i<d.vertex_first+d.vertex_count;++i)
                if(std::abs(uv_device.vertices[i].u-u)>1e-6f||std::abs(uv_device.vertices[i].v-v)>1e-6f)return false;
            return true;
        }
        return false;
    };
    check(draw_highlight(0)==2&&highlight_uv_is(.5f,.5f),"unbound highlight submits fixed GX Q=0 UV");
    gf::SceneLighting light;light.declared=true;
    gf::SceneLight sun;sun.flags=5;sun.position[0]=1;light.lights.push_back(sun);
    auto other_view=camera;other_view.eye[0]=2;
    check(uv_render.set_scene_lighting(light),"inspection fill light");
    const auto original_view=camera;camera=other_view;
    check(draw_highlight(1)==2&&highlight_uv_is(.5f,.5f)&&
          highlight_scene.asset().scene.meshes[0].vertices[0].uv[0]==0,
          "moving the view and adding host fill do not bind an HSD texgen light or mutate the asset");
    camera=original_view;
    uv_render.shutdown();
    auto unsupported=highlight_source;
    character_model_fixture::scalar(unsupported,32+0x51c,2);
    check(highlight_bank.load(view(unsupported),&error,true,gf::HsdStaticMaterialPolicy::BodyInspection)&&
          !highlight_scene.load_preview_with_record_bank(highlight_bank,view(prefix),view(profile),&error),
          "nonidentity highlight SRT still refuses");

    data.clear();prefix.clear();profile.clear();check(scene.set_world_pose(world,state,&error),"source asset lifetime ended: component owns metadata/geometry/textures");render.shutdown();
    unsigned real=0,weighted=0,source_frames=0;
    for(const char* name:{"g-red","sasuke"}){const std::string base=std::string("local/content/startup-characters/")+name+"/";auto bytes=read(base+"model.hsd"),p=read(base+"model-prefix.bin"),layout=read(std::string("local/content/character-pose-bindings/")+name+".gfcp");if(bytes.empty()||p.empty()||layout.empty())continue;
        const bool ok=scene.load({bytes.data(),bytes.size()},{p.data(),p.size()},{layout.data(),layout.size()},&error);if(!ok)std::fprintf(stderr,"%s refused: %s\n",name,error.c_str());check(ok,"source-selected complete record0 model component loads");
        unsigned envelopes=0;for(const auto& mesh:scene.asset().meshes)envelopes+=mesh.kind==gf::HsdSkinKind::Envelope;
        check(std::string(name)=="sasuke"?envelopes==15&&scene.asset().meshes.size()==28:envelopes==0&&scene.asset().meshes.size()==41,"exact complete selected geometry counts, not earlier rigid-only partial counts");
        check(render.initialize({640,480,false,true,"original model regression"},nullptr,&error),"headless component regression init");scene.upload_textures(render);
        gf::Camera view;view.eye[1]=65;view.eye[2]=700;view.target[1]=65;view.target[2]=0;view.near_plane=1;view.far_plane=3000;view.fov_y_degrees=70;
        for(unsigned choice:{0,1,3,4,6,7}){
            auto clip_bytes=read(std::string("local/content/startup-motion-clips/")+name+"/startup-choice-0"+std::to_string(choice)+".hsd");gf::HsdBoneMotionClip clip;
            check(gf::hsd_bone_motion_load({clip_bytes.data(),clip_bytes.size()},scene.rig().binding,&clip,&error),"source clip for complete evaluated model");
            for(unsigned time=0;time<=unsigned(clip.end_frame);++time){
                gf::HsdBoneMotionSample current;std::array<gf::CharacterPoseLane,4> requested;
                check(gf::hsd_bone_motion_sample(clip,time,&current,&error),"original complete-model channel frame");requested[0].current=requested[1].current=&current;
                check(gf::character_pose_sample(scene.rig(),requested,&local,&error)&&gf::character_pose_initial_frame(scene.rig(),gf::Mat4{},&frame,&error)&&gf::character_pose_direct_world(scene.rig(),local,frame,&world,&error)&&scene.set_world_pose(world,{},&error),"all source-direct clip frames evaluate complete rigid/envelope geometry");
                render.begin_frame({source_frames,0,0,0});render.reset_arena();const unsigned count=scene.draw(render,view);render.end_frame();
                check(count==scene.asset().meshes.size()&&scene.error().empty(),"complete record0 draws without silently dropping weights/reflection or source meshes");++source_frames;
            }
        }
        render.shutdown();weighted+=envelopes;++real;
    }
    std::printf("{\"character_model_checks\":%u,\"real_models\":%u,\"real_envelope_meshes\":%u,\"complete_model_frames\":%u,\"actor_controller_or_startup_admission\":false,\"all_passed\":true}\n",checks,real,weighted,source_frames);
}
