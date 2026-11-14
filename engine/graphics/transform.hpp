#pragma once
// Affine transform for scene instances: the rewrite draws pre-transformed
// vertices, so a scene (an HSD JOBJ tree, a hand-built set) composes its own
// world matrices here rather than relying on a GPU matrix stack.
//
// Convention, deliberately fixed and documented so it cannot drift silently:
//   * Matrices are 4x4 stored row-major, `m[row * 4 + column]`.
//   * A point is a column vector: `out = M * in` (translation lives in column 3).
//   * A local transform is applied as `T * R * S`: scale first, then rotate,
//     then translate, which is what every model format means by "SRT".
//   * Euler rotation is radians, applied X then Y then Z in the local frame,
//     i.e. `R = Rz * Ry * Rx`. HSD stores Euler angles; the order is pinned here
//     (and by the transform test) because getting it wrong silently tips parts
//     of a room sideways instead of failing loudly.
#include <cmath>

#include "engine/core/types.hpp"

namespace gf {

struct Mat4 {
    f32 m[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    f32& at(unsigned row, unsigned column) { return m[row * 4 + column]; }
    f32 at(unsigned row, unsigned column) const { return m[row * 4 + column]; }
};

inline Mat4 mat4_identity() { return Mat4{}; }

inline Mat4 mat4_multiply(const Mat4& a, const Mat4& b) {
    Mat4 out;
    for (unsigned row = 0; row < 4; ++row) {
        for (unsigned column = 0; column < 4; ++column) {
            f32 sum = 0.0f;
            for (unsigned k = 0; k < 4; ++k) sum += a.at(row, k) * b.at(k, column);
            out.at(row, column) = sum;
        }
    }
    return out;
}

inline Mat4 mat4_translation(const f32 translation[3]) {
    Mat4 out;
    out.at(0, 3) = translation[0];
    out.at(1, 3) = translation[1];
    out.at(2, 3) = translation[2];
    return out;
}

inline Mat4 mat4_scale(const f32 scale[3]) {
    Mat4 out;
    out.at(0, 0) = scale[0];
    out.at(1, 1) = scale[1];
    out.at(2, 2) = scale[2];
    return out;
}

inline Mat4 mat4_rotation_x(f32 radians) {
    const f32 c = std::cos(radians), s = std::sin(radians);
    Mat4 out;
    out.at(1, 1) = c;
    out.at(1, 2) = -s;
    out.at(2, 1) = s;
    out.at(2, 2) = c;
    return out;
}

inline Mat4 mat4_rotation_y(f32 radians) {
    const f32 c = std::cos(radians), s = std::sin(radians);
    Mat4 out;
    out.at(0, 0) = c;
    out.at(0, 2) = s;
    out.at(2, 0) = -s;
    out.at(2, 2) = c;
    return out;
}

inline Mat4 mat4_rotation_z(f32 radians) {
    const f32 c = std::cos(radians), s = std::sin(radians);
    Mat4 out;
    out.at(0, 0) = c;
    out.at(0, 1) = -s;
    out.at(1, 0) = s;
    out.at(1, 1) = c;
    return out;
}

// Local transform from Euler rotation (radians), scale and translation.
inline Mat4 mat4_from_srt(const f32 rotation[3], const f32 scale[3],
                          const f32 translation[3]) {
    const Mat4 r = mat4_multiply(mat4_rotation_z(rotation[2]),
                                 mat4_multiply(mat4_rotation_y(rotation[1]),
                                               mat4_rotation_x(rotation[0])));
    return mat4_multiply(mat4_translation(translation), mat4_multiply(r, mat4_scale(scale)));
}

inline void mat4_transform_point(const Mat4& matrix, const f32 in[3], f32 out[3]) {
    for (unsigned row = 0; row < 3; ++row) {
        out[row] = matrix.at(row, 0) * in[0] + matrix.at(row, 1) * in[1] +
                   matrix.at(row, 2) * in[2] + matrix.at(row, 3);
    }
}

// Directions ignore translation. Normals need the inverse transpose below.
inline void mat4_transform_direction(const Mat4& matrix, const f32 in[3], f32 out[3]) {
    for (unsigned row = 0; row < 3; ++row) {
        out[row] = matrix.at(row, 0) * in[0] + matrix.at(row, 1) * in[1] +
                   matrix.at(row, 2) * in[2];
    }
}

struct NormalTransform {f32 cofactor[3][3];f32 orientation;};
inline NormalTransform mat4_normal_transform(const Mat4& m) {
    // Cofactor matrix = determinant * inverse transpose. Preserve orientation
    // under reflected transforms; normalize removes the determinant magnitude.
    NormalTransform result;auto& c=result.cofactor;
    for (unsigned r=0;r<3;++r) for (unsigned s=0;s<3;++s) {
        const unsigned r1=(r+1)%3,r2=(r+2)%3,s1=(s+1)%3,s2=(s+2)%3;
        c[r][s]=m.at(r1,s1)*m.at(r2,s2)-m.at(r1,s2)*m.at(r2,s1);
    }
    const f32 det=m.at(0,0)*c[0][0]+m.at(0,1)*c[0][1]+m.at(0,2)*c[0][2];
    result.orientation=det<0?-1.0f:1.0f;return result;
}
inline void mat4_transform_normal(const NormalTransform& transform,const f32 in[3],f32 out[3]){
    const auto& c=transform.cofactor;
    for (unsigned r=0;r<3;++r) out[r]=(c[r][0]*in[0]+c[r][1]*in[1]+c[r][2]*in[2])*transform.orientation;
    const f32 n=std::sqrt(out[0]*out[0]+out[1]*out[1]+out[2]*out[2]);
    if (n>1e-12f) for(unsigned r=0;r<3;++r) out[r]/=n;
    else for(unsigned r=0;r<3;++r) out[r]=0;
}
inline void mat4_transform_normal(const Mat4& m,const f32 in[3],f32 out[3]){
    mat4_transform_normal(mat4_normal_transform(m),in,out);
}

} // namespace gf
