#pragma once

#include "engine/gameplay/battle_partition_route.hpp"
#include "engine/gameplay/native_battle_mvp.hpp"

namespace gf {

struct PortableRosterInput {
    GamePadState movement;
    BorgBattleInputs attack;
    u8 aim_target=255;
};

// Standalone roster CPU: produces ordinary pad and action inputs, never starts
// a move, spends stock, or accepts a hit. Reset with the admitted round.
class PortableRosterAi {
public:
    void reset(){slots_={};}

    PortableRosterInput input(const NativeBattleMvp& world,unsigned slot,const IBattleMvpTerrain* terrain=nullptr){
        const unsigned first_cpu=world.config().roster_local_players;
        if(slot<first_cpu||slot>=world.actor_count()||!world.ready()||world.paused()||
           world.result()!=BattleMvpResult::Fighting||!world.actors()[slot].health)return {};
        const auto nearest=world.target_of(slot);
        const auto& profile=world.config().packages[slot];
        if(nearest==255||!profile||profile->movement==BorgBattleMovement::NormalGRed)return {};

        const unsigned cpu=slot-first_cpu;
        auto& state=slots_[cpu];
        const auto tick=world.tick();
        if(state.sampled==tick)return state.output;
        const bool previous_boost=state.sampled!=unsampled&&state.output.movement.b;
        const bool previous_dash=state.sampled!=unsampled&&state.output.movement.l;
        const auto opening=cpu==0?0u:30u-5u*cpu; // spread opening attacks by CPU order
        if(state.sampled==unsampled)state.next_start=opening;
        state.sampled=tick;
        state.output={world.roster_ai_pad(slot),{}};
        auto& [pad,attack,aim]=state.output;
        pad.a=pad.b=false; // no legacy fire bit or periodic jump in this mode

        const auto& self=world.actors()[slot];
        const auto& action=world.package_action(slot);
        const bool boost_leg=state.burst_leg==BurstLeg::Boost;
        const bool dash_leg=state.burst_leg==BurstLeg::Dash;
        const bool burst_active=(boost_leg&&profile->cruise&&self.package_boosting&&self.package_boost_meter.value736>0)||
            (dash_leg&&self.grounded&&self.dash_ticks&&!self.dash_airborne);
        if(state.burst_leg_until>tick&&burst_active&&!self.hit_flash&&self.movement_speed>.05f&&
           action.move==BorgBattleActionState::none){
            // Keep the checked stick direction through the native burst.
            // Collision, interruption and stock can still end the plan.
            pad.stick_x=state.burst_leg_x;pad.stick_y=state.burst_leg_y;pad.b=boost_leg;pad.l=dash_leg;
            return state.output;
        }
        state.burst_leg=BurstLeg::None;
        const auto preferred=std::any_of(profile->moves.begin(),profile->moves.end(),[](const auto& move){
            return move.button==BorgBattleButton::B&&supported(move);
        })?BorgBattleButton::B:BorgBattleButton::X;
        const auto fits_context=[&](const auto& move){
            return move.context==BorgBattleContext::Any||(move.context==BorgBattleContext::Grounded)==self.grounded;
        };
        const f32 radius=world.config().actor_radius;
        const f32 body_height=world.config().actor_height;
        const auto height_overlaps=[&](const BorgBattleMelee& shape,f32 target_y){
            return native_borg_melee_height_overlap(self.position[1]+shape.height,shape.half_height,target_y,body_height);
        };

        f32 any_reach=0,current_reach=0;
        for(const auto& move:profile->moves){
            if(move.button!=preferred||!supported(move))continue;
            const auto reach=attack_reach(move,radius);
            any_reach=std::max(any_reach,reach);
            if(fits_context(move))current_reach=std::max(current_reach,reach);
        }
        const auto reach=current_reach>0?current_reach:any_reach;
        const auto point=[&](unsigned actor){const auto& p=world.actors()[actor].position;return BattlePoint{p[0],p[2]};};
        const auto start=point(slot);
        const auto has_entry_stock=[&](const BorgBattleMove& move){
            return !move.entry_stock||action.stock_ready(*profile,move.entry_stock->stock,move.entry_stock->minimum);
        };
        const auto ready_attack=[&](unsigned rival,bool blocked_approach)->const BorgBattleMove*{
            const auto goal=point(rival);
            const auto distance=std::hypot(goal.x-start.x,goal.z-start.z);
            const auto toward=native_borg_direction(start,goal,self.facing);
            const f32 facing=self.facing.x*toward.x+self.facing.z*toward.z;
            std::array<const BorgBattleMove*,2> presses{},charges{};
            for(const auto& move:profile->moves){
                if(!has_entry_stock(move)||!fits_context(move)||(move.entry_range&&distance>move.entry_range))continue;
                // Select the top entry before probing later debits. Don't
                // bypass a shadowing press or an empty ungated priority move.
                const auto button=unsigned(move.button);
                if(move.activation==BorgBattleActivation::Press){
                    if(!presses[button]||move.priority>presses[button]->priority)presses[button]=&move;
                }else if(move.activation==BorgBattleActivation::ReleaseWindow&&move.maximum_hold){
                    if(!charges[button]||move.priority>charges[button]->priority)charges[button]=&move;
                }
            }
            for(unsigned button=0;button<presses.size();++button){
                const auto* move=presses[button]?presses[button]:charges[button];
                if(!move||!supported(*move)||!first_stock_ready(*profile,action,*move))continue;
                const bool melee=borg_battle_direct_melee(*move);
                if(blocked_approach&&!melee)continue;
                if(distance>attack_reach(*move,radius)&&
                   (blocked_approach||!follow_charge_reaches(world,slot,rival,*move,distance,terrain)))continue;
                if(melee&&(!height_overlaps(*move->melee,world.actors()[rival].position[1])||
                           !native_borg_melee_path_clear(terrain,start,goal)))continue;
                if(!field_height_allows(world,slot,rival,*move))continue;
                const bool needs_heading=move->melee||move->charge||(damaging_field(*move)&&move->field->forward>0);
                if(!needs_heading||facing>=.7f)return move; // ready B before ready X
            }
            return nullptr;
        };
        auto target=nearest;
        const bool short_range=reach>0&&std::isfinite(reach);
        const bool melee_only=short_range&&std::none_of(profile->moves.begin(),profile->moves.end(),[](const auto& move){
            return supported(move)&&!borg_battle_direct_melee(move);
        });
        const bool can_dash=profile->movement==BorgBattleMovement::Grounded&&self.grounded&&
            !profile->dash_disabled&&profile->dash>profile->walk&&!self.dash_ticks&&!self.dash_cooldown&&
            !self.hit_flash&&!state.button&&action.move==BorgBattleActionState::none;
        bool detour=false,keep_route_probe=false;
        const BorgBattleMove* close_strike=nullptr;
        if(short_range&&profile->movement==BorgBattleMovement::Grounded&&terrain&&!terrain->body_partitions().empty()){
            const auto clear=[&](unsigned rival){return battle_partition_route_clear(*terrain,start,point(rival),radius);};
            detour=!clear(nearest);
            // The padded body route and the authored melee line are
            // separate. Don't abandon a visible rival already in reach.
            if(detour)close_strike=ready_attack(nearest,true);
            if(world.config().local_aim_cycle&&!close_strike){
                const bool height_limited=std::none_of(profile->moves.begin(),profile->moves.end(),[](const auto& move){
                    return supported(move)&&!borg_battle_direct_melee(move)&&!simple_field_height(move);
                });
                const auto can_reach=[&](unsigned rival,bool allow_other_enemy){
                    const auto& rival_profile=world.config().packages[rival];
                    // Ordinary airborne opponents can land. Only filter
                    // sustained cruise height when the CPU has no attack
                    // with unknown height reach or a ranged fallback.
                    if(!height_limited||!rival_profile||!rival_profile->cruise)return true;
                    const auto y=world.actors()[rival].position[1];
                    f32 support=self.position[1];
                    if(!self.grounded&&!profile->cruise&&
                       (!terrain->height(start.x,start.z,self.position[1],&support)||!std::isfinite(support)||
                        std::abs(support)>20000||support>self.position[1]))return true;
                    return std::any_of(profile->moves.begin(),profile->moves.end(),[&](const auto& move){
                        if(!supported(move))return false;
                        if(simple_field_height(move)){
                            // Don't forecast placement after an ordinary jump
                            // or landing; a cruiser never takes that ground route.
                            if(!fits_context(move))return !profile->cruise;
                            return field_height_allows(world,slot,rival,move,allow_other_enemy);
                        }
                        if(!borg_battle_direct_melee(move))return false;
                        if(fits_context(move)&&height_overlaps(*move.melee,y))return true;
                        if(profile->cruise)return false;
                        const auto& shape=*move.melee;
                        if(move.context!=BorgBattleContext::Airborne&&native_borg_melee_height_overlap(
                            support+shape.height,shape.half_height,y,body_height))return true;
                        return move.context!=BorgBattleContext::Grounded&&
                            melee_height_reachable(shape,support,y,body_height,profile->jump,world.config().gravity);
                    });
                };
                if(detour||!can_reach(nearest,true)){
                    f32 best=std::numeric_limits<f32>::infinity();
                    for(u8 i=0;i<world.actor_count();++i){
                        const auto& other=world.actors()[i];
                        if(i==nearest||!other.health||other.team==self.team)continue;
                        const auto p=point(i);
                        const f32 dx=p.x-start.x,dz=p.z-start.z,d=dx*dx+dz*dz;
                        if(d<=best&&clear(i)&&can_reach(i,false)){best=d;target=i;}
                    }
                    if(target!=nearest){
                        if(detour&&can_reach(nearest,false)){
                            const auto nearby=point(nearest);
                            const auto half_straight=.5*std::sqrt(f64(best));
                            if(std::hypot(f64(nearby.x)-start.x,f64(nearby.z)-start.z)<half_straight){
                                // Keep the clear route unless the nearer
                                // rival's detour saves over half the walk.
                                if(route_needs_replan(state,*terrain,start,nearby,radius,nearest,tick))
                                    remember_route(state,battle_partition_route(*terrain,start,nearby,world.config().arena_half,radius),
                                                   nearby,nearest,tick);
                                keep_route_probe=true;
                                if(battle_partition_route_length(start,state.route,state.waypoint)<half_straight)target=nearest;
                            }
                        }
                        if(target!=nearest){aim=target;detour=false;}
                    }else if(detour){
                        const auto routable=[&](unsigned rival){
                            const auto& other=world.actors()[rival];
                            return other.health&&other.team!=self.team&&can_reach(rival,false);
                        };
                        const auto cached=state.route_target;
                        if(cached>=world.actor_count()||(!routable(cached)&&(state.route.count||cached!=nearest))||
                           route_needs_replan(state,*terrain,start,point(cached),radius,cached,tick)){
                            // Compare only when a plan expires or becomes
                            // invalid, not once per rival on every tick.
                            const auto preferred_route=state.route.count?cached:nearest;
                            f64 best_length=std::numeric_limits<f64>::infinity();
                            BattlePartitionRoute best_route;
                            u8 choice=nearest;
                            for(u8 i=0;i<world.actor_count();++i)if(routable(i)){
                                const auto route=battle_partition_route(*terrain,start,point(i),world.config().arena_half,radius);
                                // A one-radius preference avoids switching
                                // sides for a negligible route saving.
                                const auto length=battle_partition_route_length(start,route)-(i==preferred_route?radius:0.f);
                                if(length<best_length){best_length=length;best_route=route;choice=i;}
                            }
                            remember_route(state,best_route,point(choice),choice,tick);
                        }
                        if(state.route.count&&state.route_target!=nearest){
                            aim=target=state.route_target;
                            close_strike=ready_attack(target,true);
                        }
                    }
                }
            }
        }
        // A short-range fighter can get stuck tailing the nearest runner.
        // Only native stage rosters can supply the one-tick rival hint.
        if(!detour&&target==nearest&&melee_only&&self.grounded&&profile->movement==BorgBattleMovement::Grounded&&
           world.config().local_aim_cycle&&terrain&&!terrain->body_partitions().empty()){
            const auto alternate=[&](unsigned i){
                const auto& other=world.actors()[i];
                return i!=nearest&&other.health&&other.team!=self.team&&other.grounded&&
                    std::hypot(other.velocity.x,other.velocity.z)<=profile->walk*.5f&&
                    battle_partition_route_clear(*terrain,start,point(i),radius)&&
                    std::any_of(profile->moves.begin(),profile->moves.end(),[&](const auto& move){
                        return supported(move)&&borg_battle_direct_melee(move)&&fits_context(move)&&
                            height_overlaps(*move.melee,other.position[1]);
                    });
            };
            const bool busy=state.button||self.hit_flash||self.dash_ticks||action.move!=BorgBattleActionState::none;
            const bool can_switch=!busy&&!ready_attack(nearest,false);
            if((busy||can_switch)&&state.chase_target<world.actor_count()&&
               (busy||tick<state.chase_until)&&alternate(state.chase_target)){
                aim=target=state.chase_target;
            }else{
                state.chase_target=255;
                if(can_switch){
                    const auto& rival=world.actors()[nearest];
                    const auto p=point(nearest),toward=native_borg_direction(start,p,self.facing);
                    const f32 away=toward.x*rival.velocity.x+toward.z*rival.velocity.z;
                    const auto horizon=std::min(30u,world.config().dash_cooldown);
                    const BattlePoint projected{p.x+horizon*rival.velocity.x,p.z+horizon*rival.velocity.z};
                    const auto bound=world.config().arena_half-radius;
                    if(rival.grounded&&!rival.hit_flash&&!rival.dash_ticks&&away>profile->walk+.5f&&
                       std::hypot(p.x-start.x,p.z-start.z)>reach&&std::abs(projected.x)<=bound&&std::abs(projected.z)<=bound&&
                       battle_partition_route_clear(*terrain,p,projected,radius)){
                        f32 best=std::hypot(projected.x-start.x,projected.z-start.z)-radius;
                        for(u8 i=0;i<world.actor_count();++i)if(alternate(i)){
                            const auto q=point(i);
                            const auto& other=world.actors()[i];
                            const f32 cost=std::hypot(q.x-start.x,q.z-start.z)+horizon*std::hypot(other.velocity.x,other.velocity.z);
                            if(cost<best){best=cost;target=i;}
                        }
                        if(target!=nearest){
                            aim=state.chase_target=target;
                            state.chase_until=u64(tick)+horizon;
                        }
                    }
                }
            }
        }else state.chase_target=255;
        const auto goal=point(target);
        const BattlePoint delta{goal.x-start.x,goal.z-start.z};
        const f32 distance=std::hypot(delta.x,delta.z);
        const BattlePoint toward=distance>0?BattlePoint{delta.x/distance,delta.z/distance}:self.facing;
        const f32 facing=self.facing.x*toward.x+self.facing.z*toward.z;

        const auto& enemy=world.actors()[target];
        const auto height_now=[&](const BorgBattleMelee& shape){
            return height_overlaps(shape,enemy.position[1]);
        };
        const auto clear_burst=[&](BattlePoint direction,f32 travel){
            const BattlePoint end{start.x+travel*direction.x,start.z+travel*direction.z};
            return !terrain||battle_partition_route_clear(*terrain,start,end,radius+4.f);
        };
        const auto dash_fits=[&](BattlePoint displacement,f32 span,f32 reserve){
            const auto burst=profile->dash*world.config().dash_duration;
            return can_dash&&span>burst+reserve&&clear_burst({displacement.x/span,displacement.z/span},burst);
        };
        const auto chase_dash_fits=[&](f32 reserve){
            if(!can_dash||previous_dash||!melee_only||!world.config().local_aim_cycle||!terrain||
               distance<=reach||facing<.8f||!enemy.grounded||enemy.dash_ticks||enemy.hit_flash)return false;
            const f32 away=toward.x*enemy.velocity.x+toward.z*enemy.velocity.z;
            if(away<=profile->walk+.5f||away>=profile->dash)return false;
            const auto visits=world.config().dash_duration;
            const f32 travel=profile->dash*visits;
            const f32 gap=distance+(away-profile->dash)*visits;
            const f32 across=(toward.x*enemy.velocity.z-toward.z*enemy.velocity.x)*visits;
            // A running rival opens a longer runway than its current
            // gap. Only use that estimate if the burst could end in
            // striking range while still leaving the normal stop reserve.
            if(gap<reserve||std::hypot(gap,across)>reach||!clear_burst(toward,travel))return false;
            const BattlePoint step{toward.x*travel,toward.z*travel};
            const BattlePoint end{start.x+step.x,start.z+step.z};
            const BattlePoint projected{goal.x+enemy.velocity.x*visits,goal.z+enemy.velocity.z*visits};
            const auto bound=world.config().arena_half-radius;
            if(std::abs(end.x)>bound||std::abs(end.z)>bound||std::abs(projected.x)>bound||std::abs(projected.z)>bound||
               !battle_partition_route_clear(*terrain,goal,projected,radius))return false;
            f32 own_floor=0,enemy_floor=0;
            if(!terrain->height(end.x,end.z,self.position[1],&own_floor)||own_floor!=self.position[1]||
               !terrain->height(projected.x,projected.z,enemy.position[1],&enemy_floor)||enemy_floor!=enemy.position[1])return false;
            for(unsigned i=0;i<world.actor_count();++i){
                const auto& peer=world.actors()[i];
                f32 portion=0;
                if(i!=slot&&i!=target&&peer.health&&std::abs(peer.position[1]-self.position[1])<body_height&&
                   body_blocks_leg(start,step,peer,radius,&portion))return false;
            }
            return true;
        };
        const auto steer_around_ally=[&](BattlePoint direction,f32 room){
            if(profile->movement!=BorgBattleMovement::Grounded||!self.grounded||
               self.dash_ticks||self.hit_flash||state.button||action.move!=BorgBattleActionState::none)return false;
            const auto burst=pad.l?profile->dash*world.config().dash_duration:0.f;
            const auto lookahead=std::min(room,std::max(3*radius,burst));
            if(const auto side=pass_ally(world,slot,start,direction,lookahead,terrain)){
                pad.l=false;
                pad.stick_x=s8(std::lround(127*side->x));
                pad.stick_y=s8(std::lround(127*side->z));
                return true;
            }
            return false;
        };
        const auto boost_travel=[&]()->f32{
            if(!profile->cruise||self.grounded||profile->dash<=profile->walk||
               self.hit_flash||state.button||action.move!=BorgBattleActionState::none)return 0;
            const auto& spec=*profile->cruise;
            // A new boost cannot cancel until the authored minimum. Once
            // latched, release as the leg runs out; the mover owns the latch.
            const unsigned visits=self.package_boosting?1u:spec.minimum;
            return self.package_boost_meter.value736>=int(visits*spec.cost)?profile->dash*visits:0.f;
        };
        const auto boost_toward=[&](BattlePoint destination,f32 reserve){
            const f32 travel=boost_travel();
            if(travel<=0||std::hypot(destination.x-start.x,destination.z-start.z)<=travel+reserve)return false;
            const auto direction=native_borg_direction(start,destination);
            if(std::abs(native_borg_aim_delta(self.facing,direction))>
               std::min(.08f,world.config().turn_per_tick*.5f))return false;
            return clear_burst(direction,travel);
        };
        const auto* candidate=detour?close_strike:ready_attack(target,false);
        bool air_in_range=false,jump_for_melee=false;
        if(!detour&&!candidate)for(const auto& move:profile->moves){
            if(!has_entry_stock(move))continue;
            if(supported(move)&&distance<=attack_reach(move,radius)&&first_stock_ready(*profile,action,move)){
                const bool melee=borg_battle_direct_melee(move);
                if(move.button==preferred&&!melee&&move.context==BorgBattleContext::Airborne)air_in_range=true;
                // Do not invent an aerial variant of a grounded move or
                // assume powered flight. Check the ordinary hop envelope.
                if(melee&&self.grounded&&!profile->cruise&&move.context!=BorgBattleContext::Grounded&&
                   (move.context==BorgBattleContext::Airborne||!height_now(*move.melee))&&
                   melee_height_reachable(*move.melee,self.position[1],enemy.position[1],body_height,
                                          profile->jump,world.config().gravity))
                    jump_for_melee=true;
            }
        }
        if(detour){
            // Keep a legal close strike in place rather than moving away
            // during its windup. Otherwise, steer only along the routed leg.
            pad.stick_x=pad.stick_y=0;pad.l=false;
            const f32 arrival=profile->cruise?std::max(6.f,2*profile->walk):6.f;
            if(!candidate)if(const auto waypoint=route_waypoint(state,*terrain,start,goal,world.config().arena_half,radius,target,tick,arrival)){
                const BattlePoint step{waypoint->x-start.x,waypoint->z-start.z};
                const f32 span=std::hypot(step.x,step.z);
                if(span>0){
                    const f32 pace=std::clamp(span/std::max(12.f,6*profile->walk),.12f,1.f);
                    pad.stick_x=s8(std::lround(127*step.x*pace/span));
                    pad.stick_y=s8(std::lround(127*step.z*pace/span));
                    // A long leg can use an ordinary ground dash. Leave room
                    // to slow before the corner, and check the padded sweep
                    // for this burst rather than dashing at the rival.
                    pad.l=dash_fits(step,span,radius);
                    pad.b=boost_toward(*waypoint,radius);
                    if(!steer_around_ally({step.x/span,step.z/span},span)&&!pad.l&&!pad.b){
                        const auto dash_ticks=world.config().dash_duration;
                        const f32 heading=(self.facing.x*step.x+self.facing.z*step.z)/span;
                        const bool extend_dash=can_dash&&!previous_dash&&heading>=.8f&&
                            profile->dash*dash_ticks>span&&span>=4*profile->walk*dash_ticks;
                        const bool extend_boost=!previous_boost&&!self.package_boosting&&boost_travel()>span;
                        const auto visits=extend_boost?profile->cruise->minimum:dash_ticks;
                        if((extend_dash||extend_boost)&&
                           route_burst_extension_fits(world,slot,*terrain,state.route,state.waypoint,pad,visits,extend_boost)){
                            state.burst_leg=extend_boost?BurstLeg::Boost:BurstLeg::Dash;
                            state.burst_leg_x=s8(pad.stick_x);state.burst_leg_y=s8(pad.stick_y);
                            state.burst_leg_until=u64(tick)+visits;
                            pad.b=extend_boost;pad.l=extend_dash;
                        }
                    }
                }
            }
        }else if(!keep_route_probe){state.route_target=255;state.route.count=0;}
        if(!detour&&short_range){
            const auto stop=std::max(2*radius+4,reach*.77f);
            const f32 pace=profile->cruise?1.f:
                distance>stop||facing<.8f?std::clamp((distance-stop)/70.f,.35f,1.f):0.f;
            pad.stick_x=s8(std::lround(127*toward.x*pace));
            pad.stick_y=s8(std::lround(127*toward.z*pace));
            // Finish a direct burst short of the rival with room to walk
            // into range. A ready move (including X) takes precedence.
            pad.l=!candidate&&(dash_fits(delta,distance,stop)||chase_dash_fits(stop));
            pad.b=!candidate&&boost_toward(goal,std::max(stop,reach-profile->dash));
            if(!candidate&&distance>reach)steer_around_ally(toward,distance-stop);
        }

        if(state.button){
            const bool interrupted=self.hit_flash||(state.release_window&&!(action.armed&state.button));
            const auto* active=action.move<profile->moves.size()?&profile->moves[action.move]:nullptr;
            const bool empty=active&&active->held_fire&&BorgBattleInputs::button(active->button)==state.button&&
                !action.stock_ready(*profile,active->held_fire->stock,active->held_fire->cost);
            if(!interrupted&&!empty&&tick<state.release_at)attack.held=state.button;
            else{
                attack.released=std::exchange(state.button,u8(0));
                if(interrupted||empty)state.next_start=std::min(state.next_start,u64(tick)+12);
            }
            return state.output;
        }
        if(tick<state.next_start||self.hit_flash||self.dash_ticks||action.move!=BorgBattleActionState::none)
            return state.output;
        if(!candidate){
            if(self.grounded&&profile->movement!=BorgBattleMovement::Stationary&&
               (jump_for_melee||(!current_reach&&air_in_range))){
                pad.b=true;pad.l=false;
            }
            return state.output;
        }
        const auto hold=hold_for(*candidate);
        state.button=attack.pressed=attack.held=BorgBattleInputs::button(candidate->button);
        state.release_window=candidate->activation==BorgBattleActivation::ReleaseWindow;
        state.release_at=u64(tick)+hold;
        state.next_start=u64(tick)+std::max(30u,hold+8);
        return state.output;
    }
private:
    static constexpr u32 unsampled=std::numeric_limits<u32>::max();
    enum class BurstLeg:u8 {None,Dash,Boost};
    struct Slot {
        u32 sampled=unsampled;
        u64 release_at=0,next_start=0,replan_at=0,burst_leg_until=0,chase_until=0;
        u8 button=0,route_target=255,chase_target=255;
        s8 burst_leg_x=0,burst_leg_y=0;
        BurstLeg burst_leg=BurstLeg::None;
        bool release_window=false;
        PortableRosterInput output{};
        BattlePoint route_goal{};
        BattlePartitionRoute route{};
        unsigned waypoint=0;
    };
    std::array<Slot,kBattleMvpMaxActors-1> slots_{};

