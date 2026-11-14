// Scene-draw primitive tests: the CPU-side geometry the menu set, billboards,
// reticle and (now) textured art all share. These pin the contract the backends
// rely on: vertex counts, the near/depth depth value, projection rejection and
// texture coordinates, so a change here cannot silently reorder or mis-texture
// the scene.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "engine/core/headless_backends.hpp"
#include "engine/graphics/camera.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/graphics/scene_draw.hpp"

namespace {

void check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "scene_draw test failed: %s\n", message);
        std::abort();
    }
}

gf::Camera make_camera() {
    gf::Camera camera;
    camera.eye[0] = 0.0f;
    camera.eye[1] = 0.0f;
    camera.eye[2] = 0.0f;
    camera.target[0] = 0.0f;
    camera.target[1] = 0.0f;
    camera.target[2] = -1.0f;
    camera.up[0] = 0.0f;
    camera.up[1] = 1.0f;
    camera.up[2] = 0.0f;
    camera.fov_y_degrees = 45.0f;
    camera.near_plane = 1.0f;
    camera.far_plane = 100.0f;
    camera.viewport_width = 640;
    camera.viewport_height = 480;
    return camera;
}

constexpr gf::f32 kEps = 1e-3f;

bool near(gf::f32 a, gf::f32 b) { return std::fabs(a - b) <= kEps; }

// Small array wrapper: `Camera::project` takes `const f32[3]`, and a temporary
// array keeps the call sites readable without a named local per case.
struct Point {
    gf::f32 v[3];
};
Point point(gf::f32 x, gf::f32 y, gf::f32 z) { return Point{{x, y, z}}; }

// Every vertex of a run must carry the same z, and that z must be the seam's
// perspective value: near/depth, so a nearer object has a LARGER z.
void check_depth_run(const std::vector<gf::Vertex>& vertices, std::size_t first,
                     std::size_t count, gf::f32 expected_z) {
    for (std::size_t i = first; i < first + count; ++i) {
        check(near(vertices[i].z, expected_z), "vertex z does not match near/depth");
    }
}

void test_projection() {
    gf::Camera camera = make_camera();
    gf::f32 x = 0.0f, y = 0.0f, scale = 0.0f;
    check(camera.project(point(0.0f, 0.0f, -10.0f).v, &x, &y, &scale),
          "a point in front must project");
    check(near(x, 320.0f) && near(y, 240.0f), "centre point must land at the viewport centre");
    check(near(scale, 0.1f), "scale must be near/depth");

    x = y = scale = 0.0f;
    check(!camera.project(point(0.0f, 0.0f, 10.0f).v, &x, &y, &scale),
          "a point behind the camera must be rejected");
    check(!camera.project(point(0.0f, 0.0f, -200.0f).v, &x, &y, &scale),
          "a point beyond the far plane must be rejected");
    check(!camera.project(point(0.0f, 0.0f, -0.5f).v, &x, &y, &scale),
          "a point inside the near plane must be rejected");

    // Off-centre points move right/up in screen space with the right sign.
    check(camera.project(point(5.0f, 0.0f, -10.0f).v, &x, &y, &scale), "right point projects");
    check(x > 320.0f, "positive x must project right of centre");
    check(camera.project(point(0.0f, 5.0f, -10.0f).v, &x, &y, &scale), "upper point projects");
    check(y < 240.0f, "positive y must project above centre (screen y grows down)");

    // A degenerate camera draws nothing rather than mirrored geometry.
    gf::Camera broken = make_camera();
    broken.target[0] = broken.eye[0];
    broken.target[1] = broken.eye[1];
    broken.target[2] = broken.eye[2];
    gf::Camera::Basis basis{};
    check(!broken.basis(&basis), "eye == target is degenerate");
}

