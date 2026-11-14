#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/option_scene.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"option widgets: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const char* p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    // Owned native-profile fixture and decoded leaf scene; no proprietary bytes.
    std::vector<gf::u8> file(1368),asset={1,2,3};gf::HsdScene source;
    auto word=[&](unsigned at,gf::u32 v){gf::store_be32(file.data()+at,v);};
    auto real=[&](unsigned at,float v){word(at,gf::f32_to_bits(v));};
    word(0,0x47464f57);word(4,1);word(8,gf::menu_asset_hash({asset.data(),asset.size()}));word(12,23);word(16,23);real(20,6.28318530718f/65536);word(24,gf::u32(-512));
    for(unsigned i=0;i<23;++i){word(32+i*4,0x100+i*0x40);gf::HsdJoint j;j.archive_offset=0x100+i*0x40;source.joints.push_back(j);source.meshes.push_back({});source.mesh_joint.push_back(i);}
    for(unsigned i=0;i<10;++i)real(124+i*4,100-i*10);
    const unsigned group[]={0,0,1,3,4,7,8,8,9,9};for(unsigned i=0;i<10;++i)word(164+i*4,group[i]);
    for(unsigned i=0;i<7;++i)word(204+i*4,i+1);
    for(unsigned i=0;i<4;++i)word(232+i*4,i==0?0:i==1?7:i==2?8:9);
    word(248,0x33cd33ff);word(252,0x009a00ff);word(256,35);word(260,30);
    unsigned n=0;const unsigned counts[]={1,5,2,3,7,5};
    for(unsigned kind=0;kind<6;++kind)for(unsigned id=0;id<counts[kind];++id){const unsigned at=264+n++*48;
        file[at]=gf::u8(kind);file[at+1]=gf::u8(id);const unsigned count=(kind==5||(kind==1&&id==0)||(kind==4&&id>=3))?2:1;
        file[at+2]=gf::u8(count);word(at+4,0x100);if(count==2)word(at+8,0x140);}
    gf::OptionWidgetProfile profile;std::string error;
    auto load=[&](const std::vector<gf::u8>& bytes){return gf::option_widget_profile_load({bytes.data(),bytes.size()},{asset.data(),asset.size()},source,&profile,&error);};
    check(load(file),"owned bounded native profile loads");
    for(unsigned offset:{0u,4u,8u,12u,16u,28u,32u,128u,164u,204u,232u,251u,256u,264u,265u,266u,267u,268u,276u}){
        auto bad=file;bad[offset]=offset==251?0:255;check(!load(bad),"malformed native record refuses");}
    {auto bad=file;bad.resize(100);check(!load(bad),"truncated profile refuses");}
    {auto bad=file;gf::store_be32(bad.data()+124,gf::f32_to_bits(NAN));check(!load(bad),"NaN row refuses");}
    {auto bad=file;gf::store_be32(bad.data()+264+24,gf::f32_to_bits(INFINITY));check(!load(bad),"infinite template rotation refuses");}
    // Owned renderer fixture proves code-driven matrix order is not lost in the
    // cached projection update path. Check composed vertices directly.
    gf::HsdScene s;gf::HsdJoint j;s.joints.push_back(j);gf::HsdMesh mesh;gf::HsdVertex vertex;vertex.position[0]=1;mesh.vertices.push_back(vertex);s.meshes.push_back(mesh);s.mesh_joint.push_back(0);
    gf::MenuScene renderer;check(renderer.load_scene(s,&error),"owned matrix fixture");gf::HsdAnimPose pose;pose.joints.resize(1);pose.joints[0].matrix_override=true;pose.joints[0].matrix[3]=10;
    check(renderer.set_pose(&pose)&&renderer.scene().meshes[0].vertices[0].position[0]==11,"explicit local matrix composes once");
    check(renderer.set_pose(&pose)&&renderer.scene().meshes[0].vertices[0].position[0]==11,"repeated matrix pose doesn't accumulate");
    pose.joints[0].matrix[0]=NAN;check(!renderer.set_pose(&pose)&&renderer.scene().meshes[0].vertices[0].position[0]==11,"invalid matrix publishes no partial pose");
    bool real_present=false;const auto archive=read("local/analysis/arc/optn00_mdl.arc"),content=read("local/content/option-widgets.gfow");
    if(!archive.empty()&&!content.empty()){
        gf::OptionScene scene;check(scene.load({archive.data(),archive.size()},{content.data(),content.size()},&error),"real source-qualified widget instances load");
        check(scene.scene().meshes.size()==33&&scene.pose().joints.size()==33,"23 templates instantiate 33 original models, logo excluded");
        gf::OptionSettings values;unsigned last_cursor=0;
        for(unsigned row=0;row<10;++row){check(scene.update(row,values,true,&error),"all ten row poses evaluate");unsigned hidden=0;for(const auto& p:scene.pose().joints)hidden+=p.hidden;
            check(hidden==1,"exactly one alternate cursor hidden at every row");
            check(scene.pose().joints[0].matrix[7]==scene.profile().row_y[row],"highlight uses content row positions");last_cursor+=unsigned(scene.pose().joints[7].matrix_override);}
        check(last_cursor==10,"instances use explicit matrices");
        values.mono=1;values.volume={0,2,3};values.rumble_mask=5;check(scene.update(5,values,true,&error),"independent sound/volumes/four-port rumble pose");
        check(!scene.update(10,values,true,&error),"invalid row refuses");values.volume[0]=5;check(!scene.update(0,values,true,&error),"invalid settings refuse");real_present=true;
    }
    std::printf("{\"option_widget_checks\":%u,\"real_present\":%s,\"all_passed\":true}\n",checks,real_present?"true":"false");
}
