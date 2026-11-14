#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/hsd_model.hpp"
#include "engine/assets/hsd_skin.hpp"
#include "engine/gameplay/menu_scene.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"HSD skin: %s\n",why);std::abort();}}
bool near(float a,float b){return std::abs(a-b)<1e-4f;}
void word(std::vector<gf::u8>& b,unsigned at,gf::u32 v){gf::store_be32(b.data()+at,v);}
void scalar(std::vector<gf::u8>& b,unsigned at,float v){gf::store_be_f32(b.data()+at,v);}
std::vector<gf::u8> fixture(){
    std::vector<gf::u8> b(0x500,0);std::vector<gf::u32> reloc;
    auto ptr=[&](unsigned at,unsigned target){word(b,at,target);reloc.push_back(at);};
    for(unsigned j:{0x40u,0x80u}){word(b,j+4,j==0x40?0x20000002:0x20000000);for(unsigned k=0;k<3;++k)scalar(b,j+0x20+k*4,1);ptr(j+0x38,j==0x40?0xc0:0xf0);}
    ptr(0x48,0x80);ptr(0x50,0x120);ptr(0x128,0x130);ptr(0x12c,0x150);ptr(0x13c,0x280);
    for(unsigned matrix:{0xc0u,0xf0u})for(unsigned i=0;i<3;++i)scalar(b,matrix+i*16+i*4,1);
    scalar(b,0xc0+12,-10);scalar(b,0xf0+12,-20);
    ptr(0x158,0x180);word(b,0x15c,0xa0000003);ptr(0x160,0x200);ptr(0x164,0x300); // envelope + cullback, three DL blocks
    for(unsigned i=0;i<3;++i){const unsigned at=0x180+i*24;word(b,at,i==0?0:i==1?9:10);word(b,at+4,1);word(b,at+8,i==1?1:0);word(b,at+12,i==0?0:4);}
    word(b,0x1c8,255);word(b,0x280,0xffffffff);word(b,0x284,0xffffffff);word(b,0x288,0xffffffff);scalar(b,0x28c,1);
    b[0x200]=0x90;gf::store_be16(b.data()+0x201,3);
    for(unsigned i=0;i<3;++i){const unsigned at=0x203+i*25;b[at]=i?3:0;scalar(b,at+1,float(i));scalar(b,at+5,float(i));scalar(b,at+13,1);scalar(b,at+17,1);}
    ptr(0x300,0x320);ptr(0x304,0x350);ptr(0x320,0x40);scalar(b,0x324,1);
    ptr(0x350,0x40);scalar(b,0x354,0.25f);ptr(0x358,0x80);scalar(b,0x35c,0.75f);
    std::vector<gf::u8> out(32,0);out.insert(out.end(),b.begin(),b.end());
    for(auto field:reloc){const auto at=out.size();out.resize(at+4);gf::store_be32(out.data()+at,field);}
    const auto at=out.size();out.resize(at+8);gf::store_be32(out.data()+at,0x40);out.insert(out.end(),{'s','c','e','n','e',0});
    word(out,0,out.size());word(out,4,b.size());word(out,8,reloc.size());word(out,12,1);return out;
}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    auto bytes=fixture();gf::HsdSkinAsset asset;std::string error;
    auto load=[&](const auto& b){return gf::hsd_skin_load({b.data(),b.size()},&asset,&error);};
    const bool loaded=load(bytes);if(!loaded)std::fprintf(stderr,"fixture refused: %s\n",error.c_str());
    check(loaded&&asset.scene.joints.size()==2&&asset.meshes.size()==1,"owned published skeleton/palette fixture loads");
    check(asset.scene.meshes[0].vertices[0].matrix_index==0&&asset.scene.meshes[0].vertices[1].matrix_index==3&&asset.meshes[0].palettes.size()==2,"raw GX matrix row retained and bound to declared palette");
    check(asset.scene.model_roots==std::vector<gf::u32>{0x40}&&asset.scene.joints[0].model_root==0&&asset.scene.joints[1].model_root==0,"published model-root identity propagates through skeleton children");
    gf::MenuScene menu;check(!menu.load_scene(asset.scene,&error),"generic MenuScene still refuses unevaluated deformation");
    std::vector<gf::Mat4> pose(2);pose[0].at(0,3)=10;pose[1].at(0,3)=20;gf::HsdSkinGeometry result;
    check(gf::hsd_skin_sample(asset,pose,&result,&error)&&near(result.world_vertices[0][0].position[0],10)&&near(result.world_vertices[0][1].position[0],1),"source full-weight direct world versus blended world*inverse-bind paths differ correctly");
    pose[1].at(0,3)=24;check(gf::hsd_skin_sample(asset,pose,&result,&error)&&near(result.world_vertices[0][1].position[0],4),"changed bone pose updates weighted geometry without mutating bind/source data");
    check(asset.scene.meshes[0].vertices[1].position[0]==1,"sampling doesn't accumulate into source vertices");
    const auto unit_weight_asset=asset;asset.meshes[0].palettes[0].influences[0].weight=1-std::numeric_limits<gf::f32>::epsilon();
    check(gf::hsd_skin_sample(asset,pose,&result,&error)&&near(result.world_vertices[0][0].position[0],0),"actual game threshold is exactly one: near-full weight uses inverse-bind blend, not Melee's fast path");asset=unit_weight_asset;
    const auto envelope_asset=asset;auto shared_bytes=fixture();word(shared_bytes,32+0x15c,0x80000003);word(shared_bytes,32+0x164,0x80);
    check(load(shared_bytes)&&asset.meshes[0].kind==gf::HsdSkinKind::Shared&&asset.scene.meshes[0].deformation_required,"shared two-joint POBJ is no longer mislabeled static");
    pose[1].at(0,3)=20;check(gf::hsd_skin_sample(asset,pose,&result,&error)&&near(result.world_vertices[0][0].position[0],10)&&near(result.world_vertices[0][1].position[0],21),"shared PNMTX0/PNMTX1 use different joint worlds");
    check(!menu.load_scene(asset.scene,&error),"generic static renderer refuses shared-joint palette too");asset=envelope_asset;
    const auto original=asset;asset.joints[1].has_inverse=false;
    check(!gf::hsd_skin_sample(asset,pose,&result,&error)&&result.world_vertices.empty(),"missing required inverse bind refuses atomically");asset=original;
    auto nonfinite=pose;nonfinite[1].m[0]=NAN;check(!gf::hsd_skin_sample(asset,nonfinite,&result,&error)&&result.world_vertices.empty(),"nonfinite pose refuses before publishing geometry");
    auto singular=pose;singular[0].at(0,0)=0;check(!gf::hsd_skin_sample(asset,singular,&result,&error),"singular full-weight palette refuses, no copied-normal fallback");
    asset.scene.meshes[0].vertices[0].matrix_index=1;check(!gf::hsd_skin_sample(asset,pose,&result,&error),"non-multiple-of-three GX row selector refuses");asset=original;
    asset.scene.meshes[0].vertices[0].matrix_index=30;check(!gf::hsd_skin_sample(asset,pose,&result,&error),"unknown vertex palette slot refuses");asset=original;
    asset.meshes[0].palettes[1].influences[0].weight=0.1f;check(!gf::hsd_skin_sample(asset,pose,&result,&error),"non-unit weights aren't silently normalized");asset=original;
    asset.meshes[0].palettes[1].influences[0].joint=999;check(!gf::hsd_skin_sample(asset,pose,&result,&error),"out-of-skeleton influence refuses");asset=original;
    // Normal must use inverse transpose of the BLENDED matrix, not a blend of
    // already inverse-transposed normals. Own both matrices/weights explicitly.
    for(auto& bind:asset.joints){bind.has_inverse=true;bind.inverse={};}
    asset.meshes[0].palettes[1].influences={{0,0.5f},{1,0.5f}};pose.assign(2,{});pose[0].at(0,0)=2;pose[1].at(1,1)=4;
    check(gf::hsd_skin_sample(asset,pose,&result,&error)&&near(result.world_vertices[0][1].normal[0]/result.world_vertices[0][1].normal[1],5.0f/3),"inverse transpose is taken AFTER position-matrix blending");
    pose[0].at(0,0)=-2;check(gf::hsd_skin_sample(asset,pose,&result,&error)&&result.world_vertices[0][0].normal[0]<0,"reflected normal orientation preserved");
    // Metadata ownership and strict source-pointer gates.
    check(load(bytes),"owned asset reload");bytes.clear();check(gf::hsd_skin_sample(asset,std::vector<gf::Mat4>(2),&result,&error),"asset owns all skin metadata after source archive lifetime ends");
    bytes=fixture();for(unsigned field:{0x164u,0x300u,0x350u}){auto bad=bytes;word(bad,32+field,0x480);check(!load(bad)&&!asset.loaded,"invalid source skin pointer doesn't publish partial asset");}
    auto bad=bytes;scalar(bad,32+0x354,NAN);check(!load(bad),"nonfinite source weight refuses");
    bad=bytes;word(bad,32+0x15c,0x90000003);check(!load(bad),"shape animation is not admitted as skin");
    bad=bytes;word(bad,32+0x44,0x20000004);check(!load(bad),"ENVELOPE_MODEL bit alone doesn't qualify the no-correction path");
    bad=bytes;word(bad,32+0x44,0x20000000);check(!load(bad),"non-root envelope/ancestor correction refuses until qualified");
    std::vector<gf::u8> model(256,0);word(model,0,256);model.insert(model.end(),bytes.begin(),bytes.end());model.push_back(0);gf::ByteView archive;
    check(gf::hsd_model_archive({model.data(),model.size()},&archive,&error)&&archive.size==bytes.size()&&archive.data==model.data()+256,"model wrapper uses declared HSD offset and excludes only checked trailing padding");
    model.back()=1;check(!gf::hsd_model_archive({model.data(),model.size()},&archive,&error)&&!archive.data,"nonzero model trailer refuses atomically");model.back()=0;
    word(model,0,0xfffffffcu);check(!gf::hsd_model_archive({model.data(),model.size()},&archive,&error),"out-of-range declared model section refuses, no candidate scan fallback");
    unsigned real=0;
    for(const auto& name:{"pl0615_mdl.arc","pl000a_mdl.arc"}){auto data=read(std::string("local/recomp-evidence/native-character-skin-20261109/")+name);if(data.empty())continue;
        const bool ok=gf::hsd_model_archive({data.data(),data.size()},&archive,&error)&&gf::hsd_skin_load(archive,&asset,&error);if(!ok)std::fprintf(stderr,"%s refused: %s\n",name,error.c_str());
        check(ok,"source-identified startup model loads through declared wrapper");pose.resize(asset.scene.joints.size());
        for(unsigned i=0;i<pose.size();++i){const auto& joint=asset.scene.joints[i];const auto local=gf::mat4_from_srt(joint.rotation,joint.scale,joint.translation);pose[i]=joint.parent==gf::kHsdNoJoint?local:gf::mat4_multiply(pose[joint.parent],local);}
        check(gf::hsd_skin_sample(asset,pose,&result,&error)&&result.world_vertices.size()==asset.scene.meshes.size(),"actual complete model with supplied file-SRT matrices (not source actor/clip qualification)");
        unsigned changed=0;const auto rest=result;
        const auto slot=asset.scene.meshes[0].vertices[0].matrix_index/3;
        const auto& influences=asset.meshes[0].palettes[slot].influences;
        auto changed_joint=influences[0].joint;for(const auto& influence:influences)if(influence.weight>0){changed_joint=influence.joint;break;}
        pose[changed_joint].at(0,3)+=2;
        check(gf::hsd_skin_sample(asset,pose,&result,&error),"owned diagnostic supplied-bone-matrix change on actual model");
        for(unsigned m=0;m<result.world_vertices.size();++m)for(unsigned v=0;v<result.world_vertices[m].size();++v)changed+=result.world_vertices[m][v].position[0]!=rest.world_vertices[m][v].position[0];
        check(changed>0,"actual model geometry responds to supplied bone pose, no frozen source-vertex cache");++real;
    }
    std::printf("{\"hsd_skin_checks\":%u,\"real_startup_models\":%u,\"startup_clips_or_render_admission_ported\":false,\"all_passed\":true}\n",checks,real);
}
