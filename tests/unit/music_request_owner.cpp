#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/music_request_owner.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"music request: %s\n",why);std::abort();}}
gf::MusicRequestProfile profile(){using namespace gf;std::array<u8,64> bytes{};const std::array<u32,16> words{0x47464d51,1,64,64,4,2,7,0x42fe0000,0,0x3f19999a,0x3f333333,0x3f4ccccd,0x3f666666,0x3f800000,0x2c,0};for(unsigned i=0;i<16;++i)store_be32(bytes.data()+4*i,words[i]);MusicRequestProfile p;std::string error;check(p.load({bytes.data(),bytes.size()},&error),"owned source request recipe");return p;}}
int main(){
    using namespace gf;const auto recipe=profile();std::string error;
    for(unsigned level=0;level<5;++level){
        SoundControlOwner sound;OptionSettings settings;settings.volume[0]=u8(level);check(sound.initialize(std::array<u8,1>{0},settings,&error),"native shared OPTION owner");
        MusicRequestOwner music;check(music.initialize_native(recipe,sound.snapshot(),&error),"fresh music request/control epoch");
        MusicRequestOwner fork;check(!fork.initialize_native(recipe,sound.snapshot(),&error),"cannot fork native music queue behind same sound epoch");
        const auto before=music.snapshot();MusicRequestOutcome out;check(music.request_start(-1,sound,&out,&error)&&out.ignored&&music.snapshot().state()->revision==before.state()->revision,"negative member branch preserves controls/queue");
        check(music.request_start(30,sound,&out,&error)&&out.load_queued&&out.play_queued&&!out.ignored,"actual load/play words admitted");
        const auto current=music.snapshot();const auto* state=current.state();const std::array<u32,6> words{0,2,0,30,1,2};
        check(state&&state->ring.pending==6&&state->ring.write==6&&state->ring.read==0&&state->player2.sequence0a==1&&state->player2.request1b==1,"source control/count and two-command queue");
        for(unsigned i=0;i<6;++i)check(state->ring.words[i]==words[i],"exact source queue words");
        for(unsigned i=6;i<64;++i)check(!state->ring.words[i],"unwritten ring storage is unknown");
        check(state->player2.value0c==f32(127.0f*recipe.gain(level))&&state->player2.maximum10==state->player2.value0c&&state->player2.minimum14==0&&state->player2.slope1c==0&&!state->player2.script34_present&&!state->player2.field38&&!state->player2.flags23,"E99C8/E95A4 scalar stores from BGM, not SFX setting");
        check(before.state()->ring.pending==0&&before.state()->player2.value0c==127,"old snapshot immutable");
        SoundControlOwner foreign;check(foreign.initialize(std::array<u8,1>{0},settings,&error)&&!music.request_start(7,foreign,&out,&error)&&music.snapshot().state()->ring==state->ring,"foreign equal settings refuse");
        settings.volume[0]=4;check(sound.apply_preferences(settings,&error)&&music.request_start(7,sound,&out,&error)&&music.snapshot().state()->player2.value0c==127&&music.snapshot().state()->ring.pending==12,"latest same-owner BGM preferences used without resetting queue");
        MusicRequestOwner moved=std::move(music);check(moved.ready()&&!music.ready()&&current.live(),"scope survives move");sound.reset();check(!moved.ready()&&!current.live(),"sound epoch retirement invalidates music request scope");
    }
    const std::array<u32,4> load{0,2,0,7};const std::array<u32,2> play{1,2};bool accepted=false;
    MusicRequestRing ring;ring.read=60;ring.write=62;ring.pending=2;ring.words[60]=91;ring.words[61]=92;
    check(music_request_enqueue(ring,load,&accepted,&error)&&accepted&&ring.write==2&&ring.pending==6&&ring.words[60]==91&&ring.words[61]==92&&ring.words[62]==0&&ring.words[63]==2&&ring.words[0]==0&&ring.words[1]==7,"wrapped source enqueue retains preceding words");
    ring={};ring.write=61;ring.pending=61;const auto before=ring;
    check(music_request_enqueue(ring,load,&accepted,&error)&&!accepted&&ring==before,"source collision drops whole command");
    check(music_request_enqueue(ring,play,&accepted,&error)&&accepted&&ring.pending==63&&ring.write==63,"later shorter command can still fit");
    const auto full=ring;check(music_request_enqueue(ring,play,&accepted,&error)&&!accepted&&ring==full,"full queue drops without overwriting");
    ring={};ring.write=60;ring.pending=60;const auto ambiguous=ring;check(!music_request_enqueue(ring,load,&accepted,&error)&&!accepted&&ring==ambiguous,"ambiguous source full/empty write refuses before mutation");
    SoundControlOwner sound;check(sound.initialize(std::array<u8,1>{0},OptionSettings{},&error),"bounded request owner");MusicRequestOwner music;check(music.initialize_native(recipe,sound.snapshot(),&error),"bounded fresh queue");
    MusicRequestOutcome outcome;for(unsigned i=0;i<10;++i)check(music.request_start(7,sound,&outcome,&error),"ten genuine queued starts");
    const auto prior=music.snapshot();check(!music.request_start(7,sound,&outcome,&error)&&music.snapshot().state()->revision==prior.state()->revision&&music.snapshot().state()->ring==prior.state()->ring,"unsupported ring boundary has no partial control/queue commit");
    std::printf("music request owner: %u checks PASS; no playback execution\n",checks);
}
