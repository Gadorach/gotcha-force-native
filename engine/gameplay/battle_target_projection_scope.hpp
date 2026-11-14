#pragma once
#include <exception>
#include <memory>
#include "engine/assets/battle_target_projection.hpp"
#include "engine/gameplay/challenge_stage_inputs.hpp"
#include "engine/graphics/source_intersection.hpp"
namespace gf {
struct BattleTargetClampBounds {bool declared=false;f32 minimum_x=0,maximum_x=0,minimum_z=0,maximum_z=0;};
struct BattleTargetProjectionInput {
    bool declared=false;std::array<f32,3> destination2e8{};f32 owner_y24=0;u8 peer_kind83=0;f32 peer_height668=0;
    StageGroundActor peer_ground;BattleTargetClampBounds bounds;
};
struct BattleTargetProjectionFields {
    std::array<f32,3> after_ground{},after_clamp{},dynamic_start{},dynamic_end{};
    f32 initial_ceiling=0,following_ceiling=0;bool clamped=false,dynamic_query_pending=false,target_position_publication_pending=false;
};
namespace battle_target_projection_detail {
struct Context {BattleTargetProjectionFields fields;u32 scratch20=0;bool live=true;};
inline bool same_bits(const std::array<f32,3>& a,const std::array<f32,3>& b){for(unsigned i=0;i<3;++i)if(f32_to_bits(a[i])!=f32_to_bits(b[i]))return false;return true;}
}
class BattleTargetProjectionPreparation;
//A DIFFERENT call-path witness from ChallengeSpawnBroadphaseWitness. Only one
//source scratch word is represented. Word16 is an unrepresented backchain, NOT
//zero, a source constant, or a cast native stack address. No registry/result ACK.
class BattleTargetBroadphaseWitness {
public:
    bool valid()const{const auto c=context_.lock();return c&&c->live;}
    bool permits_candidate_broadphase_elision(const std::array<f32,3>& start,const std::array<f32,3>& end)const{const auto c=context_.lock();return c&&c->live&&battle_target_projection_detail::same_bits(start,c->fields.dynamic_start)&&battle_target_projection_detail::same_bits(end,c->fields.dynamic_end);}
    u8 source_stack_known_mask()const{return valid()?2:0;}
    u32 source_scratch_word20()const{const auto c=context_.lock();return c&&c->live?c->scratch20:0;}
private:std::weak_ptr<const battle_target_projection_detail::Context> context_;friend class BattleTargetProjectionPreparation;
};
//241C0 static peer path: initial ceiling==source high34 skips3E978; following
//ceiling>ownerY skips245D0. Shared ground/ceiling kernels and declared clamp bounds
//compute the ACTUAL supplied-input path endpoints before the pending37B98 call.
//Not source actor-world/lifecycle/clamp-value provenance or full target movement.
class BattleTargetProjectionPreparation {
public:
    BattleTargetProjectionPreparation()=default;BattleTargetProjectionPreparation(const BattleTargetProjectionPreparation&)=delete;BattleTargetProjectionPreparation& operator=(const BattleTargetProjectionPreparation&)=delete;
    BattleTargetProjectionPreparation(BattleTargetProjectionPreparation&&)=default;BattleTargetProjectionPreparation& operator=(BattleTargetProjectionPreparation&&)=default;
    ~BattleTargetProjectionPreparation(){retire();}
    bool begin(const BattleTargetProjectionProfile& profile,const BattleTargetProjectionInput& in,const ChallengeStageInputs& stage,std::string* error){
        auto fail=[&](const char* why){if(error)*error=std::string("battle target projection preparation: ")+why;return false;};
        if(attempted_)return fail("fresh one-shot context required");
        attempted_=true;
        using namespace stage_ground_detail;
        if(!battle_target_projection_profile_valid(profile)||!stage.ready()||!in.declared||!in.peer_ground.fields_declared||in.peer_ground.attached1db||!in.bounds.declared||!vector(in.destination2e8)||!number(in.owner_y24))return fail("declared static peer/destination/stage/bounds required");
        const auto& b=in.bounds;if(!number(b.minimum_x)||!number(b.maximum_x)||!number(b.minimum_z)||!number(b.maximum_z)||b.minimum_x>b.maximum_x||b.minimum_z>b.maximum_z)return fail("clamp bound numeric domain");
        const f32 peer_height=in.peer_kind83?0.f:in.peer_height668;if(!number(peer_height)||!number(in.peer_ground.current24))return fail("peer field numeric domain");
        const auto& p=stage.parameters();
        if(p.bounds_min!=-7500.f||p.bounds_max!=7500.f)return fail("source3FB00 fixed bounds required");
        if(!inside(p,in.destination2e8))return fail("initial3D6E4 nested3F320 scratch producer not reached");
        const std::array<const StihGrid*,3> grids{stage.grid(0),stage.grid(1),stage.grid(2)};BattleTargetProjectionFields next;
        if(!ceiling(grids,p,in.destination2e8,in.destination2e8[1],&next.initial_ceiling))return fail("initial ceiling query domain");
        if(next.initial_ceiling!=p.high34)return fail("optional3E978 path requires separate scratch proof");
        const f32 peer_top=f32(peer_height+in.peer_ground.current24);const f32 height=f32(next.initial_ceiling-peer_top);if(!number(peer_top)||!number(height))return fail("initial height numeric domain");
        next.after_ground=in.destination2e8;if(!stage.ground(in.peer_ground,in.destination2e8,height,&next.after_ground[1],error))return false;
        if(!ceiling(grids,p,next.after_ground,next.after_ground[1],&next.following_ceiling))return fail("following ceiling query domain");
        if(!(next.following_ceiling>in.owner_y24))return fail("245D0 branch does not reach this dynamic query");
        next.after_clamp=next.after_ground;
        if(next.after_clamp[0]>b.maximum_x){next.after_clamp[0]=b.maximum_x;next.clamped=true;}else if(next.after_clamp[0]<b.minimum_x){next.after_clamp[0]=b.minimum_x;next.clamped=true;}
        if(next.after_clamp[2]>b.maximum_z){next.after_clamp[2]=b.maximum_z;next.clamped=true;}else if(next.after_clamp[2]<b.minimum_z){next.after_clamp[2]=b.minimum_z;next.clamped=true;}
        if(next.clamped){const f32 following_height=f32(next.following_ceiling-peer_top);f32 ground=0;if(!number(following_height)||!stage.ground(in.peer_ground,next.after_clamp,following_height,&ground,error))return false;next.after_clamp[1]=ground;}
        next.dynamic_end=next.after_clamp;next.dynamic_start=next.dynamic_end;next.dynamic_start[1]=f32(next.dynamic_start[1]+profile.ray_lift());if(!vector(next.dynamic_start)||!vector(next.dynamic_end))return fail("dynamic endpoints numeric domain");
        const auto delta=source_intersection_detail::sub(next.dynamic_start,next.dynamic_end);const f32 squared=source_intersection_detail::dot(delta,delta);if(!std::isfinite(squared)||!(squared>0))return fail("positive-length dynamic segment required");
        const u32 word=std::bit_cast<u32>(stage.grid(1)->minimum_x())^0x80000000u;if((word&0x7f800000u)!=0x7f800000u||!(word&0x007fffffu))return fail("known scratch20 is not a NaN word; no midpoint substitute");
        next.dynamic_query_pending=next.target_position_publication_pending=true;
        try{auto context=std::make_shared<battle_target_projection_detail::Context>();context->fields=next;context->scratch20=word;context_=std::move(context);}catch(const std::exception&){return fail("uncertain context allocation; no partial witness");}
        if(error)error->clear();
        return true;
    }
    bool prepared()const{return context_&&context_->live;}
    const BattleTargetProjectionFields* fields()const{return prepared()?&context_->fields:nullptr;}
    BattleTargetBroadphaseWitness witness()const{BattleTargetBroadphaseWitness w;if(prepared())w.context_=context_;return w;}
    bool current(const BattleTargetBroadphaseWitness& w)const{return prepared()&&w.context_.lock()==context_;}
    void retire(){if(context_)context_->live=false;context_.reset();attempted_=true;}
private:std::shared_ptr<battle_target_projection_detail::Context> context_;bool attempted_=false;
};
} // namespace gf
