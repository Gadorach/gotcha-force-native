#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_contact.hpp"
#include "engine/gameplay/collision_registration.hpp"
#include "engine/graphics/source_basis.hpp"

namespace gf {
struct GRedSasukeFreshSphere {
    bool initialized_declared=false;
    u8 phase10=0,effective_kind58=255;
    std::array<f32,3> center64{};
    f32 radius88=0,broad_radius7c=0;
};
struct GRedSasukeFreshPairInput {
    bool opposing_direct_scope_declared=false,lists_declared=false;
    CollisionRegistrationPlan attack_list,receiver_list;
    GRedBeamSasukeContactInput contact_fields;
    GRedSasukeFreshSphere beam,body;
    u8 receiver_flags1da=0;
    u16 receiver_suppressed272=0;
};
enum class GRedSasukeFreshPairRoute:u8 {NotListed,ReceiverHeld,RepeatToken,Suppressed,BroadMiss,Overlap};
struct GRedSasukeFreshPairPlan {
    GRedSasukeFreshPairRoute route=GRedSasukeFreshPairRoute::NotListed;
    bool narrow_sampled=false;
    std::array<f32,3> point{};
    GRedBeamSasukeContactPrefix prefix;
};

// Scoped 2DB58 (477C -> 417C) with exact opposing direct owners. The
// lists and first initialized spheres are declared snapshots, not live
// handles. 2FD7C's token test precedes 39F6C/30500. Do not route a
// moving kind-2 beam through this first-sphere-only query.
inline bool normal_gred_beam_sasuke_fresh_pair(NormalGRedNeutralBSource source,const GRedBeamShapeOperands& attack,
                                              const GRedSasukeFreshPairInput& in,GRedSasukeFreshPairPlan* out){
    using namespace source_intersection_detail;
    using R=GRedSasukeFreshPairRoute;
    if(!out||!in.opposing_direct_scope_declared||!in.lists_declared||in.contact_fields.admitted_pair_declared||
       !normal_gred_beam_sasuke_pair_scope(source,attack,in.contact_fields))return false;
    GRedSasukeFreshPairPlan next;
    auto finish=[&](R route){next.route=route;*out=next;return true;};
    if(!in.attack_list.contains(CollisionRegistrationList::L477C)||
       !in.receiver_list.contains(CollisionRegistrationList::L417C))return finish(R::NotListed);
    if(in.receiver_flags1da&2u)return finish(R::ReceiverHeld);
    for(u16 token:in.contact_fields.receiver_tokens20c)
        if(token==in.contact_fields.beam_token26c)return finish(R::RepeatToken);
    if(in.receiver_suppressed272&2u)return finish(R::Suppressed);

    for(const auto* sphere:{&in.beam,&in.body}){
        if(!sphere->initialized_declared||sphere->phase10!=1||sphere->effective_kind58||
           !valid(sphere->center64)||!source_basis_detail::number(sphere->radius88)||sphere->radius88<0||
           sphere->broad_radius7c!=sphere->radius88)return false;
    }
    const auto delta=sub(in.beam.center64,in.body.center64);
    const f32 squared=dot(delta,delta);
    if(!std::isfinite(squared))return false;
    const f32 distance=f32(std::sqrt(f64(squared)));
    constexpr f32 min_radius=bits_to_f32(0x3dcccccdu); // source 39F6C
    if(in.beam.broad_radius7c<=min_radius||in.body.broad_radius7c<=min_radius||
       distance>f32(in.beam.broad_radius7c+in.body.broad_radius7c))return finish(R::BroadMiss);
    // Equal broad and physical radii in these exact fresh sphere records:
    // 30500's physical-distance gate also passes, including tangency.
    next.narrow_sampled=true;next.point=in.body.center64;
    if(squared>0){
        auto direction=delta;
        if(!source_basis_detail::normalize(&direction))return false;
        const auto offset=scale(direction,in.body.radius88);
        for(unsigned i=0;i<3;++i)next.point[i]=f32(next.point[i]+offset[i]);
    }
    auto contact=in.contact_fields;contact.admitted_pair_declared=true;contact.contact_vector=next.point;
    if(!normal_gred_beam_sasuke_contact_prefix(source,attack,contact,&next.prefix))return false;
    return finish(R::Overlap);
}
}
