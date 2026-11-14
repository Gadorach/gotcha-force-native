#pragma once
//801C86B8/8700 and8CF0/8D3C, source group28 before actors. Not full scene/audio.
#include "engine/assets/startup_intro_props.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/gameplay/attachment_alternate_scene.hpp"
#include "engine/gameplay/startup_director.hpp"
namespace gf {
struct StartupIntroPropState {
    u8 path_phase=0,path_substate=0,fade_phase=0,fade_substate=0;bool path_visible=false,fade_visible=false;
    f32 alpha=0;u32 path_updates=0,alpha_updates=0;
};
enum class StartupIntroPropMode {DeclaredSceneObservations};
class StartupIntroPropScene {
public:
    bool load(ByteView raw,const HsdRecordAnimationBank& bank,const StartupIntroPropProfile& profile,ByteView style,
              StartupIntroPropMode mode,std::string* error){
        storage_.reset();ready_=published_=false;
        if(mode!=StartupIntroPropMode::DeclaredSceneObservations||!startup_intro_prop_profile_valid(profile)||bank.hash()!=profile.bank_hash||bank.records()!=profile.bank_records)return fail(error,"source bank/profile/scope");
        u32 hash=2166136261u;if(!raw.data||raw.size>(64u<<20))return fail(error,"raw style-source bank budget");for(std::size_t i=0;i<raw.size;++i){hash^=raw.data[i];hash*=16777619u;}if(hash!=profile.bank_hash)return fail(error,"same bank for both render paths");
        auto s=std::make_unique<Storage>();s->profile=profile;
        if(!s->path.load_paths(bank,profile.path_record,1,0,0,error)||s->path.asset()->model_roots!=std::vector<u32>{profile.path_root}||!s->fade.load(raw,profile.fade_record,style,error)||s->fade.asset().scene.model_roots!=std::vector<u32>{profile.fade_root})return fail(error,"explicit path/alternate source bindings");
        const auto& root=s->fade.asset().scene.joints[0];s->fade_world.world=mat4_from_srt(root.rotation,root.scale,root.translation);s->fade_world.valid=true;
        storage_=std::move(s);ready_=true;if(error)error->clear();return true;
    }
    bool before_actors(const StartupDirectorState& director,u8 mode,u8 scene34,std::string* error){
        published_=false;if(!ready_||!storage_||director.updates!=storage_->frames+1||director.updates>8192||director.phase>12)return refuse(error,"consecutive group28 update");
        auto& s=*storage_;auto next=s.state;
        if(next.path_phase==0){if(!s.path.request(s.profile.first_frame,s.profile.rate,error))return refuse(error,"initial path request");next.path_phase=1;next.path_visible=true;}
        else if(next.path_phase==2){next.path_phase=3;next.path_visible=false;}
        else if(next.path_phase==3){s.path.release();next.path_phase=4;next.path_visible=false;}
        if(next.path_phase==1){bool end=false;if(next.path_substate==1){if(!s.path.advance(s.profile.rate,error))return refuse(error,"authored path callback");++next.path_updates;end=director.phase!=2;}
            else if(director.phase>1)next.path_substate=1;
            if(end||mode!=1){next.path_phase=2;next.path_visible=false;}}
        if(next.fade_phase==0){next.alpha=s.profile.alpha_start;if(!s.fade.set_world_alpha(s.fade_world,next.alpha,error))return refuse(error,"initial source alpha replacement");next.fade_phase=1;next.fade_visible=true;}
        else if(next.fade_phase==2){next.fade_phase=3;next.fade_visible=false;}
        else if(next.fade_phase==3){s.fade=AttachmentAlternateModelScene{};next.fade_phase=4;next.fade_visible=false;}
        if(next.fade_phase==1){bool end=false;if(next.fade_substate==1){next.alpha=f32(next.alpha-s.profile.alpha_step);if(next.alpha<=s.profile.zero){next.alpha=s.profile.zero;end=true;}if(!s.fade.set_world_alpha(s.fade_world,next.alpha,error))return refuse(error,"source fade alpha update");++next.alpha_updates;}
            else if(director.phase>1)next.fade_substate=1;
            if(end||mode!=1){next.fade_phase=2;next.fade_visible=false;}}
        s.path.set_enabled(next.path_visible&&scene34==0);s.state=next;s.frames=director.updates;s.display=scene34==0;published_=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){if(!ready_||!storage_)return 0;const auto n=storage_->path.upload_textures(render,base);return n+storage_->fade.upload_textures(render,base+n);}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){if(!ready_||!storage_)return 0;return storage_->path.upload_textures_shared(render,base)+storage_->fade.upload_textures_shared(render,base);}
    unsigned draw_model(unsigned part,RenderEngine& render,const Camera& camera){if(part>1||!ready_||!storage_||!published_||!storage_->display)return 0;return part==0?storage_->path.draw(render,camera):storage_->state.fade_visible?storage_->fade.draw(render,camera):0;}
    unsigned draw(RenderEngine& render,const Camera& camera){if(!ready_||!storage_||!published_||!storage_->display)return 0;unsigned n=storage_->path.draw(render,camera);if(storage_->state.fade_visible)n+=storage_->fade.draw(render,camera);return n;}
    bool ready()const{return ready_&&bool(storage_);}
    u32 frames()const{return storage_?storage_->frames:0;}
    const StartupIntroPropState* state()const{return ready()?&storage_->state:nullptr;}
    const AnimatedModelScene* path()const{return ready()?&storage_->path:nullptr;}
    const AttachmentAlternateModelScene* fade()const{return ready()?&storage_->fade:nullptr;}
    void release(){storage_.reset();ready_=published_=false;}
private:
    struct Storage {StartupIntroPropProfile profile;StartupIntroPropState state;AnimatedModelScene path;AttachmentAlternateModelScene fade;AttachmentBoneTransform fade_world;u32 frames=0;bool display=false;};
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup intro prop scene: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const auto detail=error?*error:std::string{};ready_=published_=false;if(storage_){storage_->path.set_enabled(false);storage_->fade.discard_pose();}fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    std::unique_ptr<Storage> storage_;bool ready_=false,published_=false;
};
} // namespace gf
