#pragma once
#include "tests/unit/character_preview_directional_fixture.hpp"
namespace character_preview_actions_fixture {
using namespace gf;
struct Fixture:character_preview_directional_fixture::Fixture {
    bool load(std::string* e){if(!character_preview_directional_fixture::Fixture::load(e))return false;
        CharacterProgramAsset p;const auto a=assets();if(!character_program_load_control_flow(a.program,character_program_model_hash(a.model),&p,e))return false;
        //Owned idle has the real caller distinction: constructor overrides0/0;
        //source reset keeps the authored mode2/duration10 on the SAME actor.
        for(auto& q:p.programs)if(!q.group&&!q.choice){q.bytes[3]=2;q.bytes[4]=10;}
        program=character_preview_controls_fixture::encode(p);return true;
    }
};
inline ForceSpecActorActionProfile profile(){ForceSpecActorActionProfile p;p.loaded=true;p.bank_hash=123;p.models={0x615,10};p.demonstration={1,1};p.cooldown=60;p.lane_mask=15;p.demo_choice=40;p.pitch=0x800;p.duration=p.mode=-1;p.start=-1;return p;}
inline unsigned count(const CharacterPreviewOutput& out,CharacterProgramOperationKind kind){unsigned n=0;for(const auto& op:out.motion)n+=op.kind==kind;return n;}
}
