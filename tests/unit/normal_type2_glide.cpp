#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/borgs/normal_type2_glide.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal type-2 glide: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using S=NormalType2GlideSource;
    using C=Type2GlideContinuation;
    using T=Type2GlideFloorTail;
    static_assert(S::meter.matches_operands(10000,0,2000,{150,50,450,80}));
    for(const S source:std::array<S,2>{{{0xa00,0},{0xb00,0}}}){
        CharacterMovementInputState input;
        Type2GlideRequest request;
        const auto try_request=[&](u8 from,u32 flags,u8 expected){
            check(normal_type2_glide_request(source,from,flags,true,input,&request)&&request.selector==expected,
                "reviewed selector suffix");
        };
        try_request(S::fall,0x40,S::fall);
        input.previous_held=0x100;
        try_request(S::fall,0x40,S::glide);
        try_request(S::fall,0x42,S::fall);
        try_request(S::glide,0x40,S::glide);
        input.previous_held|=0x10000;
        try_request(S::fall,0x40,S::steer); // steering before bare held glide
        try_request(S::glide,0x40,S::steer);
        input.directions5d8=0xa828;
        try_request(S::glide,0x40,S::directional_dash);
        check(request.consume_direction&&request.direction58f==2&&request.remaining_directions5d8==0xa808,
            "only the consumed dash nibble is removed");
        try_request(S::fall,0x40,S::directional_dash);
        input.previous_held&=~0x100u;
        try_request(S::glide,0x42,S::fall); // release first even with a lock/flick
        check(!request.consume_direction&&request.remaining_directions5d8==input.directions5d8,"release does not consume dash");
        check(!normal_type2_glide_request(source,S::fall,0,false,input,&request)&&request.selector==S::fall,
            "earlier handlers cannot be skipped");

        Type2GlideFrameScalars frame{5.f,255};
        check(normal_type2_glide_frame_scalars(source,0x840,&frame)&&f32_to_bits(frame.offset6d0)==0&&frame.cached87==1,
            "ordinary offset producer is zero with a logically airborne root");
        check(normal_type2_glide_frame_scalars(source,0x80,&frame)&&frame.cached87==0,"inhibit bit is not airborne cache");
        for(u32 flags:{0x40u,0x80u,0xc0u}){
            CharacterAirResourceState r{3000,0,3};
            check(normal_type2_glide_refill(source,r,flags,&r)&&r.value736==(flags&0x80u?3000:5000)&&r.air_actions747==3,
                "inhibit gate independent of grounding and unused count");
        }

        Type2GlideFloorInput contact{98,100,frame.offset6d0,0xc0,S::glide,C::Landing,T::QualifiedStaticSnap};
        Type2GlideFloorPlan plan;
        check(normal_type2_glide_floor_tail(source,contact,&plan)&&plan.snapped&&plan.root_y==100&&plan.selector==S::landing&&
            plan.flags5e0==0x8c0&&plan.continuation==C::None&&plan.clear_clock69c&&plan.clear_motion_and_turn,
            "installed callback requests landing, but glide tail still marks airborne");
        Type2GlideLandingPlan landing;
        check(normal_type2_glide_landing_flags(source,plan.selector,0,plan.flags5e0|0x2000,&landing)&&
            landing.flags5e0==0xa80&&landing.cached87==0,"landing clears logical airborne, not refill inhibit");
        check(!normal_type2_glide_landing_flags(source,plan.selector,1,landing.flags5e0,&landing)&&landing.flags5e0==0xa80,
            "only actual phase-zero entry publishes flags");
        CharacterAirResourceState r{3000,1,3};
        check(normal_type2_glide_refill(source,r,landing.flags5e0,&r)&&r.delay73a==1&&r.value736==3000,
            "landing with inhibit cannot refill or decrement delay");

        contact.selector=S::steer;contact.continuation=C::None;contact.flags5e0=0;
        check(normal_type2_glide_floor_tail(source,contact,&plan)&&plan.snapped&&plan.root_y==100&&plan.selector==S::steer&&
            plan.flags5e0==0x840&&!plan.clear_clock69c&&!plan.clear_motion_and_turn,
            "same height without a callback does not select landing or clear glide motion");
        check(normal_type2_glide_refill(source,r,plan.flags5e0,&r)&&r.delay73a==0&&r.value736==3000,
            "stage-supplied airborne visit ticks delay only");
        check(normal_type2_glide_refill(source,r,plan.flags5e0,&r)&&r.value736==5000,
            "a separate airborne resource update can refill");
        const auto retained=plan;
        contact.continuation=C::Unresolved;
        check(!normal_type2_glide_floor_tail(source,contact,&plan)&&plan==retained,"unresolved is not a null callback");
        contact.continuation=C::Landing;
        check(!normal_type2_glide_floor_tail(source,contact,&plan)&&plan==retained,"unsupported steering callback refuses");
        contact.continuation=C::None;contact.tail=T::Unresolved;
        check(!normal_type2_glide_floor_tail(source,contact,&plan)&&plan==retained,"no snap may be inferred from height alone");
        contact.tail=T::QualifiedStaticSnap;contact.root_y=101;
        check(!normal_type2_glide_floor_tail(source,contact,&plan)&&plan==retained,"above-floor input contradicts supplied evidence");
        contact.root_y=98;contact.offset6d0=2;
        check(!normal_type2_glide_floor_tail(source,contact,&plan)&&plan==retained,"this scope does not fabricate a hover offset");
        contact.offset6d0=0;contact.floor_y=std::numeric_limits<f32>::quiet_NaN();contact.tail=T::PlainReturn;
        check(normal_type2_glide_floor_tail(source,contact,&plan)&&!plan.snapped&&plan.root_y==98&&plan.flags5e0==0x840,
            "ordinary no-response return requires no invented floor");
        contact.tail=T::QualifiedStaticSnap;
        check(!normal_type2_glide_floor_tail({source.model,1},contact,&plan)&&
            !normal_type2_glide_floor_tail({0x0d00,0},contact,&plan)&&!plan.snapped,
            "other variants and plane controllers do not inherit this plan");
    }
    std::printf("normal Wing Soldier / Quick Valkrie glide contact and independent refill: %u checks PASS\n",checks);
}
