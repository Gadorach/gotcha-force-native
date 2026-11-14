#pragma once
#include "engine/gameplay/startup_actor_frame.hpp"
#include "tests/unit/character_motion_driver_fixture.hpp"
#include "tests/unit/startup_actor_bookkeeping_fixture.hpp"
#include "tests/unit/actor_fade_fixture.hpp"
namespace startup_actor_frame_fixture {
struct Fixture {
    character_motion_driver_fixture::Fixture motion;
    std::vector<gf::u8> prefix=character_model_fixture::prefix(),pose=character_model_fixture::profile(),program,setup,runtime,root=std::vector<gf::u8>(52),fade=actor_fade_fixture::profile();
    std::array<gf::u8,24> classes=startup_actor_class_fixture::profile();
    bool load(std::string* error){
        using namespace gf;if(!motion.load(error))return false;const auto hash=motion.profile.model_hash;program=character_program_fixture::bytes();store_be32(program.data()+8,hash);setup=startup_actor_setup_fixture::profile();store_be32(setup.data()+8,hash);store_be16(setup.data()+48+8,2000);
        pose[8]=3;pose[9]=1;pose[10]=pose[11]=2;
        setup[480+2]=motion.rig.lane_mask;for(unsigned i=0;i<3;++i)setup[480+12+i]=motion.rig.cuts[i+1];for(unsigned i=0;i<motion.rig.order.size();++i){setup[480+0x8c+i]=motion.rig.order[i];setup[480+0x6c+i]=motion.rig.mirror[i];}
        runtime=startup_actor_bookkeeping_fixture::runtime();store_be32(runtime.data()+8,hash);store_be32(root.data(),0x47465253);store_be16(root.data()+4,1);store_be16(root.data()+6,9);store_be32(root.data()+8,52);
        const std::array<f32,9> values={1,10,1,16384,bits_to_f32(0x3f666666),bits_to_f32(0x3dcccccd),5,4,bits_to_f32(0x38c90fdb)};for(unsigned i=0;i<values.size();++i)store_be_f32(root.data()+16+i*4,values[i]);return true;
    }
    gf::StartupActorFrameAssets assets()const{return {{motion.model.data(),motion.model.size()},{prefix.data(),prefix.size()},{pose.data(),pose.size()},{program.data(),program.size()},{setup.data(),setup.size()},{runtime.data(),runtime.size()},{classes.data(),classes.size()},{root.data(),root.size()},{fade.data(),fade.size()},motion.clips};}
};
} // namespace startup_actor_frame_fixture
