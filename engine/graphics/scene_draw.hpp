#pragma once
// Screen-space drawing helpers built on Camera: boxes for the 3D menu set,
// camera-facing billboards for the menu seals, and a ring for the reticle.
//
// Everything is emitted as screen-space triangles with z used for depth, so the
// render seam stays the same whether the backend is the headless recorder or GL.
#include <cmath>
#include <cstddef>
#include <vector>

#include "engine/core/backends.hpp"
#include "engine/graphics/camera.hpp"
#include "engine/graphics/render_engine.hpp"

namespace gf {

// Depth convention for this seam: z is a normalised perspective value where
// LARGER means NEARER (z = near_plane / depth). Backends must test with
// "greater or equal" and clear to 0; the GL backend does. Documented here because
// getting it backwards silently draws the far geometry on top.
struct Tint {
    u8 r = 255, g = 255, b = 255, a = 255;
};

// Face shading for boxes: a fixed light direction keeps the 3D set readable and,
// more importantly, deterministic.
inline Tint shade(Tint base, f32 amount) {
    const f32 clamped = amount < 0.25f ? 0.25f : (amount > 1.0f ? 1.0f : amount);
    return Tint{u8(base.r * clamped), u8(base.g * clamped), u8(base.b * clamped), base.a};
}

class ScreenDraw {
public:
    // Adds a triangle with a depth value so the backend's depth test orders the
    // 3D set correctly.
    static void triangle(RenderEngine& render, std::vector<Vertex>& out, f32 x0, f32 y0, f32 z0,
                         f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2, Tint tint) {
        (void)render;
        push(out, x0, y0, z0, tint);
        push(out, x1, y1, z1, tint);
        push(out, x2, y2, z2, tint);
    }

    static void push(std::vector<Vertex>& out, f32 x, f32 y, f32 z, Tint tint, f32 u = 0.0f,
                     f32 v_coord = 0.0f) {
        Vertex v;
        v.x = x;
        v.y = y;
        v.z = z;
        v.r = tint.r;
        v.g = tint.g;
        v.b = tint.b;
        v.a = tint.a;
        v.u = u;
        v.v = v_coord;
        out.push_back(v);
    }

    // Axis-aligned screen-space quad carrying texture coordinates. The caller
    // uploads the texture once (`RenderEngine::upload_texture`) and sets
    // `state().texture` for the draw; this only emits geometry, so the same
    // helper works for decoded TPL/HSD art and for any future UI atlas.
    static void textured_quad(std::vector<Vertex>& out, f32 x0, f32 y0, f32 x1, f32 y1, f32 z,
                              f32 u0, f32 v0, f32 u1, f32 v1, Tint tint = Tint{}) {
        push(out, x0, y0, z, tint, u0, v0);
        push(out, x1, y0, z, tint, u1, v0);
        push(out, x1, y1, z, tint, u1, v1);
        push(out, x0, y0, z, tint, u0, v0);
        push(out, x1, y1, z, tint, u1, v1);
        push(out, x0, y1, z, tint, u0, v1);
    }

    // Camera-facing textured billboard, the same projection and depth rules as
    // the flat one but sampling a whole texture.
    static bool textured_billboard(const Camera& camera, const f32 world[3], f32 world_size,
                                   Tint tint, std::vector<Vertex>& out, std::size_t* first,
                                   std::size_t* count) {
        f32 sx = 0.0f, sy = 0.0f, scale = 0.0f;
        if (!camera.project(world, &sx, &sy, &scale)) return false;
        const f32 half = camera.size_scale(world_size, scale) * 0.5f;
        if (!(half > 0.0f)) return false;
        *first = out.size();
        textured_quad(out, sx - half, sy - half, sx + half, sy + half, scale, 0.0f, 0.0f, 1.0f,
                      1.0f, tint);
        *count = out.size() - *first;
        return true;
    }

    // Axis-aligned box in world space, projected and drawn as twelve triangles.
    // Returns the vertex count appended (0 when the box is not visible at all).
    static std::size_t box(RenderEngine& render, const Camera& camera, const f32 center[3],
                           const f32 half[3], Tint tint, std::vector<Vertex>& out) {
        const std::size_t first = out.size();
        f32 px[8], py[8], pz[8];
        bool visible = false;
        for (unsigned i = 0; i < 8; ++i) {
            const f32 corner[3] = {center[0] + ((i & 1) ? half[0] : -half[0]),
                                   center[1] + ((i & 2) ? half[1] : -half[1]),
                                   center[2] + ((i & 4) ? half[2] : -half[2])};
            f32 scale = 0.0f;
            if (!camera.project(corner, &px[i], &py[i], &scale)) {
                // A corner behind the camera drops the whole box: partial boxes
                // would need clipping, which the menu set does not require.
                out.resize(first);
                return 0;
            }
            pz[i] = scale;
            visible = true;
        }
        if (!visible) return 0;

        struct Face { unsigned a, b, c, d; f32 light; };
        // Corners are indexed bitwise: x=1, y=2, z=4.
        const Face faces[6] = {
            {1, 3, 7, 5, 1.00f},   // +x
            {0, 4, 6, 2, 0.72f},   // -x
            {2, 6, 7, 3, 0.90f},   // +y
            {0, 1, 5, 4, 0.55f},   // -y
            {4, 5, 7, 6, 0.80f},   // +z
            {0, 2, 3, 1, 0.62f},   // -z
        };
        for (const Face& face : faces) {
            const Tint f = shade(tint, face.light);
            quad(out, px[face.a], py[face.a], pz[face.a], px[face.b], py[face.b], pz[face.b],
                 px[face.c], py[face.c], pz[face.c], px[face.d], py[face.d], pz[face.d], f);
        }
        (void)render;
        return out.size() - first;
    }

