#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/character_program_fixture.hpp"
#include "engine/assets/character_motion.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"character program: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    auto bytes=character_program_fixture::bytes();gf::CharacterProgramAsset asset;std::string error;
    auto load=[&](const auto& b){return gf::character_program_load({b.data(),b.size()},0x12345678,&asset,&error);};
    check(load(bytes)&&asset.programs.size()==1&&asset.descriptors.size()==2,"owned strict GFMP tables/commands load");
    for(std::size_t size=0;size<bytes.size();++size)check(!gf::character_program_load({bytes.data(),size},0x12345678,&asset,&error)&&!asset.loaded,"all truncated profile views refuse atomically");
    for(unsigned offset:{0u,4u,6u,7u,8u,12u,16u,20u,24u,28u,34u,36u,40u,46u,92u,95u,96u,97u}){
        auto bad=bytes;bad[offset]=255;check(!load(bad)&&!asset.loaded&&asset.programs.empty(),"malformed/unsupported header, mode, opcode, tables refuse");
    }
    auto bad=bytes;bad.back()=0;bad[bad.size()-3]=0;check(!load(bad),"missing terminal wait refuses");
    check(load(bytes),"restore profile");
    gf::CharacterProgramVm vm;gf::CharacterProgramActor actor;actor.default_group=5;actor.auxiliary=255;actor.behaviour=0xabcdef01;
    character_program_fixture::Clock driver;std::vector<gf::CharacterProgramOperation> operations;
    check(vm.load(asset,actor,{},&error)&&vm.request(0,0,3,-1,1,driver,&operations,&error),"request resolves source default group and drives prefix");
    check(vm.state().actor.auxiliary==64&&vm.state().actor.control0[0]==2&&vm.state().actor.control1[0]==1&&vm.state().actor.behaviour==((actor.behaviour&~0xff0u)|0x230u),"lane-gated prefix state/mask replace/fresh lane1 auxiliary clear");
    check(vm.state().lanes[0].clock==0&&vm.state().actor.models==std::array<gf::u8,2>{2,3},"descriptor model defaults and initial clock without advance");
    for(unsigned time=1;time<=2;++time){check(vm.update(3,1,driver,&operations,&error),"old wait advances supplied motion");check(vm.state().actor.control0[0]==2&&vm.state().lanes[0].motion.frame==time&&vm.state().lanes[0].offset==0,"wait tests precede advancement; saw-clip isn't rewound");}
    check(vm.update(3,1,driver,&operations,&error)&&vm.state().actor.control0[0]==3&&vm.state().actor.default_group==4&&vm.state().lanes[0].motion.frame==3,"continuation without opcode1 advances immediately before next wait");
    check(vm.update(3,1,driver,&operations,&error)&&vm.state().lanes[0].motion.finished==1,"nonterminal end wait advances until driver finished");
    check(vm.update(3,1,driver,&operations,&error)&&vm.state().lanes[0].descriptor_choice==1&&vm.state().lanes[0].offset==4&&vm.state().lanes[0].clock==4&&vm.state().lanes[0].motion.frame==0,"second opcode1 adds prior end and selects instant clip without extra tick");
    check(vm.state().lanes[1].metadata[4]==26,"nonnegative event-script index retained as metadata, not silently implemented");
    check(vm.update(3,1,driver,&operations,&error)&&vm.update(3,1,driver,&operations,&error),"negative frame wait uses end-relative threshold");
    check(vm.update(3,1,driver,&operations,&error)&&vm.state().actor.auxiliary==0&&vm.state().lanes[0].motion.frame==3&&!vm.state().lanes[0].done,"model/auxiliary continuation outputs and immediate no-clip advance");
    check(vm.update(3,1,driver,&operations,&error)&&vm.state().lanes[0].done&&vm.state().lanes[1].done&&operations.empty(),"terminal wait stops without advance or continuation replay");
    check(vm.update(3,1,driver,&operations,&error)&&operations.empty(),"completed mode0 remains stopped");
    check(vm.load(asset,actor,{},&error)&&vm.request(5,0,1,-1,1,driver,&operations,&error)&&vm.state().actor.auxiliary==255&&vm.state().actor.control1[0]==0,"lane0-only request does not reset/mutate lane1 actor fields");
    check(vm.request(5,0,2,2.5f,1,driver,&operations,&error)&&vm.state().lanes[1].motion.frame==2.5f&&vm.state().lanes[0].motion.frame==0,"explicit fractional start and independent lane requests");
    check(vm.update(1,.5f,driver,&operations,&error)&&vm.state().lanes[0].clock==.5f&&vm.state().lanes[1].clock==2.5f,"masked update keeps independent fractional clocks");
    driver.rewind=true;check(vm.update(1,.5f,driver,&operations,&error)&&vm.state().lanes[0].offset==4&&vm.state().lanes[0].clock==5,"supplied rewound observation adds clip end (not saw-clip flag)");driver.rewind=false;
    const auto saved=vm.state().lanes[0].clock;driver.refuse_select_lane=1;
    check(!vm.request(5,0,3,0,1,driver,&operations,&error)&&!vm.ready()&&operations.empty()&&vm.state().lanes[0].clock==saved,"failed later lane select publishes no partial state/operation journal");
    check(!vm.update(3,1,driver,&operations,&error),"failed request cannot replay old frame/update implicitly");driver.refuse_select_lane=-1;
    check(vm.request(5,0,3,30,1,driver,&operations,&error)&&vm.state().lanes[0].clock==30&&vm.state().lanes[0].motion.frame==30,"beyond-end source request30 retained, not clamped to4");
    check(!vm.request(0x80,0,3,0,1,driver,&operations,&error)&&operations.empty(),"unported common program bank refuses explicitly");
    check(vm.request(5,0,3,0,1,driver,&operations,&error),"explicit request restores readiness");driver.refuse_advance_lane=1;
    check(!vm.update(3,1,driver,&operations,&error)&&!vm.ready()&&operations.empty()&&vm.state().lanes[0].clock==0,"failed advance atomically retains old state but invalidates readiness");driver.refuse_advance_lane=-1;
    bytes.clear();asset={};check(vm.request(5,0,3,0,1,driver,&operations,&error),"VM owns immutable profile beyond input/parsed asset lifetime");
    bytes=character_program_fixture::bytes();check(load(bytes),"restore profile for distinct wait modes");auto tiny=asset;
    tiny.programs[0].bytes={0,1,255,255};std::array<gf::CharacterProgramMotion,2> initial;initial[0]={true,0,1,4,0,0,{}};
    driver.rewind=true;actor.models={0,0};check(vm.load(tiny,actor,initial,&error)&&vm.request(5,0,1,-1,0,driver,&operations,&error),"owned no-clip prefix preserves supplied motion and advances with explicit zero delta");
    check(vm.update(1,0,driver,&operations,&error)&&vm.state().lanes[0].done&&operations.empty(),"terminal negative wait accepts rewound before advance even without finished");
    tiny.programs[0].bytes={0,0,255,255,7,0,1,1,0,1,255,255};
    check(vm.load(tiny,actor,initial,&error)&&vm.request(5,0,1,-1,0,driver,&operations,&error)&&vm.update(1,0,driver,&operations,&error)&&vm.state().actor.models==std::array<gf::u8,2>{0,0}&&!vm.state().lanes[0].done,"nonterminal negative wait does NOT accept rewound alone or execute following attachment command");driver.rewind=false;
    tiny.programs[0].bytes={0,0,0,2,0,1,255,255};initial[0]={true,0,100,500,0,0,{}};
    check(vm.load(tiny,actor,initial,&error)&&vm.request(5,0,1,-1,0,driver,&operations,&error)&&vm.update(1,1,driver,&operations,&error)&&vm.state().lanes[0].wait==0,"ordinary positive wait reads elapsed, not far-ahead animation frame");
    check(vm.update(1,1,driver,&operations,&error)&&vm.state().lanes[0].wait==0&&vm.update(1,1,driver,&operations,&error)&&vm.state().lanes[0].wait==4,"elapsed threshold continuation fires on the next pre-advance test");
    bool mutations=true;unsigned seed=0x71ab01;
    for(unsigned n=0;n<512;++n){auto changed=bytes;seed=seed*1664525u+1013904223u;const auto at=seed%changed.size();seed=seed*1664525u+1013904223u;changed[at]^=gf::u8((seed>>24)|1);
        const bool ok=load(changed);mutations=mutations&&(ok||(asset.programs.empty()&&asset.descriptors.empty()&&!asset.loaded));}
    check(mutations,"512 deterministic malformed GFMP probes preserve atomic refusal");
    unsigned real=0,real_requests=0,updates=0;
    for(const char* name:{"g-red","sasuke"}){
        auto encoded=read(std::string("local/content/character-program-bindings/")+name+".gfmp"),model=read(std::string("local/content/startup-characters/")+name+"/model.hsd");if(encoded.empty()||model.empty())continue;
        check(gf::character_program_load({encoded.data(),encoded.size()},gf::character_program_model_hash({model.data(),model.size()}),&asset,&error),"actual source-bound GFMP profile loads without DOL/AFS/PZZ");
        check(asset.programs.size()==6&&asset.descriptors.size()==7&&asset.programs.back().bytes.size()==(std::string(name)=="g-red"?76:88),"complete imported programs/descriptors retain both request7 continuations");
        auto prefix=read(std::string("local/content/startup-characters/")+name+"/model-prefix.bin");gf::HsdModelBones bones;
        check(gf::hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bones,&error),"real model binding for source clip end observations");
        for(unsigned choice:{0,1,3,4,7,8,9}){auto clip_bytes=read(std::string("local/content/startup-motion-programs/")+name+"/motions/motion-0"+std::to_string(choice)+".hsd");gf::HsdBoneMotionClip clip;
            check(gf::hsd_bone_motion_load({clip_bytes.data(),clip_bytes.size()},bones,&clip,&error),"source clip end loaded as test input, not a source playback policy");driver.ends[choice]=gf::s16(clip.end_frame);
        }
        for(unsigned choice:{0,1,3,4,6,7}){
            actor.default_group=5;check(vm.load(asset,actor,{},&error)&&vm.request(5,gf::u8(choice),3,-1,1,driver,&operations,&error),"actual program runs under explicit owned-clock test driver");
            if(choice==3)check(vm.state().lanes[0].motion.frame==30,"actual request3 retains time30 independently of source clip end");
            bool done=false;for(unsigned tick=0;tick<512;++tick){check(vm.update(3,1,driver,&operations,&error),"actual program bounded wait/prefix update");++updates;
                if(vm.state().lanes[0].done&&vm.state().lanes[1].done){done=true;break;}}
            check(done,"source program reaches terminal waits under supplied test observations");
            if(choice==7)check(vm.state().lanes[0].descriptor_choice==8&&vm.state().lanes[1].descriptor_choice==8,"both request7 lanes traverse second descriptor8");
            ++real_requests;
        }
        ++real;
    }
    std::printf("{\"character_program_checks\":%u,\"source_profiles\":%u,\"source_requests\":%u,\"owned_clock_updates\":%u,\"original_motion_driver_actor_effects_startup_admitted\":false,\"all_passed\":true}\n",checks,real,real_requests,updates);
}
