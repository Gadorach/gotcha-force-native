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
    std::printf("{\"attachment_transform_checks\":%u,\"all_passed\":true,\"numeric_bone_branch_only\":true,\"attachment_lifecycle_or_rendering_admitted\":false}\n",checks);
}
