#pragma once
// Source nonloop instant-motion driver. Retained per-bone AObj/FObj state,
// source lane order and frame overwrite rules; no root/event/render effects.
#include <span>
#include "engine/gameplay/character_pose.hpp"
#include "engine/gameplay/character_program.hpp"
#include "engine/gameplay/character_motion_direction.hpp"
#include "engine/gameplay/character_motion_root_state.hpp"
namespace gf {
struct CharacterMotionClipBinding {u8 group=0,index=0;HsdBoneMotionClip clip;};
struct CharacterMotionTrackTrace {
    u32 channel=0,state=0,flags=0,op=0,interpolation=0;std::size_t cursor=0;
    f64 time=0,value=0;u64 callbacks=0;
};
struct CharacterMotionBoneTrace {bool has_aobj=false,first_play=false,stopped=false;f32 frame=0;std::vector<CharacterMotionTrackTrace> tracks;};
namespace character_motion_driver_detail {
struct Library {CharacterPoseRig rig;CharacterProgramAsset profile;std::vector<CharacterMotionClipBinding> clips;bool transitions=false,frozen_target=false,directional=false,directional_transitions=false;std::array<s8,2> model_bones={-1,-1};CharacterLocalPose initial_pose;};
struct DirectionSeed {f32 elapsed=0,previous_elapsed=0,previous_frame=0,time=0,duration=0,weight=0;bool blending=false;u8 mode=0;};
struct Bone {bool has_aobj=false,first_play=false,stopped=false,rewound=false;f32 frame=0;std::vector<hsd_anim_detail::TrackMachine> tracks;};
inline bool same(const CharacterProgramMotion& a,const CharacterProgramMotion& b){return a.selected==b.selected&&a.elapsed==b.elapsed&&a.frame==b.frame&&a.end_frame==b.end_frame&&a.finished==b.finished&&a.rewound==b.rewound;}
} // namespace character_motion_driver_detail
class CharacterMotionDriver;
class CharacterMotionSnapshot final:public CharacterProgramMotionContext {
public:
    const HsdBoneMotionSample& sample()const{return sample_;}
    bool mirrored()const{return mirrored_;}
    unsigned lane()const{return lane_;}
    u8 group()const{return library_->clips[clip_].group;}
    u8 index()const{return library_->clips[clip_].index;}
    f32 previous_elapsed()const{return previous_elapsed_;}
    f32 previous_frame()const{return previous_frame_;}
    bool looping()const{return looping_;}u8 loop_start()const{return loop_start_;}
    bool blending()const{return blending_;}f32 blend_time()const{return blend_time_;}f32 blend_duration()const{return blend_duration_;}f32 previous_weight()const{return old_weight_;}
    u8 transition_mode()const{return blend_mode_;}
    CharacterMotionEvaluation evaluation()const{return evaluation_;}
    // Extended mode publishes converted/blended lane output. sample() remains
    // the unblended callback data; do not convert frozen quaternions to Euler.
    const CharacterLocalPose* local_pose()const{return library_->transitions?&local_pose_:nullptr;}
    std::vector<CharacterMotionBoneTrace> trace()const{
        std::vector<CharacterMotionBoneTrace> result;
        const auto& source=library_->clips[clip_].clip;
        for(unsigned i=0;i<bones_.size();++i){const auto& b=bones_[i];CharacterMotionBoneTrace row;row.has_aobj=b.has_aobj;row.first_play=b.first_play;row.stopped=b.stopped;row.frame=b.frame;
            for(unsigned j=0;j<b.tracks.size();++j){const auto& m=b.tracks[j];row.tracks.push_back({source.bones[i].tracks[j].channel,m.state,m.flags,m.op,m.op_intrp,m.pos,m.time,m.value,m.callbacks});}
            result.push_back(std::move(row));
        }
        return result;
    }
private:
    friend class CharacterMotionDriver;
    CharacterMotionSnapshot()=default;
    CharacterMotionSnapshot(const CharacterMotionSnapshot&)=default;
    std::shared_ptr<const character_motion_driver_detail::Library> library_;
    unsigned lane_=0,clip_=0;bool mirrored_=false;
    f32 previous_elapsed_=0,previous_frame_=0; // source motion+10/+14 for future event dispatch
    CharacterProgramMotion observed_;
    HsdBoneMotionSample sample_;
    std::vector<character_motion_driver_detail::Bone> bones_;
    bool looping_=false,blending_=false;u8 loop_start_=0,blend_mode_=0;f32 blend_time_=0,blend_duration_=0,old_weight_=0;
    bool directional_target_=false; //raw target output; temporal blend belongs to the dual owner
    CharacterMotionEvaluation evaluation_=CharacterMotionEvaluation::Request;s8 root_scale_selector_=-1;
    std::weak_ptr<const CharacterProgramMotionContext> source_context_; //no retained player/history chain
    f32 root_callback_clock_=0;bool root_callback_seen_=false;
    CharacterLocalPose local_pose_,frozen_pose_;
};
// Explicit follow-motion component, not an actor/VM execution acknowledgement.
// Two retained snapshots share one immutable library; no previous-pose chains.
class CharacterDirectionalMotionSnapshot final {
public:
    const CharacterProgramMotion& primary()const{return motions_[0];}
    const CharacterProgramMotion& side()const{return motions_[1];}
    const CharacterLocalPose& local_pose()const{return pose_;}
    const std::array<u8,2>& models()const{return models_;}
    CharacterMotionTarget primary_target()const{return primary_target_;}
    CharacterMotionTarget side_target()const{return side_target_;}
    f32 side_weight()const{return side_weight_;}
    bool side_rebound()const{return side_rebound_;}
    unsigned lane()const{return lane_;}
    bool blending()const{return blending_;}
    f32 blend_time()const{return blend_time_;}f32 blend_duration()const{return blend_duration_;}f32 previous_weight()const{return old_weight_;}
    const CharacterLocalPose& directional_pose()const{return directional_pose_;}
    const CharacterLocalPose& frozen_pose()const{return frozen_pose_;}
    const std::array<u8,2>& primary_models()const{return centre_models_;}
    const std::array<u8,2>& alternate_models()const{return side_models_;}
    const CharacterProgramDescriptor& primary_descriptor()const{return primary_descriptor_;}
    const CharacterProgramDescriptor& side_descriptor()const{return side_descriptor_;}
private:
    friend class CharacterMotionDriver;
    CharacterDirectionalMotionSnapshot()=default;
    std::shared_ptr<const character_motion_driver_detail::Library> library_;
    std::array<CharacterProgramMotion,2> motions_;
    CharacterMotionTarget primary_target_,side_target_;
    std::array<u8,2> centre_models_{},side_models_{},models_{};
    CharacterLocalPose pose_;unsigned lane_=0;f32 side_weight_=0;bool side_rebound_=false;
    CharacterLocalPose directional_pose_,frozen_pose_;std::array<u8,2> frozen_models_{};
    bool blending_=false;f32 blend_time_=0,blend_duration_=0,old_weight_=0;
    CharacterProgramDescriptor primary_descriptor_,side_descriptor_;
};
class CharacterMotionDriver final:public ICharacterProgramMotion {
public:
    bool load(ByteView model,const CharacterPoseRig& rig,const CharacterProgramAsset& profile,
               std::span<const CharacterMotionClipBinding> clips,std::string* error){
        return load_impl(model,rig,profile,clips,nullptr,error);
    }
    bool load_transitions(ByteView model,const CharacterPoseRig& rig,const CharacterProgramAsset& profile,
                          std::span<const CharacterMotionClipBinding> clips,const CharacterLocalPose& initial_pose,std::string* error){
        return load_impl(model,rig,profile,clips,&initial_pose,error);
    }
    // Source487EC/4A00C mode1 holds the NEW target frame during blending.
    // Explicit driver entry only: existing GFMP1/2/VM admission is unchanged.
    bool load_frozen_target_transitions(ByteView model,const CharacterPoseRig& rig,const CharacterProgramAsset& profile,
                                        std::span<const CharacterMotionClipBinding> clips,const CharacterLocalPose& initial_pose,std::string* error){
        return load_impl(model,rig,profile,clips,&initial_pose,error,true);
    }
    // Source49B98/49F10/49CC4/49DAC instant follow path. Model-selection bones
    // are explicit source config42/43, not guessed from mesh/bone names.
    bool load_directional_follow(ByteView model,const CharacterPoseRig& rig,const CharacterProgramAsset& profile,
                                 std::span<const CharacterMotionClipBinding> clips,const CharacterLocalPose& initial_pose,
                                 const std::array<s8,2>& model_bones,std::string* error){
        return load_impl(model,rig,profile,clips,&initial_pose,error,true,&model_bones);
    }
    bool load_directional_transitions(ByteView model,const CharacterPoseRig& rig,const CharacterProgramAsset& profile,
                                      std::span<const CharacterMotionClipBinding> clips,const CharacterLocalPose& initial_pose,
                                      const std::array<s8,2>& model_bones,std::string* error){
        return load_impl(model,rig,profile,clips,&initial_pose,error,true,&model_bones,true);
    }
private:
    bool load_impl(ByteView model,const CharacterPoseRig& rig,const CharacterProgramAsset& profile,
                    std::span<const CharacterMotionClipBinding> clips,const CharacterLocalPose* initial_pose,std::string* error,bool frozen_target=false,const std::array<s8,2>* model_bones=nullptr,bool directional_transitions=false){
        library_.reset();auto fail=[&](const char* why){if(error)*error=std::string("character motion driver: ")+why;return false;};
        if(!model.data||model.size>(4u<<20)||!character_pose_detail::rig(rig)||!character_program_detail::valid_impl(profile,initial_pose!=nullptr,frozen_target&&profile.control_flow)||
           profile.lane_mask!=rig.lane_mask||character_program_model_hash(model)!=profile.model_hash||clips.empty()||clips.size()>64)return fail("model/rig/profile/clip binding");
        if(initial_pose&&!character_pose_detail::local(rig,*initial_pose))return fail("explicit initial local pose required");
        if(model_bones)for(auto bone:*model_bones)if(bone< -1||(bone>=0&&unsigned(bone)>=rig.binding.bones.size()))return fail("directional model-selection bone");
        hsd_detail::Reader reader;if(!hsd_anim_detail::open_container(model,&reader,error))return false;
        for(const auto& b:rig.binding.bones){u32 child=0,next=0;const auto at=b.joint_offset;
            if(!reader.has(at,0x40)||!reader.ptr(at+8,&child)||!reader.ptr(at+12,&next)||
               child!=(b.child==kHsdNoBone?0:rig.binding.bones[b.child].joint_offset)||next!=(b.next==kHsdNoBone?0:rig.binding.bones[b.next].joint_offset))return fail("rig/archive topology");
            for(unsigned k=0;k<3;++k)if(reader.block.bef32(at+0x2c+k*4)!=b.translation[k])return fail("rig/archive rest translation");
        }
        std::size_t bytes=0;
        for(unsigned c=0;c<clips.size();++c){const auto& entry=clips[c];const auto& clip=entry.clip;
            if(entry.group>=16||entry.index>=128||!clip.loaded||!clip.public_bone_count||clip.public_bone_count>rig.binding.bones.size()||clip.bones.size()!=rig.binding.bones.size()||
               !std::isfinite(clip.end_frame)||clip.end_frame<0||clip.end_frame>8192||std::floor(clip.end_frame)!=clip.end_frame)return fail("clip metadata/domain");
            for(unsigned i=0;i<c;++i)if(clips[i].group==entry.group&&clips[i].index==entry.index)return fail("duplicate clip key");
            for(unsigned i=0;i<clip.bones.size();++i){const auto& bone=clip.bones[i];u16 channels=0;
                if(bone.joint_offset!=rig.binding.bones[i].joint_offset||bone.tracks.size()>9||(!bone.has_aobj&&!bone.tracks.empty())||
                   (i>=clip.public_bone_count&&bone.has_aobj)||(bone.has_aobj&&(bone.tracks.empty()||clip.end_frame<=0)))return fail("bone/public/AObj binding");
                for(const auto& t:bone.tracks){const auto ch=t.channel;
                    if(!((ch>=1&&ch<=3)||(ch>=5&&ch<=10))||(channels&(1u<<ch))||t.data.empty()||t.data.size()>hsd_anim_detail::kMaxTrackBytes||
                       t.data.size()>(32u<<20)-bytes||!std::isfinite(t.startframe)||t.startframe<0||t.startframe>8192||std::floor(t.startframe)!=t.startframe)return fail("track/channel/byte/start domain");
                    channels|=u16(1u<<ch);bytes+=t.data.size();if(!stream(t))return fail("unsupported packed track/wait domain");
                }
            }
        }
        auto next=std::make_shared<character_motion_driver_detail::Library>();next->rig=rig;next->profile=profile;next->clips.assign(clips.begin(),clips.end());next->transitions=initial_pose!=nullptr;next->frozen_target=frozen_target;if(model_bones){next->directional=true;next->model_bones=*model_bones;next->directional_transitions=directional_transitions;}if(initial_pose)next->initial_pose=*initial_pose;library_=std::move(next);return true;
    }
public:
    const CharacterMotionSnapshot* snapshot(const CharacterProgramMotion& motion)const{
        const auto* s=dynamic_cast<const CharacterMotionSnapshot*>(motion.context.get());
        return library_&&s&&s->library_==library_&&character_motion_driver_detail::same(motion,s->observed_)?s:nullptr;
    }
    bool shares_library(const CharacterMotionDriver& other)const{return library_&&library_==other.library_;}
    bool successor(const CharacterProgramMotion& before,const CharacterProgramMotion& after)const{
        const auto* prior=snapshot(before);const auto* next=snapshot(after);
        return prior&&next&&prior->lane_==next->lane_&&next->source_context_.lock()==before.context;
    }
    // Raw primary root AFTER B76C mirror conversion, not the displayed temporal
    // blend or a world-space root. A public null AObj is a separate explicit mode:
    // its B76C callback leaves the lane clock unchanged; it has NO root player.
    // Missing roots/mirror aliases remain refused; old default admission is kept.
    bool root_observation(const CharacterProgramMotion& motion,CharacterMotionRootObservation* out,CharacterMotionRootObservationMode mode=CharacterMotionRootObservationMode::AObjOnly)const{
        if(out)*out={};
        const auto* s=snapshot(motion);if(!out||!s||!s->root_callback_seen_||library_->rig.mirror[0]!=0||(mode!=CharacterMotionRootObservationMode::AObjOnly&&mode!=CharacterMotionRootObservationMode::AObjOrPublicNull)||(!s->bones_[0].has_aobj&&mode==CharacterMotionRootObservationMode::AObjOnly))return false;
        const auto& rig=library_->rig;bool root=false;for(unsigned i=rig.cuts[s->lane_];i<rig.cuts[s->lane_+1];++i)root|=rig.order[i]==0;
        if(!root)return false;
        CharacterMotionRootObservation result;const auto& sample=s->sample_.bones[0];
        for(unsigned i=0;i<3;++i){result.translation[i]=(sample.present&(1u<<(5+i)))?sample.value[5+i]:rig.binding.bones[0].translation[i];if(s->mirrored_&&i==0)result.translation[i]=-result.translation[i];if(!character_pose_detail::finite(result.translation[i]))return false;}
        result.frame=s->root_callback_clock_;result.scale_selector=s->root_scale_selector_;result.clock_source=s->bones_[0].has_aobj?CharacterMotionRootClockSource::RootAObj:CharacterMotionRootClockSource::InheritedMotion;
        if(!character_program_vm_detail::time(result.frame))return false;
        *out=result;return true;
    }
    u32 model_hash()const{return library_?library_->profile.model_hash:0;}
    const CharacterPoseRig* rig()const{return library_?&library_->rig:nullptr;}
    const CharacterLocalPose* initial_local_pose()const{return library_&&library_->transitions?&library_->initial_pose:nullptr;}
    std::array<s8,2> model_bones()const{return library_?library_->model_bones:std::array<s8,2>{-1,-1};}
    bool directional_transitions_mode()const{return library_&&library_->directional_transitions;}
    bool matches_program(const CharacterProgramAsset& p)const{
        if(!library_)return false;
        const auto& a=library_->profile;
        if(a.model_hash!=p.model_hash||a.lane_mask!=p.lane_mask||a.default_models!=p.default_models||a.control_flow!=p.control_flow||a.programs.size()!=p.programs.size()||a.descriptors.size()!=p.descriptors.size())return false;
        for(unsigned i=0;i<a.programs.size();++i)if(a.programs[i].group!=p.programs[i].group||a.programs[i].choice!=p.programs[i].choice||a.programs[i].bytes!=p.programs[i].bytes)return false;
        for(unsigned i=0;i<a.descriptors.size();++i)if(a.descriptors[i].group!=p.descriptors[i].group||a.descriptors[i].choice!=p.descriptors[i].choice||a.descriptors[i].bytes!=p.descriptors[i].bytes)return false;
        return true;
    }
    // Assemble only each active lane's owned slots. Other slots retain the
    // explicitly supplied initial pose; never overwrite one animated lane with
    // another snapshot's inactive defaults. Caller supplies accepted VM state.
    bool compose_local_pose(const std::array<CharacterProgramMotion,2>& motions,CharacterLocalPose* out,std::string* error)const{
        if(out)*out={};
        auto fail=[&](){if(error)*error="character motion driver: transition lane pose binding";return false;};
        if(!out||!library_||!library_->transitions)return fail();
        auto pose=library_->initial_pose;const auto& rig=library_->rig;
        for(unsigned lane=0;lane<2;++lane)if(library_->profile.lane_mask&(1u<<lane)){
            const auto* s=snapshot(motions[lane]);if(!s||s->lane_!=lane||!character_pose_detail::local(rig,s->local_pose_))return fail();
            for(unsigned i=rig.cuts[lane];i<rig.cuts[lane+1];++i){const auto slot=rig.order[i];pose.bones[slot]=s->local_pose_.bones[slot];}
        }
        if(!character_pose_detail::local(rig,pose))return fail();
        *out=std::move(pose);if(error)error->clear();return true;
    }
    bool select(unsigned lane,const CharacterProgramDescriptor& d,bool mirrored,const CharacterProgramMotion& previous,
                 f32 start,f32 delta,CharacterProgramMotion* out)const override{
        return select_impl(lane,d,mirrored,previous,start,delta,0,0,out);
    }
    bool select_transition(unsigned lane,const CharacterProgramDescriptor& d,bool mirrored,const CharacterProgramMotion& previous,
                           f32 start,f32 delta,u8 mode,u8 duration,CharacterProgramMotion* out)const override{
        if(!library_||!library_->transitions||(mode!=0&&mode!=2&&!(mode==1&&library_->frozen_target))||(mode==0&&duration))return false;
        return select_impl(lane,d,mirrored,previous,start,delta,mode,duration,out);
    }
private:
    bool select_impl(unsigned lane,const CharacterProgramDescriptor& d,bool mirrored,const CharacterProgramMotion& previous,
                      f32 start,f32 delta,u8 mode,u8 duration,CharacterProgramMotion* out,const character_motion_driver_detail::DirectionSeed* seed=nullptr)const{
        if(!out||!library_||lane>=2||!(library_->profile.lane_mask&(1u<<lane))||!character_program_vm_detail::motion(previous,library_->transitions)||!character_program_vm_detail::time(start)||!rate(delta))return false;
        const auto* declared=character_program_detail::descriptor(library_->profile,d.group,d.choice);
        if(!declared||declared->bytes!=d.bytes)return false;
        const unsigned at=4+lane*8;const auto group=d.bytes[at],index=d.bytes[at+1];
        // Common bank and loop/rewind descriptors need separate AObj branches.
        if(group>=16||index>=128||(!library_->transitions&&(d.bytes[at+2]&4)))return false;
        const CharacterMotionSnapshot* old=nullptr;
        if(previous.selected){old=snapshot(previous);if(!old||old->lane_!=lane)return false;}
        else if(previous.context||previous.elapsed||previous.frame||previous.end_frame||previous.finished||previous.rewound)return false;
        if(old&&old->directional_target_&&mode)return false; //must freeze the displayed dual pose, not its raw centre
        bool expected=old?old->mirrored_:false;const auto mirror=d.bytes[at+2]&3;
        if(mirror==3)expected=!expected;else if(mirror)expected=mirror==2;
        if(expected!=mirrored)return false;
        unsigned clip_index=unsigned(library_->clips.size());
        for(unsigned i=0;i<library_->clips.size();++i)if(library_->clips[i].group==group&&library_->clips[i].index==index)clip_index=i;
        if(clip_index==library_->clips.size())return false; // no invented null-bank fallback
        auto next=std::shared_ptr<CharacterMotionSnapshot>(new CharacterMotionSnapshot);
        next->library_=library_;next->lane_=lane;next->clip_=clip_index;next->mirrored_=mirrored;
        next->source_context_=previous.context;next->root_scale_selector_=std::bit_cast<s8>(d.bytes[0]);next->evaluation_=seed?CharacterMotionEvaluation::Directional:CharacterMotionEvaluation::Request;
        next->looping_=(d.bytes[at+2]&4)!=0;next->loop_start_=d.bytes[at+3];
        if((mode==1||mode==2)&&duration){
            next->blend_duration_=f32(unsigned(duration)+1);if(delta>next->blend_duration_)return false; // source extrapolation not admitted
            next->blend_time_=delta;next->old_weight_=f32(1-f32(delta/next->blend_duration_));next->blending_=true;next->blend_mode_=mode;
            next->frozen_pose_=old?old->local_pose_:library_->initial_pose;
            if(!character_pose_detail::local(library_->rig,next->frozen_pose_))return false;
        }
        next->previous_elapsed_=0;next->previous_frame_=start; //8004871C
        next->observed_=previous;next->observed_.context.reset();next->observed_.selected=true;next->observed_.elapsed=0;next->observed_.frame=start;next->observed_.finished=0;
        if(seed){next->directional_target_=true;next->observed_.elapsed=seed->elapsed;next->previous_elapsed_=seed->previous_elapsed;next->previous_frame_=seed->previous_frame;next->blending_=seed->blending;next->blend_mode_=seed->mode;next->blend_time_=seed->time;next->blend_duration_=seed->duration;next->old_weight_=seed->weight;}
        if(!bind_bones(*next,delta,false)||!publish_pose(*next))return false;
        return commit(next,out);
    }
public:
    bool advance(unsigned lane,const CharacterProgramMotion& previous,f32 delta,CharacterProgramMotion* out)const override{
        const auto* old=snapshot(previous);if(!out||!old||lane!=old->lane_||!rate(delta)||(old->directional_target_&&old->blending_))return false;
        auto next=std::shared_ptr<CharacterMotionSnapshot>(new CharacterMotionSnapshot(*old));
        next->source_context_=previous.context;next->evaluation_=old->directional_target_?CharacterMotionEvaluation::Directional:CharacterMotionEvaluation::Advance;
        next->previous_elapsed_=previous.elapsed;next->previous_frame_=previous.frame; //80049E58
        next->observed_.elapsed+=delta;
        if(next->blending_){next->blend_time_=f32(next->blend_time_+delta);
            if(!old->directional_target_)next->evaluation_=CharacterMotionEvaluation::FrozenBlend;
            if(next->blend_time_>=next->blend_duration_){
                if(!old->directional_target_)next->evaluation_=CharacterMotionEvaluation::ReRequest;
                next->observed_.frame=f32(next->observed_.frame+f32(next->blend_time_-next->blend_duration_));
                //4A00C/4A0E0 re-request (request flag1), then clear2F. During that request
                //4B76C still observes blending and suppresses finished.
                if(!bind_bones(*next,delta,true))return false;
                next->blending_=false;next->blend_mode_=0;
            }else{next->old_weight_=f32(1-f32(next->blend_time_/next->blend_duration_));
                if(next->blend_mode_==2){next->observed_.frame+=delta;for(unsigned j=library_->rig.cuts[lane];j<library_->rig.cuts[lane+1];++j)if(!interpret(*next,library_->rig.order[j],delta,false))return false;}
                else if(next->blend_mode_!=1)return false;
            }
        }else{next->observed_.frame+=delta;for(unsigned j=library_->rig.cuts[lane];j<library_->rig.cuts[lane+1];++j)if(!interpret(*next,library_->rig.order[j],delta,false))return false;}
        return publish_pose(*next)&&commit(next,out);
    }
    bool request_directional_follow(unsigned lane,const std::array<u8,6>& controls,s16 steering,u8 default_group,
                                     f32 start,f32 delta,std::array<CharacterProgramMotion,2> previous,
                                     std::array<u8,2> current_models,std::shared_ptr<const CharacterDirectionalMotionSnapshot>* out)const{
        if(out)out->reset();
        if(!out||!library_||!library_->directional||lane>=2||!direction_model_scope(lane)||!models_valid(current_models)||!rate(delta)||!character_program_vm_detail::time(start))return false;
        const auto direction=character_motion_direction(controls,steering);
        const auto* centre=direction_descriptor(direction.centre,default_group);const auto* side=direction_descriptor(direction.side,default_group);
        if(!centre||!side)return false;
        auto next=std::shared_ptr<CharacterDirectionalMotionSnapshot>(new CharacterDirectionalMotionSnapshot);
        next->library_=library_;next->lane_=lane;next->primary_target_=direction.centre;next->side_target_=direction.side;next->side_weight_=direction.side_weight;next->side_rebound_=true;
        next->primary_descriptor_=*centre;next->side_descriptor_=*side;
        const auto* old_side=previous[1].selected?snapshot(previous[1]):nullptr;if(previous[1].selected&&!old_side)return false;
        auto side_seed=old_side?direction_seed(*old_side):character_motion_driver_detail::DirectionSeed{};side_seed.elapsed=0;side_seed.previous_elapsed=0;side_seed.previous_frame=start;
        //49B98 copies clocks before either request; both start at the same
        //requested frame, even if primary callbacks later overwrite its frame.
        if(!direction_request(lane,*centre,previous[0],start,delta,&next->motions_[0])||
           !direction_request(lane,*side,previous[1],start,delta,&next->motions_[1],&side_seed))return false;
        next->centre_models_=descriptor_models(*centre);next->side_models_=descriptor_models(*side);next->models_=lane==0?next->centre_models_:current_models;
        if(!direction_pose(*next))return false;
        *out=std::move(next);return true;
    }
    bool advance_directional_follow(std::shared_ptr<const CharacterDirectionalMotionSnapshot> previous,
                                     const std::array<u8,6>& controls,s16 steering,u8 default_group,f32 delta,
                                     std::array<u8,2> current_models,std::shared_ptr<const CharacterDirectionalMotionSnapshot>* out)const{
        if(out)out->reset();
        if(!out||!library_||!library_->directional||!previous||previous->library_!=library_||!models_valid(current_models)||!rate(delta))return false;
        const auto* primary=snapshot(previous->motions_[0]);const auto* alternate=snapshot(previous->motions_[1]);
        if(!primary||!alternate||previous->blending_||primary->blending_||(alternate->blending_&&!alternate->directional_target_))return false;
        const auto direction=character_motion_direction(controls,steering);const auto target=direction.side;
        const bool changed=target.group!=previous->side_target_.group||target.index!=previous->side_target_.index;
        auto next=std::shared_ptr<CharacterDirectionalMotionSnapshot>(new CharacterDirectionalMotionSnapshot(*previous));
        next->side_weight_=direction.side_weight;next->side_rebound_=changed;next->models_=current_models;
        //49F10 increments PRIMARY clocks, then49CC4/49DAC copies all four
        //into alternate BEFORE primary/alternate callbacks. Do not advance the
        //alternate's independent metadata clock or re-request a matching target.
        const f32 elapsed=f32(primary->observed_.elapsed+delta),frame=f32(primary->observed_.frame+delta);
        if(!character_program_vm_detail::accumulated_time(elapsed,true)||!character_program_vm_detail::time(frame)||
           !advance(next->lane_,previous->motions_[0],delta,&next->motions_[0]))return false;
        std::shared_ptr<CharacterMotionSnapshot> side;
        if(changed){const auto* descriptor=direction_descriptor(target,default_group);CharacterProgramMotion requested;
            auto side_seed=direction_seed(*alternate);side_seed.elapsed=elapsed;side_seed.previous_elapsed=primary->observed_.elapsed;side_seed.previous_frame=primary->observed_.frame;
            if(!descriptor||!direction_request(next->lane_,*descriptor,previous->motions_[1],frame,delta,&requested,&side_seed))return false;
            side=std::shared_ptr<CharacterMotionSnapshot>(new CharacterMotionSnapshot(*snapshot(requested)));
            next->side_models_=descriptor_models(*descriptor);next->side_target_=target;next->side_descriptor_=*descriptor;
        }else{
            side=std::shared_ptr<CharacterMotionSnapshot>(new CharacterMotionSnapshot(*alternate));side->observed_.frame=frame;
            for(unsigned j=library_->rig.cuts[next->lane_];j<library_->rig.cuts[next->lane_+1];++j)if(!interpret(*side,library_->rig.order[j],delta,false))return false;
            if(!publish_pose(*side))return false;
        }
        side->observed_.elapsed=elapsed;side->previous_elapsed_=primary->observed_.elapsed;side->previous_frame_=primary->observed_.frame;
        if(!commit(side,&next->motions_[1])||!direction_pose(*next))return false;
        *out=std::move(next);return true;
    }
    // Source48F9C/49210. The ordinary-motion entry requires explicit primary
    // model-buffer bytes: source4B94C copies1D74/75, NOT displayed1D94/95.
    bool request_directional_transition(unsigned lane,const std::array<u8,6>& controls,s16 steering,u8 default_group,
                                        f32 start,f32 delta,u8 duration,CharacterProgramMotion previous,
                                        std::array<u8,2> primary_models,std::array<u8,2> current_models,
                                        std::shared_ptr<const CharacterDirectionalMotionSnapshot>* out)const{
        if(out)out->reset();
        if(!library_||!library_->directional_transitions)return false;
        const auto* old=previous.selected?snapshot(previous):nullptr;
        if(previous.selected&&(!old||old->lane_!=lane||old->directional_target_))return false;
        return direction_transition_request(lane,controls,steering,default_group,start,delta,duration,{previous,{}},old?old->local_pose_:library_->initial_pose,primary_models,current_models,out);
    }
    bool interrupt_directional_transition(std::shared_ptr<const CharacterDirectionalMotionSnapshot> previous,
                                          const std::array<u8,6>& controls,s16 steering,u8 default_group,
                                          f32 start,f32 delta,u8 duration,std::array<u8,2> current_models,
                                          std::shared_ptr<const CharacterDirectionalMotionSnapshot>* out)const{
        if(out)out->reset();
        if(!library_||!library_->directional_transitions||!previous||previous->library_!=library_)return false;
        return direction_transition_request(previous->lane_,controls,steering,default_group,start,delta,duration,previous->motions_,previous->pose_,previous->centre_models_,current_models,out);
    }
    bool advance_directional_transition(std::shared_ptr<const CharacterDirectionalMotionSnapshot> previous,
                                        const std::array<u8,6>& controls,s16 steering,u8 default_group,f32 delta,
                                        std::array<u8,2> current_models,std::shared_ptr<const CharacterDirectionalMotionSnapshot>* out)const{
        if(out)out->reset();
        if(!out||!library_||!library_->directional_transitions||!previous||previous->library_!=library_||!models_valid(current_models)||!rate(delta))return false;
        if(!previous->blending_)return advance_directional_follow(previous,controls,steering,default_group,delta,current_models,out);
        const auto* primary=snapshot(previous->motions_[0]);const auto* alternate=snapshot(previous->motions_[1]);if(!primary||!alternate)return false;
        auto next=std::shared_ptr<CharacterDirectionalMotionSnapshot>(new CharacterDirectionalMotionSnapshot(*previous));const auto direction=character_motion_direction(controls,steering);
        next->side_weight_=direction.side_weight;next->models_=current_models;next->blend_time_=f32(next->blend_time_+delta);
        auto primary_seed=direction_seed(*primary);primary_seed.elapsed=f32(primary->observed_.elapsed+delta);primary_seed.previous_elapsed=primary->observed_.elapsed;primary_seed.previous_frame=primary->observed_.frame;primary_seed.time=next->blend_time_;
        auto side_seed=direction_seed(*alternate);side_seed.elapsed=primary_seed.elapsed;side_seed.previous_elapsed=primary_seed.previous_elapsed;side_seed.previous_frame=primary_seed.previous_frame;
        if(!character_program_vm_detail::accumulated_time(primary_seed.elapsed,true))return false;
        if(next->blend_time_>=next->blend_duration_){
            const auto* centre=direction_descriptor(direction.centre,default_group);const auto* side=direction_descriptor(direction.side,default_group);
            const f32 frame=f32(primary->observed_.frame+f32(next->blend_time_-next->blend_duration_));
            if(!centre||!side||!direction_request(next->lane_,*centre,previous->motions_[0],frame,delta,&next->motions_[0],&primary_seed)||
               !direction_request(next->lane_,*side,previous->motions_[1],frame,delta,&next->motions_[1],&side_seed))return false;
            next->primary_target_=direction.centre;next->side_target_=direction.side;next->centre_models_=descriptor_models(*centre);next->side_models_=descriptor_models(*side);next->side_rebound_=true;
            next->primary_descriptor_=*centre;next->side_descriptor_=*side;
            if(next->lane_==0)next->models_=next->centre_models_;
            if(!direction_pose(*next))return false;
            //49B98 samples while primary2F is still6; only then is2F cleared.
            auto completed=std::shared_ptr<CharacterMotionSnapshot>(new CharacterMotionSnapshot(*snapshot(next->motions_[0])));completed->blending_=false;completed->blend_mode_=0;
            if(!commit(completed,&next->motions_[0]))return false;
            next->blending_=false;next->directional_pose_=next->pose_;
        }else{
            next->old_weight_=f32(1-f32(next->blend_time_/next->blend_duration_));primary_seed.weight=next->old_weight_;
            const bool changed=direction.side.group!=previous->side_target_.group||direction.side.index!=previous->side_target_.index;next->side_rebound_=changed;
            if(changed){const auto* descriptor=direction_descriptor(direction.side,default_group);
                if(!descriptor||!direction_request(next->lane_,*descriptor,previous->motions_[1],primary->observed_.frame,delta,&next->motions_[1],&side_seed))return false;
                next->side_target_=direction.side;next->side_models_=descriptor_models(*descriptor);next->side_descriptor_=*descriptor;
            }else if(!direction_step(previous->motions_[1],delta,primary->observed_.frame,side_seed,&next->motions_[1]))return false;
            if(!direction_step(previous->motions_[0],delta,primary->observed_.frame,primary_seed,&next->motions_[0])||!direction_transition_pose(*next))return false;
        }
        *out=std::move(next);return true;
    }
    bool rebase_directional_models(std::shared_ptr<const CharacterDirectionalMotionSnapshot> previous,
                                    std::array<u8,2> primary,std::array<u8,2> alternate,std::array<u8,2> displayed,
                                    std::shared_ptr<const CharacterDirectionalMotionSnapshot>* out)const{
        if(out)out->reset();
        if(!out||!library_||!previous||previous->library_!=library_||!models_valid(primary)||!models_valid(alternate)||!models_valid(displayed))return false;
        auto next=std::shared_ptr<CharacterDirectionalMotionSnapshot>(new CharacterDirectionalMotionSnapshot(*previous));next->centre_models_=primary;next->side_models_=alternate;next->models_=displayed;*out=std::move(next);return true;
    }
    bool select_from_directional(unsigned lane,const CharacterProgramDescriptor& descriptor,bool mirrored,
                                 std::shared_ptr<const CharacterDirectionalMotionSnapshot> previous,f32 start,f32 delta,u8 mode,u8 duration,CharacterProgramMotion* out)const{
        if(!out||!library_||!library_->directional_transitions||!previous||previous->library_!=library_||previous->lane_!=lane||(mode!=0&&mode!=1&&mode!=2)||(!mode&&duration))return false;
        const auto* raw=snapshot(previous->primary());if(!raw)return false;
        auto displayed=std::shared_ptr<CharacterMotionSnapshot>(new CharacterMotionSnapshot(*raw));displayed->directional_target_=false;displayed->local_pose_=previous->pose_;CharacterProgramMotion seed;
        if(!commit(displayed,&seed))return false;
        return select_impl(lane,descriptor,mirrored,seed,start,delta,mode,duration,out);
    }
private:
    static character_motion_driver_detail::DirectionSeed direction_seed(const CharacterMotionSnapshot& snapshot){return {snapshot.observed_.elapsed,snapshot.previous_elapsed_,snapshot.previous_frame_,snapshot.blend_time_,snapshot.blend_duration_,snapshot.old_weight_,snapshot.blending_,snapshot.blend_mode_};}
    bool direction_transition_request(unsigned lane,const std::array<u8,6>& controls,s16 steering,u8 default_group,
                                      f32 start,f32 delta,u8 duration,std::array<CharacterProgramMotion,2> previous,
                                      const CharacterLocalPose& frozen,std::array<u8,2> frozen_models,std::array<u8,2> current_models,
                                      std::shared_ptr<const CharacterDirectionalMotionSnapshot>* out)const{
        if(!out||!library_||!library_->directional_transitions||lane>=2||!direction_model_scope(lane)||!models_valid(frozen_models)||!models_valid(current_models)||!rate(delta)||!character_program_vm_detail::time(start)||!character_pose_detail::local(library_->rig,frozen))return false;
        if(!duration)return request_directional_follow(lane,controls,steering,default_group,start,delta,previous,current_models,out);
        const f32 total=f32(unsigned(duration)+1);if(delta>total)return false;
        const auto direction=character_motion_direction(controls,steering);const auto* centre=direction_descriptor(direction.centre,default_group);const auto* side=direction_descriptor(direction.side,default_group);if(!centre||!side)return false;
        const auto* old=previous[0].selected?snapshot(previous[0]):nullptr;if(previous[0].selected&&(!old||old->lane_!=lane))return false;
        auto next=std::shared_ptr<CharacterDirectionalMotionSnapshot>(new CharacterDirectionalMotionSnapshot);next->library_=library_;next->lane_=lane;next->primary_target_=direction.centre;next->side_target_=direction.side;next->side_weight_=direction.side_weight;next->side_rebound_=true;
        next->primary_descriptor_=*centre;next->side_descriptor_=*side;
        next->blending_=true;next->blend_time_=delta;next->blend_duration_=total;next->old_weight_=f32(1-f32(delta/total));next->frozen_pose_=frozen;next->frozen_models_=frozen_models;
        character_motion_driver_detail::DirectionSeed primary_seed{0,0,start,delta,total,next->old_weight_,true,6};auto side_seed=old?direction_seed(*old):character_motion_driver_detail::DirectionSeed{};side_seed.elapsed=0;side_seed.previous_elapsed=0;side_seed.previous_frame=start;
        //4B94C clones primary motion metadata into alternate;49210 requests
        //SIDE first into scratch, then CENTRE into primary, without changing
        //the saved displayed pose/quaternion buffer.
        if(!direction_request(lane,*side,previous[0],start,delta,&next->motions_[1],&side_seed)||
           !direction_request(lane,*centre,previous[0],start,delta,&next->motions_[0],&primary_seed))return false;
        next->centre_models_=descriptor_models(*centre);next->side_models_=descriptor_models(*side);next->models_=lane==0?next->centre_models_:current_models;
        if(!direction_transition_pose(*next))return false;
        *out=std::move(next);return true;
    }
    bool direction_step(const CharacterProgramMotion& previous,f32 delta,f32 frame,const character_motion_driver_detail::DirectionSeed& seed,CharacterProgramMotion* out)const{
        const auto* old=snapshot(previous);if(!old||!out||!rate(delta)||!character_program_vm_detail::time(frame))return false;
        auto next=std::shared_ptr<CharacterMotionSnapshot>(new CharacterMotionSnapshot(*old));next->observed_.frame=frame;next->observed_.elapsed=seed.elapsed;next->previous_elapsed_=seed.previous_elapsed;next->previous_frame_=seed.previous_frame;next->blend_time_=seed.time;next->old_weight_=seed.weight;
        //496D8 calls A830 with flags0 for both retained targets. ADC8 therefore
        //does NOT advance AObj/FObj or write45; B76C still copies each visited
        //JObj's pose/frame/finished fields. A changed side separately re-requests.
        for(unsigned i=library_->rig.cuts[next->lane_];i<library_->rig.cuts[next->lane_+1];++i)if(!copy_pose_callback(*next,library_->rig.order[i]))return false;
        return publish_pose(*next)&&commit(next,out);
    }
    bool direction_transition_pose(CharacterDirectionalMotionSnapshot& state)const{
        const auto* centre=snapshot(state.motions_[0]);const auto* side=snapshot(state.motions_[1]);if(!centre||!side)return false;const auto& rig=library_->rig;auto pose=centre->local_pose_;const f32 w=state.side_weight_,cw=f32(1-w);
        for(unsigned i=rig.cuts[state.lane_];i<rig.cuts[state.lane_+1];++i){const auto slot=rig.order[i];auto& p=pose.bones[slot];const auto& s=side->local_pose_.bones[slot];
            for(unsigned k=0;k<3;++k)p.translation[k]=s.translation[k]*w+p.translation[k]*cw;
            f32 q[4];if(!character_pose_detail::blend_kernel(s.quaternion,p.quaternion,cw,q))return false;std::copy_n(q,4,p.quaternion);
            for(unsigned model=0;model<2;++model)if(library_->model_bones[model]==s8(slot)){if(cw>.5f)state.centre_models_[model]=state.side_models_[model];break;}
        }
        if(!character_pose_detail::local(rig,pose))return false;
        state.directional_pose_=pose;
        if(!character_pose_detail::blend_lane(rig,state.lane_,state.old_weight_,state.frozen_pose_.bones,pose.bones)||!character_pose_detail::local(rig,pose))return false;
        // Side D244 writes buffer1's model bytes even while the old POSE in
        // buffer1 stays frozen.4A8CC therefore reads the current side models,
        // not the copied pre-transition primary model bytes.
        for(unsigned i=rig.cuts[state.lane_];i<rig.cuts[state.lane_+1];++i)for(unsigned model=0;model<2;++model)if(library_->model_bones[model]==s8(rig.order[i])){state.models_[model]=state.old_weight_>.5f?state.side_models_[model]:state.centre_models_[model];break;}
        state.pose_=std::move(pose);return true;
    }
    static bool models_valid(const std::array<u8,2>& models){return models[0]<128&&models[1]<128;}
    bool direction_model_scope(unsigned lane)const{
        // D244 lane1 does not replace the global primary/alternate model bytes.
        // A matching model bone there needs a shared actor-owned buffer pair;
        // this independent lane component cannot fabricate that provenance.
        if(lane==0)return true;
        for(unsigned i=library_->rig.cuts[lane];i<library_->rig.cuts[lane+1];++i)for(auto bone:library_->model_bones)if(bone==s8(library_->rig.order[i]))return false;
        return true;
    }
    const CharacterProgramDescriptor* direction_descriptor(CharacterMotionTarget target,u8 default_group)const{
        if(target.group>=128||target.index>=128||default_group>=128)return nullptr; //common bank remains separate
        return character_program_detail::descriptor(library_->profile,target.group?target.group:default_group,target.index);
    }
    std::array<u8,2> descriptor_models(const CharacterProgramDescriptor& descriptor)const{
        std::array<u8,2> models{};for(unsigned i=0;i<2;++i){const auto raw=descriptor.bytes[9+i];models[i]=character_program_detail::signed_byte(raw)<0?library_->profile.default_models[i]:raw;}return models;
    }
    bool direction_request(unsigned lane,const CharacterProgramDescriptor& descriptor,const CharacterProgramMotion& previous,f32 frame,f32 delta,CharacterProgramMotion* out,const character_motion_driver_detail::DirectionSeed* seed=nullptr)const{
        const auto* before=previous.selected?snapshot(previous):nullptr;if(previous.selected&&!before)return false;
        bool mirrored=before?before->mirrored_:false;const auto mode=descriptor.bytes[6+lane*8]&3;if(mode==3)mirrored=!mirrored;else if(mode)mirrored=mode==2;
        const character_motion_driver_detail::DirectionSeed instant{0,0,frame};
        return select_impl(lane,descriptor,mirrored,previous,frame,delta,0,0,out,seed?seed:&instant);
    }
    bool direction_pose(CharacterDirectionalMotionSnapshot& state)const{
        const auto* centre=snapshot(state.motions_[0]);const auto* side=snapshot(state.motions_[1]);const auto& rig=library_->rig;
        if(!centre||!side||centre->lane_!=state.lane_||side->lane_!=state.lane_||!std::isfinite(state.side_weight_)||state.side_weight_<0||state.side_weight_>2)return false;
        auto pose=centre->local_pose_;const f32 weight=state.side_weight_;
        for(unsigned i=rig.cuts[state.lane_];i<rig.cuts[state.lane_+1];++i){const auto slot=rig.order[i];auto& target=pose.bones[slot];const auto& other=side->local_pose_.bones[slot];
            for(unsigned k=0;k<3;++k)target.translation[k]=target.translation[k]*(1-weight)+other.translation[k]*weight;
            f32 q[4];if(!character_pose_detail::blend_kernel(target.quaternion,other.quaternion,weight,q))return false;
            std::copy_n(q,4,target.quaternion);
            for(unsigned model=0;model<2;++model)if(library_->model_bones[model]==s8(slot)){state.models_[model]=weight>.5f?state.side_models_[model]:state.centre_models_[model];break;}
        }
        if(!character_pose_detail::local(rig,pose))return false;
        state.pose_=std::move(pose);return true;
    }
    static hsd_anim_detail::TrackMachine requested_track(const HsdAnimTrack& t,f32 frame,const hsd_anim_detail::TrackMachine* old=nullptr){
        hsd_anim_detail::TrackMachine m;m.data=t.data.data();m.length=t.data.size();m.frac_value=t.frac_value;m.frac_slope=t.frac_slope;m.time=f32(t.startframe+frame);m.float_time=true;
        if(old){m.flags=old->flags&~0x40u;m.callbacks=old->callbacks;m.value=old->value;}return m;
    }
    static bool bind_bones(CharacterMotionSnapshot& state,f32 delta,bool rebind){
        const auto& lib=*state.library_;const auto& clip=lib.clips[state.clip_].clip;const auto lane=state.lane_;
        auto old=std::move(state.bones_);state.bones_={};state.bones_.resize(clip.bones.size());state.sample_={};state.sample_.bones.resize(clip.bones.size());state.root_callback_seen_=false;
        for(unsigned i=0;i<clip.bones.size();++i)state.sample_.bones[i].joint_offset=clip.bones[i].joint_offset;
        for(unsigned j=lib.rig.cuts[lane];j<lib.rig.cuts[lane+1];++j){const auto slot=lib.rig.order[j];if(slot>=clip.public_bone_count){state.observed_.end_frame=0;state.observed_.frame=0;continue;}
            const auto& source=clip.bones[slot];if(!source.has_aobj){root_callback_clock(state,slot);continue;}auto& player=state.bones_[slot];player.has_aobj=true;player.first_play=true;player.frame=state.observed_.frame;
            for(unsigned t=0;t<source.tracks.size();++t){const auto* prior=rebind&&slot<old.size()&&t<old[slot].tracks.size()?&old[slot].tracks[t]:nullptr;player.tracks.push_back(requested_track(source.tracks[t],player.frame,prior));}
            state.observed_.end_frame=s16(clip.end_frame);state.observed_.rewound=0;if(!interpret(state,slot,delta,true))return false;
        }return true;
    }
    static bool publish_pose(CharacterMotionSnapshot& state){
        const auto& lib=*state.library_;if(!lib.transitions)return true;
        if(!character_pose_detail::sample(lib.rig,state.sample_))return false;
        auto pose=lib.initial_pose;
        if(!character_pose_detail::assign_lane(lib.rig,state.lane_,state.sample_,state.mirrored_,pose.bones))return false;
        if(state.blending_&&!state.directional_target_&&!character_pose_detail::blend_lane(lib.rig,state.lane_,state.old_weight_,state.frozen_pose_.bones,pose.bones))return false;
        if(!character_pose_detail::local(lib.rig,pose))return false;
        state.local_pose_=std::move(pose);return true;
    }
    static bool stream(const HsdAnimTrack& track){
        hsd_anim_detail::TrackMachine m;m.data=track.data.data();m.length=track.data.size();m.frac_value=track.frac_value;m.frac_slope=track.frac_slope;std::string error;
        for(unsigned steps=0;steps<hsd_anim_detail::kMaxTrackSteps;++steps){
            if(m.pos==m.length)return true;
            u32 next=0;if(!hsd_anim_detail::load_data(m,&next,&error)||!next)return false;
            if(!std::isfinite(m.p1)||!std::isfinite(m.d1))return false;
            if(m.op==5)continue;
            if(!hsd_anim_detail::load_wait(m,&next,&error)||m.fterm>65535)return false;
            if(next==6)return true;
            m.state=2;
        }
        return false;
    }
    static bool rate(f32 delta){return std::isfinite(delta)&&delta>=0&&delta<=16;}
    static bool apply(CharacterMotionSnapshot& state,unsigned bone,unsigned track,f64 delta){
        auto& m=state.bones_[bone].tracks[track];const auto before=m.callbacks;std::string error;
        if(!hsd_anim_detail::track_step(m,&error,delta)||m.stopped)return false;
        if(m.callbacks!=before){const f32 value=f32(m.value);if(!std::isfinite(value)||std::abs(value)>1e7f)return false;
            auto& sample=state.sample_.bones[bone];const auto channel=state.library_->clips[state.clip_].clip.bones[bone].tracks[track].channel;sample.present|=u16(1u<<channel);sample.value[channel]=value;}
        return true;
    }
    static bool interpret(CharacterMotionSnapshot& state,unsigned slot,f32 delta,bool initial){
        auto& player=state.bones_[slot];if(!player.has_aobj){root_callback_clock(state,slot);return true;}
        const auto& clip=state.library_->clips[state.clip_].clip;
        if(!player.stopped){
            f32 step=player.first_play?0:delta;if(!player.first_play)player.frame+=delta;player.first_play=false;player.rewound=false;
            if(state.looping_&&player.frame>=clip.end_frame){
                if(clip.end_frame<=state.loop_start_)player.frame=clip.end_frame;
                else{
                    for(unsigned i=0;i<player.tracks.size();++i){auto& m=player.tracks[i];if(m.op_intrp==6&&!apply(state,slot,i,step))return false;m.state=0;}
                    const f32 numerator=f32(player.frame-f32(state.loop_start_)),denominator=f32(clip.end_frame-f32(state.loop_start_));
                    player.frame=f32(f64(state.loop_start_)+std::fmod(f64(numerator),f64(denominator)));
                    for(unsigned i=0;i<player.tracks.size();++i){const auto old=player.tracks[i];player.tracks[i]=requested_track(clip.bones[slot].tracks[i],player.frame,&old);}
                }
                step=0;player.rewound=true;
            }
            for(unsigned i=0;i<player.tracks.size();++i){if(!apply(state,slot,i,step))return false;
                // Do not fabricate a value for an initial track that never
                // invokes its scalar callback (ambiguous/uninitialized path).
                if(initial&&!player.tracks[i].callbacks)return false;
            }
            if(!state.looping_&&player.frame>=clip.end_frame){
                //8024895C flushes ONLY current KEY interpolation at the supplied
                // framerate, then clears FObj state. The AObj frame does NOT
                // advance again during this stop flush.
                for(unsigned i=0;i<player.tracks.size();++i){auto& m=player.tracks[i];if(m.op_intrp==6&&!apply(state,slot,i,delta))return false;m.state=0;}
                player.stopped=true;
            }
        }
        //8004B76C after every visited bone, even an already-stopped AObj.
        state.observed_.frame=player.frame;
        root_callback_clock(state,slot);
        if(!state.blending_&&state.observed_.end_frame<=state.observed_.frame)state.observed_.finished=1;
        if(!initial)state.observed_.rewound=player.rewound?1:0;
        return character_program_vm_detail::time(player.frame);
    }
    static bool copy_pose_callback(CharacterMotionSnapshot& state,unsigned slot){
        const auto& player=state.bones_[slot];
        if(player.has_aobj){state.observed_.frame=player.frame;if(!state.blending_&&state.observed_.end_frame<=state.observed_.frame)state.observed_.finished=1;}
        root_callback_clock(state,slot);return character_program_vm_detail::time(state.observed_.frame);
    }
    static void root_callback_clock(CharacterMotionSnapshot& state,unsigned slot){if(!slot){state.root_callback_seen_=true;state.root_callback_clock_=state.observed_.frame;}}
    static bool commit(const std::shared_ptr<CharacterMotionSnapshot>& state,CharacterProgramMotion* out){
        if(!character_program_vm_detail::motion(state->observed_,state->library_->transitions))return false;
        auto result=state->observed_;result.context=state;*out=std::move(result);return true;
    }
    std::shared_ptr<const character_motion_driver_detail::Library> library_;
};
} // namespace gf
