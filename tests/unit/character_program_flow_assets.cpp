#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/assets/character_program.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"program flow assets: %s\n",why);std::abort();}}std::vector<gf::u8> read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}}
int main(int argc,char** argv){using namespace gf;std::string error;CharacterProgramAsset p;check(!character_program_load_control_flow({},0,&p,&error)&&!p.loaded,"empty flow asset refuses");
    if(argc==2){const std::filesystem::path root=argv[1],r=root/"local/recomp-evidence/native-force-editor-20261111";for(const char* name:{"g-red","sasuke"}){auto file=read(r/"program-flow-inputs-v1"/name/"control-flow.gfmp"),model=read(r/"spec-motion-inputs-v2"/name/"model.hsd");const auto hash=character_program_model_hash({model.data(),model.size()});check(character_program_load_control_flow({file.data(),file.size()},hash,&p,&error)&&p.control_flow&&p.programs.size()==2&&character_program_detail::valid_control_flow(p),"actual recovered reachable source program validates structurally");const auto action=std::find_if(p.programs.begin(),p.programs.end(),[](const auto& value){return value.group==0&&value.choice==40;});check(action!=p.programs.end(),"explicit action identity");auto raw=read(r/"spec-demo-paths-v1"/(std::string(name)+"-source-span.bin"));check(action->bytes==raw,"all raw bytes including signed branch/gap preserved");check(!character_program_load({file.data(),file.size()},hash,&p,&error)&&!character_program_load_transitions({file.data(),file.size()},hash,&p,&error),"old readers reject GFMP3");auto truncated=file;truncated.pop_back();check(!character_program_load_control_flow({truncated.data(),truncated.size()},hash,&p,&error)&&!p.loaded,"truncated source program refuses");}}
    else check(argc==1,"arguments");
    std::printf("GFMP3 actual-program structural assets: %u checks PASS; no actor execution\n",checks);return 0;}
