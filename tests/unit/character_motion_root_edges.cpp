#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/character_motion_root.hpp"
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"root edges: %s\n",why);std::abort();}}}
int main(){using namespace gf;std::string error;for(int side:{-1,0,1}){character_motion_directional_fixture::Fixture f;check(f.load(&error),"fixture");auto& root=f.clips[0].clip.bones[0];auto x=character_motion_driver_fixture::linear(0,side*4e-30f,4),z=character_motion_driver_fixture::linear(0,-4,4);z.channel=7;root.tracks={x,z};CharacterMotionDriver driver;check(f.directional(driver,&error),"native driver");CharacterProgramMotion a,b;const auto& d=f.profile.descriptors[0];check(driver.select(0,d,false,{},0,1,&a)&&driver.advance(0,a,1,&b),"negative-Z actual root step");CharacterMotionRootOwner owner;CharacterMotionRootState out;CharacterMotionRootFields fields;fields.delta5f4=1;
    check(owner.bind(driver,a,{0,0,0},&error)&&owner.accept(b,fields,&out,&error)&&out.heading1db0==(side<0?-32768:32767),"signed16 heading saturates positive pi and retains negative pi");
    fields.apply_position1d9b=1;fields.scale_b4=std::numeric_limits<f32>::infinity();check(owner.bind(driver,a,{0,0,0},&error)&&!owner.accept(b,fields,&out,&error)&&!owner.ready()&&out.updates==0,"consumed nonfinite actor scale refuses atomically");
    fields.scale_b4=-1e7f;check(owner.bind(driver,a,{0,0,1e7f},&error)&&!owner.accept(b,fields,&out,&error)&&out.position==std::array<f32,3>{},"derived position budget failure discards candidate output");
    CharacterProgramMotion requested;check(driver.select(0,d,false,a,0,1,&requested)&&owner.bind(driver,a,{0,0,0},&error)&&owner.accept(requested,fields,&out,&error)&&out.position==std::array<f32,3>{},"request branch does not consume invalid integration fields");
}
    std::printf("ordinary root heading/scalar/rollback edges: %u checks PASS\n",checks);return 0;
}