void test_box() {
    // `box` takes the render engine for symmetry with the other emitters but
    // only writes geometry; a headless backend keeps the call honest (no null
    // reference) without touching a GPU.
    gf::headless::Render backend;
    gf::RenderEngine render(backend);
    gf::Camera camera = make_camera();
    std::vector<gf::Vertex> vertices;
    const gf::f32 centre[3] = {0.0f, 0.0f, -10.0f};
    const gf::f32 half[3] = {1.0f, 1.0f, 1.0f};
    const gf::Tint tint{100, 120, 140, 200};
    const std::size_t added = gf::ScreenDraw::box(render, camera, centre, half, tint, vertices);
    check(added == 36, "a box is six quads, 36 vertices");
    check(vertices.size() == 36, "box vertices appended");
    // A box spans depth, so each corner carries its own near/depth value: the
    // nearest corner (depth 9) must be the largest, the farthest (11) smallest.
    for (const gf::Vertex& v : vertices) {
        check(v.z >= 1.0f / 11.0f - kEps && v.z <= 1.0f / 9.0f + kEps,
              "box corner z is near/depth for that corner");
    }
    bool saw_nearest = false;
    for (const gf::Vertex& v : vertices) {
        if (near(v.z, 1.0f / 9.0f)) saw_nearest = true;
    }
    check(saw_nearest, "the box's nearest corners carry the larger depth value");
    for (const gf::Vertex& v : vertices) {
        check(v.a == 200, "box keeps the tint alpha");
        check(v.x >= 0.0f && v.x <= 640.0f && v.y >= 0.0f && v.y <= 480.0f,
              "a centred box projects inside the viewport");
    }

    // Every face must be shaded differently: the fixed light is what makes the
    // set readable without lighting hardware.
    for (std::size_t face = 0; face < 6; ++face) {
        check(vertices[face * 6].r == vertices[face * 6 + 5].r, "a face shares one shade");
        for (std::size_t other = face + 1; other < 6; ++other) {
            check(!(vertices[face * 6].r == vertices[other * 6].r &&
                    vertices[face * 6].g == vertices[other * 6].g),
                  "faces must be shaded differently");
        }
    }

    // A nearer box must carry a larger depth value than a farther one.
    std::vector<gf::Vertex> near_box, far_box;
    gf::ScreenDraw::box(render, camera,
                        point(0.0f, 0.0f, -5.0f).v, half, tint, near_box);
    gf::ScreenDraw::box(render, camera,
                        point(0.0f, 0.0f, -50.0f).v, half, tint, far_box);
    check(near_box[0].z > far_box[0].z, "nearer geometry must have a larger z");

    // A box with a corner behind the camera is dropped whole (no clipping).
    std::vector<gf::Vertex> clipped;
    const std::size_t dropped = gf::ScreenDraw::box(render, camera, point(0.0f, 0.0f, 0.0f).v,
                                                    point(2.0f, 2.0f, 2.0f).v, tint, clipped);
    check(dropped == 0 && clipped.empty(), "a box spanning the near plane is dropped");

    // Determinism: identical input, identical stream.
    std::vector<gf::Vertex> again;
    gf::ScreenDraw::box(render, camera, centre, half, tint, again);
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        check(near(again[i].x, vertices[i].x) && near(again[i].y, vertices[i].y) &&
                  near(again[i].z, vertices[i].z) && again[i].r == vertices[i].r,
              "box output must be deterministic");
    }
}

