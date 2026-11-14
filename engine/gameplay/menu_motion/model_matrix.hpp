#pragma once
#include "engine/assets/hsd.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
// Original 801CDCD0: authored T * Rz * Ry * Rx, then primary local Y rotation.
// Identifier rotation is added to the AUTHORED Y component, before Rx. The
// update writes 0x16C = second record (0x150) + rotation-Y (0x1C), not rotation-Z.
// This callback constructs a rotation/translation matrix (not authored scale).
// No guest pointers or imported snapshot matrices.
inline Mat4 menu_motion_matrix(const HsdJoint& joint,f32 local_y,f32 identifier_y){
    const f32 rotation[3]={joint.rotation[0],joint.rotation[1]+identifier_y,joint.rotation[2]};
    const f32 scale[3]={1,1,1};
    Mat4 matrix=mat4_from_srt(rotation,scale,joint.translation);
    const f32 spin[3]={0,local_y,0},zero[3]={};
    return mat4_multiply(matrix,mat4_from_srt(spin,scale,zero));
}
} // namespace gf
