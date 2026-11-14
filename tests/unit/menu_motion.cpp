#include <cstdio>
#include <cstdlib>
#include "engine/assets/menu_motion.hpp"
#include "engine/gameplay/menu_scene.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* text){++checks;if(!ok){std::fprintf(stderr,"menu motion: %s\n",text);std::abort();}}
struct Backend final:gf::IRenderBackend{
    std::vector<gf::Vertex> vertices;std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}
    bool upload_vertices(const gf::Vertex* p,std::size_t n)override{vertices.assign(p,p+n);return true;}
    bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}
    bool upload_matrix(gf::u32,const float*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();}
    void submit(const gf::DrawCall& d)override{calls.push_back(d);}
    void end_frame()override{}void present()override{}
};
gf::HsdScene fixture(){
    gf::HsdScene s;s.menu_selection=0;auto& motion=s.menu_motion;motion.present=true;
    motion.angle_unit=6.28318530718f/65536;motion.selector_step=1024;motion.identifier_step=-256;motion.selector_joint=900;
    auto add=[&](gf::u32 descriptor,gf::u8 colour,float x,float y,gf::u32 parent=gf::kHsdNoJoint){
        gf::HsdJoint joint;joint.archive_offset=descriptor;joint.parent=parent;joint.translation[0]=x;joint.translation[1]=y;joint.translation[2]=parent==gf::kHsdNoJoint?-100:0;
        s.joints.push_back(joint);gf::HsdMesh mesh;mesh.cull_backface=false;mesh.material_colour[0]=colour;
        for(const auto& xy:std::vector<std::pair<float,float>>{{-2,-1},{2,-1},{1,3}}){gf::HsdVertex v;v.position[0]=xy.first;v.position[1]=xy.second;v.normal[2]=1;mesh.vertices.push_back(v);}
        mesh.indices={0,1,2};s.meshes.push_back(mesh);s.mesh_joint.push_back(static_cast<gf::u32>(s.joints.size()-1));
    };
    for(unsigned item=0;item<7;++item){
        const gf::u32 gear=100+item*3,id=gear+1,caption=gear+2;
        motion.bindings.push_back({item,gear,id,caption,512});
        const float x=float(item)*8-24;
        add(gear,gf::u8(10+item),x,0);add(id,gf::u8(40+item),x,5);
        if(item==0)add(800,60,0,0,static_cast<gf::u32>(s.joints.size()-1));
        add(caption,gf::u8(70+item),x,10);
        for(auto& v:s.meshes.back().vertices){v.position[2]=v.position[1];v.position[1]=0;v.normal[1]=1;v.normal[2]=0;}
    }
    add(900,99,0,0);return s;
}
void word(std::vector<gf::u8>& b,gf::u32 n){for(int shift=24;shift>=0;shift-=8)b.push_back(gf::u8(n>>shift));}
std::vector<gf::u8> content(const gf::HsdScene::MenuMotion& c){
    std::vector<gf::u8> b;for(gf::u32 x:{0x47464d56u,1u,123u,7u,gf::f32_to_bits(c.angle_unit),gf::u32(c.selector_step),gf::u32(c.identifier_step),c.selector_joint})word(b,x);
    for(const auto& x:c.bindings)for(gf::u32 n:{x.item,x.gear,x.identifier,x.caption,gf::u32(x.step)})word(b,n);
    return b;
}
void draw(gf::MenuScene& scene,Backend& b){gf::RenderEngine r(b);r.begin_frame({});r.reset_arena();gf::Camera c;c.near_plane=1;c.far_plane=1000;
    check(scene.draw(r,c)>0,"scene draws");r.end_frame();}
