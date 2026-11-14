#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/startup_graphics_scene.hpp"
#include "engine/gameplay/frame_digest.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"graphic join: %s\n",why);std::abort();}}
class Backend final:public gf::IRenderBackend {
public:
    unsigned uploads=0;std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{++uploads;return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
gf::HsdScene menu_fixture(){
    using namespace gf;HsdScene s;s.menu_selection=5;s.menu_motion.present=true;s.menu_motion.angle_unit=bits_to_f32(0x38c90fdb);s.menu_motion.selector_step=128;s.menu_motion.identifier_step=64;
    auto root=[&](unsigned id,unsigned item,bool xz){HsdJoint j;j.archive_offset=id;j.translation[0]=float(int(item)-3);s.joints.push_back(j);HsdMesh mesh;mesh.cull_backface=false;mesh.render_mode=1;mesh.dobj_offset=id+16;const float points[3][3]={{-1,-1,0},{0,1,0},{1,-1,0}};for(const auto& p:points){HsdVertex v;v.position[0]=p[0];v.position[xz?2:1]=p[1];v.normal[2]=1;mesh.vertices.push_back(v);}mesh.indices={0,1,2};s.meshes.push_back(mesh);s.mesh_joint.push_back(unsigned(s.joints.size()-1));};
    for(unsigned i=0;i<7;++i){const u32 base=0x100+i*0x100;s.menu_motion.bindings.push_back({i,base,base+0x40,base+0x80,0});root(base,i,false);root(base+0x40,i,false);root(base+0x80,i,true);}
    s.menu_motion.selector_joint=0x1000;root(0x1000,5,false);root(0x1100,3,false);return s;
}
}
int main(){
    using namespace gf;std::string error;Backend backend;RenderEngine render(backend);check(render.initialize({640,480,false,true,"owned graphics join"},nullptr,&error),"render init");
    const std::array<u8,4> red={255,0,0,255},blue={0,0,255,255};TextureDesc texture;texture.width=texture.height=1;texture.pixels=red.data();texture.linear=false;
    check(render.upload_texture_shared(7,texture)&&render.upload_texture_shared(7,texture)&&backend.uploads==1&&render.shared_texture_count()==1,"immutable bank image reused without duplicate GPU upload");
    auto other=texture;other.pixels=blue.data();check(!render.upload_texture_shared(7,other)&&render.shared_texture_upload_failed()&&backend.uploads==1,"equal native handle cannot silently alias different pixels");render.clear_shared_texture_upload_error();other=texture;other.wrap_s=0;check(!render.upload_texture_shared(7,other)&&backend.uploads==1,"sampler is part of immutable identity");render.clear_shared_texture_upload_error();
    check(render.upload_texture(7,other)&&render.shared_texture_count()==0&&render.upload_texture_shared(7,texture)&&backend.uploads==3,"ordinary mutable upload invalidates immutable cache instead of stale acceptance");
    auto bad=texture;bad.width=0;check(!render.upload_texture_shared(8,bad),"zero-size texture guarded");render.clear_shared_texture_upload_error();
    FrameDigest original,changed;DrawCall draw;original.add_draw(draw);draw.post_rgb_multiply={127,255,255};changed.add_draw(draw);check(original.value()!=changed.value(),"source colour-class multiplication participates in draw identity");
    std::vector<StartupRegistrationItem> items;
    // TL36 then5 allocated by prepend; late2 then0; props0 then9.
    items.push_back({136,0,0x28,1,0,0x15,true,false});items.push_back({105,31,0x28,1,0,0x45,true,false});items.push_back({102,32,0x28,1,0,0x44,true,false});items.push_back({100,34,0x28,1,0,0x44,true,false});items.push_back({200,37,0x28,1,0,0x47,true,false});items.push_back({209,46,0x28,1,0,1,true,false});
    items.push_back({300,0,1,0x41,0,0x47,true,false});items.push_back({301,1,1,0x41,0,0x47,true,false});items.push_back({400,47,0x19,1,0,0x44,true,false});items.push_back({401,47,0x19,1,1,0x44,true,false});items.push_back({65537,1001,0x19,1,0,0x24,true,false});
    items.push_back({600,2000,0x28,1,0,0x47,true,true});items.push_back({601,2000,0x28,1,1,0x47,true,true});std::vector<StartupRegisteredModel> order;
    check(startup_registration_plan(items,{true,false},false,&order),"source registration plan");const std::vector<u32> expected={209,200,100,102,105,136,600,601,65537,400,401,1,301,300};check(order.size()==expected.size(),"all source objects visited once");for(unsigned n=0;n<expected.size();++n)check(order[n].model==expected[n],"prepend allocation, tail UI, per-parent groups and parent-first callbacks");
    check(startup_registration_plan(items,{false,false},true,&order)&&order.size()==8,"menu policy registers only groups28/38, not stopped actor/jet/landing groups");
    for(unsigned n=0;n<256;++n){auto invalid=items;invalid[0].render_group=u8(n);const bool supported=n==0||n==1||n==0x41||n==0x42;check(startup_registration_plan(invalid,{true,true},false,&order)==supported,"all render-family fields checked");}
    auto duplicate=items;duplicate.push_back(items[0]);check(!startup_registration_plan(duplicate,{true,true},false,&order)&&order.empty(),"duplicate model native ID clears plan");
    auto fixture=menu_fixture();MenuScene scene;check(scene.load_scene(fixture,&error),"owned source-menu-root set");Camera camera;camera.eye[2]=20;camera.fov_y_degrees=90;camera.far_plane=100;unsigned world=0;
    render.begin_frame({});render.reset_arena();check(render.begin_model_pass(1),"UI world enters ordinary source model pass");for(unsigned item=0;item<7;++item)for(unsigned part=0;part<2;++part)world+=scene.draw_menu_world_model(render,camera,item,part);check(render.end_model_pass()&&world==13,"seven gears and six unselected identifiers; no duplicate environment");check(scene.draw_menu_selector(render,camera)==1&&scene.draw_menu_caption(render,camera)==1,"selector then selected caption are separate later layers");render.end_frame();check(backend.calls.size()==15,"complete source UI art partition preserves each selected-root mesh");
    StartupGraphicsScene unloaded;StartupGraphicsAssets assets;StartupGraphicsFrame frame;std::array<GamePadState,4> pads;check(!unloaded.load(assets,static_cast<StartupGraphicsMode>(99),640,480,&error)&&!unloaded.ready(),"unknown full-join scope refuses before assets");check(!unloaded.load(assets,StartupGraphicsMode::DeclaredIsolatedConstructorsPlacementAndReadiness,0,480,&error),"invalid canvas refuses");check(!unloaded.step(pads,&frame,&error)&&!unloaded.draw(render,&error),"unloaded join cannot replay graphics");
    std::printf("{\"graphic_join_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
