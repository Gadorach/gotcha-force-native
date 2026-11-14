#pragma once
#include "engine/gameplay/startup_landing_scene.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
#include "tests/unit/startup_static_graphics_fixture.hpp"
namespace startup_landing_fixture {
inline gf::StartupLandingProfile profile(gf::u32 hash){
    using namespace gf;StartupLandingProfile p;p.loaded=true;p.model_hash=hash;p.ground=stage_ground_fixture::parameters();p.rejection_fraction=.5f;p.one=1;p.angle_unit=bits_to_f32(0x38c90fdb);
    for(unsigned i=0;i<3;++i){auto& e=p.entries[i];e.record=i==0?31:i==1?32:5;e.duration=i==0?10:i==1?15:25;e.scale={LinearKey<3>{0,{1,1,1}},LinearKey<3>{s32(e.duration),{1.25f,1.25f,1.25f}}};if(i)e.alpha={LinearKey<1>{0,{1}},LinearKey<1>{s32(e.duration),{0}}};}return p;
}
inline gf::StartupLandingRequest request(gf::u8 kind=2){gf::StartupLandingRequest r;r.kind=kind;r.factor=1;r.height668=80;r.ground_actor.fields_declared=true;return r;}
inline bool grids(std::array<gf::StihGrid,3>* out,std::string* error){
    auto floor=stage_ground_fixture::bytes({stage_ground_fixture::triangle(0)}),empty=stage_ground_fixture::bytes({});
    return out->at(0).load({floor.data(),floor.size()},error)&&out->at(1).load({empty.data(),empty.size()},error)&&out->at(2).load({empty.data(),empty.size()},error);
}
} // namespace startup_landing_fixture
