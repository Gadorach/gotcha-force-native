// Owned bounded leaf-material fixtures; optional private OPTION archive sweep.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/hsd_leaf_material.hpp"
#include "engine/gameplay/menu_scene.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* what){++checks;if(!ok){std::fprintf(stderr,"leaf material: %s\n",what);std::abort();}}
bool near(float a,float b){return std::fabs(a-b)<1e-6f;}
struct Fixture {
    std::vector<gf::u8> block=std::vector<gf::u8>(0x400);std::vector<unsigned> fields;
    void word(unsigned at,gf::u32 v){gf::store_be32(block.data()+at,v);}
    void real(unsigned at,float v){word(at,gf::f32_to_bits(v));}
    void ptr(unsigned at,unsigned v){word(at,v);fields.push_back(at);}
    void stream(unsigned at,float a,float b){
        // Two LIN segments, first spans one time unit then holds the second key.
        for(unsigned i=0;i<3;++i){block[at+i*6]=2;const auto bits=gf::f32_to_bits(i?b:a);
            for(unsigned k=0;k<4;++k)block[at+i*6+1+k]=gf::u8(bits>>(k*8));
            block[at+i*6+5]=i?8:1;}
    }
    // Published scene->model table->record->one leaf and one material clip.
    // MOBJ AObj channel4 animates red; TEXANIM channel3 animates T translation.
    Fixture(){
        ptr(0,0x20);ptr(0x20,0x30);ptr(0x30,0x40);ptr(0x38,0x260);
        ptr(0x50,0x90);ptr(0x98,0xb0);ptr(0xb8,0xe0);
        real(0xfc,2);real(0x100,4);real(0x104,1);real(0x108,0.25f);real(0x10c,-0.5f);
        word(0xe8,3);block[0x11c]=3;block[0x11d]=2;real(0x124,1);
        ptr(0x260,0x270);ptr(0x278,0x280);ptr(0x284,0x2a0);ptr(0x288,0x2b0);
        real(0x2a4,8);ptr(0x2a8,0x300);
        word(0x304,18);block[0x30c]=4;ptr(0x310,0x340);stream(0x340,0.5f,0.9f);
        word(0x2b4,3);ptr(0x2b8,0x2d0);real(0x2d4,8);ptr(0x2d8,0x320);
        word(0x324,18);block[0x32c]=3;ptr(0x330,0x360);stream(0x360,-0.5f,-1);
    }
    gf::HsdScene scene()const{
        gf::HsdScene s;gf::HsdJoint j;j.archive_offset=0x40;s.joints.push_back(j);
        gf::HsdMesh m;m.dobj_offset=0x90;m.material_offset=0xb0;s.meshes.push_back(m);s.mesh_joint.push_back(0);
        s.material_animation_roots.push_back({0x270,0x40});return s;
    }
    std::vector<gf::u8> archive(int omit=-1)const{
        auto relocated=fields;relocated.erase(std::remove(relocated.begin(),relocated.end(),unsigned(omit)),relocated.end());
        std::vector<gf::u8> b(32+block.size()+relocated.size()*4+8+11);
        gf::store_be32(b.data(),gf::u32(b.size()));gf::store_be32(b.data()+4,gf::u32(block.size()));
        gf::store_be32(b.data()+8,gf::u32(relocated.size()));gf::store_be32(b.data()+12,1);
        std::copy(block.begin(),block.end(),b.begin()+32);unsigned at=unsigned(32+block.size());
        for(unsigned field:relocated){gf::store_be32(b.data()+at,field);at+=4;}
        at+=8;const char name[]="scene_data";std::copy(name,name+11,b.begin()+at);return b;
    }
};
gf::HsdAnimTrackView view(const std::vector<gf::u8>& b){return {{b.data(),b.size()},0,0,0};}
void track_cases(){
    Fixture fixture;
    const std::vector<gf::u8> data(fixture.block.begin()+0x340,fixture.block.begin()+0x352);
    float old=99,requested=99;std::string error;
    check(gf::hsd_anim_sample_track(view(data),0,&old,&error)&&old==0,"legacy zero remains unstepped");
    check(gf::hsd_anim_sample_requested_track(view(data),0,&requested,&error)&&requested==0.5f,"requested zero decodes initial key");
    for(unsigned t=1;t<=8;++t)check(gf::hsd_anim_sample_track(view(data),t,&old,&error)&&
        gf::hsd_anim_sample_requested_track(view(data),t,&requested,&error)&&old==requested,"positive times agree with old playback");
    auto offset=view(data);offset.startframe=0.5f;
    check(gf::hsd_anim_sample_requested_track(offset,0,&requested,&error)&&near(requested,0.7f),"fractional FObj start offset");
    offset.startframe=-1;
    check(gf::hsd_anim_sample_requested_track(offset,0,&requested,&error)&&requested==0,"before-start state has no key yet");
    offset.startframe=NAN;requested=77;
    check(!gf::hsd_anim_sample_requested_track(offset,0,&requested,&error)&&requested==77,"nonfinite start refuses atomically");
    check(!gf::hsd_anim_sample_requested_track(view(data),100001,&requested,&error)&&requested==77,"requested time budget");
    for(const auto& malformed:std::vector<std::vector<gf::u8>>{{7},{2,0,0},{2,0,0,0,0,0x80}}){
        check(!gf::hsd_anim_sample_requested_track(view(malformed),0,&requested,&error)&&requested==77,"malformed stream has no output");
    }
    auto nonfinite=data;const auto bits=gf::f32_to_bits(NAN);
    for(unsigned k=0;k<4;++k)nonfinite[1+k]=gf::u8(bits>>(k*8));
    check(!gf::hsd_anim_sample_requested_track(view(nonfinite),0,&requested,&error)&&requested==77,"nonfinite key refuses");
    // CON and quantised pack: two u8/128 samples (.5 then 1).
    std::vector<gf::u8> quant={0x11,64,1,128,4};auto q=view(quant);q.frac_value=0x87;
    check(gf::hsd_anim_sample_requested_track(q,0,&requested,&error)&&requested==0.5f,"quantised CON initial key");
    // At EOF the carry is the last consumed wait1, NOT the newly loaded
    // wait4. The old decoder's fterm reuse jumped early at time1.
    check(gf::hsd_anim_sample_requested_track(q,1,&requested,&error)&&requested==0.5f,"quantised CON EOF retains prior value until source threshold");
    check(gf::hsd_anim_sample_requested_track(q,4,&requested,&error)&&requested==1,"quantised CON EOF reaches next key at source threshold4");
}
void leaf_cases(){
    Fixture f;auto b=f.archive();auto scene=f.scene();gf::HsdLeafMaterialClip clip;std::string error;
    auto load=[&](const std::vector<gf::u8>& bytes,const gf::HsdScene& s){return gf::hsd_leaf_material_clip_load({bytes.data(),bytes.size()},s,0x40,&clip,&error);};
    check(load(b,scene),"owned published leaf loads");
    gf::HsdAnimPose::Material sample;
    check(gf::hsd_leaf_material_sample(clip,0,&sample,&error)&&sample.diffuse[0]==127/255.0f,"initial material state uses callback byte truncation");
    check(sample.uv_scale[0]==1.5f&&sample.uv_scale[1]==0.5f&&sample.uv_offset[0]==-0.375f&&sample.uv_offset[1]==0.25f,
        "UV = inverse-scale * repeat * negative-translation (not ordinary SRT)");
    check(gf::hsd_leaf_material_sample(clip,1,&sample,&error)&&sample.diffuse[0]==229/255.0f&&sample.uv_offset[1]==0.5f,"material and texture seek together");
    sample.diffuse[0]=0.123f;
    check(!gf::hsd_leaf_material_sample(clip,9,&sample,&error)&&sample.diffuse[0]==0.123f,"out-of-domain no partial sample");
    auto reject=[&](const Fixture& bad,int omit=-1){check(!load(bad.archive(omit),scene)&&!clip.baseline.present,"malformed leaf refuses without partial publication");};
    reject(f,0x330);reject(f,0x278);reject(f,0xb8);reject(f,0x38);
    {auto bad=f;bad.ptr(0x320,0x320);reject(bad);}
    {auto bad=f;bad.word(0x2d0,1);reject(bad);}
    {auto bad=f;bad.real(0x2d4,INFINITY);reject(bad);}
    {auto bad=f;bad.real(0x2d4,4);reject(bad);}
    {auto bad=f;bad.ptr(0xb0,0x40);reject(bad);}
    {auto bad=f;bad.ptr(0xe0,0x40);reject(bad);}
    {auto bad=f;bad.word(0x324,65537);reject(bad);}
    {auto bad=f;bad.ptr(0x2bc,0x340);reject(bad);}
    {auto bad=f;bad.ptr(0x2c0,0x340);reject(bad);}
    {auto bad=f;bad.word(0x2c4,0x10000);reject(bad);}
    {auto bad=f;bad.word(0x2b4,4);reject(bad);}
    {auto bad=f;bad.ptr(0x2b0,0x2b0);reject(bad);}
    {auto bad=f;bad.ptr(0x28c,0x2b0);reject(bad);}
    {auto bad=f;bad.ptr(0x48,0x40);reject(bad);}
    {auto bad=f;bad.ptr(0x94,0x90);reject(bad);}
    {auto bad=f;bad.ptr(0xe4,0xe0);reject(bad);}
    {auto bad=f;bad.real(0xf0,0.5f);reject(bad);}
    {auto bad=f;bad.word(0x118,2);reject(bad);}
    {auto bad=f;bad.block[0x11c]=0;reject(bad);}
    {auto bad=f;bad.block[0x32c]=1;reject(bad);}
    {auto bad=f;bad.block[0x30c]=11;reject(bad);}
    {auto bad=f;bad.block[0x32f]=1;reject(bad);}
    {auto bad=f;bad.ptr(0x264,0x270);reject(bad);}
    {auto bad=scene;bad.material_animation_roots[0].first=0x280;check(!load(b,bad),"scene cannot forge published clip mapping");}
    {auto bad=scene;bad.meshes.push_back(bad.meshes[0]);bad.mesh_joint.push_back(0);check(!load(b,bad),"multiple leaf meshes refuse");}
    {auto bad=scene;bad.meshes[0].material_offset=0xe0;check(!load(b,bad),"MOBJ identity mismatch refuses");}
    {auto bad=f;bad.real(0xfc,0);check(load(bad.archive(),scene)&&gf::hsd_leaf_material_sample(clip,1,&sample,&error)&&sample.uv_scale[0]==0&&sample.uv_offset[0]==0,"zero scale follows HSD zero matrix rule");}
    {auto bad=f;bad.block[0x32c]=8;check(load(bad.archive(),scene),"zero-rotation tracks may bind");sample.diffuse[0]=0.123f;
        check(!gf::hsd_leaf_material_sample(clip,1,&sample,&error)&&sample.diffuse[0]==0.123f,"nonzero animated rotation refuses atomically after material work");}
    {auto bad=f;bad.block[0x360]=7;check(load(bad.archive(),scene),"payload not eagerly interpreted");sample.diffuse[0]=0.123f;
        check(!gf::hsd_leaf_material_sample(clip,1,&sample,&error)&&sample.diffuse[0]==0.123f,"malformed texture opcode publishes no partial material");}
    {auto bad=f;bad.block[0x30c]=10;check(load(bad.archive(),scene)&&gf::hsd_leaf_material_sample(clip,0,&sample,&error)&&sample.alpha==0.5f,"material transparency is one minus track alpha");}
    {auto bad=f;bad.block[0x30c]=7;check(load(bad.archive(),scene)&&gf::hsd_leaf_material_sample(clip,0,&sample,&error)&&sample.specular[0]==127/255.0f,"specular channels use callback truncation");}
    {auto bad=f;bad.block[0x32c]=9;bad.stream(0x360,0.25f,0.75f);check(load(bad.archive(),scene)&&
        gf::hsd_leaf_material_sample(clip,1,&sample,&error)&&sample.texture_weight==0.75f,"texture blend track is distinct from material opacity");}
    {auto bad=f;bad.stream(0x340,2,2);check(load(bad.archive(),scene),"out-of-domain channel binds before sampling");
        check(!gf::hsd_leaf_material_sample(clip,0,&sample,&error),"out-of-domain material channel refuses rather than wrapping byte cast");}
    {auto bad=b;bad.resize(40);check(!load(bad,scene),"truncated archive refuses");}
    check(load(b,scene),"valid clip reload");
    // set_pose must reject bad material data before float-to-byte conversion,
    // retaining the previous valid pose/cache. No graphics backend is needed.
    gf::MenuScene renderer;check(renderer.load_scene(scene,&error),"owned scene loads for pose validation");
    gf::HsdAnimPose pose;pose.joints.resize(1);pose.materials.resize(1);
    check(gf::hsd_leaf_material_sample(clip,0,&pose.materials[0],&error)&&renderer.set_pose(&pose),"requested state accepted as renderer pose");
    pose.materials[0].uv_offset[0]=NAN;check(!renderer.set_pose(&pose)&&renderer.pose()->materials[0].uv_offset[0]==-0.375f,"nonfinite UV pose preserves previous state");
    pose.materials[0].has_texture=false;pose.materials[0].diffuse[1]=INFINITY;check(!renderer.set_pose(&pose),"nonfinite color rejected");
}
bool real_cases(){
    std::ifstream file("local/analysis/arc/optn00_mdl.arc",std::ios::binary);if(!file)return false;
    std::vector<gf::u8>b((std::istreambuf_iterator<char>(file)),{});gf::HsdScene s;std::string error;
    check(gf::hsd_load({b.data(),b.size()},&s,&error),"real OPTION scene loads");
    check(s.material_animation_roots.size()==5,"five explicit OPTION material roots");
    unsigned choices=0,bars=0;
    for(const auto& binding:s.material_animation_roots){
        gf::HsdLeafMaterialClip clip;gf::HsdAnimPose::Material zero,one;
        check(gf::hsd_leaf_material_clip_load({b.data(),b.size()},s,binding.second,&clip,&error),"real explicit leaf binding");
        check(gf::hsd_leaf_material_sample(clip,0,&zero,&error)&&gf::hsd_leaf_material_sample(clip,1,&one,&error),"real zero and one requested times");
        if(!clip.material_tracks.empty()){
            ++choices;check(zero.diffuse[0]==127/255.0f&&zero.diffuse[1]==127/255.0f&&zero.diffuse[2]==127/255.0f,"boolean initial gray");
            check(one.diffuse[0]==229/255.0f&&one.diffuse[1]==203/255.0f&&one.diffuse[2]==25/255.0f,"boolean selected yellow uses callback byte conversion");
        }else{
            ++bars;float prior=0;
            for(unsigned time=1;time<=5;++time){gf::HsdAnimPose::Material sample;
                check(gf::hsd_leaf_material_sample(clip,time,&sample,&error)&&sample.uv_offset[1]>prior&&sample.texture_weight==1&&sample.uv_scale[1]==1,"five distinct volume texture positions");
                check(near(sample.uv_offset[1],time*0.0446166992f),"volume translation has correct sign and quantization");prior=sample.uv_offset[1];}
        }
    }
    check(choices==4&&bars==1,"all four boolean choices plus volume template qualified");return true;
}
}
int main(){track_cases();leaf_cases();const bool real=real_cases();
    std::printf("{\"hsd_leaf_material_checks\":%u,\"real_present\":%s,\"all_passed\":true}\n",checks,real?"true":"false");}
