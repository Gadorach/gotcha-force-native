#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/battle_camera_pose.hpp"
#include "engine/graphics/camera.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"battle camera pose: %s\n",why);std::abort();}}bool near(float a,float b,float e=.002f){return std::abs(a-b)<=e;}}
int main(){
    using namespace gf;std::string error;std::array<u8,128> raw{};
    const std::array<u32,32> words{0x47465643,1,128,0,0x8000bda4,0x8000c5f8,0x8000c660,0x8000c314,0x8000d46c,0x80010980,0x8008ae10,0x8008ae60,0x80045204,0x80045238,0x8020b61c,0x8000bef4,0x3f800000,0x3f000000,0x358637be,0x40800000,0x3fa66666,0x3f99999a,0x3f666666,0x38c90fdb,1536,61,32,12,2,0,0,0};
    for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    BattleCameraEntryProfile profile;check(profile.load({raw.data(),raw.size()},&error),"source recipe");
    BattleFirstCameraInput input;input.actor_position={100,20,300};input.values88c[0]=180;input.values88c[3]=600;input.scale_b4=1;
    BattleFirstCameraPose pose;check(battle_first_camera_pose(profile,input,&pose,&error),"first normal camera");
    check(pose.target==std::array<f32,3>{100,200,300}&&pose.distance348==2400&&pose.height350==180&&pose.scale354==1&&pose.actor5b0==-32768,"source operands and signed heading wrap");
    const double pitch=3*std::acos(-1.)/64;
    check(near(pose.eye[0],100)&&near(pose.eye[1],float(200+2400*std::sin(pitch)))&&near(pose.eye[2],float(300-2400*std::cos(pitch))),"source polar eye around the actor-derived target");
    float eye[3],target[3];mat4_transform_point(pose.view394,pose.eye.data(),eye);mat4_transform_point(pose.view394,pose.target.data(),target);
    check(near(eye[0],0)&&near(eye[1],0)&&near(eye[2],0)&&near(target[0],0)&&near(target[1],0)&&near(target[2],-2400),"shared negative-Z view transform");
    for(unsigned c=0;c<3;++c){float length=0;for(unsigned r=0;r<3;++r)length+=pose.world114.at(r,c)*pose.world114.at(r,c);check(near(length,1,2e-6f),"source positive-forward collider basis remains orthonormal");}
    for(unsigned k=0;k<3;++k)check(pose.world114.at(k,3)==pose.eye[k],"114 contains camera eye");
    input.scale_b4=2;input.layout29=1;input.split3d=1;check(battle_first_camera_pose(profile,input,&pose,&error)&&pose.distance348==2400&&pose.scale354==1.5f,"rank camera scalar is distinct from initial orbit radius");
    input.split3d=0;check(battle_first_camera_pose(profile,input,&pose,&error)&&near(pose.distance348,3120)&&near(pose.height350,216),"source vertical split distance/height modifiers");
    input.layout29=2;input.split3d=1;check(battle_first_camera_pose(profile,input,&pose,&error)&&near(pose.distance348,3120),"four-player branch uses the same modifiers");
    input.group582=1;check(!battle_first_camera_pose(profile,input,&pose,&error),"later camera parameter groups remain closed");input.group582=0;
    input.values88c[3]=1e-9f;check(!battle_first_camera_pose(profile,input,&pose,&error),"degenerate source normalization branch refuses");
    input.values88c[3]=600;input.actor_position[0]=std::numeric_limits<float>::infinity();check(!battle_first_camera_pose(profile,input,&pose,&error),"nonfinite actor observation refuses");
    raw[80]^=1;check(!profile.load({raw.data(),raw.size()},&error)&&!battle_first_camera_pose(profile,{},&pose,&error),"changed source recipe closes admission");
    Camera base;base.eye[0]=220;base.eye[1]=340;base.eye[2]=650;base.target[1]=120;
    base.fov_y_degrees=55;base.near_plane=5;base.far_plane=25000;
    auto combat=base;
    check(camera_dolly_to_fit_bounds(combat,{-35,0,-30},{35,200,30})&&combat.cache_key()==base.cache_key(),
          "ordinary body keeps the existing composition");
    const std::array<f32,3> low{-540,0,-380},high{540,1180,380};
    for(const auto& size:{std::array{900,460},std::array{460,900}}){
        base.viewport_width=size[0];base.viewport_height=size[1];combat=base;
        check(camera_dolly_to_fit_bounds(combat,low,high)&&combat.cache_key()!=base.cache_key(),"oversized body causes a dolly");
        Camera::Basis before,after;
        check(base.basis(&before)&&combat.basis(&after),"both frames have a valid basis");
        for(unsigned k=0;k<3;++k)check(near(before.forward[k],after.forward[k],1e-6f)&&combat.target[k]==base.target[k],
                                      "dolly preserves aim and camera-relative controls");
        for(unsigned corner=0;corner<8;++corner){
            f32 p[3];for(unsigned k=0;k<3;++k)p[k]=corner&(1u<<k)?high[k]:low[k];
            f32 x=0,y=0;
            check(combat.project(p,&x,&y,nullptr)&&x>=.059f*size[0]&&x<=.941f*size[0]&&
                  y>=.059f*size[1]&&y<=.941f*size[1],"entire body fits at current aspect");
        }
    }
    combat=base;combat.far_plane=800;
    const auto limited=combat.cache_key();
    check(!camera_dolly_to_fit_bounds(combat,low,high)&&combat.cache_key()==limited,"out-of-domain fit leaves camera unchanged");
    combat=base;combat.far_plane=800;
    check(camera_dolly_to_fit_bounds(combat,low,high,true)&&combat.far_plane>800,
          "opted-in fit grows the far plane instead of refusing");
    std::printf("battle camera pose: %u checks PASS\n",checks);
}