const gf::Vertex* colour(const Backend& b,unsigned red){for(const auto& c:b.calls)if(b.vertices[c.vertex_first].r==red)return &b.vertices[c.vertex_first];return nullptr;}
}
int main(){
    auto source=fixture();std::string error;
    auto bytes=content(source.menu_motion);gf::HsdScene::MenuMotion parsed;
    check(gf::menu_motion_load({bytes.data(),bytes.size()},123,source,&parsed,&error),"bound content loads");
    check(parsed.bindings.size()==7&&parsed.identifier_step==-256,"signed phase steps retained");
    auto reject=[&](std::vector<gf::u8> b){check(!gf::menu_motion_load({b.data(),b.size()},123,source,&parsed,&error)&&!parsed.present,"malformed content refuses without partial output");};
    {auto b=bytes;b.resize(171);reject(b);}
    {auto b=bytes;b[11]^=1;reject(b);}
    {auto b=bytes;b[16]=0x7f;b[17]=0xc0;reject(b);}
    {auto b=bytes;b[35]=7;reject(b);}
    {auto b=bytes;b[39]=0;reject(b);}
    {auto b=bytes;b[52]=0;b[53]=0;b[54]=0;b[55]=0;reject(b);}
    {auto b=bytes;b[20]=0x7f;reject(b);}
    gf::MenuMotionState state;
    gf::menu_motion_step(source.menu_motion,0,state);
    check(state.gear_phase[0]==512&&state.gear_phase[6]==512,"all primary gears advance");
    check(state.identifier_y_phase[0]==0&&state.identifier_y_phase[1]==-256,"selected identifier freezes while hidden");
    check(state.selector_phase==1024,"selector rotates independently");
    state.gear_phase[0]=65530;source.menu_motion.bindings[0].step=16;
    gf::menu_motion_step(source.menu_motion,1,state);check(state.gear_phase[0]==10,"16-bit angle wraps without signed overflow");
    check(std::fabs(gf::menu_phase_angle(32768,source.menu_motion.angle_unit)+3.14159265f)<1e-5f,"signed binary-angle conversion");
    {
        gf::HsdJoint j;j.translation[0]=9;j.rotation[0]=0.4f;
        auto actual=gf::menu_motion_matrix(j,0.6f,0.2f);const float authored[3]={0.4f,0.2f,0},ry[3]={0,0.6f,0},scale[3]={1,1,1},zero[3]={};
        const auto expected=gf::mat4_multiply(gf::mat4_from_srt(authored,scale,j.translation),gf::mat4_from_srt(ry,scale,zero));
        for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col)check(std::fabs(actual.at(row,col)-expected.at(row,col))<1e-6f,"authored orientation precedes local Y spin");
    }
    {
        gf::HsdJoint j;const float point[3]={1,2,0};float rotated[3]={};
        gf::mat4_transform_point(gf::menu_motion_matrix(j,0,1.57079632679f),point,rotated);
        check(std::fabs(rotated[0])<1e-5f&&std::fabs(rotated[1]-2)<1e-5f&&std::fabs(rotated[2]+1)<1e-5f,
              "identifier rotates around authored Y: height stays fixed instead of cartwheeling over gear");
        const float eye[3]={0,0,10},origin[3]={};gf::Mat4 caption;
        check(gf::menu_caption_matrix(origin,eye,&caption),"caption eye-facing basis exists");
        const float artwork[3]={1,0,2};gf::mat4_transform_point(caption,artwork,rotated);
        check(rotated[0]==1&&rotated[1]==-2&&rotated[2]==0,"local X/Z caption artwork faces camera, local Z points screen-down");
        const float degenerate[3]={10,0,0};check(!gf::menu_caption_matrix(origin,degenerate,&caption),"degenerate caption basis refuses");
    }
    source=fixture();gf::MenuScene scene;Backend b;
    check(scene.load_scene(source,&error),"motion scene loads");draw(scene,b);
    check(!colour(b,40)&&!colour(b,60),"selected identifier and descendant are hidden");
    check(colour(b,41)&&colour(b,70)&&!colour(b,71),"unselected identifier visible, only selected static caption visible");
    check(colour(b,10)&&colour(b,99),"primary gear and actual selector mesh remain");
    scene.set_menu_focus(0,false);draw(scene,b);
    check(colour(b,10)&&colour(b,70)&&!colour(b,11)&&!colour(b,41),"transition focus keeps only active item's models");
    check(!colour(b,99),"transition suppresses selector without drawing its world-space source mesh");
    scene.set_menu_focus(7,true);draw(scene,b);
    const auto selector0=*colour(b,99),gear0=*colour(b,11),caption0=*colour(b,70);
    scene.advance_motion();draw(scene,b);
    check(std::fabs(colour(b,99)->x-selector0.x)>0.01f,"flat selector mesh rotates");
    check(std::fabs(colour(b,11)->x-gear0.x)+std::fabs(colour(b,11)->y-gear0.y)>0.0001f,"primary local Y rotation reaches projected vertices");
    const auto vertex_count=b.vertices.size();
    for(unsigned tick=0;tick<130;++tick){scene.advance_motion();draw(scene,b);check(b.vertices.size()==vertex_count,"partial cache rebuild replaces geometry instead of appending trails");}
    check(colour(b,70)->x==caption0.x&&colour(b,70)->y==caption0.y,"selected caption is static");
    for(const auto& call:b.calls)if(b.vertices[call.vertex_first].r==99)check(!call.depth_test&&!call.depth_write,"selector is screen-space overlay");
    unsigned selector_order=0,caption_order=0;
    for(unsigned i=0;i<b.calls.size();++i){const auto& call=b.calls[i];const unsigned red=b.vertices[call.vertex_first].r;
        if(red==99)selector_order=i;
        if(red==70){caption_order=i;check(!call.depth_test&&!call.depth_write,"foreground text cannot be rejected by scene/selector depth");}}
    check(caption_order>selector_order,"selected text submitted in front of gear selector");
    scene.set_selection(1);draw(scene,b);
    check(colour(b,40)&&colour(b,60)&&!colour(b,41),"selection change swaps identifier visibility including children");
    check(!colour(b,70)&&colour(b,71),"selection change swaps static caption");
    check(std::fabs(colour(b,99)->x-selector0.x)>1,"selector follows selected model origin");
    scene.set_interaction(false);const auto phase=scene.motion_state().selector_phase;scene.advance_motion();draw(scene,b);
    check(colour(b,40)&&colour(b,41)&&colour(b,70)&&colour(b,71),"raw inspection does not hide identifiers/captions");
    check(scene.motion_state().selector_phase==phase,"diagnostic scene does not tick idle controller");
    source.menu_motion.bindings[0].item=10;check(!scene.load_scene(source,&error),"invalid owned motion binding refuses safely");
    std::printf("{\"menu_motion_checks\":%u,\"all_passed\":true}\n",checks);
}
