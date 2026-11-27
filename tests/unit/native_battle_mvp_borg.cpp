#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/native_battle_mvp_borg.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"MVP Borg animation: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void pose_followup(gf::ByteView model,gf::ByteView prefix,gf::ByteView pose,gf::ByteView motions){
    using namespace gf;
    const std::string common="GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|8|13|15\n"
        "health=180\nrig-prefix=body.prefix\npose-profile=body.pose\n"
        "idle=0:0\nwalk=0:1\nair=0:7\ndash=0:2\nmelee=slash|6|8|88|38|52|44|12\n"
        "contact-pose-hold-receive=normal\n";
    auto profile=[&](const std::string& rows){
        auto p=std::make_shared<BorgGameplayProfile>();std::string error;
        if(!borg_read_gameplay_profile(common+rows,p.get(),&error))check(false,error.c_str());
        return p;
    };
    const std::string lead_rules="move=slash|b|10|grounded|12|0:0\n";
    auto switched=profile(lead_rules+"pose-followup=slash|4|0:1|2\n");
    auto lead=profile(lead_rules);
    auto strike=profile("move=slash|b|10|grounded|12|0:1\npose-start=slash|2\n");
    NativeBattleMvpBorg body,before,after;
    auto load=[&](NativeBattleMvpBorg& into,const auto& rules){
        std::string error;
        if(!into.load_package(rules,model,prefix,pose,motions,&error))check(false,error.c_str());
    };
    load(body,switched);load(before,lead);load(after,strike);
    auto sample=[](NativeBattleMvpBorg& into,u16 frame,u32 tick){
        BattleMvpActor actor;actor.package_move=0;actor.package_frame=frame;
        check(into.update(actor,tick,nullptr),"follow-up fixture samples a live package pose");
        std::array<BattleMvpVec,20> points{};
        for(unsigned bone=0;bone<points.size();++bone)
            check(into.bone_point(bone,{2,3,4},&points[bone]),"follow-up fixture samples each transformed bone");
        return points;
    };
    check(sample(body,3,3)==sample(before,3,3),"the switch does not affect the lead-in");
    sample(after,0,3); // Match the locomotion clock; only the action clock is reset.
    const auto switched_pose=sample(body,4,4);
    check(switched_pose==sample(after,0,4),"switch tick samples the declared starting frame of the exact second clip");
    check(switched_pose!=sample(before,4,4),"switch tick does not keep the lead-in pose");
    check(sample(body,7,7)==sample(after,3,7),"the second clip clock is relative to the switch tick");
    check(sample(body,0,0)==sample(before,0,0),"restart does not keep the second clip's clock");
    NativeBattleMvpBorg live;
    load(live,switched);
    body.reset_control_test_pose();
    sample(body,3,3);sample(live,3,3);
    BattleMvpVec low{},high{},held_low{},held_high{},live_low{},live_high{};
    check(body.bounds(&low,&high),"bound moving skeleton before pause");
    BattleMvpActor held;held.package_move=0;held.package_frame=4;held.contact_pose_hold_ticks=2;
    check(body.update(held,4,nullptr)&&body.package_pose_held()&&body.bounds(&held_low,&held_high)&&
          held_low==low&&held_high==high,"held visual bounds use the earlier local sample across a clip switch");
    sample(live,4,4);
    check(live.bounds(&live_low,&live_high)&&(live_low!=low||live_high!=high),"the live clip would change the visible geometry");
    for(unsigned bone=0;bone<20;++bone){
        BattleMvpVec held_bone{},live_bone{};
        check(body.bone_point(bone,{2,3,4},&held_bone)&&live.bone_point(bone,{2,3,4},&live_bone)&&held_bone==live_bone,
              "held visual does not leak into the authoritative bone sample");
    }
    held.package_frame=5;held.contact_pose_hold_ticks=1;held.position={15,0,0};
    check(body.update(held,5,nullptr)&&body.package_pose_held()&&body.bounds(&held_low,&held_high)&&
          std::abs(held_low[0]-low[0]-15)<.001f&&std::abs(held_high[0]-high[0]-15)<.001f,
          "held animation sample follows the new body root");
    held.contact_pose_hold_ticks=0;
    check(body.update(held,6,nullptr)&&!body.package_pose_held(),"live pose resumes without resetting the move clock");
    body.reset_control_test_pose();held.contact_pose_hold_ticks=2;
    check(body.update(held,0,nullptr)&&!body.package_pose_held(),"no snapshot crosses a round reset");
    for(bool missing:{true,false}){
        load(body,switched);
        sample(body,4,4);
        auto bad=std::make_shared<BorgGameplayProfile>(*switched);
        auto& follow=*bad->moves.front().pose_followup;
        if(missing)follow.clip={15,127};else follow.start=600;
        std::string error;BattleMvpVec point{};
        check(!body.load_package(bad,model,prefix,pose,motions,&error)&&
              error.find("follow-up")!=error.npos&&!body.bone_point(0,{},&point),
              "missing or too short follow-up refuses without publishing the old actor pose");
    }
}
}
int main(){
    using namespace gf;NativeBattleMvpBorg borg;BattleMvpVec point{},low{},high{};std::string error;BattleMvpActor actor;actor.model=0x615;
    check(!borg.update(actor,0,&error)&&!borg.bone_point(0,{},&point)&&!borg.bounds(&low,&high),"no prepared/missing model can supply a live pose");
    check(!borg.load(0,{}, {}, {}, {},&error),"unsupported model identity refuses");
    check(!borg.load(0x615,{}, {}, {}, {},&error),"missing normal model data refuses");
    unsigned source_models=0;
    for(const auto& pair:{std::pair{"g-red",u16(0x615)},std::pair{"sasuke",u16(10)}}){
        const std::string base=std::string("local/content/native-battle-mvp-v1/")+pair.first;
        auto model=read(base+".hsd");if(model.empty())continue;
        auto prefix=read(base+".prefix"),pose=read(base+".pose"),motions=read(base+".motions");
        auto view=[](const auto& bytes){return ByteView{bytes.data(),bytes.size()};};
        if(pair.second==10)pose_followup(view(model),view(prefix),view(pose),view(motions));
        check(borg.load(pair.second,view(model),view(prefix),view(pose),view(motions),&error),"real normal model and nonnull locomotion clips load");
        NativeBattleMvpBorg reference;
        if(pair.second==0x615)check(reference.load(pair.second,view(model),view(prefix),view(pose),view(motions),&error),"reference normal G-Red pose loads");
        check(!borg.bone_point(7,{},&point),"loaded art alone is not an evaluated gameplay pose");
        model.clear();prefix.clear();pose.clear();motions.clear();
        actor={};actor.model=pair.second;
        check(borg.update(actor,0,&error)&&borg.bounds(&low,&high),"owned source bytes survive caller buffer release");
        BattleMvpVec foot;check(borg.bone_point(16,{},&foot),"evaluated native actor has a real skeleton");
        std::array<BattleMvpVec,20> rest{};
        for(unsigned bone=0;bone<rest.size();++bone)check(borg.bone_point(bone,{},&rest[bone]),"rest pose bone exists");
        actor.reaction_yaw=.5f;actor.reaction_pitch=.2f;actor.reaction_ticks=BattleMvpActor::reaction_duration;
        check(borg.update(actor,0,&error),"a reaction republishes the pose on the same tick");
        bool reaction_moved=false;
        for(unsigned bone=0;bone<rest.size();++bone){
            BattleMvpVec posed;check(borg.bone_point(bone,{},&posed),"reaction pose bone exists");
            for(unsigned k=0;k<3;++k)reaction_moved|=std::abs(posed[k]-rest[bone][k])>.001f;
        }
        check(reaction_moved,"reaction angles rotate the upper body");
        actor.reaction_yaw=actor.reaction_pitch=0;actor.reaction_ticks=0;
        check(borg.update(actor,0,&error),"clearing the reaction republishes the pose");
        bool reaction_restored=true;
        for(unsigned bone=0;bone<rest.size();++bone){
            BattleMvpVec posed;check(borg.bone_point(bone,{},&posed),"cleared reaction bone exists");
            for(unsigned k=0;k<3;++k)reaction_restored&=std::abs(posed[k]-rest[bone][k])<.001f;
        }
        check(reaction_restored,"clearing the reaction restores the rest pose");
        actor.position={30,0,-20};actor.movement_speed=5;actor.move_forward=1;actor.travel_distance=40;actor.action=BattleMvpAction::Move;
        for(u32 tick=1;tick<20;++tick){actor.travel_distance+=5;check(borg.update(actor,tick,&error),"distance-based locomotion and transition sampling");}
        actor.move_forward=0;actor.move_right=1;
        for(u32 tick=20;tick<30;++tick){actor.travel_distance+=5;actor.attack_ticks=5;check(borg.update(actor,tick,&error),"moving lower body and attacking upper body blend independently");}
        actor.grounded=false;actor.position[1]=100;actor.action=BattleMvpAction::Jump;check(borg.update(actor,30,&error),"airborne presentation");
        if(pair.second==0x615){
            actor.grounded=true;actor.position[1]=0;actor.dash_duration=12;
            u32 tick=31;for(auto direction:{BattleMvpDashDirection::Forward,BattleMvpDashDirection::Backward,BattleMvpDashDirection::Left,BattleMvpDashDirection::Right}){
                actor.dash_motion=direction;for(unsigned remaining:{12u,6u,1u}){actor.dash_ticks=remaining;check(borg.update(actor,tick++,&error),"classified directional dash source poses sample");}
            }
            actor.grounded=false;actor.dash_airborne=true;actor.dash_ticks=6;check(borg.update(actor,tick++,&error),"classified forward air-dash source pose samples");
            actor.dash_ticks=0;actor.takeoff_ticks=4;check(borg.update(actor,tick++,&error),"classified takeoff source pose samples");
            actor.takeoff_ticks=0;actor.vertical_speed=8;check(borg.update(actor,tick++,&error),"classified ascending source pose samples");
            actor.vertical_speed=0;check(borg.update(actor,tick++,&error),"classified apex source pose samples");
            actor.vertical_speed=-8;check(borg.update(actor,tick++,&error),"classified falling source pose samples");
            actor.grounded=true;actor.landing_ticks=3;check(borg.update(actor,tick++,&error),"classified landing source pose samples");
            actor={};actor.model=pair.second;check(borg.update(actor,0,&error),"reset before short-phase regression");
            tick=1;actor.grounded=false;actor.vertical_speed=13;
            for(unsigned remaining=BattleMvpActor::takeoff_duration;remaining;--remaining){actor.takeoff_ticks=remaining;check(borg.update(actor,tick++,&error),"takeoff progresses through its full gameplay window");}
            check(reference.update(actor,100,&error),"unblended takeoff endpoint samples");
            for(unsigned bone=0;bone<20;++bone){
                BattleMvpVec actual,expected;check(borg.bone_point(bone,{},&actual)&&reference.bone_point(bone,{},&expected),"takeoff endpoint bone exists");
                for(unsigned k=0;k<3;++k)check(std::abs(actual[k]-expected[k])<.001f,"takeoff completes without lingering idle blend");
            }
            actor.grounded=true;actor.takeoff_ticks=0;actor.vertical_speed=0;
            for(unsigned remaining=BattleMvpActor::landing_duration;remaining;--remaining){actor.landing_ticks=remaining;check(borg.update(actor,tick++,&error),"landing progresses through its full gameplay window");}
            check(reference.update(actor,0,&error),"unblended landing endpoint samples");
            for(unsigned bone=0;bone<20;++bone){
                BattleMvpVec actual,expected;check(borg.bone_point(bone,{},&actual)&&reference.bone_point(bone,{},&expected),"landing endpoint bone exists");
                for(unsigned k=0;k<3;++k)check(std::abs(actual[k]-expected[k])<.001f,"landing completes without lingering airborne blend");
            }
        }
        actor={};actor.model=pair.second;check(borg.update(actor,0,&error),"restart resets displayed pose and transition clocks");
        check(!borg.load(0,{}, {}, {}, {},&error)&&!borg.bone_point(7,{},&point)&&!borg.bounds(&low,&high),"failed reload retires previous pose/muzzle observations");
        ++source_models;
    }
    check(source_models==0||source_models==2,"optional real-model coverage must include both normal Borgs");
    std::printf("MVP shared locomotion/blending/pose lifetimes: %u checks PASS; optional real models=%u\n",checks,source_models);
}
