#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/character_pose_height.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"pose-height family: %s\n",why);std::abort();}}}
int main(){using namespace gf;CharacterPoseHeightProfile p;p.loaded=true;p.zero=0;p.half=.5f;p.negative_factor=bits_to_f32(0xbd4ccccd);p.gain=bits_to_f32(0x3f333333);p.step=5;CharacterPoseHeightFields fields;fields.behaviour5e0=0x40;fields.scale_b4=2;CharacterPoseHeightState state{7,2,999},out;std::string error;
    for(unsigned id=0;id<65536;++id){fields.model_group3e8=u8(id>>8);check(character_pose_height_evaluate(p,fields,state,{},&out,&error)&&out.offset6d0==(id>>8==5?0.f:60.f),"high model byte selects family5 branch, not full ID5");}
    fields.model_group3e8=6;check(character_pose_height_command(p,fields,state,{},2,1,&out,&error)&&out.mode71d==1&&out.offset6d0==0&&out.multiplier6d8==2,"G-Red family6 and explicit mode1");fields.model_group3e8=0;check(character_pose_height_command(p,fields,state,{},2,1,&out,&error)&&out.mode71d==1&&out.offset6d0==0,"Sasuke family0 actual operand1");
    fields.world_positions_declared=true;fields.actor_y=10;fields.height66c=40;fields.radius670=3;fields.middle_bone=2;fields.lower_bones={3,-1,-1,-1};const std::array<f32,4> y={10,100,80,50};state.mode71d=0;check(character_pose_height_evaluate(p,fields,state,y,&out,&error)&&out.offset6d0==49,"dynamic group0 source arithmetic");
    auto alias=state;check(character_pose_height_evaluate(p,fields,alias,y,&alias,&error)&&alias.offset6d0==49&&alias.mode71d==0,"current typed state direct alias");auto ignored=fields;ignored.model_group3e8=5;ignored.world_positions_declared=false;ignored.scale_b4=NAN;check(character_pose_height_command(p,ignored,state,{},2,u8(-20),&out,&error)&&out.offset6d0==0&&out.multiplier6d8==1,"family guard consumes no unread pose/scale");
    std::printf("pose-height model-family binding: %u checks PASS (65536 numeric IDs, not actors)\n",checks);return 0;
}
