#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_preview_height_fixture.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"height owner boundary: %s\n",why);std::abort();}}}
int main(){using namespace gf;using namespace character_preview_height_fixture;std::string error;CharacterPreviewOutput output;
    for(bool branch:{false,true}){Fixture f;check(f.load(&error),"fixture");const std::array<u8,4> command=branch?std::array<u8,4>{6,0,255,254}:std::array<u8,4>{7,0,5,6};f.program.insert(f.program.begin()+80,command.begin(),command.end());store_be32(f.program.data()+16,f.program.size());store_be32(f.program.data()+40,f.program.size()-68);CharacterPreviewFrame actor;
        check(actor.load_with_actor_height(f.assets(),preview(),CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements,0,height(),&output,&error)&&actor.step({},&output,&error),"owned height program accepts before unsupported continuation");const auto* accepted=actor.state();const auto root=accepted->world.matrices[0];const auto vm=accepted->vm.state();
        check(!actor.step({},&output,&error)&&!actor.ready()&&output.motion.empty()&&output.frame==0&&accepted->frames==2&&accepted->vm.state().actor.models==vm.actor.models&&accepted->vm.state().actor.behaviour==vm.actor.behaviour&&std::equal(root.m,root.m+16,accepted->world.matrices[0].m),branch?"unconsumed actor branch pulse refuses whole candidate":"unconsumed model-selection change refuses whole candidate");
    }
    Fixture f;check(f.load(&error),"ordinary fresh owner");CharacterPreviewFrame actor;check(actor.load_with_actor_height(f.assets(),preview(),CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements,0,height(),&output,&error)&&actor.state()->scalars.field708==0&&output.auxiliary_requirements.size()==2,"height owner does not forge C74EC or sound/aux acknowledgement");
    std::printf("preview height actor-boundary guards: %u checks PASS\n",checks);return 0;}
