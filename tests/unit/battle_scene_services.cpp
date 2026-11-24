#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_preparation_bootstrap.hpp"
namespace {unsigned checks=0;void check(bool v,const char* message){++checks;if(!v){std::fprintf(stderr,"battle scene services: %s\n",message);std::abort();}}}
int main(){
    using namespace gf;std::string error;
    SoundControlOwner closed;check(!closed.initialize_battle_controls(&error)&&!closed.snapshot().live(),"closed sound owner cannot acknowledge setup");
    SoundControlOwner no_bank0;check(no_bank0.initialize(std::array<u8,1>{1},OptionSettings{},&error),"owned bank1");const auto before=no_bank0.snapshot();
    check(!no_bank0.initialize_battle_controls(&error)&&no_bank0.ready()&&no_bank0.snapshot().revision()==before.revision()&&!no_bank0.snapshot().battle_flags(),"bank0 required, failure preserves owner");
    SoundControlOwner sound;check(sound.initialize(std::array<u8,3>{0,1,2},OptionSettings{},&error),"actual initialized normal banks");const auto old=sound.snapshot();
    check(!old.battle_flags()&&sound.initialize_battle_controls(&error),"battle flags unknown until explicit reset");const auto now=sound.snapshot();
    check(now.same_owner(old)&&now.revision()==old.revision()+1&&now.battle_flags()&&*now.battle_flags()==sound_control_owner_detail::BattleFlags{}&&!old.battle_flags(),"immutable old snapshot retained; source flags set in same owner");
    SoundControlFrameInput frame;check(now.frame(1,&frame,&error)&&frame.banks.size()==3,"actual initialized gains reach shared parameter input");
    for(const auto& bank:frame.banks)for(auto value:bank.programs)check(value==127,"source initial program gains retained, no second OPTION gain");
    check(sound.set_disabled(true,&error)&&sound.set_muted(true,&error)&&sound.initialize_battle_controls(&error),"reset flags does not clear unrelated mute/gate");
    StartupSoundCommandObservation command;check(sound.snapshot().commands(&command,&error)&&command.disabled==1,"command gate preserved");
    check(sound.snapshot().frame(2,&frame,&error)&&frame.controls.mute==1,"mute preserved");
    SoundControlOwner moved=std::move(sound);check(moved.ready()&&now.live(),"sound scope follows move");moved.reset();check(!now.live()&&!now.battle_flags(),"retirement expires snapshot");
    SourceByteRngOwner rng;check(rng.initialize_explicit({91,47}),"explicit RNG source");const auto prior=rng.snapshot();
    BattlePreparationBootstrap bootstrap;std::array<const ForceInventory*,2> owners{};
    check(!bootstrap.initialize_scene_services(no_bank0,rng,owners,&error)&&rng.current(prior)&&!bootstrap.services(owners),"closed bootstrap cannot spend RNG or accept public completion flags");
    std::printf("battle scene services: %u owned checks PASS\n",checks);
}
