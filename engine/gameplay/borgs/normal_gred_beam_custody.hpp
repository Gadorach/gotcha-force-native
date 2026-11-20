#pragma once
#include "engine/assets/character_collision_bank.hpp"
#include "engine/gameplay/borgs/normal_gred_neutral_b.hpp"

namespace gf {
// Detached allocator inputs. A resolved group is a bucket, not an actor ID.
struct GRedBeamPoolCall {
    u8 pool_class=0,group=0,resolved_group=0,kind83=0,descriptors=0;
    bool append=false;
    bool operator==(const GRedBeamPoolCall&)const=default;
};
enum class GRedBeamPoolSample:u8 {Unobserved,Refused,Allocated};
struct GRedBeamFactoryPlan {
    GRedBeamPoolCall call;
    bool propose_header=false;
    u8 active00=0,type10=0,argument11=0,parent_slot=255,credit_slot=255;
    u16 copied_model94=0;
    u32 update0c=0,draw10c=0;
};
namespace gred_beam_custody_detail {
inline GRedBeamPoolCall beam(u8 slot){return {3,8,u8(9+slot),16,1,false};}
inline bool sample(GRedBeamPoolSample s){
    return s==GRedBeamPoolSample::Unobserved||s==GRedBeamPoolSample::Refused||s==GRedBeamPoolSample::Allocated;
}
}

// Exact direct normal parent/credit, with no copied aim target. The caller
// supplies a branch sample, NOT a SceneObject lease or proof of a live actor.
// A refused slot after debit does not propose header writes or a stock refund.
inline bool normal_gred_beam_factory_plan(NormalGRedNeutralBSource source,const GRedNeutralBReleasePlan& paid,
                                          bool reviewed_direct_no_target,GRedBeamPoolSample result,GRedBeamFactoryPlan* out){
    if(!out||!source.reviewed()||!paid.request_beam||paid.owner_slot!=source.slot||paid.object_type!=2||paid.argument!=43||
       !reviewed_direct_no_target||!gred_beam_custody_detail::sample(result))return false;
    GRedBeamFactoryPlan next;next.call=gred_beam_custody_detail::beam(source.slot);
    if(result==GRedBeamPoolSample::Allocated){
        next.propose_header=true;next.active00=1;next.type10=2;next.argument11=43;
        next.parent_slot=next.credit_slot=source.slot;next.copied_model94=source.model;
        next.update0c=0x800c3cdc;next.draw10c=0x800c4468;
    }
    *out=next;return true;
}

enum class GRedBeamInitRoute:u8 {RetireCredit,AwaitRegistration,RetireRegistration,MuzzleBranch};
struct GRedBeamInitPlan {
    GRedBeamInitRoute route=GRedBeamInitRoute::RetireCredit;
    u8 phase18=0,owner_slot=255;
    u16 lifetime1c=0;
    bool request_shape=false;
};
// First dispatch only (new phase 0). The credit guard precedes the phase
// switch. A shape refusal still follows the phase-1/300 seed; it stages
// phase 2 and skips the matrix reads. No initializer or removal is run.
inline bool normal_gred_beam_first_dispatch(NormalGRedNeutralBSource source,const GRedBeamFactoryPlan& factory,
                                            s8 credit_phase18,u16 credit_model,GRedBeamPoolSample shape_result,
                                            GRedBeamInitPlan* out){
    if(!out||!source.reviewed()||!factory.propose_header||factory.active00!=1||factory.type10!=2||factory.argument11!=43||
       factory.parent_slot!=source.slot||factory.credit_slot!=source.slot||factory.copied_model94!=source.model||
       factory.call!=gred_beam_custody_detail::beam(source.slot)||!gred_beam_custody_detail::sample(shape_result))return false;
    GRedBeamInitPlan next;
    next.owner_slot=source.slot;
    if(credit_phase18>=2||credit_model!=factory.copied_model94){
        if(shape_result!=GRedBeamPoolSample::Unobserved)return false; // impossible ordering in this leaf
        next.phase18=2;
    }else{
        next.phase18=1;next.lifetime1c=300;next.request_shape=true;
        switch(shape_result){
        case GRedBeamPoolSample::Unobserved:next.route=GRedBeamInitRoute::AwaitRegistration;break;
        case GRedBeamPoolSample::Refused:next.route=GRedBeamInitRoute::RetireRegistration;next.phase18=2;break;
        case GRedBeamPoolSample::Allocated:next.route=GRedBeamInitRoute::MuzzleBranch;break;
        }
    }
    *out=next;return true;
}

struct GRedBeamShapeOperands {
    GRedBeamPoolCall call;
    u8 attack_group=0,descriptor=0,hit_row=0,credited_slot=255;
    bool beam_transform114=false;
    s16 start=0,end=0;
    f32 local_radius=0;
};
inline bool normal_gred_beam_shape_scope(NormalGRedNeutralBSource source,const GRedBeamShapeOperands& shape){
    return source.reviewed()&&shape.call==GRedBeamPoolCall{4,48,u8(49+source.slot),26,0,false}&&
        !shape.attack_group&&shape.descriptor==15&&!shape.hit_row&&shape.credited_slot==source.slot&&
        shape.beam_transform114&&!shape.start&&shape.end==-2&&shape.local_radius==20.f;
}
// 8AC80 uses the credited actor's slot-2 bank and hit table. Its class-4
// shape follows the BEAM transform at +114 because beam kind83 is nonzero,
// not key 0 of the credited actor. No registry insertion or geometry solve.
inline bool normal_gred_beam_shape_operands(NormalGRedNeutralBSource source,const GRedBeamInitPlan& init,
                                             ByteView credit_bank,GRedBeamShapeOperands* out){
    if(!out||!source.reviewed()||init.route!=GRedBeamInitRoute::AwaitRegistration||!init.request_shape||
       init.owner_slot!=source.slot||init.phase18!=1||init.lifetime1c!=300||!credit_bank.data||credit_bank.size!=7840)return false;
    u32 hash=2166136261u;for(std::size_t i=0;i<credit_bank.size;++i)hash=(hash^credit_bank.data[i])*16777619u;
    if(hash!=0x81539104u)return false; // normal 0615 slot2
    CharacterCollisionBank bank;
    if(!bank.load(credit_bank,nullptr))return false;
    const auto ids=bank.attack_ids(0);
    CharacterCollisionDescriptor d;
    if(ids.size()!=1||ids[0]!=15||!bank.descriptor(CharacterCollisionTable::Attack,ids[0],&d,nullptr)||
       d.shape||d.binding1||d.parameter4||d.flags2!=1||d.start6||d.end8!=-2||d.scalar(24)!=20.f)return false;
    *out={{4,48,u8(49+source.slot),26,0,false},0,ids[0],u8(d.parameter4),source.slot,true,d.start6,d.end8,d.scalar(24)};
    return true;
}
}
