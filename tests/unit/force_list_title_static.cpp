#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/force_list_title.hpp"
int main(){using namespace gf;HsdMesh mesh;mesh.vertices.resize(4);mesh.indices={0,1,2,1,3,2};for(unsigned i=0;i<4;++i){auto& v=mesh.vertices[i];v.position[0]=(i&1)?10.f:-10.f;v.position[1]=(i&2)?-5.f:5.f;v.uv[0]=(i&1)?1.f:0;v.uv[1]=(i&2)?1.f:0;}HsdTexture texture;texture.width=8;texture.height=4;texture.rgba.resize(8*4*4);for(unsigned y=1;y<3;++y)for(unsigned x=1;x<3;++x)for(unsigned c=0;c<4;++c)texture.rgba[(y*8+x)*4+c]=255;
    unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)std::abort();};HsdAnimPose::Material material;ForceListTitleSlot slot;check(force_list_title_slot(mesh,texture,material,&slot)&&slot.x==-5&&slot.height==5);material.present=true;check(force_list_title_slot(mesh,texture,material,&slot)&&slot.x==-5);material.uv_scale[0]=std::numeric_limits<f32>::quiet_NaN();check(force_list_title_slot(mesh,texture,material,&slot)&&slot.x==-5);material.has_texture=true;check(!force_list_title_slot(mesh,texture,material,&slot)&&slot.height==0);material.uv_scale[0]=1;check(force_list_title_slot(mesh,texture,material,&slot)&&slot.x==-5);material.uv_offset[0]=-1;check(!force_list_title_slot(mesh,texture,material,&slot));std::printf("list-title static-vs-animated UV: %u new checks PASS\n",checks);return 0;
}
