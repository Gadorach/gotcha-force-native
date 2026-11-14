// Transform tests: the affine maths a scene uses to place instances. These pin
// the documented conventions (row-major 4x4, column-vector points, T*R*S order,
// Euler X then Y then Z) so a "harmless" refactor cannot silently tip a room.
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "engine/graphics/transform.hpp"

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "transform failure: %s\n", what);
        std::abort();
    }
}

constexpr gf::f32 kEps = 1e-5f;
bool near(gf::f32 a, gf::f32 b) { return std::fabs(a - b) <= kEps; }

struct V {
    gf::f32 v[3];
};

V point(gf::f32 x, gf::f32 y, gf::f32 z) { return V{{x, y, z}}; }

V apply(const gf::Mat4& m, V in) {
    V out{{0.0f, 0.0f, 0.0f}};
    gf::mat4_transform_point(m, in.v, out.v);
    return out;
}

V apply_dir(const gf::Mat4& m, V in) {
    V out{{0.0f, 0.0f, 0.0f}};
    gf::mat4_transform_direction(m, in.v, out.v);
    return out;
}

bool same(V a, V b) { return near(a.v[0], b.v[0]) && near(a.v[1], b.v[1]) && near(a.v[2], b.v[2]); }

} // namespace

int main() {
    // Identity leaves points and directions untouched.
    {
        const gf::Mat4 identity = gf::mat4_identity();
        check(same(apply(identity, point(1.0f, -2.0f, 3.0f)), point(1.0f, -2.0f, 3.0f)),
              "identity keeps a point");
        check(same(apply_dir(identity, point(0.0f, 1.0f, 0.0f)), point(0.0f, 1.0f, 0.0f)),
              "identity keeps a direction");
    }

    // Translation moves points but not directions.
    {
        const gf::Mat4 t = gf::mat4_translation(point(10.0f, 20.0f, 30.0f).v);
        check(same(apply(t, point(1.0f, 1.0f, 1.0f)), point(11.0f, 21.0f, 31.0f)),
              "translation moves a point");
        check(same(apply_dir(t, point(1.0f, 1.0f, 1.0f)), point(1.0f, 1.0f, 1.0f)),
              "translation leaves a direction alone");
    }

    // Scale multiplies each axis.
    {
        const gf::Mat4 s = gf::mat4_scale(point(2.0f, 3.0f, 4.0f).v);
        check(same(apply(s, point(1.0f, 1.0f, 1.0f)), point(2.0f, 3.0f, 4.0f)),
              "scale multiplies per axis");
    }

    // Right-handed rotation about each axis by +90 degrees.
    {
        const gf::f32 half_pi = 3.14159265358979f / 2.0f;
        check(same(apply(gf::mat4_rotation_x(half_pi), point(0.0f, 1.0f, 0.0f)),
                   point(0.0f, 0.0f, 1.0f)),
              "rotate X: +Y becomes +Z");
        check(same(apply(gf::mat4_rotation_y(half_pi), point(0.0f, 0.0f, 1.0f)),
                   point(1.0f, 0.0f, 0.0f)),
              "rotate Y: +Z becomes +X");
        check(same(apply(gf::mat4_rotation_z(half_pi), point(1.0f, 0.0f, 0.0f)),
                   point(0.0f, 1.0f, 0.0f)),
              "rotate Z: +X becomes +Y");
    }

    // SRT applies scale, then rotation, then translation.
    {
        const gf::f32 rotation[3] = {0.0f, 0.0f, 3.14159265358979f / 2.0f};
        const gf::f32 scale[3] = {2.0f, 2.0f, 2.0f};
        const gf::f32 translation[3] = {100.0f, 0.0f, 0.0f};
        const gf::Mat4 m = gf::mat4_from_srt(rotation, scale, translation);
        // (1,0,0) scaled to (2,0,0), rotated Z+90 to (0,2,0), translated.
        check(same(apply(m, point(1.0f, 0.0f, 0.0f)), point(100.0f, 2.0f, 0.0f)),
              "SRT scales before rotating and translates last");
        // The local origin lands exactly on the translation.
        check(same(apply(m, point(0.0f, 0.0f, 0.0f)), point(100.0f, 0.0f, 0.0f)),
              "SRT puts the origin at the translation");
    }

    // Euler order: X is applied first, so a Y rotation afterwards still moves the
    // vector that X produced. With (90 deg X, 90 deg Y) +Y maps to +Y -> +X.
    {
        const gf::f32 rotation[3] = {3.14159265358979f / 2.0f, 3.14159265358979f / 2.0f, 0.0f};
        const gf::f32 scale[3] = {1.0f, 1.0f, 1.0f};
        const gf::f32 translation[3] = {0.0f, 0.0f, 0.0f};
        const gf::Mat4 m = gf::mat4_from_srt(rotation, scale, translation);
        // Rz*Ry*Rx applied to +Y: Rx(+Y)=+Z, then Ry(+Z)=+X.
        check(same(apply(m, point(0.0f, 1.0f, 0.0f)), point(1.0f, 0.0f, 0.0f)),
              "Euler order is X then Y then Z");
    }

    // Composition: parent * child places a child in the parent's frame.
    {
        const gf::Mat4 parent =
            gf::mat4_from_srt(point(0.0f, 0.0f, 3.14159265358979f / 2.0f).v,
                              point(1.0f, 1.0f, 1.0f).v, point(10.0f, 0.0f, 0.0f).v);
        const gf::Mat4 child = gf::mat4_translation(point(1.0f, 0.0f, 0.0f).v);
        const gf::Mat4 world = gf::mat4_multiply(parent, child);
        // Child offset +X rotates to +Y inside the parent, then translates.
        check(same(apply(world, point(0.0f, 0.0f, 0.0f)), point(10.0f, 1.0f, 0.0f)),
              "parent * child composes");
    }

    // Multiplication is associative, and the identity is neutral on both sides.
    {
        const gf::Mat4 a = gf::mat4_translation(point(1.0f, 2.0f, 3.0f).v);
        const gf::Mat4 b = gf::mat4_scale(point(2.0f, 3.0f, 4.0f).v);
        const gf::Mat4 c = gf::mat4_rotation_z(0.7f);
        const gf::Mat4 left = gf::mat4_multiply(gf::mat4_multiply(a, b), c);
        const gf::Mat4 right = gf::mat4_multiply(a, gf::mat4_multiply(b, c));
        for (unsigned i = 0; i < 16; ++i) check(near(left.m[i], right.m[i]), "multiply is associative");
        const gf::Mat4 identity = gf::mat4_identity();
        const gf::Mat4 with_left = gf::mat4_multiply(identity, a);
        const gf::Mat4 with_right = gf::mat4_multiply(a, identity);
        for (unsigned i = 0; i < 16; ++i) {
            check(near(with_left.m[i], a.m[i]), "identity is neutral on the left");
            check(near(with_right.m[i], a.m[i]), "identity is neutral on the right");
        }
    }

    std::printf("{\"transform_cases\":8,\"all_passed\":true}\n");
    return 0;
}
