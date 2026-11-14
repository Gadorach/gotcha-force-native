#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/animated_shape_model_scene.hpp"
#include "tests/unit/hsd_shape_material_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"fractional shape: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;auto fixture=hsd_shape_material_fixture::fixture();const auto data=fixture.archive();HsdRecordAnimationBank bank;HsdShapeMaterialRecord record;AnimatedShapeModelScene scene;std::string error;
    check(bank.load({data.data(),data.size()},&error)&&record.load(bank,0,&error)&&scene.load(bank,0,&error),"owned shape/material binding");
    for(unsigned i=0;i<=32;++i){const f32 time=i*.125f;std::vector<std::vector<HsdVertex>> vertices;std::vector<f32> blends;check(record.sample_shapes_fractional(time,&vertices,&blends,&error)&&blends[0]==time*.25f&&vertices[0][0].position[0]==-1.f-time*.25f,"eighth-frame request is not truncated");check(scene.sample_fractional(time,0,Mat4{},&error)&&scene.pose(),"fractional shape through shared pose consumer");if(i%8==0){std::vector<std::vector<HsdVertex>> legacy;check(record.sample_shapes(i/8,&legacy,nullptr,&error)&&legacy[0][0].position[0]==vertices[0][0].position[0],"integer and fractional shape request agree at integral time");}}
    check(!scene.sample_fractional(-.125f,0,Mat4{},&error)&&!scene.pose(),"negative request refuses without replay");check(!scene.sample_fractional(NAN,0,Mat4{},&error)&&!scene.pose(),"nonfinite request refuses");check(!scene.sample_fractional(4.125f,0,Mat4{},&error)&&!scene.pose(),"beyond clip endpoint refuses");
    const HsdAnimTrackView view{{fixture.block.data()+0xf40,18},0,0,0};f32 value=99;check(!hsd_anim_sample_requested_track_fractional(view,-1,&value,&error)&&value==99,"fractional track guard preserves caller output");check(hsd_anim_sample_requested_track(view,2,&value,&error)&&value==.5f,"old requested-track entry retained");
    std::printf("fractional shape: %u owned checks PASS\n",checks);
}
