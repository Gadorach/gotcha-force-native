#pragma once
#include "engine/gameplay/startup_thruster_scene.hpp"
#include "tests/unit/startup_actor_frame_fixture.hpp"
#include "tests/unit/startup_auxiliary_fixture.hpp"
namespace startup_thruster_fixture {
inline gf::StartupAuxiliaryTurnProfile turning(){
    using namespace gf;StartupAuxiliaryTurnProfile p;p.loaded=true;
    p.movement_threshold=bits_to_f32(0x3c23d70a);p.pi=bits_to_f32(0x40490fdb);p.binary_divisor=32768;
    p.history_step=bits_to_f32(0x3d0efa35);p.orientation_factor=bits_to_f32(0x3c8efa35);p.cross_epsilon_sq=bits_to_f32(0x2edbe6fe);
    p.cos_min=-1;p.cos_max=1;p.fallback={0,1,0};return p;
}
struct Fixture {
    startup_actor_frame_fixture::Fixture actor;
    std::vector<gf::u8> model=startup_auxiliary_fixture::model();gf::StartupAuxiliaryProfile profile;
    gf::StartupAuxiliaryTurnProfile turn=turning();
    bool load(std::string* error,gf::u8 channels=5){
        using namespace gf;if(!actor.load(error))return false;
        // Owned two-bone actor: select logical bone1, not actual G-Red bone3.
        auto raw=startup_auxiliary_fixture::profile(model);for(unsigned e=0;e<2;++e)for(unsigned n=0;n<2;++n)raw[24+e*52+4+n*24+1]=1;
        if(!startup_auxiliary_load({raw.data(),raw.size()},&profile,error))return false;
        const std::vector<u8> commands={11,1,channels,0,1,5,0,0,0,0,0,0,0,0,0,2,11,0,channels,0,0,1,255,255};
        actor.program.resize(92);actor.program.insert(actor.program.end(),commands.begin(),commands.end());
        store_be32(actor.program.data()+40,commands.size());store_be32(actor.program.data()+16,actor.program.size());return true;
    }
    bool bind(gf::StartupActorFrame& a,gf::StartupThrusterScene& jets,std::string* error,gf::u8 mode=0,std::array<bool,2> available={true,true})const{
        gf::StartupActorFrameOutput out;
        return a.load(actor.assets(),gf::StartupActorFrameMode::IsolatedRequirements,0,&out,error)&&jets.bind(a,{model.data(),model.size()},profile,turn,gf::StartupThrusterMode::DeclaredIsolatedConstructionAndPlacement,mode,available,error);
    }
};
} // namespace startup_thruster_fixture