void test_billboard() {
    gf::Camera camera = make_camera();
    const gf::f32 anchor_near[3] = {0.0f, 0.0f, -10.0f};
    const gf::f32 anchor_far[3] = {0.0f, 0.0f, -40.0f};
    const gf::Tint tint{10, 20, 30, 255};

    std::vector<gf::Vertex> near_quad, far_quad;
    std::size_t first = 0, count = 0;
    check(gf::ScreenDraw::billboard(camera, anchor_near, 2.0f, tint, near_quad, &first, &count),
          "billboard in front projects");
    check(first == 0 && count == 6, "a billboard is one quad");
    check_depth_run(near_quad, 0, 6, 0.1f);

    check(gf::ScreenDraw::billboard(camera, anchor_far, 2.0f, tint, far_quad, &first, &count),
          "far billboard projects");
    check_depth_run(far_quad, 0, 6, 0.025f);

    // Same world size, different depth: the nearer quad must be bigger.
    const gf::f32 near_width = near_quad[1].x - near_quad[0].x;
    const gf::f32 far_width = far_quad[1].x - far_quad[0].x;
    check(near_width > far_width, "billboards shrink with distance");
    check(near(near_width, far_width * 4.0f), "billboard width scales as 1/depth");

    // Centred on the projected anchor.
    gf::f32 sx = 0.0f, sy = 0.0f, scale = 0.0f;
    check(camera.project(anchor_near, &sx, &sy, &scale), "anchor projects");
    check(near((near_quad[0].x + near_quad[1].x) * 0.5f, sx), "billboard is centred in x");
    check(near((near_quad[0].y + near_quad[2].y) * 0.5f, sy), "billboard is centred in y");

    // Behind the camera: no geometry and no partial append.
    std::vector<gf::Vertex> behind;
    check(!gf::ScreenDraw::billboard(camera, point(0.0f, 0.0f, 10.0f).v, 2.0f, tint, behind,
                                     &first, &count),
          "billboard behind the camera is rejected");
    check(behind.empty(), "rejected billboard appends nothing");

    // Zero size is rejected rather than emitting a degenerate quad.
    std::vector<gf::Vertex> zero;
    check(!gf::ScreenDraw::billboard(camera, anchor_near, 0.0f, tint, zero, &first, &count),
          "zero-size billboard is rejected");
}

void test_ring() {
    gf::Camera camera = make_camera();
    const gf::f32 anchor[3] = {0.0f, 0.0f, -10.0f};
    std::vector<gf::Vertex> ring;
    std::size_t first = 0, count = 0;
    check(gf::ScreenDraw::ring(camera, anchor, 2.0f, 0.4f, 24, gf::Tint{255, 96, 40, 255}, ring,
                               &first, &count),
          "reticle ring projects");
    check(count == 24 * 6, "a 24-segment ring is 24 quads");
    check_depth_run(ring, 0, count, 0.1f);
    for (std::size_t i = 0; i < count; ++i) check(near(ring[i].r, 255.0f) && near(ring[i].g, 96.0f),
                                                  "ring keeps the reticle tint");

    std::vector<gf::Vertex> degenerate;
    check(!gf::ScreenDraw::ring(camera, anchor, 2.0f, 0.4f, 2, gf::Tint{}, degenerate, &first,
                                &count),
          "a ring needs at least three segments");
    check(!gf::ScreenDraw::ring(camera, point(0.0f, 0.0f, 10.0f).v, 2.0f, 0.4f, 24,
                                gf::Tint{}, degenerate, &first, &count),
          "a ring behind the camera is rejected");
}

void test_disc() {
    gf::Camera camera = make_camera();
    const gf::f32 anchor[3] = {0.0f, 0.0f, -10.0f};
    std::vector<gf::Vertex> disc;
    std::size_t first = 0, count = 0;
    check(gf::ScreenDraw::disc(camera, anchor, 2.0f, 20, gf::Tint{10, 200, 30, 200}, disc, &first,
                               &count),
          "disc projects");
    check(count == 20 * 3, "a 20-segment disc is 20 triangles");
    check_depth_run(disc, 0, count, 0.1f);
    // The fan is centred on the projected anchor.
    gf::f32 sx = 0.0f, sy = 0.0f, scale = 0.0f;
    check(camera.project(anchor, &sx, &sy, &scale), "anchor projects");
    check(near(disc[0].x, sx) && near(disc[0].y, sy), "first vertex is the centre");
    for (const gf::Vertex& v : disc) check(v.g == 200 && v.a == 200, "disc keeps the tint");

    std::vector<gf::Vertex> degenerate;
    check(!gf::ScreenDraw::disc(camera, anchor, 2.0f, 2, gf::Tint{}, degenerate, &first, &count),
          "a disc needs at least three segments");
    check(!gf::ScreenDraw::disc(camera, point(0.0f, 0.0f, 10.0f).v, 2.0f, 20, gf::Tint{},
                                degenerate, &first, &count),
          "a disc behind the camera is rejected");
}

