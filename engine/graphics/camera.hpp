#pragma once
// Camera: view/projection for the rewrite, with CPU-side world→screen projection.
//
// Render submissions carry screen-space vertices (the backends map game space to
// NDC), so a menu set of boxes and 2D billboards is transformed here rather than
// needing a vertex-shader matrix stack. That keeps the render seam small and lets
// the same scene draw under the headless recorder and the GL backend.
#include <cmath>
#include <array>
#include <algorithm>
#include <limits>

#include "engine/core/types.hpp"

namespace gf {

struct Camera {
    f32 eye[3] = {0.0f, 0.0f, 0.0f};
    f32 target[3] = {0.0f, 0.0f, -1.0f};
    f32 up[3] = {0.0f, 1.0f, 0.0f};
    f32 fov_y_degrees = 45.0f;
    f32 near_plane = 0.5f;
    f32 far_plane = 400.0f;
    s32 viewport_width = 640;
    s32 viewport_height = 480;
    s32 viewport_x = 0;
    s32 viewport_y = 0;
    // Zero retains pixel-derived aspect for existing scenes. Source projections
    // may explicitly differ from the viewport's pixel ratio.
    f32 projection_aspect = 0;

    std::array<u32,17> cache_key() const {
        std::array<u32,17> key{};unsigned at=0;
        for(const f32* vector:{eye,target,up})for(unsigned k=0;k<3;++k)key[at++]=f32_to_bits(vector[k]);
        for(f32 value:{fov_y_degrees,near_plane,far_plane})key[at++]=f32_to_bits(value);
        for(s32 value:{viewport_width,viewport_height,viewport_x,viewport_y})key[at++]=u32(value);
        key[at]=f32_to_bits(projection_aspect);return key;
    }

    struct Basis {
        f32 right[3], up[3], forward[3];
        f32 eye[3];
        f32 tan_half_fov;
        f32 aspect;
    };

    // Orthonormal basis; returns false if the camera is degenerate (eye==target or
    // up parallel to the view direction), in which case nothing should be drawn.
    bool basis(Basis* out) const {
        if (viewport_width <= 0 || viewport_height <= 0 || !std::isfinite(fov_y_degrees) ||
            fov_y_degrees <= 0 || fov_y_degrees >= 179 || !std::isfinite(near_plane) ||
            !std::isfinite(far_plane) || near_plane <= 0 || far_plane <= near_plane ||
            viewport_x < 0 || viewport_y < 0 ||
            s64(viewport_x)+viewport_width>0x7fffffff || s64(viewport_y)+viewport_height>0x7fffffff ||
            !std::isfinite(projection_aspect) || projection_aspect < 0) return false;
        const f32 tangent=static_cast<f32>(std::tan(fov_y_degrees * 3.14159265f / 360.0f));
        const f32 aspect=projection_aspect>0 ? projection_aspect
            : static_cast<f32>(viewport_width) / static_cast<f32>(viewport_height);
        const f32 horizontal=tangent*aspect;
        if(!std::isfinite(tangent)||!std::isfinite(horizontal)||!(tangent>0)||!(horizontal>0))return false;
        if(!orientation(out))return false;
        if(out){
            out->tan_half_fov = tangent;
            out->aspect = aspect;
        }
        return true;
    }

    //Projection-independent shared orientation. Source view initialization can
    //use this without inventing a field of view, clipping range or viewport.
    //Only axes/eye are written; tan_half_fov/aspect remain the caller's fields.
    bool orientation(Basis* out) const {
        for(unsigned i=0;i<3;++i)
            if(!std::isfinite(eye[i]) || !std::isfinite(target[i]) || !std::isfinite(up[i])) return false;
        f32 f[3] = {target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]};
        const f32 flen = length(f);
        if (!(flen > 1e-6f)) return false;
        for (unsigned i = 0; i < 3; ++i) f[i] /= flen;
        f32 r[3] = {f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2],
                    f[0] * up[1] - f[1] * up[0]};
        const f32 rlen = length(r);
        if (!(rlen > 1e-6f)) return false;
        for (unsigned i = 0; i < 3; ++i) r[i] /= rlen;
        f32 u[3] = {r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2],
                    r[0] * f[1] - r[1] * f[0]};
        if (out) {
            for (unsigned i = 0; i < 3; ++i) {
                out->right[i] = r[i];
                out->up[i] = u[i];
                out->forward[i] = f[i];
                out->eye[i] = eye[i];
            }
        }
        return true;
    }

    // Projects a world point to pixel coordinates plus a perspective scale factor
    // (1.0 at the near plane distance, smaller further away). Returns false when
    // the point is behind the camera or outside the depth range, so callers skip
    // it instead of drawing mirrored geometry.
    bool project(const f32 world[3], f32* screen_x, f32* screen_y, f32* scale) const {
        Basis b;
        if (!basis(&b)) return false;
        const f32 d[3] = {world[0] - b.eye[0], world[1] - b.eye[1], world[2] - b.eye[2]};
        const f32 depth = dot(d, b.forward);
        if (depth <= near_plane || depth > far_plane) return false;
        const f32 x = dot(d, b.right);
        const f32 y = dot(d, b.up);
        const f32 tan = b.tan_half_fov;
        if (!(tan > 0.0f)) return false;
        const f32 ndc_x = x / (depth * tan * b.aspect);
        const f32 ndc_y = y / (depth * tan);
        if (screen_x) *screen_x = viewport_x + (ndc_x * 0.5f + 0.5f) * static_cast<f32>(viewport_width);
        if (screen_y) *screen_y = viewport_y + (0.5f - ndc_y * 0.5f) * static_cast<f32>(viewport_height);
        if (scale) *scale = near_plane / depth;
        return true;
    }

    // Pixel scale for a world-space size at a given depth.
    // Pixel size of a world-space length at a given projection scale. The scale
    // returned by project() is near/depth, so the depth has to be recovered from
    // it: a previous version multiplied by near, which made every billboard and
    // reticle shrink by the near-plane distance (invisible once a scene used a
    // small near plane).
    f32 size_scale(f32 world_size, f32 project_scale) const {
        Basis b;
        if (!basis(&b) || b.tan_half_fov <= 0.0f) return 0.0f;
        if (!(project_scale > 0.0f) || !(near_plane > 0.0f) || viewport_height <= 0) return 0.0f;
        const f32 depth = near_plane / project_scale;
        return world_size * (static_cast<f32>(viewport_height) * 0.5f) / (depth * b.tan_half_fov);
    }

