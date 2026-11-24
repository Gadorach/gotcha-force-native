#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/attachment_direct_bone_transform.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<Mat4,2> bones{mat4_identity(),mat4_identity()};bones[1]=mat4_rotation_x(.3f);bones[1].at(0,3)=100;bones[1].at(1,3)=-50;
    AttachmentDirectBoneTransformInput input;input.bone=1;input.position20={7,8,9};input.scale58={2,3,4};input.angles={123,8192,456};
    AttachmentBoneTransform result;check(attachment_direct_bone_transform(bones,input,&result,&error)&&result.valid&&result.position==input.position20);
    auto expected=bones[1];for(unsigned k=0;k<3;++k)expected.at(k,3)=input.position20[k];expected=mat4_multiply(expected,mat4_rotation_y(f32(input.angles[1])*bits_to_f32(0x38c90fdb)));expected=mat4_multiply(expected,mat4_scale(input.scale58.data()));
    for(unsigned i=0;i<16;++i)check(result.world.m[i]==expected.m[i]);
    for(unsigned flags=0;flags<256;++flags)if(flags!=0x54){auto bad=input;bad.flags=u8(flags);check(!attachment_direct_bone_transform(bones,bad,&result,&error));}
    AttachmentBoneTransformInput legacy;legacy.flags=0x54;check(!attachment_bone_transform(bones,legacy,&result,&error)&&!attachment_identity_transform(legacy,&result,&error));
    auto bad=input;bad.bone=2;check(!attachment_direct_bone_transform(bones,bad,&result,&error));bones[1].m[15]=0;check(!attachment_direct_bone_transform(bones,input,&result,&error));
    std::printf("source54 copied-bone basis/direct20 world transform: %u checks PASS\n",checks);
}
