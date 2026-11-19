#include <cstdio>
#include <cstdlib>
#include "engine/assets/borg_move_profile.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg move profile: %s\n",why);std::abort();}
}
std::string replace(std::string text,std::string_view search,std::string_view value){
    const auto at=text.find(search);check(at!=std::string::npos,"test field is present");
    text.replace(at,search.size(),value);
    return text;
}
}
int main(){
    using namespace gf;
    const std::string moves=R"(GFBORG-MOVES 1
move=bolt|base|b|projectile|Training Bolt
contexts=grounded|air-dash
fallback=none
ammo=finite:5|unresolved
refill=all-at-once
activation=press
guard=has-ammo
priority=30
replenishment=magazine-refresh
interrupt=unresolved
phase=flight|shot-spawn|fire|bolt-trail|bolt-sound|non-solid|borgs|none
phase=impact|shot-contact|none|bolt-flash|unresolved|not-applicable|not-applicable|on-contact
move=burst|base|b|projectile|Training Burst
contexts=unresolved
fallback=bolt
ammo=unlimited
refill=unresolved
activation=charge
guard=charge-ready
priority=10
replenishment=unresolved
interrupt=charge-cancel
phase=burst|release|unresolved|burst-flash|none|solid|partial|present
outcome=ally-buff|shot-contact|stat-modifier|ally-hit|buff-life|stat-bonus
outcome=enemy-inversion|shot-contact|control-modifier|enemy-hit|debuff-life|invert-controls
move=spin|air-form|none|special|Contact Spin
contexts=contact
fallback=unresolved
ammo=none
refill=none
activation=automatic
guard=contact-enable
priority=unresolved
replenishment=none
interrupt=unresolved
phase=contact|contact|none|unresolved|none|not-applicable|not-applicable|not-applicable
outcome=contact-hit|contact|passive-contact|contact-target|none|spin-hit
move=melee|base|b|melee|Training Punch
contexts=grounded
fallback=none
ammo=none
refill=none
activation=press
guard=target-close
priority=20
replenishment=none
interrupt=unresolved
phase=punch|start|punch|none|none|not-applicable|not-applicable|not-applicable
move=impact|base|x|special|Landing Impact
contexts=airborne
fallback=none
ammo=none
refill=none
activation=press
guard=above-ground
priority=10
replenishment=none
interrupt=destroyed-before-contact
phase=landing|grounded-contact|landing|impact-flash|impact-sound|not-applicable|not-applicable|present
outcome=blast|grounded-contact|area-impact|impact-area|none|area-payload
move=dry|base|b|no-action|No Action
contexts=grounded
fallback=none
ammo=none
refill=none
activation=press
guard=empty-and-out-of-range
priority=40
replenishment=none
interrupt=none
)";
    BorgMoveProfile profile;
    check(borg_read_move_profile(moves,&profile)&&profile.moves.size()==6,"one profile holds base and alternate form moves");
    const auto* bolt=profile.find("bolt");const auto* burst=profile.find("burst");const auto* spin=profile.find("spin");
    check(bolt&&burst&&spin&&bolt->button==BorgActionButton::B&&bolt->kind==BorgActionKind::Projectile&&
        bolt->activation==BorgMoveActivation::Press&&bolt->contexts==BorgMoveContextRule::Listed&&bolt->context_mask==9&&
        bolt->phases.size()==2&&bolt->fallback=="none"&&bolt->priority==30&&bolt->replenishment=="magazine-refresh"&&
        burst->button==BorgActionButton::B&&burst->activation==BorgMoveActivation::Charge&&
        profile.find("melee")->kind==BorgActionKind::Melee&&profile.find("dry")->kind==BorgActionKind::NoAction,
        "one button can select melee, shots, a charged branch, or an explicit no-op");
    check(bolt->ammo==BorgMoveAmmoKind::Finite&&bolt->ammo_at_level[0]==5&&!bolt->ammo_at_level[1]&&
        bolt->refill==BorgMoveRefill::AllAtOnce&&burst->ammo==BorgMoveAmmoKind::Unlimited&&spin->ammo==BorgMoveAmmoKind::None,
        "known endpoint, unknown endpoint, unlimited and not-applicable ammo never collapse");
    const auto& flight=bolt->phases[0];const auto& contact=bolt->phases[1];
    check(flight.solidity==BorgMoveSolidity::NonSolid&&flight.penetration==BorgMovePenetration::Borgs&&
        flight.explosion==BorgMoveExplosion::None&&flight.animation_role=="fire"&&flight.sound_cue=="bolt-sound"&&
        contact.explosion==BorgMoveExplosion::OnContact&&contact.sound_cue=="unresolved"&&
        burst->phases[0].explosion==BorgMoveExplosion::Present,
        "projectile phases support distinct mechanics without inferring an unknown trigger or cue");
    check(burst->outcomes.size()==2&&burst->outcomes[0].kind==BorgMoveOutcomeKind::StatModifier&&
        burst->outcomes[0].target==BorgMoveOutcomeTarget::AllyHit&&burst->outcomes[0].duration=="buff-life"&&
        burst->outcomes[1].kind==BorgMoveOutcomeKind::ControlModifier&&burst->outcomes[1].target==BorgMoveOutcomeTarget::EnemyHit&&
        spin->button==BorgActionButton::None&&spin->outcomes[0].kind==BorgMoveOutcomeKind::PassiveContact,
        "mechanical outcomes and targeted status effects are stored separately from presentation");
    const auto* impact=profile.find("impact");
    check(impact&&impact->kind==BorgActionKind::Special&&impact->interrupt=="destroyed-before-contact"&&
        impact->outcomes[0].kind==BorgMoveOutcomeKind::AreaImpact&&impact->outcomes[0].event=="grounded-contact"&&
        impact->outcomes[0].payload=="area-payload","an area outcome may be tied to contact and a separate interrupt reference");
    for(const auto& invalid:{
        replace(moves,"GFBORG-MOVES 1","GFBORG-MOVES 2"),
        replace(moves,"move=burst","move=bolt"),
        replace(moves,"phase=impact","phase=flight"),
        replace(moves,"outcome=enemy-inversion","outcome=ally-buff"),
        replace(moves,"control-modifier|enemy-hit","control-modifier|all"),
        replace(moves,"fallback=none","fallback=burst"),
        replace(moves,"fallback=bolt","fallback=spin"),
        replace(moves,"fallback=bolt","fallback=missing"),
        replace(moves,"move=burst|base|b","move=burst|base|x"),
        replace(moves,"contexts=grounded|air-dash","contexts=grounded|grounded"),
        replace(moves,"contexts=grounded|air-dash","contexts=grounded|any"),
        replace(moves,"ammo=finite:5|unresolved","ammo=finite:5"),
        replace(moves,"ammo=finite:5|unresolved","ammo=finite:65536|unresolved"),
        replace(moves,"refill=all-at-once\n",""),
        replace(moves,"refill=all-at-once","refill=all-at-once\nrefill=gradual"),
        replace(moves,"replenishment=magazine-refresh","replenishment=../timer"),
        replace(moves,"priority=30","priority=-30"),
        moves+"outcome=noop|unresolved|damage|enemy-hit|none|unresolved\n",
        replace(moves,"fire|bolt-trail","../outside|bolt-trail"),
        replace(moves,"not-applicable|on-contact","not-applicable|yes"),
        moves+std::string(kBorgMoveProfileBytes,'\n')
    }){
        check(!borg_read_move_profile(invalid,&profile)&&profile.moves.size()==6&&profile.find("bolt")->fallback=="none",
            "bad reference, cycle, duplicate, incomplete record or out-of-domain data refuses atomically");
    }
    std::puts("Borg move profiles: form, context, phases, unknowns and fallback validation PASS");
}