private:
    static f32 dot(const f32 a[3], const f32 b[3]) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }
    static f32 length(const f32 a[3]) { return std::sqrt(dot(a, a)); }
};

// Reframe evaluated geometry at the horizontal or vertical limit, whichever
// is narrower. Retain the viewing direction and the caller's clip planes.
inline bool camera_fit_bounds(Camera& camera,const std::array<f32,3>& low,const std::array<f32,3>& high,
                              f64 min_distance,f64 max_distance){
    Camera::Basis basis;
    if(!camera.basis(&basis)||!std::isfinite(min_distance)||!std::isfinite(max_distance)||
       min_distance<=0||max_distance<min_distance)return false;
    std::array<f64,3> size;
    for(unsigned k=0;k<3;++k){
        size[k]=f64(high[k])-low[k];
        if(!std::isfinite(size[k])||size[k]<0)return false;
    }
    const f64 radius=.5*std::hypot(size[0],size[1],size[2]);
    const f64 tangent=f64(basis.tan_half_fov)*std::min(1.f,basis.aspect);
    const f64 sine=tangent/std::hypot(1.,tangent);
    const f64 distance=std::max({min_distance,1.15*radius/sine,radius+2*camera.near_plane});
    if(distance>max_distance||distance+radius>=camera.far_plane)return false;
    Camera fitted=camera;
    for(unsigned k=0;k<3;++k){
        fitted.target[k]=f32(f64(low[k])+.5*size[k]);
        fitted.eye[k]=f32(fitted.target[k]-distance*basis.forward[k]);
    }
    if(!fitted.basis(nullptr))return false;
    camera=fitted;return true;
}

// Back the camera up only as far as needed, keeping its target and view
// direction. Fit horizontal and vertical limits independently: a bounding
// sphere would zoom out for depth even when the bodies fit on screen.
// grow_far lets a caller whose subject cannot fit at all extend the far plane
// to the measured depth instead of refusing to draw. The default keeps the
// caller's clip planes so ordinary subjects never change.
inline bool camera_dolly_to_fit_bounds(Camera& camera,const std::array<f32,3>& low,const std::array<f32,3>& high,
                                       bool grow_far=false){
    Camera::Basis basis;
    if(!camera.basis(&basis))return false;
    for(unsigned axis=0;axis<3;++axis)
        if(!std::isfinite(low[axis])||!std::isfinite(high[axis])||low[axis]>high[axis])return false;
    const f64 vertical=.88*f64(basis.tan_half_fov),horizontal=vertical*basis.aspect; // 6% on each edge
    f64 retreat=0,farthest=-std::numeric_limits<f64>::infinity();
    for(unsigned corner=0;corner<8;++corner){
        f64 x=0,y=0,depth=0;
        for(unsigned k=0;k<3;++k){
            const f64 offset=f64(corner&(1u<<k)?high[k]:low[k])-basis.eye[k];
            x+=offset*basis.right[k];y+=offset*basis.up[k];depth+=offset*basis.forward[k];
        }
        farthest=std::max(farthest,depth);
        retreat=std::max(retreat,std::max({std::abs(x)/horizontal,std::abs(y)/vertical,1.01*camera.near_plane})-depth);
    }
    if(retreat+farthest>=camera.far_plane){
        if(!grow_far||!std::isfinite(retreat+farthest))return false;
        camera.far_plane=f32(retreat+farthest+std::max(1.,.1*(retreat+farthest)));
    }
    if(retreat==0)return true;
    Camera fitted=camera;
    for(unsigned k=0;k<3;++k)fitted.eye[k]=f32(basis.eye[k]-retreat*basis.forward[k]);
    if(!fitted.basis(nullptr))return false;
    camera=fitted;return true;
}

} // namespace gf