    static bool body_blocks_leg(BattlePoint from,BattlePoint step,const BattleMvpActor& body,f32 radius,f32* fraction){
        const BattlePoint p{body.position[0],body.position[2]};
        return (p.x-from.x)*step.x+(p.z-from.z)*step.z>0&&battle_swept_hit(from,step,p,2*radius,fraction);
    }

    static bool route_burst_extension_fits(const NativeBattleMvp& world,unsigned owner,const IBattleMvpTerrain& terrain,
                                            const BattlePartitionRoute& route,unsigned waypoint,const GamePadState& pad,
                                            unsigned visits,bool cruise){
        if(waypoint+1>=route.count||(!pad.stick_x&&!pad.stick_y))return false; // never extend past the goal
        const auto& self=world.actors()[owner];
        const auto& cfg=world.config();
        const auto& profile=*cfg.packages[owner];
        const auto wish=native_borg_move_direction(native_borg_stick_axis(pad.stick_x),native_borg_stick_axis(pad.stick_y),{});
        if(cruise&&std::abs(native_borg_aim_delta(self.facing,wish))>std::min(.08f,cfg.turn_per_tick*.5f))return false;
        const auto direction=cruise?native_borg_turn(self.facing,wish,cfg.turn_per_tick):native_borg_direction({},wish);
        const f32 travel=profile.dash*visits;
        const BattlePoint from{self.position[0],self.position[2]},step{direction.x*travel,direction.z*travel};
        const BattlePoint to{from.x+step.x,from.z+step.z};
        const auto bound=cfg.arena_half-cfg.actor_radius-4.f;
        if(std::abs(to.x)>bound||std::abs(to.z)>bound||
           !battle_partition_route_clear(terrain,from,to,cfg.actor_radius+4.f))return false;
        const auto remaining=battle_partition_route_length(from,route,waypoint);
        bool progress=false;
        for(unsigned i=waypoint+1;i<route.count&&!progress;++i)
            progress=battle_partition_route_length(to,route,i)+cfg.actor_radius<remaining&&
                battle_partition_route_clear(terrain,to,route.points[i],cfg.actor_radius);
        if(!progress)return false;
        for(unsigned i=0;i<world.actor_count();++i){
            const auto& peer=world.actors()[i];
            f32 portion=0;
            if(i!=owner&&peer.health&&std::abs(peer.position[1]-self.position[1])<cfg.actor_height&&
               body_blocks_leg(from,step,peer,cfg.actor_radius,&portion))return false;
        }
        // Only preview constant-height runs; unknown support or a height
        // change needs ordinary steering, not a long boost commitment.
        const f32 support=self.position[1]-(cruise?profile.cruise->height:0);
        for(unsigned n=1;n<=visits;++n){
            const f32 length=n*profile.dash;
            f32 floor=0;
            if(!terrain.height(from.x+direction.x*length,from.z+direction.z*length,self.position[1],&floor)||
               !std::isfinite(floor)||std::abs(floor-support)>.01f)return false;
        }
        return true;
    }

