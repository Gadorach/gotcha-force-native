#pragma once
// Bounded frustum clipping before perspective divide. Attributes interpolate in
// view space at clip edges; inv_w reconstructs clip W in the backend so UV/color
// interpolation remains perspective-correct. UI vertices retain inv_w = 1.
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "engine/graphics/camera.hpp"
#include "engine/core/backends.hpp"

namespace gf {
// Reuse the basis already validated for this camera by a model submission. The
// camera and basis must describe the same view; no arithmetic approximation.
inline void project_triangle(const Camera& camera, const Camera::Basis& basis,
                             const Vertex world[3], std::vector<Vertex>& out) {
    if (camera.viewport_width <= 0 || camera.viewport_height <= 0 ||
        !(camera.near_plane > 0) || !(camera.far_plane > camera.near_plane) ||
        !(basis.tan_half_fov > 0)) return;
    struct Point { f32 x,y,z,u,v,r,g,b,a; f32 light[4]={},specular[4]={}; };
    std::array<Point, 12> first{}, second{};
    Point* a=first.data();Point* b=second.data();
    unsigned count = 3;
    for (unsigned i=0; i<3; ++i) {
        const f32 d[3] = {world[i].x-basis.eye[0],world[i].y-basis.eye[1],world[i].z-basis.eye[2]};
        auto dot = [&d](const f32* axis) { return d[0]*axis[0]+d[1]*axis[1]+d[2]*axis[2]; };
        a[i] = {dot(basis.right),dot(basis.up),dot(basis.forward),world[i].u,world[i].v,
                 f32(world[i].r),f32(world[i].g),f32(world[i].b),f32(world[i].a)};
        for(unsigned k=0;k<4;++k){a[i].light[k]=world[i].light[k];a[i].specular[k]=world[i].specular[k];}
        if (!std::isfinite(a[i].x) || !std::isfinite(a[i].y) || !std::isfinite(a[i].z)) return;
    }
    const f32 horizontal = basis.tan_half_fov*basis.aspect;
    auto distance = [&](const Point& p,unsigned plane) {
        switch (plane) {
        case 0:return p.z-camera.near_plane; case 1:return camera.far_plane-p.z;
        case 2:return p.x+p.z*horizontal; case 3:return p.z*horizontal-p.x;
        case 4:return p.y+p.z*basis.tan_half_fov;
        default:return p.z*basis.tan_half_fov-p.y;
        }
    };
    for (unsigned plane=0;plane<6 && count; ++plane) {
        // Most triangles are wholly inside a plane. Keep their original order
        // and attributes directly instead of copying the polygon six times.
        std::array<f32,12> distances{};unsigned inside=0;
        for(unsigned i=0;i<count;++i){distances[i]=distance(a[i],plane);inside+=distances[i]>=0;}
        if(inside==count)continue;
        if(!inside)return;
        unsigned n=0;
        for (unsigned i=0;i<count;++i) {
            const Point& p=a[i]; const Point& q=a[(i+1)%count];
            const f32 dp=distances[i], dq=distances[(i+1)%count];
            if (dp>=0 && n<second.size()) b[n++]=p;
            if ((dp>=0)!=(dq>=0) && n<second.size()) {
                const f32 t=dp/(dp-dq);
                b[n++] = {p.x+(q.x-p.x)*t,p.y+(q.y-p.y)*t,p.z+(q.z-p.z)*t,
                          p.u+(q.u-p.u)*t,p.v+(q.v-p.v)*t,p.r+(q.r-p.r)*t,
                           p.g+(q.g-p.g)*t,p.b+(q.b-p.b)*t,p.a+(q.a-p.a)*t};
                for(unsigned k=0;k<4;++k){b[n-1].light[k]=p.light[k]+(q.light[k]-p.light[k])*t;
                    b[n-1].specular[k]=p.specular[k]+(q.specular[k]-p.specular[k])*t;}
            }
        }
        count=n; std::swap(a,b);
    }
    auto vertex = [&](const Point& p) {
        const f32 inv=1.0f/p.z;
        auto byte=[](f32 value){return u8(std::clamp(value,0.0f,255.0f)+0.5f);};
        Vertex v;
        v.x=camera.viewport_x+(p.x*inv/horizontal*0.5f+0.5f)*camera.viewport_width;
        v.y=camera.viewport_y+(0.5f-p.y*inv/basis.tan_half_fov*0.5f)*camera.viewport_height;
        v.z=std::clamp(camera.near_plane*inv,0.0f,1.0f);
        v.u=p.u; v.v=p.v; v.inv_w=inv;
        v.r=byte(p.r);v.g=byte(p.g);v.b=byte(p.b);v.a=byte(p.a);
        for(unsigned k=0;k<4;++k){v.light[k]=byte(p.light[k]);v.specular[k]=byte(p.specular[k]);}
        return v;
    };
    for (unsigned i=1;i+1<count;++i) {
        out.push_back(vertex(a[0]));out.push_back(vertex(a[i]));out.push_back(vertex(a[i+1]));
    }
}
inline void project_triangle(const Camera& camera, const Vertex world[3], std::vector<Vertex>& out) {
    Camera::Basis basis;
    if(camera.basis(&basis))project_triangle(camera,basis,world,out);
}
} // namespace gf
