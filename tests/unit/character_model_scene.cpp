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
    gf::CharacterModelScene scene;const bool loaded=scene.load({data.data(),data.size()},{prefix.data(),prefix.size()},{profile.data(),profile.size()},&error);if(!loaded)std::fprintf(stderr,"fixture refused: %s\n",error.c_str());
    check(loaded&&!scene.ready(),"explicit evaluator/material component loads but cannot draw before accepted pose");
    gf::HsdBoneMotionSample values;values.bones.resize(2);values.bones[0].joint_offset=0x40;values.bones[1].joint_offset=0x80;
    std::array<gf::CharacterPoseLane,4> lanes;lanes[0].current=&values;gf::CharacterLocalPose local;gf::CharacterPoseFrame frame;gf::CharacterWorldPose world;
    check(gf::character_pose_sample(scene.rig(),lanes,&local,&error)&&gf::character_pose_initial_frame(scene.rig(),gf::Mat4{},&frame,&error)&&gf::character_pose_direct_world(scene.rig(),local,frame,&world,&error),"owned source-direct pose");
    gf::CharacterModelRenderState state;check(scene.set_world_pose(world,state,&error)&&scene.ready(),"complete weighted+rigid geometry evaluated before draw");
    check(std::fabs(scene.world_geometry().world_vertices[0][0].position[0]-(-.25f))<1e-5f,"weighted real matrix blend changes position, no rigid stand-in");
    gf::Camera camera;camera.eye[2]=10;camera.target[2]=0;camera.far_plane=100;float uv[2],normal[3]={1,0,0};
    check(gf::character_reflection_uv(normal,camera,uv)&&uv[0]==1&&uv[1]==.5f,"source normalized view-normal reflection coordinate");
    normal[0]=0;normal[1]=1;check(gf::character_reflection_uv(normal,camera,uv)&&uv[0]==.5f&&uv[1]==0,"reflection Y sign/post bias");
    normal[1]=0;check(!gf::character_reflection_uv(normal,camera,uv),"zero normal refuses instead of generating guessed coordinates");
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