    static bool follow_charge_reaches(const NativeBattleMvp& world,unsigned owner,unsigned rival,const BorgBattleMove& move,
                                      f32 distance,const IBattleMvpTerrain* terrain){
        const auto& cfg=world.config();
        const auto& self=world.actors()[owner];
        if(!self.grounded||!move.charge||move.charge->ticks<2||move.activation!=BorgBattleActivation::Press||
           !field_only_attack(move))return false;
        const auto& field=*move.field;
        const auto& core=*field.core_damage;
        if(!field.follow||field.socket||field.drift||field.slot||core.interval>=field.ticks)return false;
        const auto& profile=*cfg.packages[owner];
        if(profile.movement!=BorgBattleMovement::Grounded||profile.cruise)return false;
        // The dash counter decrements before movement. Only borrow travel
        // up to the first damaging pulse, not the whole following lifetime.
        const auto visits=std::min<unsigned>(move.charge->ticks-1,unsigned(field.frame)+core.interval);
        const f32 travel=profile.dash*visits;
        if(distance<=travel+2*cfg.actor_radius)return false;
        const BattlePoint from{self.position[0],self.position[2]},step{self.facing.x*travel,self.facing.z*travel};
        const BattlePoint to{from.x+step.x,from.z+step.z};
        const auto bound=cfg.arena_half-cfg.actor_radius;
        if(std::abs(to.x)>bound||std::abs(to.z)>bound)return false;
        if(terrain){
            f32 floor=0;
            if(!battle_partition_route_clear(*terrain,from,to,cfg.actor_radius+4.f)||
               !terrain->height(to.x,to.z,self.position[1],&floor)||floor!=self.position[1])return false;
        }
        for(unsigned i=0;i<world.actor_count();++i){
            const auto& peer=world.actors()[i];
            f32 fraction=0;
            if(i!=owner&&peer.health&&std::abs(peer.position[1]-self.position[1])<cfg.actor_height&&
               body_blocks_leg(from,step,peer,cfg.actor_radius,&fraction))return false;
        }
        const auto& goal=world.actors()[rival].position;
        const f32 x=to.x+self.facing.x*field.forward-goal[0],z=to.z+self.facing.z*field.forward-goal[2];
        const f32 y=self.position[1]+field.height-goal[1]-cfg.actor_height*.5f;
        return x*x+y*y+z*z<=f32(core.radius)*core.radius;
    }

