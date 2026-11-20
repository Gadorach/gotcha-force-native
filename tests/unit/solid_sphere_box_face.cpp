#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/solid_sphere_box_face.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;if(!ok){std::fprintf(stderr,"sphere-box face: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using V=std::array<f32,3>;
    // The six authored face slots, not xyz-index order.
    const std::array<SolidBoxFacePlane,6> box{{
        {{0,1,0},{2,2,0}},{{0,0,1},{2,2,2}},{{-1,0,0},{0,2,0}},
        {{0,-1,0},{2,0,0}},{{0,0,-1},{0,0,0}},{{1,0,0},{2,0,2}},
    }};
    SolidSphereBoxFaceInput input{{2.5f,1,1},{3,1,1},1,box,box,true};
    SolidSphereBoxFaceResult out;std::string error;
    auto solve=[&](){return solid_sphere_box_face(input,&out,&error);};
    check(solve()&&out==SolidSphereBoxFaceResult{true,5,{1,0,0},.5f,1.f,-.5f},"single east face proposes signed separation");
    input.center={3,1,1};input.previous_center={4,1,1};
    check(solve()&&out.contact&&out.clearance==1.f&&out.signed_separation==0.f,"inclusive radius equality still reports the face");
    input.center={2.5f,1,1};input.previous_center={2,1,1};
    check(solve()&&out.contact&&!out.previous_clearance,"previous-plane equality counts as outside");
    input.previous_center={1.75f,1,1};
    check(solve()&&!out.contact,"motion leaving a previously interior center is not a face entry");
    input.center={2.5f,4,1};input.previous_center={3,4,1};
    check(solve()&&!out.contact,"an adjacent plane outside the radius excludes the face");
    input.center={1.75f,1,1};input.previous_center={3,1,1};
    check(solve()&&out.contact&&out.clearance==-.25f&&out.signed_separation==-1.25f,"new center may be inside after a one-face entry");
    input.center={2.5f,1,1};input.previous_center={2.75f,1,1};
    for(auto& plane:input.previous_faces)plane.anchor[0]+=1;
    check(solve()&&!out.contact,"use the previous box plane, not the current plane with the old center");
    input.previous_faces=box;
    input.center={-0.5f,1,1};input.previous_center={-1,1,1};
    check(solve()&&out.contact&&out.face==2&&out.normal==V{-1,0,0},"opposite face retains its own normal and index");
    const auto retained=out;
    input.center={2.5f,2.5f,1};input.previous_center={3,3,1};
    check(!solve()&&out==retained,"two face candidates require the edge response");
    input.center={2.5f,2.5f,2.5f};input.previous_center={3,3,3};
    check(!solve()&&out==retained,"corner continuation does not publish a miss");
    input.center={1,1,1};input.previous_center={1,1,1};
    check(!solve()&&out==retained,"inside-to-inside fallback is not a face entry or miss");
    input.center={2.5f,1,1};input.previous_center={3,1,1};input.declared=false;
    check(!solve()&&out==retained,"caller must declare both plane snapshots");
    input.declared=true;input.previous_faces[2].normal={0,0,0};
    check(!solve()&&out==retained,"a missing previous plane cannot stand in for a miss");
    std::printf("sphere-box one-face proposals and unsupported continuations: %u checks PASS\n",checks);
}
