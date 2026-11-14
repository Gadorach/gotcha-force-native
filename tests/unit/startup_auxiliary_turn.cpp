#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/startup_auxiliary_turn.hpp"
#include "engine/gameplay/startup_auxiliary_scene.hpp"
#include "tests/unit/startup_auxiliary_fixture.hpp"
#include "engine/core/headless_backends.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"auxiliary turning: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
std::vector<gf::u8> profile(){std::vector<gf::u8> b(64);gf::store_be32(b.data(),0x47465455);gf::store_be16(b.data()+4,1);gf::store_be16(b.data()+6,12);gf::store_be32(b.data()+8,64);const gf::u32 words[]={0x3c23d70a,0x40490fdb,0x47000000,0x3d0efa35,0x3c8efa35,0x2edbe6fe,0xbf800000,0x3f800000,0,0x3f800000,0,0};for(unsigned i=0;i<12;++i)gf::store_be32(b.data()+16+i*4,words[i]);return b;}
bool close(float a,float b){return std::abs(a-b)<1e-5f;}
}
int main(){
    using namespace gf;using namespace startup_auxiliary_turn_detail;std::string error;auto raw=profile();StartupAuxiliaryTurnProfile turn_profile;check(startup_auxiliary_turn_load({raw.data(),raw.size()},&turn_profile,&error),"owned exact source operands");
    for(unsigned at=0;at<raw.size();++at){auto bad=raw;bad[at]^=0x80;StartupAuxiliaryTurnProfile rejected;check(!startup_auxiliary_turn_load({bad.data(),bad.size()},&rejected,&error)&&!rejected.loaded,"all changed header/operand bytes refuse unreviewed values");}
    Vec v;check(turn(turn_profile,{1,0,0},{0,0,1},turn_profile.pi/2,&v)&&close(v[0],0)&&close(v[2],1),"source cross orientation rotates+X toward+Z");
    check(turn(turn_profile,{1,0,0},{0,0,1},turn_profile.history_step,&v)&&close(v[0],std::cos(turn_profile.history_step))&&close(v[2],std::sin(turn_profile.history_step)),"angle cap is source per-callback2degrees, not elapsed-time interpolation");
    check(turn(turn_profile,{0,1,0},{0,-1,0},turn_profile.pi,&v)&&close(v[1],1),"antiparallel source fallbackY can leaveYunchanged; no smarter invented axis");
    f32 angle=0;Vec axis;check(angle_axis(turn_profile,{1,0,0},{1,1e-6f,0},1,&angle,&axis)&&axis==turn_profile.fallback,"near-parallel cross<=source epsilon uses literal source fallback");
    check(!turn(turn_profile,{0,0,0},{1,0,0},1,&v),"zero source-vector length refuses instead of NaN repair");
    Mat4 normalized;normalized.at(0,1)=1/std::sqrt(10.f);normalized.at(1,1)=3/std::sqrt(10.f);normalized.at(1,2)=1/std::sqrt(17.f);normalized.at(2,2)=4/std::sqrt(17.f);
    check(inverse_direction(normalized,{0,0,1},&v)&&close(v[0],1.f/12)&&close(v[1],-std::sqrt(10.f)/12)&&close(v[2],std::sqrt(17.f)/4),"full normalized-basis inverse is not transpose under shear");
    normalized.at(0,0)=0;check(!inverse_direction(normalized,{1,0,0},&v),"singular normalized basis refuses, no fake inverse");
    Mat4 aligned,input;input.at(0,3)=12;input.at(1,3)=23;input.at(2,3)=34;
    check(align_y(turn_profile,input,{1,0,0},turn_profile.pi/2,&aligned)&&close(aligned.at(0,1),1)&&close(aligned.at(1,1),0)&&aligned.at(0,3)==12&&aligned.at(1,3)==23&&aligned.at(2,3)==34,"positive-Y post alignment preserves source translation");
    auto model=startup_auxiliary_fixture::model(),base_raw=startup_auxiliary_fixture::profile(model);StartupAuxiliaryProfile base;check(startup_auxiliary_load({base_raw.data(),base_raw.size()},&base,&error),"owned base effect profile");
    std::array<Mat4,4> bones;bones[3].at(0,3)=10;bones[3].at(1,3)=20;bones[3].at(2,3)=30;StartupAuxiliaryParent parent;parent.model94=0x615;parent.active_count=2;parent.bones=bones;StartupAuxiliaryConstruction made;check(startup_auxiliary_construct(base,0,parent,0,true,0,&made,&error),"owned effect40 creation plan");
    StartupAuxiliaryMotionObservation motion;StartupAuxiliaryState state,next;
    check(!startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,made.state,&next,&error)&&!next.allocated,"active turn needs explicit actor20 AND source2C observations, not clip/root origin");motion.source_positions_declared=true;
    check(startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,made.state,&state,&error)&&state.matrices_valid&&state.counter==0&&state.cached_a8==0,"explicit stationary source positions run turning callback; init does not fake live-step increment");
    check(!startup_auxiliary_step(base,0,parent,made.state,&next,&error),"legacy entry retains active40 refusal; opt-in turning profile is necessary");
    motion.current20={0,0,10};check(startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,state,&next,&error)&&next.cached_a8==2&&next.previous_a9==0,"movement path runs before old-history path becomes available");state=next;
    motion.previous2c=motion.current20;motion.current20={10,0,10};check(startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,state,&next,&error)&&next.previous_a9==2&&next.matrices_valid,"previous normalized worldY captured by state prefix feeds bounded history turn");state=next;
    parent.active_count=0;motion.source_positions_declared=false;check(startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,state,&next,&error)&&next.cached_a8==0&&next.previous_a9==2,"inactive callback does not read unavailable movement/history inputs");state=next;
    parent.active_count=2;motion.source_positions_declared=true;check(startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,state,&next,&error)&&next.previous_a9==0,"reactivation clears retained-history gate");state=next;
    parent.phase18=2;check(startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,state,&next,&error)&&next.phase==2&&next.render_group==0&&next.matrices_valid,"hidden retirement still runs registered turning callback");state=next;bones[3].at(0,3)=100;
    check(startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,state,&next,&error)&&next.phase==3&&next.matrices[0].at(0,3)==102,"hidden phase3 still follows parent root while callback remains registered");state=next;parent.bones={};motion.source_positions_declared=false;
    check(startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,state,&next,&error)&&!next.allocated,"release unregisters before transform; missing positions/bones are not read");
    parent.phase18=1;parent.bones=bones;bones[3]=Mat4{};bones[3].at(0,0)=0;motion.source_positions_declared=true;
    check(!startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,made.state,&next,&error)&&!next.allocated&&made.state.phase==0,"late inverse-domain failure discards staged state/matrices without mutating input");
    const unsigned owned=checks;auto source_model=read("local/content/startup-auxiliary/efct-51.hsd"),source_base=read("local/content/startup-auxiliary/startup.gfau"),source_turn=read("local/content/startup-auxiliary-turn/turn.gftu");unsigned frames=0,draws=0;
    if(!source_model.empty()){check(startup_auxiliary_load({source_base.data(),source_base.size()},&base,&error)&&startup_auxiliary_turn_load({source_turn.data(),source_turn.size()},&turn_profile,&error),"original source base/turn operands");StartupAuxiliaryModelScene scene;check(scene.load({source_model.data(),source_model.size()},base,&error),"source-selected complete model51 resource");
        headless::Render backend;RenderEngine render(backend);render.initialize({640,480,false,true,"source turning subset"},nullptr,&error);scene.upload_textures(render);Camera camera;camera.eye[2]=1000;camera.target[2]=0;camera.far_plane=4000;camera.fov_y_degrees=100;bones[3]=Mat4{};parent.bones=bones;parent.active_count=2;parent.phase18=1;check(startup_auxiliary_construct(base,0,parent,0,true,0,&made,&error),"source40 creation plan");state=made.state;motion={};
        for(unsigned frame=0;frame<24;++frame){motion.source_positions_declared=true;motion.previous2c=motion.current20;motion.current20={float(frame)*.25f,float(frame%3),-float(frame)*.5f};check(startup_auxiliary_step_with_turning(base,turn_profile,0,parent,motion,state,&next,&error),"source40 with DECLARED positions and static supplied bone");state=next;
            for(unsigned n=0;n<2;++n){check(scene.set_root(state.matrices[n],&error),"complete source hierarchy consumes turning root");render.begin_frame({});render.reset_arena();const auto count=scene.draw(render,camera);render.end_frame();check(count==5,"source40 all five meshes per declared instance, no guessed mesh repair");draws+=count;}frames++;
        }
    }
    std::printf("{\"startup_auxiliary_turn_checks\":%u,\"owned_checks\":%u,\"declared_source_frames\":%u,\"source_mesh_draws\":%u,\"actual_positions_parent708_sound_or_visible_startup\":false,\"all_passed\":true}\n",checks,owned,frames,draws);
}