    static bool field_height_allows(const NativeBattleMvp& world,unsigned owner,unsigned rival,const BorgBattleMove& move,
                                    bool allow_other_enemy=true){
        if(!simple_field_height(move))return true;
        const auto& field=*move.field;
        const auto& cfg=world.config();
        const auto& actors=world.actors();
        const auto& self=actors[owner];
        const auto stable_height=[&](unsigned slot){
            const auto& profile=cfg.packages[slot];
            return actors[slot].grounded||(profile&&profile->cruise);
        };
        if(!stable_height(owner)||!stable_height(rival))return true;
        const f32 y=self.position[1]+field.height;
        const auto height_delta=[&](unsigned slot){return y-actors[slot].position[1]-cfg.actor_height*.5f;};
        const f32 radius=field.core_damage->radius;
        if(std::abs(height_delta(rival))<=radius)return true;
        if(!allow_other_enemy)return false;
        // The aimed rival cannot touch the core at these heights. A field
        // can still reach a different enemy without any change to aim.
        const f32 x=self.position[0]+self.facing.x*field.forward;
        const f32 z=self.position[2]+self.facing.z*field.forward;
        for(unsigned i=0;i<world.actor_count();++i){
            if(i==rival||!actors[i].health||actors[i].team==self.team)continue;
            const f32 dx=x-actors[i].position[0],dz=z-actors[i].position[2];
            const f32 dy=stable_height(i)?height_delta(i):0.f; // an ordinary airborne body can descend
            if(dx*dx+dy*dy+dz*dz<=radius*radius)return true;
        }
        return false;
    }

