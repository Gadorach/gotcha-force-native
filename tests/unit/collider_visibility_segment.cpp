#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/collider_visibility_segment.hpp"
#include "tests/unit/collider_plane_fixture.hpp"
namespace {
unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"visibility query: %s\n",why);std::abort();}}
gf::ColliderInitialShapeProfile geometry(){
 using namespace gf;std::array<u8,208> bytes{};const std::array<u32,12> words{0x47464353,1,208,0,0x8003a4b8,0x8003a8fc,0x8003b170,0x8003c3d8,0x8008a65c,0x8008aa40,0x8008a2bc,0x80089aec};
 for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
 const auto p=collider_plane_fixture::profile();for(unsigned i=0;i<8;++i)store_be32(bytes.data()+48+4*i,p.words[i]);
 const std::array<f32,24> corners{1,1,-1,1,1,1,-1,1,-1,-1,1,1,1,-1,-1,1,-1,1,-1,-1,-1,-1,-1,1};for(unsigned i=0;i<24;++i)store_be_f32(bytes.data()+80+4*i,corners[i]);
 const std::array<u8,24> faces{0,2,3,1,1,3,7,5,2,6,7,3,4,5,7,6,6,2,0,4,5,4,0,1};std::copy(faces.begin(),faces.end(),bytes.begin()+176);store_be32(bytes.data()+200,1);
 ColliderInitialShapeProfile out;std::string e;if(!out.load({bytes.data(),bytes.size()},&e))std::abort();return out;
}
}
int main(){
 using namespace gf;std::string error;std::array<u8,128> bytes{};
 const std::array<u32,32> words{0x47464356,1,128,0,0x80036e24,0x80037f20,0x800381d0,0x80038478,0x800389e8,0x80038b34,0x80038c00,0x80039564,0x80039da4,0x802cffb0,0,0x3f800000,0x3f000000,0x40000000,0x40800000,0x3c23d70a,0x3a83126f,0x80000000,13,0x800,0x270,0x286,0x26f,1,2,0,1,0};
 for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
 ColliderVisibilityProfile profile;check(profile.load({bytes.data(),bytes.size()},&error),"exact source query profile");const auto g=geometry();
 ColliderInitialFields sphere;sphere.shape58=0;sphere.conditional_complete=true;sphere.sphere.radius88=1;
 ColliderVisibilityHit hit;check(collider_visibility_segment(profile,g,sphere,{-2,0,0},{2,0,0},&hit,&error)&&hit.hit&&!hit.start_inside&&hit.point[0]==-1,"sphere crossing chooses the source root");
 check(collider_visibility_segment(profile,g,sphere,{0,0,0},{2,0,0},&hit,&error)&&hit.hit&&hit.start_inside&&hit.point[0]==1,"sphere initial-inside flag independent of root");
 check(collider_visibility_segment(profile,g,sphere,{-2,1,0},{2,1,0},&hit,&error)&&!hit.hit,"source quadratic tangent uses strict closest-distance fallback");
 check(collider_visibility_segment(profile,g,sphere,{-2,2,0},{2,2,0},&hit,&error)&&!hit.hit,"negative-discriminant source miss");
 ColliderInitialFields box;box.shape58=1;box.conditional_complete=true;
 box.box.normals={{{0,1,0},{0,0,1},{-1,0,0},{0,-1,0},{0,0,-1},{1,0,0}}};
 box.box.anchors={{{0,1,0},{0,0,1},{-1,0,0},{0,-1,0},{0,0,-1},{1,0,0}}};
 check(collider_visibility_segment(profile,g,box,{-2,0,0},{2,0,0},&hit,&error)&&hit.hit&&!hit.start_inside&&hit.box_clip_mask==3&&hit.point[0]==-1,"box ordered plane clipping");
 check(collider_visibility_segment(profile,g,box,{0,0,0},{2,0,0},&hit,&error)&&hit.hit&&hit.start_inside&&hit.box_clip_mask==2&&hit.point[0]==1,"box only-end clipping publishes source flag1");
 check(collider_visibility_segment(profile,g,box,{0,0,0},{.5f,0,0},&hit,&error)&&hit.hit&&!hit.start_inside&&!hit.box_clip_mask&&hit.point[0]==0,"box wholly inside has the source zero flag, not an invented inside classification");
 check(collider_visibility_segment(profile,g,box,{-2,3,0},{2,3,0},&hit,&error)&&!hit.hit,"box same-plane outside miss");
 ColliderInitialFields capsule;capsule.shape58=2;capsule.conditional_complete=true;capsule.capsule.radius88=.5f;
 capsule.capsule.endpoints={{{0,-1,0},{0,1,0}}};capsule.capsule.directions={{{0,2,0},{0,-2,0}}};capsule.capsule.normals={{{0,1,0},{0,-1,0}}};
 check(collider_visibility_segment(profile,g,capsule,{-2,0,.25f},{2,0,.25f},&hit,&error)&&hit.hit&&!hit.start_inside&&!hit.closest_nudged,"capsule nonparallel closest pair");
 check(collider_visibility_segment(profile,g,capsule,{-2,0,0},{2,0,0},&hit,&error)&&!hit.hit,"source closest coincidence nudges ray point X by2 before radius comparison");
 check(!collider_visibility_segment(profile,g,capsule,{0,-3,0},{0,3,0},&hit,&error),"unjoined source parallel closest table refuses, not an invented miss");
 box.box.normals[0][0]=std::numeric_limits<f32>::quiet_NaN();check(!collider_visibility_segment(profile,g,box,{-2,0,0},{2,0,0},&hit,&error),"invalid shape planes cannot fabricate an unobstructed ray");
 std::printf("source visibility segments: %u checks PASS\n",checks);
}
