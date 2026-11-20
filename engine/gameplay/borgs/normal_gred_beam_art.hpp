#pragma once
#include "engine/assets/character_program.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/gameplay/borgs/normal_gred_neutral_b.hpp"

namespace gf {
// Prepared, ordinary common effect bank only, not a scene lease. C3D2C
// selects common +48; 726C selects record 0 and requests material clip 0
// at time 0 for the normal G-Red team-0 scope.
class NormalGRedBeamArtBank {
public:
    bool load(ByteView bytes,std::string* error){
        *this={};
        if(!bytes.data||bytes.size!=417955||character_program_model_hash(bytes)!=0x5fec9e6cu){
            if(error)*error="G-Red beam art: exact ordinary common effect bank required";
            return false;
        }
        return bank_.load(bytes,error,true);
    }
    bool ready()const{return bank_.records()!=0;}
    bool prepare(NormalGRedNeutralBSource source,u8 team,AnimatedModelScene* out,std::string* error)const{
        if(!out||!ready()||!source.reviewed()||team){
            if(error)*error="G-Red beam art: exact normal team-0 request required";
            return false;
        }
        AnimatedModelScene next;
        if(!next.load_compact_textures(bank_,0,2,0,0,error)||!next.request_selectors(0,0,error))return false;
        // The holder must separately supply a matrix and enable drawing. No
        // carrier, shape, collision or automatic animation tick is created.
        *out=std::move(next);return true;
    }
private:
    HsdRecordAnimationBank bank_;
};
}