    // Local steering only. A snapshot of the solid peers can suggest a
    // sidestep; the mover still sweeps against their accepted positions.
    // No value means no ally obstructs the leg; a zero vector means wait.
    static std::optional<BattlePoint> pass_ally(const NativeBattleMvp& world,unsigned owner,BattlePoint start,
                                                BattlePoint toward,f32 lookahead,const IBattleMvpTerrain* terrain){
        if(lookahead<=0)return {};
        const auto& self=world.actors()[owner];
        const auto& cfg=world.config();
        const auto solid=[&](unsigned i){
            const auto& other=world.actors()[i];
            return i!=owner&&other.health&&std::abs(other.position[1]-self.position[1])<cfg.actor_height;
        };
        const auto blocks=[&](const BattleMvpActor& other,BattlePoint step,f32* fraction){
            return body_blocks_leg(start,step,other,cfg.actor_radius,fraction);
        };
        const BattlePoint forward{toward.x*lookahead,toward.z*lookahead};
        const BattleMvpActor* blocker=nullptr;
        f32 first=1.f;
        for(unsigned i=0;i<world.actor_count();++i){
            const auto& other=world.actors()[i];
            f32 fraction=0;
            if(solid(i)&&other.team==self.team&&blocks(other,forward,&fraction)&&fraction<=first){
                blocker=&other;first=fraction;
            }
        }
        if(!blocker)return {};
        const BattlePoint lateral{-toward.z,toward.x};
        const f32 offset=(blocker->position[0]-start.x)*lateral.x+(blocker->position[2]-start.z)*lateral.z;
        // Prefer the side away from the blocker. These local leg probes
        // are steering hints, not a change to body contacts.
        const f32 sign=offset>0?-1.f:1.f;
        const auto sidestep=[&](f32 side,f32 span)->std::optional<BattlePoint>{
            const BattlePoint dir{side*lateral.x,side*lateral.z};
            const BattlePoint step{dir.x*span,dir.z*span},end{start.x+step.x,start.z+step.z};
            const auto bound=cfg.arena_half-cfg.actor_radius;
            if(std::abs(end.x)>bound||std::abs(end.z)>bound||
               (terrain&&!battle_partition_route_clear(*terrain,start,end,cfg.actor_radius+4.f)))return {};
            for(unsigned i=0;i<world.actor_count();++i){
                f32 fraction=0;
                if(solid(i)&&blocks(world.actors()[i],step,&fraction))return {};
            }
            return dir;
        };
        const auto span=2*cfg.actor_radius+8.f;
        for(const auto side:{sign,-sign})if(const auto dir=sidestep(side,span))return dir;
        // A parked ally that is already off the approach line needs
        // less sideways travel on the open flank. Keep the normal
        // preferences above; only shorten the probe before giving up.
        const auto needed=std::max(8.f,span-std::abs(offset));
        if(needed<span)if(const auto dir=sidestep(sign,needed))return dir;
        return BattlePoint{};
    }

