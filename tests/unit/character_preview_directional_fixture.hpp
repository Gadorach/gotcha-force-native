#pragma once
#include "tests/unit/character_preview_controls_fixture.hpp"
namespace character_preview_directional_fixture {
using namespace gf;
struct Fixture:character_preview_controls_fixture::Fixture {
    bool load(std::string* e){if(!character_preview_controls_fixture::Fixture::load(e))return false;
        CharacterProgramAsset p;const auto a=assets();if(!character_program_load_control_flow(a.program,character_program_model_hash(a.model),&p,e))return false;
        auto side=p.descriptors[0];side.choice=4;side.bytes[19]=1;p.descriptors.push_back(side);
        p.programs.push_back({0,6,{13,0,4,1,4,0,0,0,0,0,0,100,0,1,255,255}});
        p.programs.push_back({0,7,{13,0,4,1,4,5,0,0,0,0,0,100,0,1,255,255}});
        p.programs.push_back({0,8,{1,0,1,2,3,0,0,0,0,0,0,100,0,1,255,255}});
        p.programs.push_back({0,9,{13,0,4,1,4,0,0,0,0,8,0,4,0,1,255,255}});
        p.programs.push_back({0,10,{13,0,4,3,4,0,0,0,0,0,0,100,0,1,255,255}});
        program=character_preview_controls_fixture::encode(p);store_be16(sound.data()+40,2);store_be16(sound.data()+52,2);return character_program_detail::valid_control_flow(p);
    }
};
inline bool load(CharacterPreviewFrame& actor,const Fixture& f,CharacterPreviewOutput& out,std::string& e,CharacterMotionRootObservationMode mode=CharacterMotionRootObservationMode::AObjOnly){return actor.load_with_directional_actor_controls(f.assets(),character_preview_height_fixture::preview(),CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements,0,character_preview_height_fixture::height(),f.event_view(),{},&out,&e,mode);}
}
