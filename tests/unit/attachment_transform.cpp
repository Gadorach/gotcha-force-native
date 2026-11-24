#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/attachment_transform.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"attachment transform: %s\n",why);std::abort();}}
bool near(float a,float b){return std::abs(a-b)<=1e-5f;}
}
int main(){
    using namespace gf;std::string error;std::array<Mat4,2> parents;
    parents[1]=mat4_translation(std::array<float,3>{10,20,30}.data());
    parents[1].at(0,0)=2;parents[1].at(1,1)=3;parents[1].at(2,2)=4;
    AttachmentBoneTransformInput input;input.flags=0x41;input.bone=1;input.local_position={1,2,3};AttachmentBoneTransform out;
    check(attachment_bone_transform(parents,input,&out,&error)&&out.valid&&out.position==std::array<float,3>{12,26,42},"position follows selected logical parent and its nonunit scale");
    check(out.world.at(0,0)==2&&out.world.at(1,1)==3&&out.world.at(2,2)==4,"copy parent basis, not identity or file SRT");
    input.flags=0x4b;input.angles={16384,12345,16384};
    check(attachment_bone_transform(parents,input,&out,&error)&&out.position==std::array<float,3>{12,26,42},"Z/X rotations do not rotate translated origin or consume disabled Y");
    const float expected[3][3]={{0,0,2},{3,0,0},{0,4,0}};
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)check(near(out.world.at(r,c),expected[r][c]),"parent*Rz*Rx (not parent*Rx*Rz)");
    input.flags=0x51;input.scale={-2,0,3};
    check(attachment_bone_transform(parents,input,&out,&error)&&out.world.at(0,0)==-4&&out.world.at(1,1)==0&&out.world.at(2,2)==12&&out.position==std::array<float,3>{12,26,42},"numeric scale permits singular/reflected matrices without inventing inverse or renderer admission");
    input.flags=0x41;input.scale[0]=std::numeric_limits<float>::quiet_NaN();
    check(attachment_bone_transform(parents,input,&out,&error),"disabled scale is not read");
    input.flags=0x51;check(!attachment_bone_transform(parents,input,&out,&error)&&!out.valid&&out.position==std::array<float,3>{},"nonfinite consumed scale clears publication");
    input.scale={1,1,1};
    for(unsigned flags=0;flags<256;++flags){input.flags=u8(flags);const bool accepted=(flags&0xe1)==0x41;
        check(attachment_bone_transform(parents,input,&out,&error)==accepted&&out.valid==accepted,"all flag combinations preserve explicit bone/local-position subset");}
    input.flags=0x41;input.bone=2;
    check(!attachment_bone_transform(parents,input,&out,&error)&&!out.valid,"out-of-range logical bone refuses");
    input.bone=0;check(!attachment_bone_transform({},input,&out,&error),"missing parent pose refuses, no fake origin");
    std::array<Mat4,33> oversized;check(!attachment_bone_transform(oversized,input,&out,&error),"source logical-slot budget");
    parents[0].m[15]=2;check(!attachment_bone_transform(parents,input,&out,&error),"projective parent refuses");
    parents[0]=Mat4{};parents[0].m[0]=std::numeric_limits<float>::infinity();check(!attachment_bone_transform(parents,input,&out,&error),"nonfinite parent refuses");
    parents[0]=Mat4{};input.local_position[1]=std::numeric_limits<float>::quiet_NaN();check(!attachment_bone_transform(parents,input,&out,&error),"nonfinite local position refuses");
    input.local_position={1e4f,0,0};parents[0].m[0]=1e7f;check(!attachment_bone_transform(parents,input,&out,&error)&&!out.valid,"late derived overflow refuses without retaining old output");
    check(!attachment_bone_transform(parents,input,nullptr,&error),"null destination refuses");
    input.local_position={};parents[0]=Mat4{};
    check(attachment_bone_transform(parents,input,&out,&error)&&error.empty()&&out.valid,"fresh success explicitly replaces refusal");
    AttachmentWorldPositionInput direct;direct.position20={10,20,30};direct.angles={16384,0,16384};
    check(attachment_world_position_transform(direct,&out,&error)&&out.position==direct.position20,"direct20 translation branch");
    const float point[3]={1,2,3};float result[3];mat4_transform_point(out.world,point,result);
    check(near(result[0],13)&&near(result[1],21)&&near(result[2],32),"direct translation with Rz*Ry*Rx order");
    AttachmentBoneTransformInput local;local.flags=0xf;local.local_position=direct.position20;local.angles=direct.angles;AttachmentBoneTransform local_out;
    check(attachment_identity_transform(local,&local_out,&error)&&std::equal(out.world.m,out.world.m+16,local_out.world.m),"identity numeric kernel shared by separate source operand domains");
    local.flags=0xe;check(!attachment_identity_transform(local,&local_out,&error)&&!local_out.valid,"local branch still refuses direct-position flags");
    direct.flags=0xf;check(!attachment_world_position_transform(direct,&out,&error)&&!out.valid,"direct branch refuses local-position flags");
    direct.flags=0xe;direct.position20[0]=std::numeric_limits<float>::quiet_NaN();check(!attachment_world_position_transform(direct,&out,&error)&&!out.valid,"nonfinite direct position refuses");
    AttachmentScaledWorldPositionInput scaled;scaled.position20={10,20,30};scaled.angles={16384,0,16384};scaled.scale58={2,3,4};
    check(attachment_scaled_world_position_transform(scaled,&out,&error)&&out.position==scaled.position20,"explicit1E translation retained before source rotations/scale");
    mat4_transform_point(out.world,point,result);check(near(result[0],22)&&near(result[1],22)&&near(result[2],36),"direct T*Rz*Ry*Rx*S, not a borrowed parent matrix");
    for(unsigned flags=0;flags<256;++flags){scaled.flags=u8(flags);check(attachment_scaled_world_position_transform(scaled,&out,&error)==(flags==0x1e),"scaled adapter admits only exact1E");}
    direct.flags=0x1e;direct.position20={};check(!attachment_world_position_transform(direct,&out,&error),"old0E adapter still refuses scale branch");
    scaled.flags=0x1e;scaled.scale58[0]=std::numeric_limits<float>::quiet_NaN();check(!attachment_scaled_world_position_transform(scaled,&out,&error)&&!out.valid,"nonfinite scale refuses without a world");
    check(attachment_world_axes_mount_transform(parents,1,{1,2,3},{0,0,-90},&out,&error)&&out.valid&&
          out.position==std::array<float,3>{12,26,42},"world-axes mount still takes its point through the full bone");
    mat4_transform_point(out.world,point,result);
    check(near(result[0],14)&&near(result[1],25)&&near(result[2],45),"world-axes mount uses local Z rotation, without bone scale or axes");
    check(!attachment_world_axes_mount_transform(parents,2,{},{},&out,&error)&&!out.valid&&
          !attachment_world_axes_mount_transform(parents,1,{},{0,0,181},&out,&error)&&!out.valid,
          "world-axes mount refuses unknown bone or invalid angles");
    check(attachment_mount_transform(parents,1,{1,2,3},{90,0,90},&out,&error,true)&&
          out.position==std::array<float,3>{12,26,42},"bone ZYX remains anchored to the same translated point");
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)check(near(out.world.at(r,c),expected[r][c]),"explicit bone ZYX keeps the source axis order");
    std::printf("{\"attachment_transform_checks\":%u,\"all_passed\":true,\"attachment_lifecycle_or_rendering_admitted\":false}\n",checks);
}
