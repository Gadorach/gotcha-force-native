#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_wheel_motion.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"wheel motion: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;ForceWheelMotionProfile p;p.bank_hash=1;p.turn_ticks=8;p.reciprocal=.125f;p.wheel_fold=300;p.wheel_step=30;p.cost_fold=200;p.cost_step=20;p.tab_fold=30;p.tab_step=3;p.half=.5f;p.angle_unit=.0000958738f;
    ForceEditorArt art;for(unsigned i=0;i<30;++i){art.offsets[i]=f32(i*8);art.angles[i]=s16(i*800);}ForceBudgetBandArt band;band.has_following=true;for(unsigned i=0;i<11;++i)band.parts.push_back({s16(i*1000),u16(i*4)});band.following={11000,44};ForceWheelMotion wheel;ForceWheelPose member,marker;std::vector<ForceWheelBandPose> parts;
    check(wheel.configure(p)&&wheel.request_turn(1)&&wheel.remaining()==8,"turn request stores8 before callback");check(!wheel.request_turn(-1),"busy turn cannot be replaced by repeated input");
    for(unsigned frame=1;frame<=8;++frame){check(wheel.step()&&wheel.remaining()==8-frame,"source countdown before pose");check(wheel.member(art,5,&member)&&member.offset==40.f-frame&&member.angle==4000-int(frame)*100,"member interpolates each callback");check(wheel.marker(art,5,&marker)&&marker.offset==36.f-frame&&marker.angle==3600-int(frame)*100,"single marker moves with midpoint");check(wheel.band(band,30,&parts)&&parts.size()==11&&parts[0].frame==4.f-f32(frame)*.5f,"band shape requests remain fractional");}
    check(!wheel.turning()&&wheel.previous()==1&&wheel.turn()==1,"endpoint commits old turn");
    check(wheel.request_turn(-1)&&wheel.step()&&wheel.band(band,30,&parts)&&parts[0].frame==.5f,"reverse turn uses opposite neighbour weight");for(unsigned i=1;i<8;++i)check(wheel.step(),"reverse callbacks");
    check(wheel.turn()==0&&wheel.request_turn(-1)&&wheel.step()&&wheel.turn()==29,"turn wrap");for(unsigned i=1;i<8;++i)check(wheel.step(),"wrap callbacks");
    check(wheel.toggle_fold()&&!wheel.request_turn(1),"folded target disables turn requests");for(unsigned i=1;i<=4;++i)check(wheel.step()&&wheel.wheel_shift()==i*30.f&&wheel.cost_shift()==i*20.f&&wheel.tab_shift()==i*3.f,"source independent fold rates");
    check(wheel.toggle_fold()&&wheel.step()&&wheel.wheel_shift()==90&&wheel.cost_shift()==60&&wheel.tab_shift()==9,"mid-fold reversal continues from current values");for(unsigned i=0;i<3;++i)check(wheel.step(),"unfold callbacks");check(wheel.wheel_shift()==0&&wheel.cost_shift()==0&&wheel.tab_shift()==0,"unfold endpoints without overshoot");
    check(wheel.toggle_fold(),"full fold request");for(unsigned i=0;i<12;++i)check(wheel.step(),"fold and hold callbacks");check(wheel.wheel_shift()==300&&wheel.cost_shift()==200&&wheel.tab_shift()==30,"fold clamps endpoints");
    const auto remaining=wheel.remaining();for(unsigned i=0;i<10;++i)check(wheel.member(art,i,&member)&&wheel.marker(art,i,&marker)&&wheel.remaining()==remaining,"pose reads do not advance clocks");
    check(!wheel.member(art,30,&member)&&!wheel.request_turn(2),"invalid slot/direction");band.has_following=false;check(!wheel.band(band,30,&parts)&&parts.empty(),"motion requires source lookahead row");p.reciprocal=.25f;check(!wheel.configure(p)&&!wheel.step(),"invalid profile clears previous owner");
    std::printf("wheel motion: %u owned checks PASS\n",checks);
}
