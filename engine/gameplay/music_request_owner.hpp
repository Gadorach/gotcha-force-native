#pragma once
#include "engine/assets/music_request.hpp"
#include "engine/gameplay/sound_control_owner.hpp"
#include <exception>
namespace gf {
struct MusicRequestRing {
    std::array<std::optional<u32>,64> words;u8 read=0,write=0;unsigned pending=0;
    bool operator==(const MusicRequestRing&)const=default;
};
//E91D4 checks the intermediate slots before writing a whole command. A source
//write that would make full indistinguishable from empty is outside this native
//subset; refuse it rather than manufacture an idle/successful-drain result.
inline bool music_request_enqueue(MusicRequestRing& ring,std::span<const u32> words,bool* accepted,std::string* error){
    if(accepted)*accepted=false;
    if(!accepted||ring.read>=64||ring.write>=64||ring.pending>63||((unsigned(ring.read)+ring.pending)&63)!=ring.write||words.empty()||words.size()>4){if(error)*error="music request ring: bounded current queue/command required";return false;}
    for(unsigned i=1;i<words.size();++i)if(((unsigned(ring.write)+i)&63)==ring.read){if(error)error->clear();return true;}
    if(words.size()>63-ring.pending){if(error)*error="music request ring: ambiguous source full-ring write refused";return false;}
    for(auto word:words){ring.words[ring.write]=word;ring.write=u8((ring.write+1)&63);}
    ring.pending+=unsigned(words.size());*accepted=true;if(error)error->clear();return true;
}
struct MusicRequestControl {
    f32 value0c=127,maximum10=127,minimum14=0,slope1c=0;
    u16 sequence0a=0;u8 request1b=0,flags23=0;u32 field38=0;bool script34_present=false;
    bool operator==(const MusicRequestControl&)const=default;
};
struct MusicRequestOutcome {bool ignored=false,load_queued=false,play_queued=false;};
struct MusicRequestState {MusicRequestControl player2;MusicRequestRing ring;u64 revision=1;};
namespace music_request_owner_detail {struct Scope {};struct Data {MusicRequestState state;SoundControlSnapshot controls;};}
class MusicRequestSnapshot {
public:
    bool live()const{return data_&&!scope_.expired()&&data_->controls.live();}
    const MusicRequestState* state()const{return live()?&data_->state:nullptr;}
    bool same_sound_owner(const SoundControlSnapshot& sound)const{return live()&&data_->controls.same_owner(sound);}
private:friend class MusicRequestOwner;std::weak_ptr<const music_request_owner_detail::Scope> scope_;std::shared_ptr<const music_request_owner_detail::Data> data_;
};
//One fresh native player2 request/control owner per shared sound epoch.
//It owns no playing stream: E95A4 updates the real scalar control, then takes its
//actual null-handle branch. Importing live CRI handles, command dispatch, gain
//application to an existing stream and PCM delivery remain unsupported.
class MusicRequestOwner {
public:
    MusicRequestOwner()=default;MusicRequestOwner(const MusicRequestOwner&)=delete;MusicRequestOwner& operator=(const MusicRequestOwner&)=delete;
    MusicRequestOwner(MusicRequestOwner&&)=default;MusicRequestOwner& operator=(MusicRequestOwner&&)=default;
    bool initialize_native(const MusicRequestProfile& profile,const SoundControlSnapshot& sound,std::string* error){
        const auto sound_scope=sound.scope_.lock();
        if(attempted_||!profile.ready()||!sound.live()||!sound_scope||sound_scope->music_claimed)return fail(error,"fresh request profile/unclaimed shared sound epoch required");
        attempted_=true;sound_scope->music_claimed=true;
        try{auto data=std::make_shared<music_request_owner_detail::Data>();data->controls=sound;profile_=profile;scope_=std::make_shared<const music_request_owner_detail::Scope>();data_=std::move(data);if(error)error->clear();return true;}
        catch(const std::exception&){return fail(error,"uncertain native request initialization; claim retained");}
    }
    bool request_start(s32 member,const SoundControlOwner& sound,MusicRequestOutcome* out,std::string* error){
        if(out)*out={};
        const auto controls=sound.snapshot();
        if(!out||!ready()||!controls.same_owner(data_->controls)||!controls.bgm_index()||data_->state.revision==std::numeric_limits<u64>::max())return fail(error,"same live sound owner/request output required");
        if(member<0){out->ignored=true;if(error)error->clear();return true;}
        if(member>65535)return fail(error,"bounded source archive member required");
        try{auto next=std::make_shared<music_request_owner_detail::Data>(*data_);next->controls=controls;
            auto& c=next->state.player2;c.maximum10=f32(127.0f*profile_.gain(*controls.bgm_index()));c.minimum14=0;c.flags23=0;c.field38=0;c.slope1c=0;c.request1b=1;c.script34_present=false;
            c.value0c=std::clamp(c.maximum10,c.minimum14,c.maximum10); //E95A4 with f1=maximum
            c.sequence0a=u16(unsigned(c.sequence0a)+1);
            MusicRequestOutcome result;const std::array<u32,4> load{0,2,0,u32(member)};const std::array<u32,2> play{1,2};
            if(!music_request_enqueue(next->state.ring,load,&result.load_queued,error)||!music_request_enqueue(next->state.ring,play,&result.play_queued,error))return false;
            ++next->state.revision;data_=std::move(next);*out=result;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain native request allocation");}
    }
    bool ready()const{return scope_&&data_&&data_->controls.live();}
    s32 battle_member()const{return ready()?profile_.battle_member():-1;}
    MusicRequestSnapshot snapshot()const{MusicRequestSnapshot out;out.scope_=scope_;out.data_=data_;return out;}
    void retire(){scope_.reset();data_.reset();attempted_=true;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("music request owner: ")+why;return false;}
    MusicRequestProfile profile_;std::shared_ptr<const music_request_owner_detail::Scope> scope_;std::shared_ptr<const music_request_owner_detail::Data> data_;bool attempted_=false;
};
} // namespace gf
