#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/challenge_initial_party.hpp"
#include "tests/unit/challenge_ai_force_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"initial party: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;SourceByteRngOwner rng,other;SourceByteRngSnapshot empty;u8 byte=77;
    check(!rng.ready()&&!rng.current(empty)&&!rng.next_byte(&byte)&&byte==77,"uninitialized owner does no work");
    check(rng.initialize_explicit({195,1})&&!rng.initialize_explicit({1,2})&&rng.state()==SourceByteRngState{195,1},"explicit once-only incoming stream, no entry reset");
    const auto snapshot=rng.snapshot();check(rng.current(snapshot)&&snapshot.state()==rng.state()&&!rng.next_byte(nullptr)&&rng.current(snapshot),"snapshot and null-output guard");
    check(other.initialize_explicit({195,1})&&!other.current(snapshot),"identical state is not the same owner");
    check(rng.next_byte(&byte)&&byte==200&&rng.state()==SourceByteRngState{200,5}&&rng.draws()==1&&rng.revision()==1&&!rng.current(snapshot),"genuine byte advance invalidates prior revision");
    const auto moving=rng.snapshot();SourceByteRngOwner moved=std::move(rng);check(!rng.ready()&&moved.current(moving)&&!rng.current(moving),"move retains logical owner, not a raw object address");
    SourceByteRngOwner zeros;check(zeros.initialize_explicit({0,0}),"zero seed allowed explicitly");const auto zero_snapshot=zeros.snapshot();check(zeros.next_byte(&byte)&&byte==0&&zeros.state()==SourceByteRngState{}&&!zeros.current(zero_snapshot)&&zeros.draws()==1,"equal RNG bytes still have a new revision");
    for(unsigned i=0;i<1000;++i){const auto prior=moved.snapshot();const auto old_draws=moved.draws(),old_revision=moved.revision();check(moved.next_byte(&byte)&&!moved.current(prior)&&moved.draws()==old_draws+1&&moved.revision()==old_revision+1&&moved.state().low==byte,"serialized genuine consumption counters");}
    SourceByteRngSnapshot expired;{SourceByteRngOwner temporary;check(temporary.initialize_explicit({9,10}),"temporary owner");expired=temporary.snapshot();}check(!moved.current(expired),"expired identity cannot authorize another owner");
    const auto catalog=challenge_ai_force_fixture::catalog();ForceBorg borg;borg.model=8;borg.variant=5;borg.level=9;borg.experience=0xdeadbeefu;borg.acquired=0xfedcba98u;borg.fresh=1;borg.reserved13=0xa5;borg.reserved14=0x1234;ChallengeRosterFields fields;
    check(challenge_roster_fields(catalog,borg,&fields)&&fields==ChallengeRosterFields{8,5,9,0xdeadbeef,0xfedcba98},"source known fields preserve high-bit XP/acquired, not freshness/reserved bytes");
    for(unsigned which=0;which<5;++which){auto bad=borg;if(which==0)bad.model=-1;if(which==1)bad.variant=6;if(which==2)bad.level=10;if(which==3)bad.edits.modified=true;if(which==4)bad.edits.values[0]=100;check(!challenge_roster_fields(catalog,bad,&fields)&&fields==ChallengeRosterFields{},"invalid/edited metadata refuses without partial fields");}
    check(!challenge_roster_fields(catalog,borg,nullptr),"null metadata output");
    ChallengeInitialPartyCandidate candidate;std::array<const ForceInventory*,2> owners{};check(!candidate.ready()&&!candidate.current(owners)&&candidate.members(6).empty()&&candidate.team(6)==-1&&!candidate.name(6),"default and indexed candidate guards");
    std::array<ChallengeSetupChoice,2> choices;ChallengeDifficultyProfile difficulty{{100,200,300},1,1};ChallengeRandomForceProfile random;ChallengeStageRngProfile stages;ChallengeAiForceProfile ai;ChallengeInitialPartyAssembly assembly;std::string error;
    check(!assembly.begin(choices,0,owners,difficulty,0,random,stages,ai,moved,&error)&&assembly.status()==ChallengeInitialPartyStatus::Failed,"no forged selection batch from plain data");
    const auto held=moved.snapshot();check(!assembly.take(owners,moved,&candidate)&&!candidate.ready()&&moved.current(held),"failed planning cannot publish or commit stream");
    check(!assembly.begin(choices,1,owners,difficulty,0,random,stages,ai,moved,&error)&&moved.current(held),"failed assembly cannot be replayed");
    std::printf("initial party: %u owned checks PASS\n",checks);
}
