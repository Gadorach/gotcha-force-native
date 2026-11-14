// Conservative rejection tested against actual float transform + clipping.
#include <cstdio>
#include <cstdlib>
#include "engine/graphics/frustum_bounds.hpp"
#include "engine/graphics/projected_triangle.hpp"
namespace {void check(bool ok){if(!ok)std::abort();}}
int main(){
 using namespace gf;Camera c;c.fov_y_degrees=90;c.near_plane=1;c.far_plane=100;Camera::Basis basis;check(c.basis(&basis));Bounds3 empty;check(frustum_bounds_visible(empty,Mat4{},c,basis));
 Bounds3 boundary;const float p[3]={0,0,-1};boundary.include(p);check(frustum_bounds_visible(boundary,Mat4{},c,basis));Mat4 shift;shift.at(0,3)=1000;check(!frustum_bounds_visible(boundary,shift,c,basis));
 u32 random=0x19ab4ff2;auto next=[&](){random=random*1664525+1013904223;return random;};unsigned rejected=0,visible=0;
 for(unsigned i=0;i<32768;++i){c.near_plane=.01f+float(i%13)*.25f;c.far_plane=c.near_plane+1+float(i%103);c.fov_y_degrees=10+float(i%165);c.viewport_width=113+i%1200;c.viewport_height=97+i%900;c.eye[0]=float(int(i%7)-3);c.target[0]=c.eye[0];check(c.basis(&basis));
  const float magnitude=i%5==0?1e5f:1;Bounds3 bounds;Vertex world[3];float local[3][3];for(auto& v:local){for(auto& x:v)x=float(int(next()%2001)-1000)*magnitude/100;bounds.include(v);}
  Mat4 matrix;for(unsigned row=0;row<3;++row){for(unsigned col=0;col<3;++col)matrix.at(row,col)=float(int(next()%201)-100)/57;matrix.at(row,3)=float(int(next()%4001)-2000)*magnitude/100;}
  for(unsigned n=0;n<3;++n){float out[3];mat4_transform_point(matrix,local[n],out);world[n].x=out[0];world[n].y=out[1];world[n].z=out[2];}
  std::vector<Vertex> clipped;project_triangle(c,world,clipped);const bool keep=frustum_bounds_visible(bounds,matrix,c,basis);if(!keep){++rejected;check(clipped.empty());}visible+=!clipped.empty();
 }
 check(rejected>1000&&visible>1000);std::printf("frustum bounds: 32768 transformed/sheared/reflected/large-coordinate triangles; %u rejections, %u visible, no false rejection PASS\n",rejected,visible);
}
