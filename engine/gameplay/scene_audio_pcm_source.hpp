#pragma once
// Explicit menu/credits/editor music + existing event-boundary SFX. One caller's
// PcmQueue remains the only sink owner. No source-call or speaker ACK is invented.
#include "engine/assets/menu_music.hpp"
#include "engine/audio/music_fade.hpp"
#include "engine/audio/pcm_player.hpp"
#include "engine/gameplay/sound_pcm_source.hpp"
#include "engine/gameplay/story_guide_voice_pcm.hpp"
namespace gf {
enum class SceneMusicCue:u8 {Menu,Credits,Editor,None};
using SceneMusicFade=music_fade_detail::Fade<SceneMusicCue,2,SceneMusicCue::None>;
class SceneMusicLibrary {
public:
    bool load(const std::array<MenuMusicProfile,3>& profiles,IFileSystem& files,const std::string& archive,std::string* error){
        if(data_)return fail(error,"fresh library required");
        const std::array<u32,3> members={32,27,6};std::array<AdxAudio,3> tracks;
        for(unsigned i=0;i<3;++i){const auto& p=profiles[i];const auto& base=profiles[0];
            if(p.entry!=members[i]||p.output_rate!=base.output_rate||p.archive_count!=base.archive_count||p.archive_size!=base.archive_size||p.directory_hash!=base.directory_hash||!menu_music_load(files,archive,p,&tracks[i],error))return fail(error,"explicit common-archive menu32/credits27/editor6 resources required");
        }
        return load_declared_decoded(std::move(tracks),profiles[0].output_rate,error);
    }
    // Owned/declared decoded-resource seam, not original file/provenance proof.
    bool load_declared_decoded(std::array<AdxAudio,3> tracks,u32 rate,std::string* error){
        if(data_)return fail(error,"fresh decoded library required");
        for(const auto& audio:tracks)if(!pcm_player_audio_valid(audio,rate))return fail(error,"bounded decoded tracks/device rate required");
        auto next=std::make_shared<Data>();next->tracks=std::move(tracks);next->rate=rate;data_=std::move(next);return true;
    }
    bool ready()const{return bool(data_);}u32 output_rate()const{return data_?data_->rate:0;}
    const AdxAudio* audio(SceneMusicCue cue)const{return data_&&u32(cue)<3?&data_->tracks[u32(cue)]:nullptr;}
    bool same_resources(const SceneMusicLibrary& other)const{return data_&&data_==other.data_;}
private:
    struct Data {std::array<AdxAudio,3> tracks;u32 rate=0;};std::shared_ptr<const Data> data_;
    static bool fail(std::string* e,const char* why){if(e)*e=std::string("scene music library: ")+why;return false;}
};
struct SceneAudioPcmSnapshot {
    SoundPcmSourceSnapshot sfx;PcmPlayerCursor music_cursor;SceneMusicFade fade;
    SceneMusicCue current=SceneMusicCue::None,wanted=SceneMusicCue::None;
    u64 music_bus_frames=0,music_generated_frames=0,switches=0,updates=0;
    bool failed=false,pending=false;
};
class SceneAudioPcmSource {
public:
    SceneAudioPcmSource()=default;
    SceneAudioPcmSource(const SceneAudioPcmSource&)=delete;
    SceneAudioPcmSource& operator=(const SceneAudioPcmSource&)=delete;
    SceneAudioPcmSource(SceneAudioPcmSource&&)=delete;
    SceneAudioPcmSource& operator=(SceneAudioPcmSource&&)=delete;
    bool load(SoundVoiceLibrary voices,SoundVoiceProfile voice,SoundEnvelopeProfile envelope,SceneMusicLibrary music,const MusicFadeConfig& fade,f32 level,std::string* error){
        if(loaded_||failed_||!music.ready())return fail(error,"fresh owner and immutable music resources required");
        MusicState next;if(!next.fade.configure(fade,level)||!sfx_.load(std::move(voices),voice,envelope,music.output_rate(),error))return false;
        next.cursor.playing=true;music_=std::move(music);state_=next;loaded_=true;return true;
    }
    bool request_music(SceneMusicCue cue,std::string* error){
        if(!ready()||pending_||u32(cue)>=3)return fail(error,"live serialized source/valid scene cue required");
        if(cue==state_.wanted)return true;
        if(!state_.fade.request(cue))return fail(error,"music request");
        state_.wanted=cue;return true;
    }
    bool set_music_level(f32 gain,bool immediate,std::string* error){
        return ready()&&!pending_&&state_.fade.set_level(gain,immediate)?true:fail(error,"music level/control boundary");
    }
    // One explicit source/UI update, NOT per queue chunk or device sample. The
    // availability observation is caller-supplied, as in the old CreditsMusic API.
    bool advance_music(bool available,PcmQueue& queue,std::string* error){
        if(!ready()||pending_)return fail(error,"serialized music update required");
        if(!available)return true;
        if(state_.updates==~u64(0))return latch(error,"music update overflow");
        auto next=state_;SceneMusicCue cue;if(!next.fade.update(&cue))return latch(error,"source fade safety refusal");++next.updates;
        if(cue!=SceneMusicCue::None){
            if(next.switches==~u64(0)||!music_.audio(cue)||!queue.cut(AudioBus::Bgm))return latch(error,"selective music cut/resource boundary");
            next.current=cue;next.cursor={};next.cursor.playing=true;++next.switches;
        }
        state_=std::move(next);return true;
    }
    bool pump(PcmQueue& queue,PcmMixer& mixer,const SoundPcmBoundaryProvider& provider,std::string* error){
        if(!ready()||pending_)return false;
        const bool ok=queue.pump(mixer,[&](unsigned frames,PcmMixer& mix){return prepare_pcm(frames,mix,provider,error);},[&](bool accepted){return complete(accepted,error);});
        if(!ok)failed_=true;
        return ok;
    }
    //Opt-in actual native guide voices join the SAME source prepare/submit/commit.
    //The old pump above remains voice-free; no second queue or public success bool.
    bool pump_with_guide_voice(PcmQueue& queue,PcmMixer& mixer,const SoundPcmBoundaryProvider& provider,StoryGuideVoicePcmSource& voice,std::string* error){
        if(!ready()||pending_||!voice.ready()||voice.pending_||voice.pumping_||voice.controlling_||voice.queue_!=&queue||voice.output_rate()!=music_.output_rate())return false;
        bool ok=false;{StoryGuideVoicePcmSource::Boundary guard(voice.pumping_);ok=queue.pump(mixer,[&](unsigned frames,PcmMixer& mix){return prepare_pcm(frames,mix,provider,error)&&voice.prepare_pcm(frames,mix,error);},[&](bool accepted){if(!accepted||!pending_||!sfx_.pending_commit_valid()||!voice.pending_valid()){(void)complete(false,error);return voice.reject_pending(error);}if(!complete(true,error))return voice.reject_pending(error);voice.accept_pending();return true;});}if(!ok){failed_=true;return voice.reject_pending(error);}if(!voice.observe_queue(error)){failed_=true;return false;}return true;
    }
    bool ready()const{return loaded_&&!failed_&&!sfx_.failed();}
    SceneAudioPcmSnapshot snapshot()const{return {sfx_.snapshot(),state_.cursor,state_.fade,state_.current,state_.wanted,state_.bus_frames,state_.generated,state_.switches,state_.updates,!ready(),bool(pending_)};}
    const SceneMusicLibrary& music_library()const{return music_;}
private:
    struct MusicState {SceneMusicFade fade;PcmPlayerCursor cursor;SceneMusicCue current=SceneMusicCue::Menu,wanted=SceneMusicCue::Menu;u64 bus_frames=0,generated=0,switches=0,updates=0;};
    bool prepare_pcm(unsigned frames,PcmMixer& mix,const SoundPcmBoundaryProvider& provider,std::string* error){
        if(!sfx_.prepare_boundaries(frames,mix,provider,error))return false;
        auto next=state_;const auto* audio=music_.audio(next.current);std::vector<s16> pcm;u32 produced=0;
        if(!audio||next.bus_frames>~u64(0)-frames)return fail(error,"bounded shared music resources");
        if(next.cursor.playing){if(!pcm_player_render_cursor(*audio,music_.output_rate(),&next.cursor,frames,&pcm,&produced))return fail(error,"bounded music cursor");}else pcm.assign(std::size_t(frames)*2,0);
        if(next.generated>~u64(0)-produced||!mix.add(AudioBus::Bgm,pcm,next.fade.envelope()))return fail(error,"bounded shared music block");
        next.bus_frames+=frames;next.generated+=produced;pending_=std::move(next);return true;
    }
    bool complete(bool accepted,std::string* error){
        if(!accepted||!pending_||!sfx_.pending_commit_valid()){(void)sfx_.complete(false);pending_.reset();return latch(error,"joint candidate rejected/uncertain; no replay");}
        if(!sfx_.complete(true)){pending_.reset();return latch(error,"validated SFX candidate commit");}
        state_=std::move(*pending_);pending_.reset();return true;
    }
    static bool fail(std::string* e,const char* why){if(e&&e->empty())*e=std::string("scene audio: ")+why;return false;}
    bool latch(std::string* e,const char* why){failed_=true;return fail(e,why);}
    SoundPcmSource sfx_;SceneMusicLibrary music_;MusicState state_;std::optional<MusicState> pending_;bool loaded_=false,failed_=false;
};
} // namespace gf
