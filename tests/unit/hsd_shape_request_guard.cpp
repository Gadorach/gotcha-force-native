#include <cstdio>
#include <cstdlib>
#include "engine/assets/hsd_shape_kernel.hpp"
#include "tests/unit/hsd_shape_fixture.hpp"
int main(){
    using namespace gf;const auto fixture=hsd_shape_fixture::fixture();hsd_shape_detail::Mesh mesh;mesh.end=4;mesh.shapes={{{{1,2,3}}}};HsdVertex vertex;vertex.source_position_index=0;mesh.original.vertices={vertex};mesh.track.data.assign(fixture.block.begin()+0xf40,fixture.block.begin()+0xf40+18);unsigned checks=0;
    for(bool fractional:{false,true})for(f64 time:{-1.,double(NAN),double(INFINITY),1e100,4.125}){std::vector<HsdVertex> out(1);out[0].position[0]=99;f32 blend=99;++checks;if(hsd_shape_detail::sample_impl(mesh,time,fractional,&out,&blend,nullptr)||out[0].position[0]!=99||blend!=99)std::abort();}
    std::printf("shape request guard: %u owned invalid-clock checks PASS\n",checks);
}
