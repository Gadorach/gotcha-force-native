#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_landing_fixture.hpp"
#include "engine/core/headless_backends.hpp"
namespace {
unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"startup landing: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
bool close(float a,float b){return std::abs(a-b)<1e-6f;}
}
int main(){
    using namespace gf;using namespace startup_landing_fixture;std::string error;auto raw=startup_static_graphics_fixture::archive();HsdRecordAnimationBank bank;check(bank.load({raw.data(),raw.size()},&error),"owned complete bank");auto p=profile(bank.hash());std::array<StihGrid,3> g;check(grids(&g,&error),"owned floor and secondary grids");const std::array<const StihGrid*,3> view={&g[0],&g[1],&g[2]};
    StartupLandingState state,next;auto input=request();check(startup_landing_construct(p,view,input,&state,&error)&&state.phase==1&&state.visible&&state.registered&&state.position[1]==0&&state.scale[0]==1&&state.alpha==1,"kind2 first callback initializes, no advance");
    for(unsigned tick=1;tick<=25;++tick){check(startup_landing_step(p,1,state,&next,&error)&&next.allocated&&next.registered&&next.visible&&next.time==tick&&close(next.scale[0],1+float(tick)/100)&&close(next.alpha,1-float(tick)/25),"curve update including final visible alpha0 transition");state=next;}
    check(state.phase==2&&startup_landing_step(p,1,state,&next,&error)&&next.phase==3&&!next.visible&&next.registered,"hidden phase3 retains callback registration");state=next;
    check(startup_landing_step(p,1,state,&next,&error)&&!next.allocated&&!next.registered,"release unregisters next callback");
    input=request(0);input.yaw5ae=16384;check(startup_landing_construct(p,view,input,&state,&error)&&state.yaw==16384,"kind0 uses supplied parent5AE, not actor body angle inference");
    for(unsigned tick=1;tick<=10;++tick){check(startup_landing_step(p,1,state,&next,&error),"kind0 first phase");state=next;}
    check(state.entry==1&&state.time==0&&close(state.base_scale[0],1.225f)&&close(state.scale[0],1.225f),"segment boundary retains previous callback scale9, no terminal-frame10 resample/fitted1.25");
    for(unsigned tick=0;tick<15;++tick){check(startup_landing_step(p,1,state,&next,&error),"kind0 second phase");state=next;}
    check(state.phase==2&&close(state.scale[0],1.225f*1.25f)&&state.alpha==0,"second segment grows prior scale then reaches zero alpha naturally");
    input=request();input.anchor[1]=40;check(startup_landing_construct(p,view,input,&state,&error)&&state.phase==2&&!state.registered&&!state.visible,"distance equal height*.5 rejects without fake ground/model");input.anchor[1]=39.99f;check(startup_landing_construct(p,view,input,&state,&error)&&state.visible,"strict near-ground side accepted");
    input=request();input.ground_actor.attached1db=1;check(!startup_landing_construct(p,view,input,&next,&error)&&!next.allocated,"moving-platform query remains explicit refusal");
    const std::array<LinearKey<1>,4> keys={LinearKey<1>{-2,{0}},LinearKey<1>{2,{4}},LinearKey<1>{4,{0}},LinearKey<1>{6,{6}}};u16 index=0;std::array<f32,1> value={99};
    check(linear_key_sample<1>(keys,10,&index,&value)&&index==1&&value[0]==-12,"large time advances exactly one key, extrapolates next span without loop/clamp");
    check(linear_key_sample<1>(keys,-2,&index,&value)&&index==1&&value[0]==12,"decreasing signed time does not rewind index");
    check(linear_key_sample<1>({},0,&index,&value)&&value[0]==12,"null advance leaves prior output; initializer uses sourceone separately");
    StartupLandingScene scene;check(scene.load(bank,p,g,StartupLandingMode::DeclaredSourceGroundAndAnchorSnapshots,&error),"owned scene preflight all three records");headless::Render backend;RenderEngine render(backend);check(render.initialize({640,480,false,true,"owned landing"},nullptr,&error),"renderer");Camera camera;camera.eye[1]=10;camera.eye[2]=10;camera.near_plane=.1f;camera.far_plane=100;
    input=request();check(scene.step(1,{&input,1},1,0,&error)&&scene.instances()==1&&scene.state(0)->time==0,"scene creation callback does not advance newly added instance");scene.upload_textures(render);
    render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)>0,"complete original-record path emits owned geometry through shared renderer");render.end_frame();
    check(scene.step(2,{},1,1,&error)&&scene.draw(render,camera)==0&&scene.state(0)->time==1,"display suppression does not freeze source lifetime");
    auto moved=std::move(scene);check(!scene.ready()&&moved.ready()&&moved.step(3,{},1,0,&error),"owned instance/resource move stable");
    const auto accepted=moved.frame();auto bad=input;bad.factor=0;check(!moved.step(4,{&bad,1},1,0,&error)&&!moved.ready()&&moved.frame()==accepted&&moved.draw(render,camera)==0,"late request failure discards staged clocks and all drawings");
    check(scene.load(bank,p,g,StartupLandingMode::DeclaredSourceGroundAndAnchorSnapshots,&error)&&scene.step(1,{&input,1},1,0,&error)&&!scene.step(3,{},1,0,&error),"skipped frame latches refusal");
    std::array<StartupLandingRequest,8> many;many.fill(input);check(scene.load(bank,p,g,StartupLandingMode::DeclaredSourceGroundAndAnchorSnapshots,&error)&&scene.step(1,many,1,0,&error)&&!scene.step(2,{&input,1},1,0,&error),"capacity bound refuses, never silently drops unknown suffix");
    auto zero=p;zero.entries[2].scale[0].value={0,0,0};check(scene.load(bank,zero,g,StartupLandingMode::DeclaredSourceGroundAndAnchorSnapshots,&error)&&!scene.step(1,{&input,1},1,0,&error)&&scene.frame()==0&&scene.draw(render,camera)==0,"late singular root publication refuses and clears all drawings without weakening shared guard");
    check(!scene.load(bank,p,g,static_cast<StartupLandingMode>(99),&error),"unknown snapshot scope refuses");auto wrong=p;wrong.model_hash^=1;check(!scene.load(bank,wrong,g,StartupLandingMode::DeclaredSourceGroundAndAnchorSnapshots,&error),"source bank mismatch refuses");
    input=request();check(startup_landing_construct(p,view,input,&state,&error),"malformed state controls setup");next=state;next.duration=0;check(!startup_landing_step(p,1,next,&state,&error),"forged duration cannot bypass source segment lifetime");
    auto bytes=read("local/recomp-evidence/native-startup-landings-20261110/source-contract-v1/startup.gfld");unsigned source=0;
    if(!bytes.empty()){StartupLandingProfile actual;check(startup_landing_load({bytes.data(),bytes.size()},&actual,&error),"actual source operand profile");check(actual.ground.high34==5000&&actual.ground.low68==-1540,"startup mode17 stage bounds from actual constructor tables");
        for(unsigned size=0;size<bytes.size();++size){StartupLandingProfile invalid;check(!startup_landing_load({bytes.data(),size},&invalid,&error),"truncated profile refuses");}
        auto model=read("local/recomp-evidence/native-startup-landings-20261110/inputs-v2/efct-slot1.hsd");HsdRecordAnimationBank original;check(original.load({model.data(),model.size()},&error),"actual full157-record efct archive");
        check(scene.load(original,actual,g,StartupLandingMode::DeclaredSourceGroundAndAnchorSnapshots,&error),"all actual selected records preflight without mesh/material omission");
        check(scene.step(1,{&input,1},1,0,&error)&&scene.model(0)->asset()->meshes.size()>0,"actual source5 uses same model hierarchy/material owner");source=3;
    }
    std::printf("{\"startup_landing_checks\":%u,\"actual_selected_records\":%u,\"full_startup\":false,\"all_passed\":true}\n",checks,source);
}
