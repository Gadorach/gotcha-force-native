// End-to-end view-local wall rendering; the Python runner supplies owned art.
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/gameplay/native_battle_mvp_view.hpp"

namespace {
using namespace gf;
void check(bool value,const char* message){
    if(!value){std::fprintf(stderr,"stage viewport render: %s\n",message);std::exit(1);}
}
class ReadOnlyFiles final:public IFileSystem {
public:
    bool read(const std::string& path,std::vector<u8>& bytes) override{
        u64 size=0;if(!file_size(path,&size)||size>(32u<<20))return false;
        std::ifstream in(path,std::ios::binary);
        if(!in)return false;
        bytes.assign(std::istreambuf_iterator<char>(in),{});
        return bytes.size()==size;
    }
    bool file_size(const std::string& path,u64* out) override{
        std::error_code ec;const auto size=std::filesystem::file_size(path,ec);
        if(ec||!out)return false;
        *out=size;return true;
    }
    bool exists(const std::string& path) override{
        std::error_code ec;return std::filesystem::exists(path,ec);
    }
    bool write(const std::string&,const u8*,std::size_t) override{return false;}
    bool erase(const std::string&) override{return false;}
};
class Capture final:public IRenderBackend {
public:
    unsigned normal_wall=0,faded_wall=0,normal_rock=0,faded_rock=0;u8 opacity=0,rock_opacity=0;
    unsigned ink_draws=0;u8 ink_alpha=0;
    std::vector<std::array<unsigned,9>> submissions;
    bool initialize(const RenderConfig&,IWindowBackend*,std::string*) override{return true;}
    void shutdown() override{}
    RenderCapabilities capabilities()const override{return {4096,false,false,"stage viewport capture"};}
    bool upload_vertices(const Vertex* vertex,std::size_t count) override{
        if(!count){vertices_.clear();return true;}
        vertices_.assign(vertex,vertex+count);return true;
    }
    bool upload_texture(u32 handle,const TextureDesc& desc) override{
        if(desc.width==kInkSplatterSize&&desc.height==kInkSplatterSize)ink_texture_=handle;
        return true;
    }
    bool upload_matrix(u32,const f32*) override{return true;}
    void begin_frame(const FrameDesc&) override{normal_wall=faded_wall=normal_rock=faded_rock=ink_draws=0;opacity=rock_opacity=ink_alpha=0;submissions.clear();}
    void submit(const DrawCall& call) override{
        const auto start=std::size_t(call.vertex_first),count=std::size_t(call.vertex_count);
        check(start<=vertices_.size()&&count<=vertices_.size()-start,"submitted vertex range");
        if(!count)return;
        const auto first=vertices_.begin()+start,last=first+count;
        const auto alpha=first->a;
        if(ink_texture_&&call.texture==ink_texture_){
            check(call.blend&&!call.depth_test&&!call.depth_write&&count==6,"ink is a textured UI quad only");
            ++ink_draws;ink_alpha=alpha;
        }
        submissions.push_back({unsigned(count),call.blend,call.depth_test,call.depth_write,call.texture,first->r,first->g,first->b,alpha});
        if(!call.blend&&call.depth_test&&call.depth_write&&alpha==255&&
           std::all_of(first,last,[](const auto& v){return v.r==0x7a&&v.g==0x87&&v.b==0x94&&v.a==255;}))++normal_wall;
        if(!call.blend&&call.depth_test&&call.depth_write&&alpha==255&&
           std::all_of(first,last,[](const auto& v){return v.r==0x54&&v.g==0x78&&v.b==0x60&&v.a==255;}))++normal_rock;
        if(call.blend&&call.depth_test&&!call.depth_write&&!call.texture&&alpha>0&&alpha<255&&
           std::all_of(first,last,[alpha](const auto& v){return !v.r&&!v.g&&!v.b&&v.a==alpha;})){
            ++faded_wall;opacity=alpha;
        }
        if(call.blend&&call.depth_test&&!call.depth_write&&alpha>0&&alpha<255&&
           std::all_of(first,last,[alpha](const auto& v){return v.r==0x20&&v.g==0x30&&v.b==0x29&&v.a==alpha;})){
            ++faded_rock;rock_opacity=alpha;
        }
    }
    void end_frame() override{}
    void present() override{}
    void describe()const{
        std::fprintf(stderr,"normal=%u faded=%u opacity=%u; count blend test write tex r g b a:\n",normal_wall,faded_wall,unsigned(opacity));
        for(const auto& row:submissions){for(auto value:row)std::fprintf(stderr,"%u ",value);std::fputc('\n',stderr);}
    }
private:
    std::vector<Vertex> vertices_;
    u32 ink_texture_=0;
};
}
int main(int argc,char** argv){
    check(argc==2,"generated stage root argument required");
    const std::string root=argv[1];ReadOnlyFiles files;Capture backend;RenderEngine render(backend);
    std::string error;
    check(render.initialize({640,480,false,true,"stage view"},nullptr,&error),error.c_str());
    BattleMvpConfig config;
    std::array<BorgBattleVisualAssets,2> assets{};
    std::array<std::array<std::vector<u8>,4>,2> bytes;
    for(unsigned i=0;i<2;++i){
        const auto base=root+"/borgs/"+(i==0?"flight-stick/":"jump-stick/");
        std::vector<u8> declaration;
        check(files.read(base+"gameplay.txt",declaration),"package declaration");
        auto profile=std::make_shared<BorgGameplayProfile>();
        const bool parsed=borg_read_gameplay_profile({reinterpret_cast<const char*>(declaration.data()),declaration.size()},profile.get(),&error);
        check(parsed,error.c_str());config.packages[i]=profile;
        constexpr std::array names{"body.hsd","body.prefix","body.pose","body.motions"};
        const auto view=[&](unsigned index){return ByteView{bytes[i][index].data(),bytes[i][index].size()};};
        for(unsigned j=0;j<names.size();++j)check(files.read(base+names[j],bytes[i][j]),"owned rig asset");
        assets[i]={view(0),view(1),view(2),view(3)};
    }
    NativeBattleMvp world;NativeBattleMvpView scene;
    const bool loaded=scene.load_packages(files,root+"/arena-wall",render,config.packages,assets,&error);
    check(loaded,error.c_str());
    check(world.start(config,&scene),"battle placement");
    check(scene.update(world,&error),"first pose publication");
    Camera blocked,clear;
    blocked.eye[1]=140;blocked.eye[2]=680;blocked.target[1]=120;blocked.target[2]=-260;
    // Steep enough that the wall is visible behind the target, not between
    // the eye and target. This lets the clear draw prove the *normal* record.
    clear.eye[1]=340;clear.eye[2]=680;clear.target[1]=120;clear.target[2]=240;
    clear.eye[0]=clear.target[0]=230;
    blocked.fov_y_degrees=80;
    blocked.far_plane=clear.far_plane=3000;
    const BattleMvpVec shot_start{0,120,400},shot_end{0,120,-400};
    NativeBorgStageProjectileHit before;
    check(scene.projectile_stage(shot_start,shot_end,&before)&&before.hit&&before.layer==2&&std::abs(before.fraction-.5f)<.001f,
          "wall collision before recording either viewport");
    const auto draw=[&](unsigned viewer,const Camera& camera){
        render.begin_frame({world.tick(),0,0,0});render.reset_arena();
        const auto count=scene.draw(render,world,640,480,&error,false,viewer,&camera);
        render.end_frame();
        check(count!=0,error.c_str());
    };
    draw(0,blocked);
    if(backend.faded_wall!=1||backend.opacity!=127)backend.describe();
    check(backend.faded_wall==1&&!backend.normal_wall&&backend.opacity==127&&scene.stage_view_alpha(0)==.5f,
          "blocked viewport submits black untextured blended wall");
    check(backend.normal_rock==1&&!backend.faded_rock&&scene.stage_view_alpha(0,4)==1.f,
          "same viewport keeps a second obstacle opaque off its camera line");
    draw(1,clear);
    if(backend.normal_wall!=1)backend.describe();
    check(backend.normal_wall==1&&!backend.faded_wall&&scene.stage_view_alpha(1)==1.f,
          "clear viewport submits only original opaque wall");
    check(backend.faded_rock==1&&!backend.normal_rock&&backend.rock_opacity==153&&scene.stage_view_alpha(1,4)==.6f,
          "second camera fades only the second obstacle");
    draw(0,blocked);
    check(backend.faded_wall==1&&!backend.normal_wall&&backend.opacity==127&&scene.stage_view_alpha(1)==1.f&&
          backend.normal_rock==1&&!backend.faded_rock&&scene.stage_view_alpha(1,4)==.6f,
          "repeated same-tick draw neither advances nor leaks opacity");
    check(world.step({},false,&scene)&&scene.update(world,&error),"next simulation tick");
    draw(0,blocked);
    check(backend.faded_wall==1&&backend.opacity<127,"next tick advances only the blocked view");
    NativeBorgStageProjectileHit after;
    check(scene.projectile_stage(shot_start,shot_end,&after)&&after.hit==before.hit&&after.layer==before.layer&&
          after.fraction==before.fraction&&after.triangle==before.triangle,"draws do not change collision geometry");
    config.spawn={BattlePoint{0,200},BattlePoint{0,-30}};
    scene.reset_control_test_pose();check(world.start(config,&scene)&&scene.update(world,&error),"ink fixture restart");
    for(unsigned i=0;i<55&&!world.ink_overlay(0);++i)
        check(world.step({},false,&scene,nullptr,nullptr,nullptr,
                         std::array{BorgBattleInputs{},BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0}})&&
              scene.update(world,&error),"run bomb to accepted ink contact");
    check(world.ink_overlay(0)&&!world.ink_overlay(1)&&world.actors()[0].package_target_visible,
          "actual opponent is inked without a vision or accuracy penalty");
    draw(0,blocked);check(backend.ink_draws==1&&backend.ink_alpha==225,"only the recipient gets the full splatter");
    draw(1,clear);check(!backend.ink_draws,"second viewport does not draw the ink");
    draw(0,blocked);check(backend.ink_draws==1&&backend.ink_alpha==225,"redrawing does not age ink");
    check(world.step({},false,&scene)&&scene.update(world,&error),"ink advances with simulation");
    draw(0,blocked);check(backend.ink_draws==1&&backend.ink_alpha<225,"alpha falls on the next simulation visit");
    for(unsigned i=0;i<24;++i)check(world.step({},false,&scene)&&scene.update(world,&error),"ink expiry");
    draw(0,blocked);check(!world.ink_overlay(0)&&!backend.ink_draws,"expired ink leaves no overlay");
    render.shutdown();
    std::puts("stage viewport render: opacity, geometry, and collision isolation PASS");
}
