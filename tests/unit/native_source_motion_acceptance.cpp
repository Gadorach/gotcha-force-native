#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native source contact: %s\n",why);std::abort();}}
struct Ledge:gf::IBattleMvpTerrain{bool height(float x,float,float,float* y)const override{*y=x>4?-200.f:0.f;return true;}};
struct Missing:gf::IBattleMvpTerrain{bool height(float,float,float,float*)const override{return false;}};
}
int main(){
    using namespace gf;NativeBattleMvp world,neighbor;NativeBorgMotionObservation observed;
    check(!world.observe_motion(&observed)&&!world.new_motion_producer(),"prepared/missing world cannot issue movement ownership");
    check(world.start()&&neighbor.start(),"two independent native worlds");
    auto producer=world.new_motion_producer(),foreign=world.new_motion_producer();
    check(producer&&foreign&&producer!=foreign&&world.observe_motion(&observed),"distinct pending producers in one current native world");
    NativeBorgMotionOffer offer{observed,observed.position,observed.facing,BattleMvpAction::GroundDash,1,producer};
    offer.proposed_position[0]+=25;
    const auto before_neighbor=neighbor.tick();
    check(!neighbor.step({},false,nullptr,nullptr,nullptr,&offer)&&neighbor.tick()==before_neighbor,"same-model neighbor rejects foreign world observation");
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==1&&world.actors()[0].position==offer.proposed_position,"source proposal enters existing native actor only after shared response");
    NativeBorgMotionReceipt receipt;
    check(world.motion_receipt(offer,&receipt)&&world.motion_receipt_current(receipt)&&receipt.serial()==1&&receipt.tick()==1,"native world issues exact accepted-contact receipt");
    auto edited=offer;edited.proposed_position[0]+=1;
    check(!world.motion_receipt(edited,&receipt),"edited proposal cannot borrow accepted receipt");
    edited=offer;edited.observed.model=10;
    check(!world.motion_receipt(edited,&receipt),"edited actor identity cannot borrow accepted receipt");
    edited=offer;edited.producer=foreign;
    check(!world.motion_receipt(edited,&receipt),"different producer cannot borrow identical-motion receipt");
    check(!world.step({},false)&&world.tick()==1,"source actor cannot silently fall back to legacy movement");
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==1,"old observation cannot be replayed");
    check(world.observe_motion(&observed),"next current source observation");
    edited={observed,observed.position,observed.facing,BattleMvpAction::Idle,2,foreign};
    check(!world.step({},false,nullptr,nullptr,nullptr,&edited)&&world.tick()==1,"only accepted producer can continue source actor");
    NativeBattleMvp copied=world;
    check(!copied.observe_motion(&observed)&&!copied.new_motion_producer(),"copying a world does not duplicate source authority");
    check(world.motion_receipt(offer,&receipt),"restore valid accepted receipt");
    check(world.start()&&!world.motion_receipt_current(receipt),"restart retires old world receipt");
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==0,"restart invalidates old producer/world offer");
    check(world.step({},false)&&world.tick()==1,"explicit fresh restart restores normal MVP mode");
    {
        NativeBattleMvp span_world;check(span_world.start(),"source-offer span spawn");
        auto span_producer=span_world.new_motion_producer();NativeBorgMotionObservation span_observed;
        check(span_world.observe_motion(&span_observed),"span observation");
        NativeBorgMotionOffer span_offer{span_observed,span_observed.position,span_observed.facing,BattleMvpAction::Idle,1,span_producer};
        const std::array<const NativeBorgMotionOffer*,2> span_offers{&span_offer,nullptr};
        std::array<GamePadState,2> span_pads{};std::array<BorgBattleInputs,2> span_inputs{};
        check(span_world.step_roster(span_pads,span_inputs,false,nullptr,{},nullptr,nullptr,nullptr,span_offers)&&
              span_world.actors()[0].source_ground_motion,"per-actor source-offer span accepts actor 0");
        NativeBorgMotionReceipt span_receipt;
        check(span_world.motion_receipt(span_offer,&span_receipt,0),"per-actor receipt reads actor 0");
    }
    BattleMvpConfig close;close.spawn={BattlePoint{-100,0},BattlePoint{100,0}};
    check(world.start(close),"native source body-sweep spawn");producer=world.new_motion_producer();
    bool body_hit=false;
    for(unsigned tick=0;tick<30;++tick){
        check(world.observe_motion(&observed),"body-sweep current observation");
        offer={observed,observed.position,observed.facing,BattleMvpAction::GroundDash,tick+1,producer};offer.proposed_position[0]+=25;
        check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt),"source proposal shares swept actor response");
        body_hit|=receipt.contact().body_hit;
        check(world.actors()[1].position[0]-world.actors()[0].position[0]>=2*close.actor_radius-.001f,"source-driven actor cannot pass through opponent");
    }
    check(body_hit,"clipped native source receipt records body contact");
    BattleMvpConfig edge;edge.spawn={BattlePoint{0,0},BattlePoint{-700,-700}};Ledge ledge;
    check(world.start(edge,&ledge),"ledge acceptance fixture");producer=world.new_motion_producer();check(world.observe_motion(&observed),"ledge observation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::GroundDash,1,producer};offer.proposed_position[0]=25;
    check(!world.step({},false,&ledge,nullptr,nullptr,&offer)&&world.tick()==0&&world.actors()[0].position==observed.position,"ground-only source offer refuses airborne tail before world mutation");
    check(world.start(edge),"height mismatch fixture");producer=world.new_motion_producer();check(world.observe_motion(&observed),"height observation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Idle,1,producer};offer.proposed_position[1]=1;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==0,"source/native floor disagreement is not silently acknowledged");
    offer.proposed_position=observed.position;offer.action=BattleMvpAction::Jump;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==0,"air action outside ground subset refuses");
    offer.action=BattleMvpAction::Idle;Missing missing;
    check(!world.step({},false,&missing,nullptr,nullptr,&offer)&&world.tick()==0,"missing acceptance terrain leaves world untouched");
    offer.action=BattleMvpAction::Jump;offer.kind=NativeBorgMotionKind::VerticalTakeoff;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==0,"ground-only producer cannot opt itself into vertical motion");
    check(world.start(),"vertical phase fixture");producer=world.new_motion_producer(true);
    check(producer&&world.observe_motion(&observed),"explicit native vertical producer");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,1,producer};
    offer.kind=NativeBorgMotionKind::VerticalAir;offer.proposed_position[1]=15;offer.vertical_speed=15;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==0,"air cannot skip accepted takeoff");
    offer.kind=NativeBorgMotionKind::VerticalTakeoff;offer.proposed_position=observed.position;offer.vertical_speed=0;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt),"takeoff accepted while still grounded");
    check(world.observe_motion(&observed),"takeoff observation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,2,producer};
    offer.kind=NativeBorgMotionKind::VerticalAir;offer.proposed_position[1]+=15;offer.vertical_speed=15;
    auto invalid=offer;invalid.proposed_position[0]+=1;
    check(!world.step({},false,nullptr,nullptr,nullptr,&invalid)&&world.tick()==1,"vertical producer cannot smuggle lateral air movement");
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt)&&!world.actors()[0].grounded&&world.actors()[0].vertical_speed==15,"airborne position/velocity accepted without native MVP gravity");
    invalid=offer;invalid.observed.vertical_speed=1;
    check(!world.motion_receipt(invalid,&receipt),"edited observed vertical speed cannot borrow receipt");
    invalid=offer;invalid.kind=NativeBorgMotionKind::Ground;
    check(!world.motion_receipt(invalid,&receipt),"edited movement kind cannot borrow receipt");
    check(!world.step({},false)&&world.tick()==2,"airborne source actor cannot fall back to MVP gravity");
    check(world.observe_motion(&observed),"airborne observation");
    invalid={observed,observed.position,observed.facing,BattleMvpAction::Jump,3,producer};
    invalid.kind=NativeBorgMotionKind::VerticalTouchdown;invalid.proposed_position[1]=0;
    check(!world.step({},false,nullptr,nullptr,nullptr,&invalid)&&world.tick()==2,"rising actor cannot invent touchdown");
    bool touchdown=false;
    for(unsigned n=0;n<100&&!touchdown;++n){
        check(world.observe_motion(&observed),"current vertical observation");
        offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,n+3,producer};
        offer.proposed_position[1]+=observed.vertical_speed;offer.vertical_speed=observed.vertical_speed-1;
        touchdown=offer.proposed_position[1]<=0;
        offer.kind=touchdown?NativeBorgMotionKind::VerticalTouchdown:NativeBorgMotionKind::VerticalAir;
        if(touchdown){offer.proposed_position[1]=0;offer.vertical_speed=0;}
        check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt),"bounded native vertical phase/contact acceptance");
    }
    check(touchdown&&world.actors()[0].grounded&&world.actors()[0].vertical_speed==0,"touchdown clips only descending crossing to native floor");
    check(world.observe_motion(&observed),"touchdown observation");
    invalid={observed,observed.position,observed.facing,BattleMvpAction::Idle,1000,producer};
    check(!world.step({},false,nullptr,nullptr,nullptr,&invalid),"touchdown cannot skip landing phase");
    offer=invalid;offer.kind=NativeBorgMotionKind::VerticalLanding;offer.action=BattleMvpAction::Land;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt),"explicit landing phase accepts grounded pose");
    check(world.observe_motion(&observed),"landing observation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Idle,1001,producer};
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt)&&world.motion_receipt_current(receipt),"landing may return to source ground ownership");
    producer.reset();check(!world.motion_receipt_current(receipt),"vertical producer retirement revokes old receipt");
    check(world.start(close),"moving jump contact fixture");
    producer=world.new_motion_producer(NativeBorgMotionPolicy::Jump);auto vertical_only=world.new_motion_producer(true);
    check(world.observe_motion(&observed),"moving jump observation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,1,vertical_only};offer.kind=NativeBorgMotionKind::JumpTakeoff;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==0,"vertical-only producer cannot authorize moving-jump family");
    offer.producer=producer;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer),"explicit moving-jump takeoff");
    check(world.observe_motion(&observed),"moving launch observation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,2,producer};
    offer.kind=NativeBorgMotionKind::JumpAir;offer.proposed_position[1]=15;offer.vertical_speed=15;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer),"moving-capable airborne entry");
    bool clipped=false,overhead_refused=false;
    for(unsigned n=0;n<60&&!overhead_refused;++n){
        check(world.observe_motion(&observed),"moving airborne observation");
        offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,n+3,producer};
        offer.kind=NativeBorgMotionKind::JumpAir;offer.proposed_position[0]=std::min(100.f,observed.position[0]+20);
        offer.proposed_position[1]+=observed.vertical_speed;offer.vertical_speed=observed.vertical_speed-1;
        const auto tick=world.tick();
        if(!world.step({},false,nullptr,nullptr,nullptr,&offer)){
            check(world.tick()==tick&&world.actors()[0].position==observed.position&&observed.position[1]>0&&
                offer.proposed_position[1]<close.actor_height&&std::abs(observed.position[0]-100)<.001f,"unknown descending vertical body overlap refuses before mutation");
            overhead_refused=true;
        }else{
            check(world.motion_receipt(offer,&receipt),"moving air contact receipt");
            clipped|=receipt.contact().body_hit;
            if(receipt.contact().body_hit)check(world.actors()[1].position[0]-world.actors()[0].position[0]>=2*close.actor_radius-.001f,"air segment shares bounded native opponent sweep");
        }
    }
    check(clipped&&overhead_refused,"moving air motion clips horizontal contact and refuses unsupported standing-on-Borg response");
    NativeBorgContactRequest crossing;crossing.position={-100,130,0};crossing.other_position={0,0,0};crossing.other_active=true;crossing.grounded=false;crossing.delta={30,0};
    NativeBorgContactResult contact;
    check(native_borg_resolve_contact(crossing,nullptr,&contact)&&!contact.body_hit,"ordinary endpoint-only proxy remains unchanged without vertical segment declaration");
    crossing.vertical_end_y=90;
    check(native_borg_resolve_contact(crossing,nullptr,&contact)&&contact.body_hit&&contact.position[0]<-84,"declared descending segment cannot tunnel through vertical body slab");
    check(world.start(),"air-dash capability fixture");producer=world.new_motion_producer(NativeBorgMotionPolicy::Jump);
    check(world.observe_motion(&observed),"jump-only observer");
    offer={observed,observed.position,observed.facing,BattleMvpAction::AirDash,1,producer};offer.kind=NativeBorgMotionKind::AirDashWindupEntry;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==0,"ordinary jump producer cannot opt into dash velocity-reset phases");
    producer=world.new_motion_producer(NativeBorgMotionPolicy::AirDash);
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,1,producer};offer.kind=NativeBorgMotionKind::JumpTakeoff;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"air-dash capable native takeoff");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,2,producer};
    offer.kind=NativeBorgMotionKind::JumpAir;offer.proposed_position[1]=15;offer.vertical_speed=15;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"accepted air state before dash");
    offer={observed,observed.position,observed.facing,BattleMvpAction::AirDash,3,producer};
    offer.kind=NativeBorgMotionKind::AirDashActiveEntry;offer.vertical_speed=-.07f;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==2,"active dash cannot skip accepted windup");
    offer.kind=NativeBorgMotionKind::AirDashWindupEntry;offer.vertical_speed=7.5f;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"explicit windup entry may pause translation and change vertical speed");
    offer={observed,observed.position,observed.facing,BattleMvpAction::AirDash,4,producer};offer.kind=NativeBorgMotionKind::AirDashWindup;
    offer.proposed_position[1]+=observed.vertical_speed;offer.vertical_speed=7.43f;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"windup movement resumes exact accepted velocity");
    offer={observed,observed.position,observed.facing,BattleMvpAction::AirDash,5,producer};offer.kind=NativeBorgMotionKind::AirDashActiveEntry;offer.vertical_speed=-.07f;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt),"active entry declares its zero-vertical launch step separately");
    invalid=offer;invalid.kind=NativeBorgMotionKind::JumpAir;
    check(!world.motion_receipt(invalid,&receipt),"dash reset receipt cannot be relabelled as ordinary integration");
    check(world.observe_motion(&observed),"active dash observation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,6,producer};offer.kind=NativeBorgMotionKind::JumpAir;
    offer.proposed_position[1]+=observed.vertical_speed;offer.vertical_speed=-.14f;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==5,"active dash cannot skip recovery to ordinary fall");
    offer.kind=NativeBorgMotionKind::AirDashActive;offer.action=BattleMvpAction::AirDash;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"explicit active dash continuation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::AirDash,7,producer};offer.kind=NativeBorgMotionKind::AirDashRecovery;
    offer.proposed_position[1]+=observed.vertical_speed;offer.vertical_speed=-1.14f;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"source recovery continues accepted old-velocity integration");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,8,producer};offer.kind=NativeBorgMotionKind::JumpAir;
    offer.proposed_position[1]+=observed.vertical_speed;offer.vertical_speed=-2.14f;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer),"recovery can return to ordinary source falling");
    check(world.observe_motion(&observed),"pre-relaunch policy observation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,9,producer};offer.kind=NativeBorgMotionKind::RelaunchEntry;
    offer.entry_vertical_delta=15;offer.proposed_position[1]+=observed.vertical_speed+15;offer.vertical_speed=18;
    const auto old_tick=world.tick();
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==old_tick,"air-dash producer cannot opt into additive relaunch");
    check(world.start(),"relaunch capability fixture");producer=world.new_motion_producer(NativeBorgMotionPolicy::Relaunch);
    check(world.observe_motion(&observed),"relaunch-capable native world");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,1,producer};offer.kind=NativeBorgMotionKind::JumpTakeoff;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"takeoff before relaunch");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,2,producer};offer.kind=NativeBorgMotionKind::RelaunchEntry;
    offer.entry_vertical_delta=15;offer.proposed_position[1]+=15;offer.vertical_speed=18;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==1,"relaunch cannot replace first accepted airborne entry");
    offer.kind=NativeBorgMotionKind::JumpAir;offer.entry_vertical_delta=0;offer.vertical_speed=15;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"ordinary air state before additive relaunch");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,3,producer};offer.kind=NativeBorgMotionKind::JumpAir;
    offer.proposed_position[1]+=observed.vertical_speed+15;offer.vertical_speed=18;offer.entry_vertical_delta=15;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==2,"ordinary integration cannot borrow relaunch additive delta");
    offer.kind=NativeBorgMotionKind::RelaunchEntry;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt),"explicit additive relaunch entry accepts old velocity plus declared step");
    invalid=offer;invalid.entry_vertical_delta+=1;
    check(!world.motion_receipt(invalid,&receipt),"edited relaunch delta cannot borrow exact native receipt");
    check(world.observe_motion(&observed),"relaunch entry observation");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,4,producer};offer.kind=NativeBorgMotionKind::RelaunchActive;
    offer.proposed_position[1]+=observed.vertical_speed;offer.vertical_speed=18;
    check(!world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.tick()==3,"relaunch cannot skip windup movement phase");
    offer.kind=NativeBorgMotionKind::RelaunchWindup;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"relaunch windup resumes exact old-velocity integration");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,5,producer};offer.kind=NativeBorgMotionKind::RelaunchActive;
    offer.proposed_position[1]+=observed.vertical_speed;offer.vertical_speed=18;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"windup may enter source active relaunch");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Jump,6,producer};offer.kind=NativeBorgMotionKind::JumpAir;
    offer.proposed_position[1]+=observed.vertical_speed;offer.vertical_speed=17;
    check(world.step({},false,nullptr,nullptr,nullptr,&offer),"relaunch release may return to ordinary source falling");
    std::printf("native source motion ownership/contact/receipt refusals: %u checks PASS; native proxies, not original contact/frame equivalence\n",checks);
}