    // Camera-facing quad anchored at a world position. `world_size` is the quad
    // extent in world units; it is projected so distance shrinks it naturally.
    static bool billboard(const Camera& camera, const f32 world[3], f32 world_size, Tint tint,
                          std::vector<Vertex>& out, std::size_t* first, std::size_t* count) {
        f32 sx = 0.0f, sy = 0.0f, scale = 0.0f;
        if (!camera.project(world, &sx, &sy, &scale)) return false;
        const f32 half = camera.size_scale(world_size, scale) * 0.5f;
        if (!(half > 0.0f)) return false;
        *first = out.size();
        quad(out, sx - half, sy - half, scale, sx + half, sy - half, scale, sx + half, sy + half,
             scale, sx - half, sy + half, scale, tint);
        *count = out.size() - *first;
        return true;
    }

    // Four swept blades with a bright centre; visual only, never a hit shape.
    static bool shuriken(const Camera& camera,const f32 world[3],f32 size,f32 spin,Tint tint,std::vector<Vertex>& out){
        f32 x=0,y=0,depth=0;
        if(!camera.project(world,&x,&y,&depth))return false;
        const auto radius=camera.size_scale(size,depth)*.5f;
        if(!(radius>0))return false;
        for(unsigned arm=0;arm<4;++arm){
            const f32 angle=spin+f32(arm)*1.57079632679f;
            const auto point=[&](f32 r,f32 offset,Tint color){
                push(out,x+r*radius*std::cos(angle+offset),y+r*radius*std::sin(angle+offset),depth,color);
            };
            point(0,0,{255,250,255,tint.a});point(.36f,-.46f,tint);point(1,.2f,tint);
            point(0,0,{255,250,255,tint.a});point(1,.2f,tint);point(.42f,.72f,tint);
        }
        return true;
    }

    // Filled disc in screen space around a projected world point: a triangle
    // fan, used for the menu's seal markers so they read as discs rather than
    // squares. Depth comes from the projected centre, like the reticle.
    static bool disc(const Camera& camera, const f32 world[3], f32 world_radius, unsigned segments,
                     Tint tint, std::vector<Vertex>& out, std::size_t* first,
                     std::size_t* count) {
        f32 sx = 0.0f, sy = 0.0f, scale = 0.0f;
        if (!camera.project(world, &sx, &sy, &scale)) return false;
        const f32 radius = camera.size_scale(world_radius, scale);
        if (!(radius > 0.0f) || segments < 3) return false;
        *first = out.size();
        for (unsigned i = 0; i < segments; ++i) {
            const f32 a0 = 6.2831853f * static_cast<f32>(i) / static_cast<f32>(segments);
            const f32 a1 = 6.2831853f * static_cast<f32>(i + 1) / static_cast<f32>(segments);
            push(out, sx, sy, scale, tint);
            push(out, sx + radius * std::cos(a0), sy + radius * std::sin(a0), scale, tint);
            push(out, sx + radius * std::cos(a1), sy + radius * std::sin(a1), scale, tint);
        }
        *count = out.size() - *first;
        return true;
    }

    // Ring (reticle) in screen space around a projected world point.
    static bool ring(const Camera& camera, const f32 world[3], f32 world_radius, f32 thickness,
                     unsigned segments, Tint tint, std::vector<Vertex>& out,
                     std::size_t* first, std::size_t* count) {
        f32 sx = 0.0f, sy = 0.0f, scale = 0.0f;
        if (!camera.project(world, &sx, &sy, &scale)) return false;
        const f32 radius = camera.size_scale(world_radius, scale);
        const f32 inner = camera.size_scale(world_radius - thickness, scale);
        if (!(radius > 0.0f) || segments < 3) return false;
        *first = out.size();
        for (unsigned i = 0; i < segments; ++i) {
            const f32 a0 = 6.2831853f * static_cast<f32>(i) / static_cast<f32>(segments);
            const f32 a1 = 6.2831853f * static_cast<f32>(i + 1) / static_cast<f32>(segments);
            const f32 c0 = std::cos(a0), s0 = std::sin(a0);
            const f32 c1 = std::cos(a1), s1 = std::sin(a1);
            // Outer and inner edge form a quad per segment.
            quad(out, sx + radius * c0, sy + radius * s0, scale, sx + radius * c1,
                 sy + radius * s1, scale, sx + inner * c1, sy + inner * s1, scale,
                 sx + inner * c0, sy + inner * s0, scale, tint);
        }
        *count = out.size() - *first;
        return true;
    }

    static void quad(std::vector<Vertex>& out, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1,
                     f32 x2, f32 y2, f32 z2, f32 x3, f32 y3, f32 z3, Tint tint) {
        push(out, x0, y0, z0, tint); push(out, x1, y1, z1, tint); push(out, x2, y2, z2, tint);
        push(out, x0, y0, z0, tint); push(out, x2, y2, z2, tint); push(out, x3, y3, z3, tint);
    }
};

} // namespace gf