void test_textured() {
    // Screen-space quad: exact corners, exact uv mapping, one depth value.
    std::vector<gf::Vertex> quad;
    gf::ScreenDraw::textured_quad(quad, 10.0f, 20.0f, 110.0f, 70.0f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f,
                                  gf::Tint{255, 255, 255, 128});
    check(quad.size() == 6, "textured quad is two triangles");
    check(near(quad[0].u, 0.0f) && near(quad[0].v, 0.0f), "first vertex is the top-left texel");
    check(near(quad[1].u, 1.0f) && near(quad[1].v, 0.0f), "second vertex is the top-right texel");
    check(near(quad[2].u, 1.0f) && near(quad[2].v, 1.0f), "third vertex is bottom-right");
    check(near(quad[5].u, 0.0f) && near(quad[5].v, 1.0f), "last vertex is bottom-left");
    for (const gf::Vertex& v : quad) {
        check(near(v.z, 0.5f), "quad carries the requested depth");
        check(v.a == 128, "quad carries the tint alpha");
    }

    // Textured billboard must be the same geometry as the flat one, with uv.
    gf::Camera camera = make_camera();
    const gf::f32 anchor[3] = {0.0f, 0.0f, -10.0f};
    std::vector<gf::Vertex> flat, textured;
    std::size_t first = 0, count = 0;
    check(gf::ScreenDraw::billboard(camera, anchor, 2.0f, gf::Tint{}, flat, &first, &count),
          "flat billboard");
    check(gf::ScreenDraw::textured_billboard(camera, anchor, 2.0f, gf::Tint{}, textured, &first,
                                             &count),
          "textured billboard");
    check(flat.size() == textured.size(), "textured billboard has the same vertex count");
    for (std::size_t i = 0; i < flat.size(); ++i) {
        check(near(flat[i].x, textured[i].x) && near(flat[i].y, textured[i].y) &&
                  near(flat[i].z, textured[i].z),
              "textured billboard geometry matches the flat one");
    }
    check(near(textured[0].u, 0.0f) && near(textured[1].u, 1.0f) &&
              near(textured[2].v, 1.0f) && near(textured[0].v, 0.0f),
          "billboard uv spans the texture");

    std::vector<gf::Vertex> behind;
    check(!gf::ScreenDraw::textured_billboard(camera, point(0.0f, 0.0f, 5.0f).v, 2.0f,
                                              gf::Tint{}, behind, &first, &count),
          "textured billboard behind the camera is rejected");
}

void test_shade() {
    const gf::Tint base{200, 100, 50, 180};
    const gf::Tint full = gf::shade(base, 1.0f);
    check(full.r == 200 && full.g == 100 && full.b == 50 && full.a == 180, "full light is identity");
    const gf::Tint dim = gf::shade(base, 0.5f);
    check(dim.r == 100 && dim.g == 50 && dim.b == 25, "shade scales colour");
    check(dim.a == 180, "shade leaves alpha alone");
    const gf::Tint floor_light = gf::shade(base, -5.0f);
    check(floor_light.r == 50, "shade clamps to a 0.25 floor rather than going black");
    const gf::Tint capped = gf::shade(base, 4.0f);
    check(capped.r == 200, "shade clamps to 1.0 rather than overflowing a u8");
}

} // namespace

int main() {
    test_projection();
    test_box();
    test_billboard();
    test_ring();
    test_disc();
    test_textured();
    test_shade();
    std::printf("{\"scene_draw_cases\":7,\"box_vertices\":36,\"ring_segments\":24,"
                "\"all_passed\":true}\n");
    return 0;
}
