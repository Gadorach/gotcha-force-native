#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/collider_sphere_shape.hpp"
using namespace gf;
int main(){
    auto check=[](bool ok){if(!ok)std::abort();};
    CharacterCollisionDescriptor d;
    d.declared=true;d.local_point={2,3,4};
    store_be_f32(d.source.data()+0x18,20);
    const Mat4 frame{{2,0,0,50,0,4,0,20,0,0,10,30,0,0,0,1}};
    ColliderSphereSeed out;
    for(u16 flags:{u16(0),u16(0x10),u16(1),u16(0x80),u16(0x91)}){
        d.flags2=flags;
        check(collider_sphere_seed(d,frame,.5f,&out));
        check(out.center64==std::array<f32,3>{54,32,70}&&out.previous_center70==out.center64&&
              out.radius88==10&&out.broad_radius7c==(flags&0x10?25:10));
        if(flags&0x81){
            const auto& c=out.center64;
            check(!out.previous_radius8c&&out.sweep==ColliderSphereSweepSeed{c,c,c,c});
        }else check(!out.sweep&&out.previous_radius8c==10);
    }
    const auto before=out;
    check(!collider_sphere_seed(d,frame,-1,&out)&&out==before);
    d.shape=4;
    check(!collider_sphere_seed(d,frame,1,&out)&&out==before);
    std::puts("fresh sphere: transformed center, physical radius and exclusive 8C histories PASS");
}
