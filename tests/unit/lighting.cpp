#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/hsd.hpp"
#include "engine/graphics/projected_triangle.hpp"

namespace {
unsigned cases=0;
void check(bool ok,const char* text){++cases;if(!ok){std::fprintf(stderr,"lighting: %s\n",text);std::abort();}}
gf::SceneLight directional(gf::u16 flags=13){gf::SceneLight l;l.flags=flags;l.position[2]=1;for(auto& c:l.colour)c=128;return l;}
struct Fixture {
    std::vector<gf::u8> bytes=std::vector<gf::u8>(256);
    gf::hsd_detail::Reader reader;
    void word(unsigned at,gf::u32 value){for(unsigned k=0;k<4;++k)bytes[at+k]=gf::u8(value>>(24-k*8));}
    void pointer(unsigned at,unsigned target){word(at,target);reader.fields.push_back(at);reader.targets.push_back(target);}
    Fixture(){
        pointer(8,32);pointer(32,64);pointer(64,96);pointer(112,160);
        word(104,13u<<16);word(108,0x80808000);word(172,gf::f32_to_bits(1));
        reset();
    }
    void reset(){reader.block={bytes.data(),bytes.size()};std::sort(reader.fields.begin(),reader.fields.end());std::sort(reader.targets.begin(),reader.targets.end());}
    bool load(gf::SceneLighting& out){std::string error;reset();return gf::hsd_detail::read_scene_lighting(reader,0,&out,&error);}
};
}
int main(){
    gf::SceneLighting rig;rig.declared=true;rig.lights={directional()};
    const float point[3]={},normal[3]={0,0,1},origin[3]={},eye[3]={0,0,10};
    const gf::u8 ambient[4]={128,128,128,255},white[4]={255,255,255,255};
    auto sample=[&](const float* n=nullptr,bool diffuse=true,bool spec=true,float shine=50){
        return gf::light_vertex(rig,point,n?n:normal,origin,eye,ambient,white,shine,diffuse,spec);
    };
    auto lit=sample();check(lit.diffuse[0]==128,"actual directional color replaces fixed white light");
    check(lit.specular[0]==128,"aligned half-vector highlight");
    check(lit.diffuse[3]==255,"RGB-only lights do not dim material alpha");
    const float back[3]={0,0,-1};lit=sample(back);check(lit.diffuse[0]==0&&lit.specular[0]==0,"back-facing surface is unlit");
    const float diagonal[3]={0.70710678f,0,0.70710678f};lit=sample(diagonal);
    check(lit.diffuse[0]==91,"Lambert contribution rounded at vertex");
    check(lit.specular[0]==5,"rational specular curve, not Phong power");
    check(sample(normal,false,false).diffuse[0]==255&&sample(normal,false,false).specular[0]==0,"unlit material is neutral");
    check(sample(diagonal,true,true,300).specular[0]<lit.specular[0],"higher shininess narrows highlight");
    check(sample(normal,true,true,1e30f).specular[0]==128,"aligned highlight remains stable at large finite shininess");
    gf::SceneLight amb;amb.flags=4;for(auto& c:amb.colour)c=128;
    rig.lights={amb};lit=sample();check(lit.diffuse[0]==64,"ambient is material ambient times scene ambient");
    rig.lights.push_back(amb);check(sample().diffuse[0]==64,"first ambient only, not cumulative");
    rig.lights={directional(),directional()};check(sample().diffuse[0]==255,"multiple lights saturate");
    rig.lights[0].flags|=32;check(sample().diffuse[0]==128,"hidden lights ignored");
    rig.lights={directional(21)};check(sample().diffuse[3]==128,"alpha light has independent alpha channel");
    rig.lights={directional()};const float shifted[3]={10,0,0};
    const auto shifted_lit=gf::light_vertex(rig,point,normal,shifted,eye,ambient,white,50,true,true);
    check(shifted_lit.specular[0]<sample().specular[0],"specular uses joint-origin view vector");
    rig.lights.assign(5,directional());check(!gf::menu_light_rig_supported(rig),"dual-channel lights count against eight-slot GX limit");
    rig.lights.resize(4);check(gf::menu_light_rig_supported(rig),"four dual-channel directional lights fit");
    rig.lights[0].position[0]=NAN;check(!gf::menu_light_rig_supported(rig),"owned nonfinite light rig refused");
    {
        Fixture f;gf::SceneLighting result;
        check(f.load(result)&&result.declared&&result.supported&&result.lights.size()==1,"published light record decodes");
        check(result.lights[0].position[2]==1&&result.lights[0].colour[0]==128,"WObj and GX color decode");
    }
    {Fixture f;f.word(112,192);gf::SceneLighting result;check(!f.load(result)&&result.lights.empty(),"unrelocated light position refused without partial output");}
    {Fixture f;f.pointer(100,96);gf::SceneLighting result;check(!f.load(result),"self-referential light chain refused");}
    {Fixture f;f.word(104,14u<<16);gf::SceneLighting result;check(f.load(result)&&!result.supported,"point light explicitly tagged unsupported");}
    {Fixture f;f.word(172,0);gf::SceneLighting result;check(!f.load(result),"zero infinite direction refused");}
    {Fixture f;f.word(172,0x7fc00000);gf::SceneLighting result;check(!f.load(result),"nonfinite direction refused");}
    {Fixture f;f.bytes.resize(150);gf::SceneLighting result;check(!f.load(result),"truncated WObj refused");}
    {Fixture f;f.pointer(68,192);gf::SceneLighting result;check(f.load(result)&&result.animated,"animation presence not confused with evaluated lights");}
    // Newly clipped vertices must interpolate BOTH lighting channels.
    {
        gf::Camera c;c.near_plane=1;c.far_plane=10;
        gf::Vertex tri[3];tri[0].z=-0.5f;tri[1].x=0.2f;tri[1].z=-2;tri[2].y=0.2f;tri[2].z=-2;
        for(unsigned k=0;k<3;++k){tri[k].light[0]=k?200:20;tri[k].specular[1]=k?180:0;}
        std::vector<gf::Vertex> clipped;gf::project_triangle(c,tri,clipped);bool edge=false;
        for(const auto& v:clipped)if(std::fabs(v.inv_w-1)<1e-5f){edge=true;check(v.light[0]==80&&v.specular[1]==60,"clip-edge lighting interpolates in view space");}
        check(edge,"fixture crossed near plane");
    }
    bool real=false;
    std::ifstream file("local/analysis/arc/stff_mdl.arc",std::ios::binary);
    if(file){
        std::vector<gf::u8> bytes((std::istreambuf_iterator<char>(file)),{});gf::HsdScene scene;std::string error;
        check(gf::hsd_load({bytes.data(),bytes.size()},&scene,&error),"real menu decodes");
        const auto& lights=scene.lighting;
        check(lights.declared&&lights.supported&&lights.lights.size()==3,"real menu declares three supported lights");
        check(lights.lights[0].flags==4&&lights.lights[0].colour[0]==152,"real colored ambient");
        check(lights.lights[1].flags==13&&std::fabs(lights.lights[1].position[0]+2.7812777f)<1e-6f,"real first directional");
        unsigned specular=0,external=0;
        for(const auto& m:scene.meshes){specular+=(m.render_mode&8u)!=0;external+=m.texture_after_lighting;}
        check(specular>0&&external==12,"real specular materials and EXT stage placement");real=true;
    }
    std::printf("{\"lighting_checks\":%u,\"real_present\":%s,\"all_passed\":true}\n",cases,real?"true":"false");
}
