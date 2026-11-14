#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/hsd_bones.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"HSD bones: %s\n",why);std::abort();}}
void word(std::vector<gf::u8>& b,std::size_t at,gf::u32 v){gf::store_be32(b.data()+at,v);}
struct Fixture {
    std::vector<gf::u8> prefix=std::vector<gf::u8>(256,0),archive;
    Fixture(){
        word(prefix,0,256);word(prefix,4,32);word(prefix,8,128);
        const gf::u8 slots[]={2,0,3,1};for(unsigned i=0;i<4;++i)prefix[128+i]=slots[i];
        const gf::u8 links[4][4]={{3,1,2,77},{255,255,2,88},{0,255,255,99},{255,255,0,111}};
        for(unsigned i=0;i<4;++i)for(unsigned k=0;k<4;++k)prefix[32+i*4+k]=links[i][k];
        std::vector<gf::u8> block(0x180,0);std::vector<gf::u32> fields;
        auto ptr=[&](unsigned at,unsigned to){word(block,at,to);fields.push_back(at);};
        ptr(0x10,0x20);ptr(0x20,0x30);ptr(0x30,0x40); // scene -> array -> record0 -> root
        ptr(0x48,0x80);ptr(0x88,0xc0);ptr(0x8c,0x100);
        for(unsigned i=0;i<4;++i)for(unsigned k=0;k<3;++k)gf::store_be_f32(block.data()+0x40+i*0x40+0x2c+k*4,float(i*10+k+1));
        archive.resize(32,0);archive.insert(archive.end(),block.begin(),block.end());
        for(auto field:fields){auto at=archive.size();archive.resize(at+4);word(archive,at,field);}
        const auto at=archive.size();archive.resize(at+8);word(archive,at,0x10);
        const char name[]="scene_data";archive.insert(archive.end(),name,name+sizeof(name));
        word(archive,0,archive.size());word(archive,4,block.size());word(archive,8,fields.size());word(archive,12,1);
    }
};
Fixture chain(unsigned count){
    Fixture f;f.prefix.assign(256,0);word(f.prefix,0,256);word(f.prefix,4,32);word(f.prefix,8,192);
    std::vector<gf::u8> block(0x40+count*0x40,0);std::vector<gf::u32> fields;
    auto ptr=[&](unsigned at,unsigned to){word(block,at,to);fields.push_back(at);};
    ptr(0x10,0x20);ptr(0x20,0x30);ptr(0x30,0x40);
    for(unsigned i=0;i<count;++i){
        if(i+1<count)ptr(0x40+i*0x40+8,0x40+(i+1)*0x40);
        if(i<32){f.prefix[192+i]=gf::u8(i);f.prefix[32+i*4]=i+1<count?gf::u8(i+1):255;f.prefix[33+i*4]=255;f.prefix[34+i*4]=i?gf::u8(i-1):255;}
    }
    f.archive.assign(32,0);f.archive.insert(f.archive.end(),block.begin(),block.end());
    for(auto field:fields){auto at=f.archive.size();f.archive.resize(at+4);word(f.archive,at,field);}
    const auto at=f.archive.size();f.archive.resize(at+8);word(f.archive,at,0x10);
    const char name[]="scene_data";f.archive.insert(f.archive.end(),name,name+sizeof(name));
    word(f.archive,0,f.archive.size());word(f.archive,4,block.size());word(f.archive,8,fields.size());word(f.archive,12,1);return f;
}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    Fixture f;gf::HsdModelBones bones;std::string error;
    auto load=[&](const Fixture& x){return gf::hsd_bones_load({x.prefix.data(),x.prefix.size()},{x.archive.data(),x.archive.size()},&bones,&error);};
    check(load(f)&&bones.loaded&&bones.bones.size()==4,"owned named model record0 binds");
    check(bones.preorder_slots==std::vector<gf::u8>{2,0,3,1},"child-before-next traversal uses nonidentity logical-slot table");
    check(bones.root_offset==0x40&&bones.bones[2].joint_offset==0x40&&bones.bones[3].joint_offset==0xc0,"logical slot != traversal or descriptor offset");
    check(bones.bones[0].child==3&&bones.bones[0].next==1&&bones.bones[0].parent==2,"wrapper links retain declared topology");
    check(bones.bones[3].translation[0]==21&&bones.bones[1].translation[2]==33,"descriptor LOCAL translations assigned by logical slot");
    const auto saved=bones;f.prefix.clear();f.archive.clear();
    check(bones.bones[3].translation[0]==21&&bones.preorder_slots==saved.preorder_slots,"metadata owns storage after archive lifetime");f=Fixture();
    auto bad=[&](Fixture x,const char* why){check(!load(x)&&!bones.loaded&&bones.bones.empty()&&bones.preorder_slots.empty(),why);};
    auto x=f;word(x.prefix,0,252);bad(x,"declared prefix extent must match split file");
    x=f;word(x.prefix,4,0xfffffffcu);bad(x,"overflowing record offset refuses");
    x=f;word(x.prefix,4,33);bad(x,"unaligned metadata records refuse");
    x=f;word(x.prefix,8,0xffffffffu);bad(x,"overflowing order offset refuses");
    x=f;word(x.prefix,8,256);bad(x,"order at prefix end refuses");
    x=f;word(x.prefix,8,8);bad(x,"header cannot masquerade as order table");
    x=f;word(x.prefix,4,124);bad(x,"table overlap refuses");
    x=f;word(x.prefix,8,254);bad(x,"truncated order table refuses");
    x=f;word(x.prefix,4,252);bad(x,"truncated logical-slot records refuse");
    x=f;x.prefix[128]=255;bad(x,"signed negative slot refuses before arithmetic");
    x=f;x.prefix[128]=128;bad(x,"signed-byte boundary refuses");
    x=f;x.prefix[128]=32;bad(x,"resource-array capacity enforced");
    x=f;x.prefix[129]=2;bad(x,"duplicate slot refuses");
    x=f;x.prefix[128]=4;bad(x,"sparse slot set refuses rather than leaving uninitialized bones");
    x=f;x.prefix[32]=1;bad(x,"child link inconsistent with HSD refuses");
    x=f;x.prefix[33]=255;bad(x,"sibling link inconsistent with HSD refuses");
    x=f;x.prefix[34]=255;bad(x,"parent link inconsistent with HSD refuses");
    x=f;for(unsigned i=0;i<4;++i)x.prefix[32+i*4+3]=255;
    check(load(x),"input record byte3 is ignored exactly as source copy does");
    x=f;word(x.archive,32+0x48,0x40);bad(x,"child cycle refuses");
    x=f;word(x.archive,32+0x8c,0xc0);bad(x,"shared child/sibling identity refuses");
    x=f;word(x.archive,32+0x48,0x170);bad(x,"truncated JOBJ descriptor refuses");
    x=f;word(x.archive,32+0x44,0x1000);bad(x,"instance traversal semantics aren't guessed");
    x=f;word(x.archive,32+0x40,0x80);bad(x,"unrelocated custom-class word refuses");
    x=f;gf::store_be_f32(x.archive.data()+32+0x40+0x2c,NAN);bad(x,"nonfinite local translation refuses");
    x=f;x.archive[x.archive.size()-2]='x';bad(x,"unpublished model isn't candidate-scanned");
    x=f;word(x.archive,32+0x30,0);bad(x,"missing model-record root refuses");
    x=f;word(x.archive,32+0x20,0);bad(x,"missing model record refuses");
    x=f;word(x.archive,12,4097);bad(x,"public-symbol budget refuses");
    x=f;const auto symbols=32+gf::load_be32(x.archive.data()+4)+gf::load_be32(x.archive.data()+8)*4;
    x.archive.insert(x.archive.begin()+symbols+8,8,0);word(x.archive,symbols+8,0x10);word(x.archive,12,2);word(x.archive,0,x.archive.size());
    bad(x,"duplicate named scene exports refuse");
    x=f;word(x.archive,symbols+4,0xffffffffu);bad(x,"symbol name offset overflow refuses");
    x=f;x.archive.back()='a';bad(x,"unterminated export name refuses");
    x=f;word(x.archive,32+0x10,0x30);bad(x,"published root table cannot be bypassed");
    check(load(chain(32))&&bones.bones.size()==32,"exact supported bone capacity accepted");
    bad(chain(33),"one over supported bone capacity refuses");
    check(!gf::hsd_bones_load({f.prefix.data(),f.prefix.size()},{f.archive.data(),f.archive.size()},nullptr,&error),"null destination refuses");
    for(std::size_t n=0;n<12;++n)check(!gf::hsd_bones_load({f.prefix.data(),n},{f.archive.data(),f.archive.size()},&bones,&error),"short wrapper header refuses");
    // Exhaust each actual bounded input prefix length; both views retain valid
    // backing storage so the parser must honor the provided lengths, not it.
    bool short_ok=true;
    for(std::size_t n=0;n<f.archive.size();++n)short_ok=short_ok&&!gf::hsd_bones_load({f.prefix.data(),f.prefix.size()},{f.archive.data(),n},&bones,&error);
    check(short_ok,"every truncated archive view refuses");
    short_ok=true;
    for(std::size_t n=0;n<f.prefix.size();++n)short_ok=short_ok&&!gf::hsd_bones_load({f.prefix.data(),n},{f.archive.data(),f.archive.size()},&bones,&error);
    check(short_ok,"every truncated wrapper view refuses");
    unsigned seed=0x9e3779b9u;
    for(unsigned n=0;n<512;++n){
        x=f;seed=seed*1664525u+1013904223u;auto& data=n&1?x.prefix:x.archive;const auto at=seed%data.size();
        seed=seed*1664525u+1013904223u;data[at]^=gf::u8((seed>>24)|1);
        const bool ok=load(x);if(!ok&&(bones.loaded||!bones.bones.empty()))std::abort();
    }
    check(true,"512 deterministic malformed-input probes are bounded and atomic on refusal");
    unsigned real=0;
    for(const char* name:{"g-red","sasuke"}){
        const std::string base=std::string("local/content/startup-characters/")+name+"/";
        auto prefix=read(base+"model-prefix.bin"),archive=read(base+"model.hsd");if(prefix.empty()||archive.empty())continue;
        check(gf::hsd_bones_load({prefix.data(),prefix.size()},{archive.data(),archive.size()},&bones,&error),"actual imported record0 wrapper maps without motion/pose admission");
        const bool sasuke=std::string(name)=="sasuke";
        const unsigned count=sasuke?20:26;check(bones.bones.size()==count,"source bone count excludes bank effect/weapon records");
        gf::HsdScene scene;check(gf::hsd_load({archive.data(),archive.size()},&scene,&error),"independent existing geometry decode of actual bank");
        std::vector<gf::u32> offsets;for(const auto& joint:scene.joints)if(joint.model_root==0)offsets.push_back(joint.archive_offset);
        bool agree=offsets.size()==count;
        for(unsigned i=0;i<count&&agree;++i)agree=bones.bones[bones.preorder_slots[i]].joint_offset==offsets[i];
        check(agree,"binding child/next replay agrees with geometry decoder selected-record traversal");
        check(sasuke?bones.preorder_slots[8]==8:bones.preorder_slots[8]==24,"G-Red nonidentity versus Sasuke identity map retained");++real;
    }
    std::printf("{\"hsd_bone_checks\":%u,\"real_models\":%u,\"pose_or_startup_admission\":false,\"all_passed\":true}\n",checks,real);
}
