#pragma once
#include "engine/assets/battle_views.hpp"
#include "engine/graphics/camera_transform.hpp"
#include <memory>
#include <optional>
#include <exception>
#include "engine/gameplay/battle_camera_pose.hpp"
#include "engine/gameplay/battle_camera_follow.hpp"
#include "engine/gameplay/challenge_stage_inputs.hpp"
#include "engine/gameplay/scene_object_pool.hpp"
#include "engine/gameplay/collision_contact_reset.hpp"
namespace gf {
struct BattleViewRecord {
    u8 active00=0,index10=0,group11=0,kind83=0,index2e5=0;
    std::array<f32,3> eye2e8{},up9c{},target300{};f32 distance348=0;
    Mat4 transform114,view394; //written by actual identity/view operations below
    u8 phase18=0,phase19=0,phase1a=0,phase1b=0,initialized2e4=0,mode2e6=0,alternate2e7=0;
    u8 collision86=0,field1d8=0,contact1da=0,roll360=0,shake35d=0;
    bool self90=false;u32 parameter27c=0;u64 pose_generation=0;
    f32 scale_b4=0,scale_c0=0,height350=0,scale354=0;std::array<f32,3> scale58{},up330{},previous_eye2f4{},previous_target30c{},horizontal318{},position20{},previous2c{};
    std::array<s16,3> angles70{},hp1c4_1c6_1c8{};std::array<u8,4> tint358{};
    std::optional<Mat4> world364;
    u8 extended35e=0,obstruction35f=0;bool floor_queried=false;
    std::array<f32,3> height_probe_eye{};StageLowestFloorResult floor_probe;
    std::optional<BattleCameraFollowPlan> first_follow;
    CollisionContactState contact;u64 contact_generation=0;
    u8 visible26f=0;
};
struct BattleViewSceneInputs {u8 humans=0,active=0,players=0,layout29=0;std::optional<u8> split3d;std::array<s8,4> controller_c0_c3{{-1,-1,-1,-1}};};
struct BattleViewCollisionAttempt {u8 view=0,record=0;SceneObjectAcquire result=SceneObjectAcquire::Refused;SceneCollisionLease lease;};
//0BB70/common-root20B61C plus explicit first0BEF4/0BDA4 consumers. View poses
//remain separate from CObj projection, later tracking and full-frame authority.
class BattleViews {
public:
    BattleViews()=default;BattleViews(const BattleViews&)=delete;BattleViews& operator=(const BattleViews&)=delete;BattleViews(BattleViews&&)=default;BattleViews& operator=(BattleViews&&)=default;
    bool initialize(const BattleViewsProfile& profile,std::string* error){
        if(attempted_||!profile.ready()){if(error)*error="battle views: fresh source profile required";return false;}attempted_=true;
        try{auto next=std::make_unique<Storage>();const auto* eye=profile.vector(0);const auto* up=profile.vector(1);const auto* target=profile.vector(2);
            for(unsigned i=0;i<13;++i){auto& v=next->views[i];v.index10=u8(i);v.group11=u8(i/6);v.index2e5=u8(i%6);v.kind83=32;for(unsigned k=0;k<3;++k){v.eye2e8[k]=eye[k];v.up9c[k]=up[k];v.target300[k]=target[k];}f32 sum=0;for(unsigned k=0;k<3;++k){const f32 d=f32(eye[k]-target[k]);sum=f32(sum+f32(d*d));}v.distance348=sum>0?f32(std::sqrt(sum)):sum;
                if(!camera_orientation_view_matrix(eye,target,up,&v.view394)){if(error)*error="battle views: initial axes invalid";return false;}v.transform114=mat4_identity();}
            next->viewer_indices.fill(-1);next->common_root_links.fill(true); //five typed SAME-root relationships, not pointer words
            next->reset_dc=0;next->reset_da=0;next->reset_d8=0; //10B50 byteDC,byteDA,halfD8 stores
            if(!camera_orientation_view_matrix(profile.vector(3),profile.vector(5),profile.vector(4),&next->common_root)){if(error)*error="battle views: common root axes invalid";return false;}
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){if(error)*error="battle views: uncertain native construction; no retry";return false;}
    }
    bool ready()const{return bool(storage_);}const BattleViewRecord* view(unsigned i)const{return ready()&&i<13?&storage_->views[i]:nullptr;}
    const Mat4* common_root()const{return ready()?&storage_->common_root:nullptr;}
    std::optional<s8> viewer_index(unsigned i)const{return ready()&&i<5?std::optional<s8>{storage_->viewer_indices[i]}:std::nullopt;}
    const Mat4* viewer_root(unsigned i)const{
        if(!ready()||i>=5)return nullptr;
        if(storage_->common_root_links[i])return &storage_->common_root;
        const auto index=storage_->viewer_indices[i];return index>=0&&index<13?&storage_->views[unsigned(index)].view394:nullptr;
    }
    std::optional<std::array<u16,3>> reset_fields_d8_da_dc()const{return ready()?std::optional<std::array<u16,3>>{{u16(storage_->reset_d8),storage_->reset_da,storage_->reset_dc}}:std::nullopt;}
    bool projection_and_active_camera_pending()const{return true;}
    const BattleViewSceneInputs* scene_inputs()const{return ready()&&storage_->inputs?&*storage_->inputs:nullptr;}
    bool initial_aliases_bound()const{const auto objects=storage_?storage_->objects.lock():nullptr;return ready()&&storage_->aliases_bound&&storage_->scope->live&&storage_->scope->bound&&!storage_->scope->arena.expired()&&objects&&objects->ready();}
    bool first_poses_ready()const{return initial_aliases_bound()&&storage_->pose_complete;}
    u64 pose_generation()const{return first_poses_ready()?storage_->pose_generation:0;}
    std::span<const BattleViewCollisionAttempt> collision_attempts()const{return first_poses_ready()?std::span<const BattleViewCollisionAttempt>{storage_->collisions}:std::span<const BattleViewCollisionAttempt>{};}
    void retire(){storage_.reset();attempted_=true;}
 private:
    friend class BattlePreparationBootstrap;
    struct ActorObservation {u8 phase18=0,behaviour43d=0,field3f1=0;BattleFirstCameraInput input;SceneActorReference reference;u64 world_generation=0;std::array<f32,3> target50c{};f32 width668=0;bool target508_bound=false;u8 lock7c9=0;};
    bool bind_scene_inputs(const BattleViewSceneInputs& input,const std::shared_ptr<SceneObjectPool>& objects,std::weak_ptr<ScenePhysicalControlFields> arena,std::string* error){
        if(!ready()||storage_->inputs||!objects||!objects->ready()||arena.expired()||!input.players||input.players>2||input.layout29!=input.players-1||(input.humans&~input.active)||input.active>=64||input.humans>=16||(input.layout29==1&&(!input.split3d||*input.split3d>1))){if(error)*error="battle views: current scene participant/display producers required";return false;}
        u8 seen=0;for(unsigned i=0;i<4;++i){const auto slot=input.controller_c0_c3[i];if(i>=input.players){if(slot!=-1)return false;continue;}if(slot<0||slot>=4||!(input.humans&(1u<<unsigned(slot)))||(seen&(1u<<unsigned(slot))))return false;seen|=u8(1u<<unsigned(slot));}
        if(seen!=input.humans)return false;
        storage_->inputs=input;storage_->objects=objects;storage_->scope->objects=objects->storage_;storage_->scope->arena=std::move(arena);storage_->scope->bound=true;if(error)error->clear();return true;
    }
    bool bind_initial_aliases(const BattleCameraEntryProfile& profile,const std::array<u8,6>& factory_active,std::string* error){
        if(!ready()||!profile.ready()||!storage_->inputs||storage_->aliases_bound||!storage_->scope->live){if(error)*error="battle view aliases: fresh factory-view source entry required";return false;}
        const auto& input=*storage_->inputs;u8 fallback=0;bool found=false;
        for(unsigned i=0;i<6;++i){if(factory_active[i]>1||bool(factory_active[i])!=bool(input.active&(1u<<i)))return false;if(!found&&(input.humans&(1u<<i))&&factory_active[i]){fallback=u8(i);found=true;}}
        //Factory43D=0 and initial view18=0 qualify10514=0. These are typed
        //same-record aliases to394; no projection or active camera is invented.
        const unsigned count=input.layout29==1?2:1;
        for(unsigned i=0;i<count;++i){
            s8 selected=input.controller_c0_c3[i];if(selected==-1||!(input.active&(1u<<unsigned(selected))))selected=s8(fallback);
            if(selected<0||selected>=6||storage_->views[unsigned(selected)].phase18)return false;
            if(input.layout29==1){for(unsigned alias:{i,i+2}){storage_->viewer_indices[alias]=selected;storage_->common_root_links[alias]=false;}}
            else for(unsigned alias=0;alias<4;++alias){storage_->viewer_indices[alias]=selected;storage_->common_root_links[alias]=false;}
            storage_->actor487[unsigned(selected)]=u8(i);
        }
        storage_->aliases_bound=true;if(error)error->clear();return true;
    }
    template<class CommitActor>
    bool activate_first_poses(const BattleCameraEntryProfile& profile,const std::array<ActorObservation,6>& actors,const BattleCommonResourceBinding& common,CommitActor&& commit_actor,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle first camera: ")+why;return false;};
        if(!initial_aliases_bound()||!profile.ready()||storage_->pose_attempted||!common.live())return reject("fresh source aliases/activation/common owner required");
        const auto pool=storage_->objects.lock();if(!pool||!pool->ready())return reject("same shared collision pool required");
        const auto bytes=common.block(0);if(!bytes.contains(244,4))return reject("source selector61 row");
        for(unsigned i=0;i<4&&bytes.data[244+i]!=255;++i)if(!bytes.contains(0x400+unsigned(bytes.data[244+i])*0x50,0x50))return reject("source selector61 descriptor extent");
        std::array<std::optional<BattleFirstCameraPose>,6> planned;
        for(unsigned i=0;i<6;++i){
            const auto& actor=actors[i];const bool active=bool(storage_->inputs->active&(1u<<i));
            if(!active){if(actor.phase18)return reject("unjoined non-factory inactive actor");continue;}
            if(actor.phase18!=1||actor.behaviour43d||actor.field3f1||actor.world_generation!=1||!actor.reference.live())return reject("same registered first-world ordinary camera inputs required");
            BattleFirstCameraPose p;if(!battle_first_camera_pose(profile,actor.input,&p,error))return false;planned[i]=p;
        }
        storage_->pose_attempted=true;
        auto fail=[&](const char* why){storage_->scope->live=false;return reject(why);};
        try{
            storage_->collisions.reserve(48);storage_->common=common;
            for(unsigned index=0;index<12;++index){
                const unsigned slot=index%6;if(!actors[slot].phase18)continue;
                auto& v=storage_->views[index];if(!(storage_->inputs->active&(1u<<slot))){v.active00=0;continue;}
                if(!planned[slot]||v.initialized2e4)return fail("view/source entry already consumed");
                const auto& p=*planned[slot];v.active00=1;v.scale354=p.scale354;v.initialized2e4=1;v.self90=true;v.scale_b4=v.scale_c0=1;v.scale58={1,1,1};
                v.parameter27c=0x802c4760;SceneViewReference self;self.scope_=storage_->scope;self.index_=u8(index);
                for(unsigned ordinal=0;ordinal<4&&bytes.data[244+ordinal]!=255;++ordinal){
                    BattleViewCollisionAttempt attempt;attempt.view=u8(index);attempt.record=bytes.data[244+ordinal];attempt.result=pool->acquire_collision(48,false,{},&attempt.lease,error);storage_->collisions.push_back(attempt);
                    if(attempt.result==SceneObjectAcquire::Refused)return fail("view collision allocator refused; prefix retained");
                    if(attempt.result!=SceneObjectAcquire::Allocated)continue;
                    SceneViewCollisionFields fields;fields.parent20=fields.parent24=fields.matrix_owner60=self;fields.source=common;fields.selector14=61;fields.record12=attempt.record;fields.descriptor_offset28=0x400+unsigned(attempt.record)*0x50;fields.parameter_key2c=v.parameter27c;fields.self5c={4,attempt.lease.index()};fields.parameter11=u8(bytes.be16(fields.descriptor_offset28+4));fields.start48=std::bit_cast<s16>(bytes.be16(fields.descriptor_offset28+6));fields.end4a=std::bit_cast<s16>(bytes.be16(fields.descriptor_offset28+8));
                    if(!pool->install_view_collision(attempt.lease,fields))return fail("same-view/common collision references refused");
                    v.collision86=1;v.field1d8=0;
                }
                v.hp1c4_1c6_1c8={32767,32767,32767};
                if(v.group11){ //0D46C:43D=0 and fresh scene1F=0 return before a pose.
                    v.active00=0;v.phase18=v.phase19=v.phase1a=v.phase1b=0;continue;
                }
                if(!commit_actor(slot,p.actor5b0))return fail("source C660 actor writes refused; prior prefix retained");
                v.phase18=1;v.phase19=v.phase1a=v.phase1b=0;v.distance348=p.distance348;v.height350=p.height350;
                v.target300=v.previous_target30c=p.target;v.horizontal318=p.horizontal318;v.eye2e8=v.previous_eye2f4=p.eye;v.up330=p.up;v.angles70={1536,actors[slot].input.heading72,0};
                //0C314 overwrites C660's intermediate basis before copying114.
                v.world364=p.world114;v.transform114=p.world114;v.view394=p.view394;v.position20=p.eye;v.previous2c=p.eye;v.tint358[3]=0;v.pose_generation=1;
            }
            for(unsigned i=0;i<6;++i)if(planned[i]){storage_->actors[i]=actors[i].reference;storage_->actor_worlds[i]=actors[i].world_generation;storage_->actor_headings[i]=planned[i]->actor5b0;}
            storage_->pose_complete=true;storage_->pose_generation=1;if(error)error->clear();return true;
        }catch(const std::exception&){return fail("uncertain view initialization; source prefix retained");}
    }
    template<class CommitActor>
    bool follow_first_spawn(const BattleCameraFollowProfile& profile,const std::array<ActorObservation,6>& actors,const ChallengeStageInputs& stage,const std::array<f32,4>& bounds,f32 high34,f32 low68,CommitActor&& commit_actor,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle spawn camera: ")+why;return false;};
        if(!first_poses_ready()||storage_->pose_generation!=1||storage_->follow_attempted||!profile.ready()||!stage.native_data_bound()||!stage.grid(0)||!storage_->common.live())return reject("current first views/native stage/source follow recipe required");
        for(float value:bounds)if(!std::isfinite(value)||std::abs(value)>1e7f)return reject("stage bounds domain");
        if(bounds[1]>=bounds[0]||bounds[3]>=bounds[2])return reject("stage bounds ordering");
        auto ground=stage.parameters();ground.high34=high34;ground.low68=low68;if(!stage_ground_parameters_valid(ground))return reject("same stage floor operands");
        std::array<std::optional<BattleCameraFollowPlan>,6> plans;std::array<StageLowestFloorResult,6> floors{};std::array<bool,6> queried{};std::array<u8,6> alternate{};
        for(unsigned slot=0;slot<6;++slot){
            const auto& actor=actors[slot];const bool active=storage_->inputs->active&(1u<<slot);
            if(!active){if(actor.phase18)return reject("unjoined inactive actor state");continue;}
            const auto& v=storage_->views[slot];
            if(!actor.reference.live()||actor.reference!=storage_->actors[slot]||actor.phase18!=1||actor.field3f1!=1||actor.behaviour43d||actor.world_generation!=2||storage_->actor_worlds[slot]!=1||!v.active00||v.pose_generation!=1||v.phase18!=1||v.phase19||v.phase1a||v.phase1b||v.mode2e6||v.contact1da||v.roll360||v.shake35d||v.tint358!=std::array<u8,4>{}||v.extended35e||v.obstruction35f)return reject("fresh ordinary first-spawn actor/view boundary required");
            BattleCameraFollowInput input;input.actor=actor.input;input.target50c=actor.target50c;input.width668=actor.width668;input.target508_bound=actor.target508_bound;input.lock7c9=actor.lock7c9;
            BattleCameraFollowState before;before.eye=v.eye2e8;before.target=v.target300;before.horizontal318=v.horizontal318;before.distance348=v.distance348;before.height350=v.height350;before.scale354=v.scale354;before.angles70=v.angles70;before.extended35e=v.extended35e;
            BattleCameraFollowPlan next;if(!battle_camera_first_follow(profile,input,before,&next,error))return false;
            const auto& eye=next.height_probe_eye;
            //10664 checks mutable stage bounds first;3DD34 then has its own
            //strict global-grid bounds and genuine minimum-floor fallback.
            if(!(eye[0]<bounds[1]||bounds[0]<eye[0]||eye[2]<bounds[3]||bounds[2]<eye[2])){
                queried[slot]=true;if(!stage_lowest_floor(*stage.grid(0),ground,eye,&floors[slot],error))return false;
                alternate[slot]=u8(floors[slot].height>eye[1]);
            }
            plans[slot]=next;
        }
        storage_->follow_attempted=true;auto fail=[&](const char* why){storage_->scope->live=false;return reject(why);};
        try{
            for(unsigned index=0;index<12;++index){
                const unsigned slot=index%6;if(!actors[slot].phase18)continue;
                auto& v=storage_->views[index];if(!(storage_->inputs->active&(1u<<slot))){v.active00=0;continue;}
                if(!v.initialized2e4||!plans[slot])return fail("view activation changed before commit");
                const auto& plan=*plans[slot];v.active00=1;v.scale354=plan.state.scale354;v.hp1c4_1c6_1c8[1]=v.hp1c4_1c6_1c8[0];
                if(v.group11){v.active00=0;v.phase18=v.phase19=v.phase1a=v.phase1b=0;continue;}
                if(!commit_actor(slot,plan.actor5b0))return fail("101C8 actor heading commit refused");
                v.previous_eye2f4=plan.previous_eye;v.previous_target30c=plan.previous_target;v.mode2e6=2;
                v.eye2e8=plan.state.eye;v.target300=plan.state.target;v.horizontal318=plan.state.horizontal318;v.distance348=plan.state.distance348;v.height350=plan.state.height350;v.angles70=plan.state.angles70;
                v.extended35e=plan.state.extended35e;v.obstruction35f=0;v.alternate2e7=alternate[slot];v.height_probe_eye=plan.height_probe_eye;v.floor_queried=queried[slot];v.floor_probe=floors[slot];v.first_follow=plan;
                v.up330=plan.up;v.world364=v.transform114=plan.world114;v.view394=plan.view394;v.position20=plan.state.eye;v.previous2c=plan.previous_eye;v.tint358[3]=0;v.pose_generation=2;
                storage_->actor_worlds[slot]=2;storage_->actor_headings[slot]=plan.actor5b0;
            }
            storage_->pose_generation=2;if(error)error->clear();return true;
        }catch(const std::exception&){return fail("uncertain camera continuation; source prefix retained");}
    }
    struct Storage {
        std::array<BattleViewRecord,13> views;std::array<s8,5> viewer_indices{};std::array<bool,5> common_root_links{};Mat4 common_root;s16 reset_d8=0;u8 reset_da=0,reset_dc=0;
        std::shared_ptr<scene_view_reference_detail::Scope> scope=std::make_shared<scene_view_reference_detail::Scope>();
        std::optional<BattleViewSceneInputs> inputs;std::weak_ptr<SceneObjectPool> objects;BattleCommonResourceBinding common;
        std::array<u8,6> actor487{};std::array<s16,6> actor_headings{};std::array<SceneActorReference,6> actors;std::array<u64,6> actor_worlds{};
        std::vector<BattleViewCollisionAttempt> collisions;bool aliases_bound=false,pose_attempted=false,pose_complete=false,follow_attempted=false;u64 pose_generation=0;
        ~Storage(){scope->live=false;}
    };
    std::unique_ptr<Storage> storage_;bool attempted_=false;
};
} // namespace gf