    static bool melee_height_reachable(const BorgBattleMelee& shape,f32 from,f32 target,f32 body_height,
                                       f32 jump_speed,f32 gravity){
        // Native ascent adds v then subtracts gravity. Sum its nonnegative
        // steps as a height bound, not a forecast of the actual hit frame.
        const f64 steps=std::floor(f64(jump_speed)/gravity)+1;
        const f64 apex=steps*jump_speed-.5*f64(gravity)*steps*(steps-1);
        const f64 bottom=f64(from)+shape.height-shape.half_height;
        const f64 top=f64(from)+shape.height+shape.half_height+apex;
        return target<=top&&f64(target)+body_height>=bottom;
    }

    static bool route_needs_replan(const Slot& state,const IBattleMvpTerrain& terrain,BattlePoint start,BattlePoint goal,
                                  f32 radius,u8 target,u32 tick){
        return state.route_target!=target||tick>=state.replan_at||
            std::hypot(goal.x-state.route_goal.x,goal.z-state.route_goal.z)>64||
            (state.waypoint<state.route.count&&!battle_partition_route_clear(terrain,start,state.route.points[state.waypoint],radius));
    }
    static void remember_route(Slot& state,const BattlePartitionRoute& route,BattlePoint goal,u8 target,u32 tick){
        state.route=route;state.route_target=target;state.route_goal=goal;state.replan_at=u64(tick)+36;state.waypoint=0;
    }
    static std::optional<BattlePoint> route_waypoint(Slot& state,const IBattleMvpTerrain& terrain,BattlePoint start,BattlePoint goal,
                                                      f32 arena_half,f32 radius,u8 target,u32 tick,f32 arrival){
        const auto clear=[&](BattlePoint to){return battle_partition_route_clear(terrain,start,to,radius);};
        if(route_needs_replan(state,terrain,start,goal,radius,target,tick))
            remember_route(state,battle_partition_route(terrain,start,goal,arena_half,radius),goal,target,tick);
        if(state.waypoint>=state.route.count)return {};
        while(state.waypoint+1<state.route.count){
            const auto point=state.route.points[state.waypoint];
            if(std::hypot(point.x-start.x,point.z-start.z)>arrival||!clear(state.route.points[state.waypoint+1]))break;
            ++state.waypoint;
        }
        return state.route.points[state.waypoint];
    }

