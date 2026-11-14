#include <cstdio>
#include <cstdlib>
#include "engine/graphics/transform.hpp"
#include "engine/graphics/lighting.hpp"
namespace {void check(bool ok){if(!ok)std::abort();}}
int main(){
 using namespace gf;Mat4 m;m.at(0,0)=2;m.at(1,1)=3;m.at(2,2)=4;const f32 in[3]={1,1,1};f32 out[3];mat4_transform_normal(mat4_normal_transform(m),in,out);const f32 length=std::sqrt(.25f+1.f/9+.0625f);check(std::abs(out[0]-.5f/length)<1e-6f&&std::abs(out[1]-(1.f/3)/length)<1e-6f&&std::abs(out[2]-.25f/length)<1e-6f);
 m.at(0,0)=-2;mat4_transform_normal(mat4_normal_transform(m),in,out);check(out[0]<0&&out[1]>0&&out[2]>0);
 for(auto& x:m.m){x=0;}mat4_transform_normal(mat4_normal_transform(m),in,out);check(out[0]==0&&out[1]==0&&out[2]==0);
 SceneLighting rig;rig.declared=true;SceneLight ambient;ambient.flags=4;for(auto& x:ambient.colour)x=100;SceneLight sun;sun.flags=13;sun.position[2]=1;for(auto& x:sun.colour)x=150;rig.lights={ambient,sun};const u8 material[4]={128,128,128,255},specular[4]={255,255,255,255};const f32 origin[3]={},eye[3]={0,0,10},normal[3]={0,0,1},away[3]={0,0,-1};lighting_detail::PreparedLightView view;view.prepare(rig,origin,eye);
 auto lit=light_vertex_prepared(rig,origin,normal,origin,eye,material,specular,50,true,true,&view);check(lit.diffuse[0]==200&&lit.specular[0]==150&&lit.diffuse[3]==255);
 lit=light_vertex_prepared(rig,origin,away,origin,eye,material,specular,50,true,true,&view);check(lit.diffuse[0]==50&&lit.specular[0]==0);
 rig.lights[1].colour[0]=30;view.prepare(rig,origin,eye);lit=light_vertex_prepared(rig,origin,normal,origin,eye,material,specular,50,true,true,&view);check(lit.diffuse[0]==80&&lit.specular[0]==30);
 std::puts("prepared vertex math: nonuniform/reflected/singular normals and analytic diffuse/specular + rig refresh PASS");
}
