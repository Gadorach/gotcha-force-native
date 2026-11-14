#pragma once
#include "engine/gameplay/character_program_directional_control.hpp"
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace character_program_directional_root_fixture {
using namespace gf;
inline CharacterPoseHeightProfile height(){CharacterPoseHeightProfile p;p.loaded=true;p.half=.5f;p.negative_factor=bits_to_f32(0xbd4ccccd);p.gain=bits_to_f32(0x3f333333);p.step=5;return p;}
struct Fixture:character_motion_directional_fixture::Fixture {
    CharacterProgramAsset p;unsigned root_lane=0;
    bool setup(unsigned lane,std::string& e){if(!load(&e))return false;root_lane=lane;if(lane)rig.order={1,0};p=profile;p.control_flow=true;
        for(auto& d:p.descriptors){d.group=0;d.bytes[19]=1;}
        for(auto& clip:clips){auto y=character_motion_driver_fixture::linear(0,clip.clip.end_frame*2,unsigned(clip.clip.end_frame));y.channel=6;clip.clip.bones[0].tracks.push_back(y);}
        p.programs={{0,0,{1,0,0,0,0,0,0,0,0,0,0,100,0,1,255,255}},
            {0,1,{13,0,1,0,2,0,0,0,0,0,0,100,0,1,255,255}},
            {0,2,{13,0,1,0,2,5,0,0,0,0,0,100,0,1,255,255}},
            {0,3,{1,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,64,8,0,0,0,0,0,0,100,0,1,255,255}},
            {0,4,{13,0,1,0,2,0,0,0,0,8,0,3,0,1,255,255}},
            {0,5,{1,0,0,2,3,0,0,0,0,0,0,100,0,1,255,255}},
            {0,6,{7,0,1,1,13,0,1,0,2,0,0,0,0,0,0,100,0,1,255,255}}};return true;
    }
    bool driver(CharacterMotionDriver& d,std::string& e)const{return d.load_directional_transitions({model.data(),model.size()},rig,p,clips,initial,{s8(root_lane?1:0),-1},&e);}
};
inline bool bind(CharacterProgramVm& vm,const CharacterMotionDriver& driver,CharacterProgramDirectionalControl& c,std::string& e,bool world=true,std::span<const f32> multipliers={},CharacterMotionRootObservationMode mode=CharacterMotionRootObservationMode::AObjOnly){
    CharacterPoseHeightFields h;h.scale_b4=1;h.actor_y=vm.state().actor.root.position[1];h.behaviour5e0=vm.state().actor.behaviour;h.height66c=8;h.radius670=2;h.middle_bone=2;h.world_positions_declared=world;
    const std::array<f32,3> y={0,50,30};CharacterMotionRootFields r;r.scale_b4=1;r.delta5f4=1;r.multipliers1dd4=multipliers;return c.bind_with_directional_root(vm,driver,height(),h,y,r,&e,mode);
}
inline bool steer(CharacterProgramVm& vm,s16 v,std::string& e){CharacterProgramClassFields f;return vm.observe_class_fields(&f)&&vm.write_class_steering(f,v,&e);}
inline unsigned count(const std::vector<CharacterProgramOperation>& ops,CharacterProgramOperationKind k){unsigned n=0;for(const auto& op:ops)n+=op.kind==k;return n;}
}