    static bool damaging_field(const BorgBattleMove& move){
        return move.field&&move.field->core_damage&&
            (move.field->core_damage->scope==BorgFieldScope::Enemies||move.field->core_damage->scope==BorgFieldScope::All);
    }
    static bool field_only_attack(const BorgBattleMove& move){
        return !move.melee&&!move.held_fire&&move.releases.empty()&&damaging_field(move);
    }
    static bool simple_field_height(const BorgBattleMove& move){
        return field_only_attack(move)&&!move.charge&&!move.field->socket&&!move.field->drift&&!move.field->slot;
    }
    static bool supported(const BorgBattleMove& move){
        return (move.melee||move.held_fire||!move.releases.empty()||damaging_field(move))&&
            (move.activation==BorgBattleActivation::Press||
             (move.activation==BorgBattleActivation::ReleaseWindow&&move.maximum_hold));
    }
    static bool first_stock_ready(const BorgGameplayProfile& profile,const BorgBattleActionState& action,
                                  const BorgBattleMove& move){
        if(move.held_fire)return action.stock_ready(profile,move.held_fire->stock,move.held_fire->cost);
        // A field-only attack has no other payload if placement cannot pay.
        // Do not gate mixed attacks on an optional field; the native marker
        // still owns each debit and can refuse it independently.
        if(field_only_attack(move)&&move.field->debit){
            const auto& debit=*move.field->debit;
            for(unsigned i=0;i<profile.stocks.size();++i)if(profile.stocks[i].id==debit.stock)
                return debit.cost<=std::numeric_limits<u8>::max()&&action.stock_ready(profile,u8(i),u8(debit.cost));
            return false;
        }
        if(move.melee||damaging_field(move)||move.releases.empty())return true;
        const auto& first=move.releases.front();
        return action.stock_ready(profile,first.stock,first.cost);
    }
    static f32 attack_reach(const BorgBattleMove& move,f32 body_radius){
        f32 reach=move.entry_range?move.entry_range:std::numeric_limits<f32>::infinity();
        if(move.melee)reach=std::min(reach,move.melee->reach+move.melee->radius+body_radius);
        else if(!move.held_fire&&move.releases.empty()&&damaging_field(move))
            reach=std::min(reach,move.field->core_damage->radius+std::max(0.f,move.field->forward));
        return reach;
    }
    static unsigned hold_for(const BorgBattleMove& move){
        if(move.activation==BorgBattleActivation::ReleaseWindow)
            return std::min<unsigned>(unsigned(move.minimum_hold)+1,move.maximum_hold);
        return move.held_fire?std::max(18u,unsigned(move.held_fire->slow_interval)+1):18u;
    }
};

} // namespace gf
