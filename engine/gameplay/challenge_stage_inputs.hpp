#pragma once
#include "engine/assets/challenge_stage_inputs.hpp"
#include "engine/gameplay/challenge_stage_data.hpp"
#include "engine/gameplay/stage_ground.hpp"
namespace gf {
//Owns the three source static grids and exact model-input extent. Readiness here
//means decoded STATIC INPUTS, not rendered models, collision response, an actor
//world, dynamic side objects or battle start. Shared STIH/ground guards stay intact.
class ChallengeStageInputs {
public:
    bool load(std::shared_ptr<const ChallengeNativeStageData> data,const ChallengeStageInputsProfile& profile,std::string* error){
        *this={};if(!data||data->slots(0)!=4){if(error)*error="Challenge stage inputs: native main slot roles required";return false;}
        const auto& selection=data->selection();const auto* row=profile.find(selection.payloads[0].id);
        if(!row||row->main_hash!=selection.payloads[0].hash||row->stage!=selection.stage){if(error)*error="Challenge stage inputs: native payload/profile identity";return false;}
        const std::array<ByteView,4> slots{data->slot(0,0),data->slot(0,1),data->slot(0,2),data->slot(0,3)};
        if(!load_declared(*row,slots,error))return false;
        data_=std::move(data);return true;
    }
    //Portable declared-input seam for source/owned checks. It cannot mint a
    //native-data receipt; native_data_bound() stays false on this entry.
    bool load_declared(const ChallengeStageInputRow& row,const std::array<ByteView,4>& slots,std::string* error){
        *this={};auto fail=[&](const char* why){if(error)*error=std::string("Challenge stage inputs: ")+why;return false;};
        if(row.stage>127||!stage_ground_parameters_valid(row.ground)||row.model_bytes<32||row.model_bytes>(64u<<20)||!slots[0].data||!slots[0].contains(0,row.model_bytes)||slots[0].be32(0)!=row.model_bytes)return fail("source model extent/ground operands");
        ChallengeStageInputs next;for(unsigned i=0;i<3;++i)if(!next.grids_[i].load(slots[i+1],error))return false;
        next.model_=std::make_shared<const std::vector<u8>>(slots[0].data,slots[0].data+row.model_bytes);next.row_=row;next.ready_=true;*this=std::move(next);if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}bool native_data_bound()const{return ready_&&bool(data_);}
    const StihGrid* grid(unsigned slot)const{return ready_&&slot<3?&grids_[slot]:nullptr;}
    ByteView model_input()const{return ready_&&model_?ByteView{model_->data(),model_->size()}:ByteView{};}
    const StageGroundParameters& parameters()const{return row_.ground;}u32 stage()const{return row_.stage;}
    bool ground(const StageGroundActor& actor,const std::array<f32,3>& position,f32 height,f32* out,std::string* error)const{
        if(!ready_){if(out)*out=0;if(error)*error="Challenge stage inputs: static grids not ready";return false;}
        return stage_ground_height({&grids_[0],&grids_[1],&grids_[2]},row_.ground,actor,position,height,out,error);
    }
private:
    std::shared_ptr<const ChallengeNativeStageData> data_;std::shared_ptr<const std::vector<u8>> model_;std::array<StihGrid,3> grids_;ChallengeStageInputRow row_;bool ready_=false;
};
} // namespace gf
