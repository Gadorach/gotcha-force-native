#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/character_directional_motion_root.hpp"
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace {
using namespace gf;using namespace character_motion_directional_fixture;
unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"ordered directional root: %s\n",why);std::abort();}}
bool close(f32 a,f32 b){return std::abs(a-b)<.0001f;}
CharacterMotionRootFields fields(){CharacterMotionRootFields f;f.delta5f4=.25f;f.scale_b4=1;f.suppress_selector1db2=1;f.apply_position1d9b=1;return f;}
bool load(Fixture& f,CharacterMotionDriver& d,CharacterProgramMotion& m,std::string& e,unsigned lane=0){return d.load_directional_transitions({f.model.data(),f.model.size()},f.rig,f.profile,f.clips,f.initial,{-1,-1},&e)&&d.select(lane,f.profile.descriptors[0],false,{},1,.25f,&m);}
bool request(CharacterDirectionalMotionRootOwner& o,const CharacterMotionRootFields& f,CharacterDirectionalRootResult& r,std::string& e,u8 duration=0){return o.request(controls,8192,5,.5f,.25f,duration,{4,5},{4,5},f,&r,&e);}
}
int main(){using namespace gf;Fixture f;std::string e;CharacterMotionDriver d;CharacterProgramMotion m;CharacterDirectionalMotionRootOwner owner;CharacterDirectionalRootResult r;auto x=fields();
    check(f.load(&e)&&load(f,d,m,e),"owned fixture");check(owner.bind(d,m,{10,20,30},{7,8,1},&e),"bind actual ordinary displayed root");
    auto invalid=x;invalid.delta5f4=0;check(request(owner,invalid,r,e)&&r.root.position==std::array<f32,3>{10,20,30}&&r.root.updates==2,"two request callbacks clear without displacement or positive-delta requirement");
    check(r.callbacks[0].motion_buffer==0&&r.callbacks[0].pose_buffer==0&&r.callbacks[0].flags==1&&r.callbacks[1].motion_buffer==1&&r.callbacks[1].pose_buffer==1&&r.callbacks[1].flags==1,"49B98 centre0 then side1 requests");
    check(close(r.displayed_translation[0],50.5f)&&close(r.callbacks[1].after.translation[0],.5f),"displayed directional root differs from last callback primary buffer");
    check(owner.advance(controls,8192,5,.25f,x,&r,&e),"native follow advance");
    check(close(r.callbacks[0].before.translation[0],50.5f)&&close(r.callbacks[0].after.translation[0],.75f)&&close(r.callbacks[0].state.position[0],-39.75f),"centre subtracts PREVIOUS DISPLAYED root, not previous raw centre");
    check(close(r.root.position[0],-39.75f)&&r.root.delta1da0==std::array<f32,3>{}&&r.root.speed1dac==0&&r.root.heading1db0==0,"later side root math writes zero fields without undoing centre position");
    check(r.callbacks[1].flags==2&&r.callbacks[1].pose_buffer==1&&r.callbacks[1].before.translation==r.callbacks[1].after.translation&&r.callbacks[0].state.speed1dac>0,"side1 writes alternate pose but AAE0 still reads primary T0");
    const auto position=r.root.position;check(owner.advance(controls,-8192,5,.25f,x,&r,&e)&&r.callbacks[1].flags==1&&r.callbacks[0].flags==2&&r.root.delta1da0==std::array<f32,3>{}&&r.root.position!=position,"changed side request clears root after centre already moved");
    auto passive=x;passive.apply_position1d9b=0;const auto prior=r.root.position;check(owner.advance(controls,-8192,5,.25f,passive,&r,&e)&&r.root.position==prior&&r.callbacks[0].state.speed1dac>0,"apply0 retains raw callback evidence but never moves actor position");
    check(request(owner,invalid,r,e,3)&&r.callbacks[0].motion_buffer==1&&r.callbacks[1].motion_buffer==0&&r.callbacks[0].pose_buffer==0&&r.callbacks[1].pose_buffer==0&&r.root.position==prior,"49210 side0 then centre0 request resets");
    const auto frozen_frame=owner.snapshot()->primary().frame;check(owner.advance(controls,8192,5,.25f,invalid,&r,&e)&&r.callbacks[0].flags==0&&r.callbacks[1].flags==0&&owner.snapshot()->primary().frame==frozen_frame&&r.root.position==prior,"496D8 flags0 copies both into primary; target players frozen, root reset");
    check(owner.advance(controls,-8192,5,.25f,invalid,&r,&e)&&r.callbacks[0].flags==1&&r.callbacks[1].flags==0&&r.root.position==prior,"active changed side requests into primary before centre flags0 copy");
    check(owner.advance(controls,0,5,4,invalid,&r,&e)&&!owner.snapshot()->blending()&&r.callbacks[0].flags==1&&r.callbacks[0].motion_buffer==0&&r.callbacks[1].pose_buffer==1&&r.root.position==prior,"completion centre0/side1 request, no displacement, before2Fclear");
    check(owner.advance(controls,0,5,.25f,x,&r,&e)&&r.callbacks[0].flags==2&&r.callbacks[1].flags==2,"ordinary follow resumes on next callback");
    //Late side validation must not publish the earlier centre displacement.
    Fixture late;CharacterMotionDriver ld;CharacterProgramMotion lm;CharacterDirectionalMotionRootOwner lo;check(late.load(&e),"late failure fixture");late.profile.descriptors[1].bytes[0]=127;
    check(load(late,ld,lm,e)&&lo.bind(ld,lm,{0,0,0},{0,0,1},&e)&&request(lo,x,r,e),"side selector127 remains unused by requests");
    auto retained=lo;const auto retained_position=retained.state()->position;auto guarded=x;guarded.suppress_selector1db2=0;const std::array<f32,1> multipliers={1};guarded.multipliers1dd4=multipliers;
    check(!lo.advance(controls,8192,5,.25f,guarded,&r,&e)&&!lo.ready()&&r.root.updates==0&&retained.state()->position==retained_position,"side zero-delta callback still requires selected multiplier; whole result refused after centre candidate");
    check(retained.advance(controls,8192,5,.25f,x,&r,&e),"speculative same-owner input survives failed sibling");
    CharacterDirectionalMotionRootOwner moved=std::move(retained);check(!retained.ready()&&moved.ready()&&!retained.advance(controls,0,5,1,x,&r,&e),"move invalidates prior owner");
    check(!moved.advance(controls,0,5,std::numeric_limits<f32>::quiet_NaN(),x,&r,&e)&&!moved.ready(),"invalid native delta latches refusal");
    CharacterMotionDriver neighbor;CharacterProgramMotion nm;check(load(f,neighbor,nm,e)&&!owner.bind(neighbor,m,{0,0,0},{},&e)&&!owner.ready(),"same-model neighbor library is not interchangeable");
    //Put root in lane1; per-descriptor D244 writes must precede root effects.
    Fixture lane1;CharacterMotionDriver sd;CharacterProgramMotion sm;CharacterDirectionalMotionRootOwner so;check(lane1.load(&e),"lane1 fixture");lane1.rig.order={1,0};for(auto& desc:lane1.profile.descriptors){desc.bytes[17]=u8(10+desc.choice);desc.bytes[18]=u8(20+desc.choice);desc.bytes[19]=desc.choice;}
    check(load(lane1,sd,sm,e,1)&&so.bind(sd,sm,{0,0,0},{99,98,97},&e)&&request(so,x,r,e),"explicit root lane1");
    check(r.callbacks[0].apply_position1d9b==0&&r.callbacks[1].apply_position1d9b==1&&r.lane1_descriptor==std::array<u8,3>{11,21,1},"initial centre→side globals stamped before each callback");
    check(so.advance(controls,8192,5,.25f,x,&r,&e)&&r.callbacks[0].apply_position1d9b==1&&r.callbacks[1].apply_position1d9b==1&&r.root.position[0]!=0,"follow centre uses retained side global apply, not centre descriptor0");
    check(request(so,x,r,e,3)&&r.callbacks[0].apply_position1d9b==1&&r.callbacks[1].apply_position1d9b==0&&r.lane1_descriptor==std::array<u8,3>{10,20,0},"mode6 entry side→centre overwrites globals in order");
    check(so.advance(controls,-8192,5,.25f,invalid,&r,&e)&&r.lane1_descriptor==std::array<u8,3>{12,22,2}&&r.callbacks[1].apply_position1d9b==2,"active side rebind global persists through centre flags0");
    //Public-null roots are explicit, retain inherited callback clock, not frame0.
    Fixture null;CharacterMotionDriver nd;CharacterProgramMotion nn;CharacterDirectionalMotionRootOwner no;check(null.load(&e),"public null fixture");null.rig.order={1,0};for(auto& clip:null.clips){clip.clip.bones[0].has_aobj=false;clip.clip.bones[0].tracks.clear();}
    check(load(null,nd,nn,e,1)&&!no.bind(nd,nn,{},{},&e)&&no.bind(nd,nn,{},{},&e,CharacterMotionRootObservationMode::AObjOrPublicNull)&&request(no,x,r,e),"public-null mode opt-in only");
    check(no.advance(controls,8192,5,.25f,x,&r,&e)&&r.callbacks[0].after.clock_source==CharacterMotionRootClockSource::InheritedMotion&&r.callbacks[0].after.frame>0&&r.root.position==std::array<f32,3>{},"public-null inherits positive callback clock without invented root AObj or motion");
    check(!no.advance(controls,8192,5,.25f,invalid,&r,&e)&&r.root.updates==0&&!no.ready(),"positive inherited clock requires5F4 even for zero displacement");
    std::printf("ordered native directional root component: %u checks PASS; VM/preview root-lane guard unchanged\n",checks);return 0;
}
