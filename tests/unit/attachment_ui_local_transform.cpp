#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/attachment_ui_local_transform.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;AttachmentUiLocalTransformInput input;AttachmentBoneTransform out;
    input.local64={-127.848f,90.029f,-185};input.scale58={-200,0,0};input.angle74=16384;
    check(attachment_ui_local_transform(input,&out,&error)&&out.valid&&out.position==input.local64);
    check(out.world.at(0,0)==1&&out.world.at(1,1)==1&&out.world.at(2,2)==1&&out.world.at(0,1)==0);
    input.scale58[0]=std::numeric_limits<f32>::quiet_NaN();
    check(attachment_ui_local_transform(input,&out,&error)&&out.world.at(0,0)==1); //flag01 does NOT consume58
    input.flags84=0x11;input.local64={-180,60,-300};input.scale58={.4f,.4f,1};
    check(attachment_ui_local_transform(input,&out,&error)&&out.position==input.local64&&out.world.at(0,0)==.4f&&out.world.at(1,1)==.4f&&out.world.at(2,2)==1);
    check(out.world.at(0,1)==0&&out.world.at(1,0)==0); //flag11 does NOT consume angle74
    input.flags84=0x19;input.local64={0,0,-400};input.scale58={.8f,.8f,1};input.angle74=0;
    check(attachment_ui_local_transform(input,&out,&error)&&out.world.at(0,0)==.8f&&out.position==input.local64);
    for(s16 angle:{s16(-32768),s16(-16384),s16(-1),s16(0),s16(1),s16(8192),s16(16384),s16(32767)}){
        input.angle74=angle;check(attachment_ui_local_transform(input,&out,&error)&&out.position==input.local64);
        const f32 radians=f32(angle)*bits_to_f32(0x38c90fdbu),c=std::cos(radians),s=std::sin(radians);
        check(out.world.at(0,0)==f32(c*.8f)&&out.world.at(0,1)==f32(-s*.8f)&&out.world.at(1,0)==f32(s*.8f)&&out.world.at(1,1)==f32(c*.8f)&&out.world.at(2,2)==1);
    }
    for(unsigned flags=0;flags<256;++flags){input.flags84=u8(flags);check(attachment_ui_local_transform(input,&out,&error)==(flags==1||flags==0x11||flags==0x19));}
    for(u8 flags:{u8(1),u8(0x11),u8(0x19)}){
        input.flags84=flags;
        for(unsigned axis=0;axis<3;++axis){
            const f32 original=input.local64[axis];input.local64[axis]=std::numeric_limits<f32>::quiet_NaN();check(!attachment_ui_local_transform(input,&out,&error));input.local64[axis]=original;
            const f32 scale=input.scale58[axis];input.scale58[axis]=std::numeric_limits<f32>::infinity();check(attachment_ui_local_transform(input,&out,&error)==(flags==1));input.scale58[axis]=scale;
        }
    }
    check(!attachment_ui_local_transform(input,nullptr,&error));
    AttachmentBoneTransformInput old;old.flags=0x11;old.local_position={0,0,-400};
    check(!attachment_identity_transform(old,&out,&error)&&!attachment_bone_transform({},old,&out,&error)); //no silent widening of older entries
    std::printf("UI identity/local64 flags01/11/19, selective operands and Z/scale: %u checks PASS\n",checks);
}
