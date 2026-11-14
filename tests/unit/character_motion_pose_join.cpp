#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_motion_driver_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"motion pose join: %s\n",why);std::abort();}}}
int main(){using namespace gf;using namespace character_motion_driver_fixture;Fixture f;std::string error;check(f.load(&error),"fixture");CharacterLocalPose seed;for(const auto& b:f.rig.binding.bones){CharacterBonePose p;p.joint_offset=b.joint_offset;std::copy_n(b.translation,3,p.translation);seed.bones.push_back(p);}seed.bones[0].translation[0]=888;seed.bones[1].translation[0]=777;
    CharacterMotionDriver driver;check(driver.load_transitions({f.model.data(),f.model.size()},f.rig,f.profile,f.clips,seed,&error),"explicit initial pose");std::array<CharacterProgramMotion,2> lanes;
    for(unsigned i=0;i<2;++i)check(driver.select(i,f.profile.descriptors[0],false,{},0,1,&lanes[i]),"select both lanes");
    check(driver.snapshot(lanes[0])->local_pose()->bones[1].translation[0]==777&&driver.snapshot(lanes[1])->local_pose()->bones[0].translation[0]==888,"inactive lane data remains supplied base");CharacterLocalPose pose;
    check(driver.compose_local_pose(lanes,&pose,&error)&&pose.bones[0].translation[0]==0&&pose.bones[1].translation[0]==10,"composed pose uses active slot from each lane");
    check(driver.select_transition(0,f.profile.descriptors[1],false,lanes[0],0,1,2,1,&lanes[0])&&driver.compose_local_pose(lanes,&pose,&error)&&pose.bones[0].translation[0]==50&&pose.bones[1].translation[0]==10,"blend one lane without replaying/restoring the other");
    CharacterPoseFrame frame;CharacterWorldPose world;check(character_pose_initial_frame(f.rig,Mat4{},&frame,&error)&&character_pose_direct_world(f.rig,pose,frame,&world,&error)&&world.matrices.size()==2,"joined local output feeds existing world kernel");
    auto swapped=lanes;std::swap(swapped[0],swapped[1]);check(!driver.compose_local_pose(swapped,&pose,&error)&&pose.bones.empty(),"cross-lane context clears output");
    auto forged=lanes;forged[1].frame=3;check(!driver.compose_local_pose(forged,&pose,&error)&&pose.bones.empty(),"forged scalar refuses");
    CharacterMotionDriver neighbor;check(neighbor.load_transitions({f.model.data(),f.model.size()},f.rig,f.profile,f.clips,seed,&error)&&!neighbor.compose_local_pose(lanes,&pose,&error),"same-model neighbor not accepted");
    auto incomplete=lanes;incomplete[1]={};check(!driver.compose_local_pose(incomplete,&pose,&error),"required active lane absent");
    check(driver.load_transitions({f.model.data(),f.model.size()},f.rig,f.profile,f.clips,seed,&error)&&!driver.compose_local_pose(lanes,&pose,&error),"reload invalidates prior generation");
    check(f.driver(driver,&error)&&!driver.compose_local_pose(lanes,&pose,&error),"legacy driver requires its legacy pose entry");
    std::printf("motion pose join: %u owned checks PASS\n",checks);
}
