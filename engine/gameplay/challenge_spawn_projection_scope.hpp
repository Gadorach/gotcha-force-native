#pragma once
#include <memory>
#include "engine/gameplay/challenge_spawn_static_tail.hpp"
namespace gf {
namespace spawn_projection_scope_detail {
struct Context {ChallengeSpawnStaticTail tail;std::array<u32,2> words{};bool live=true;};
inline bool same_bits(const std::array<f32,3>& a,const std::array<f32,3>& b){for(unsigned i=0;i<3;++i)if(f32_to_bits(a[i])!=f32_to_bits(b[i]))return false;return true;}
}
class ChallengeSpawnProjectionPreparation;
//An output-comparison elision witness ONLY. It never says a collider list is
//empty, a shape missed, a position/history is final, or an actor is ready.
class ChallengeSpawnBroadphaseWitness {
public:
    bool valid()const{const auto c=context_.lock();return c&&c->live;}
    bool permits_candidate_broadphase_elision(const std::array<f32,3>& start,const std::array<f32,3>& end)const{const auto c=context_.lock();return c&&c->live&&spawn_projection_scope_detail::same_bits(start,c->tail.actor_projection_start)&&spawn_projection_scope_detail::same_bits(end,c->tail.after_static);}
    std::array<u32,2> source_stack_words()const{const auto c=context_.lock();return c&&c->live?c->words:std::array<u32,2>{};}
private:friend class ChallengeSpawnProjectionPreparation;std::weak_ptr<const spawn_projection_scope_detail::Context> context_;
};
//Qualified first-placement call path ONLY:56D28→3F044(frame32)→3F0C0(frame192),
//then37B98(frame160), with no intervening call. Static+80/+84 overlap dynamic
//+16/+20. The conversion high43300000 and(minimumX xor80000000) make midpointZ
//NaN when the latter is a NaN word; ordered greater-than cannot reject a candidate.
//No source/native pointer is cast to float, and no uninitialized stack is read.
//Not a memory-read/FPSCR/trap/hardware proof or permission for other37B98 callers.
class ChallengeSpawnProjectionPreparation {
public:
    ChallengeSpawnProjectionPreparation()=default;ChallengeSpawnProjectionPreparation(const ChallengeSpawnProjectionPreparation&)=delete;ChallengeSpawnProjectionPreparation& operator=(const ChallengeSpawnProjectionPreparation&)=delete;
    ChallengeSpawnProjectionPreparation(ChallengeSpawnProjectionPreparation&&)=default;ChallengeSpawnProjectionPreparation& operator=(ChallengeSpawnProjectionPreparation&&)=default;
    ~ChallengeSpawnProjectionPreparation(){retire();}
    bool begin(const ChallengeFirstPlacementPlan& placement,const ChallengeStageInputs& stage,const StageRayParameters& parameters,std::string* error){
        auto fail=[&](const char* why){if(error)*error=std::string("spawn projection preparation: ")+why;return false;};
        if(attempted_)return fail("fresh one-shot initial-placement context required");
        attempted_=true;ChallengeSpawnStaticTail tail;if(!challenge_spawn_static_tail(placement,stage,parameters,&tail,error))return false;
        if(tail.static_hit.excluded_bounds)return fail("static3F0C0 was not reached; scratch provenance unavailable");
        const u32 low=std::bit_cast<u32>(stage.grid(1)->minimum_x())^0x80000000u;
        if((low&0x7f800000u)!=0x7f800000u||!(low&0x007fffffu))return fail("source minimumX temporary is not NaN; no midpoint substitution");
        const auto difference=source_intersection_detail::sub(tail.actor_projection_start,tail.after_static);const f32 squared=source_intersection_detail::dot(difference,difference);
        if(!std::isfinite(squared)||!(squared>0))return fail("positive-length source dynamic segment required");
        auto next=std::make_shared<spawn_projection_scope_detail::Context>();next->tail=tail;next->words={0x43300000u,low};context_=std::move(next);if(error)error->clear();return true;
    }
    bool prepared()const{return context_&&context_->live;}
    const ChallengeSpawnStaticTail* static_inputs()const{return prepared()?&context_->tail:nullptr;}
    ChallengeSpawnBroadphaseWitness witness()const{ChallengeSpawnBroadphaseWitness w;if(prepared())w.context_=context_;return w;}
    bool current(const ChallengeSpawnBroadphaseWitness& w)const{return prepared()&&w.context_.lock()==context_;}
    void retire(){if(context_)context_->live=false;context_.reset();attempted_=true;}
private:std::shared_ptr<spawn_projection_scope_detail::Context> context_;bool attempted_=false;
};
} // namespace gf
