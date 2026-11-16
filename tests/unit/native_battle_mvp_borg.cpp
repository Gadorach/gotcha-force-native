#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/native_battle_mvp_borg.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"MVP Borg animation: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}}
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
        check(borg.load(pair.second,view(model),view(prefix),view(pose),view(motions),&error),"real normal model and nonnull locomotion clips load");
        NativeBattleMvpBorg reference;
        if(pair.second==0x615)check(reference.load(pair.second,view(model),view(prefix),view(pose),view(motions),&error),"reference normal G-Red pose loads");
        check(!borg.bone_point(7,{},&point),"loaded art alone is not an evaluated gameplay pose");
        model.clear();prefix.clear();pose.clear();motions.clear();
        actor={};actor.model=pair.second;
        check(borg.update(actor,0,&error)&&borg.bounds(&low,&high),"owned source bytes survive caller buffer release");
        BattleMvpVec foot;check(borg.bone_point(16,{},&foot),"evaluated native actor has a real skeleton");
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
