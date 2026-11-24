#pragma once
#include <optional>
#include "engine/gameplay/challenge_stage_inputs.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
#include "engine/gameplay/scene_light_colours.hpp"
namespace gf {
struct BattleStageStaticBindings {
    SceneLightColours light_colours;std::array<const StihGrid*,3> grids{};std::array<u32,3> byte_extents{};
};
//42394(1)→42324→424CC/08A94/08CA8 native input/descriptor/lighting join.
//Prepared stage resources are not87D2C model instances or a rendered world.
class BattleStageResources {
public:
    BattleStageResources()=default;BattleStageResources(const BattleStageResources&)=delete;BattleStageResources& operator=(const BattleStageResources&)=delete;
    BattleStageResources(BattleStageResources&&)=default;BattleStageResources& operator=(BattleStageResources&&)=default;
    bool initialize(std::shared_ptr<const ChallengeNativeStageData> data,const ChallengeStageInputsProfile& profile,SceneArchivePool& archives,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle stage resources: ")+why;return false;};
        if(attempted_||!data||!profile.ready()||!archives.ready()){return reject("fresh current native stage data/archive domain required");}attempted_=true;
        try{auto next=std::make_unique<Storage>();next->data=data;if(!next->inputs.load(std::move(data),profile,error)||!next->bank.load(next->inputs.model_input(),error,true))return false;
            const auto* lights=next->bank.lighting();if(!lights||!lights->declared||!menu_light_rig_supported(*lights))return reject("stage authored light rig needs an unsupported native consumer");
            next->lights=*lights;if(!next->bank.read_authored_linear_fog(&next->fog,error)||!next->bank.read_initial_light_constructor(&next->initial_lights,error))return false;
            if(archives.acquire(&next->archive,error)!=SceneArchiveAcquire::Acquired||!archives.bind_borrowed_bank(next->archive,next->bank,error))return false;
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native stage decode/bind; no replay");}
    }
    bool current()const{const auto* bank=storage_?storage_->archive.bank():nullptr;return bank&&bank->same_source(storage_->bank);}
    const ChallengeStageInputs* inputs()const{return current()?&storage_->inputs:nullptr;}
    const HsdRecordAnimationBank* bank()const{return current()?&storage_->bank:nullptr;}
    const SceneLighting* lighting()const{return current()?&storage_->lights:nullptr;}
    //Constructor-owned light values/aliases only. No animation/frame-update ACK.
    const HsdInitialLightConstructor* initial_light_constructor()const{return current()?&storage_->initial_lights:nullptr;}
    const LinearFog* fog()const{return current()?&storage_->fog:nullptr;}
    const SceneArchiveRecord* archive_record()const{return current()?storage_->archive.inspect():nullptr;}
    bool bind_initial_state(std::string* error){
        if(!current()||storage_->bindings){if(error)*error="battle stage bindings: current unbound stage required";return false;}
        BattleStageStaticBindings next;if(!initialize_scene_light_colours(storage_->lights,&next.light_colours,error))return false;
        //3D4A0's three file+8 relationships use the existing checked STIH
        //decodes. Keep the exact source byte extents and typed owned-grid links.
        for(unsigned i=0;i<3;++i){const auto raw=storage_->data->slot(0,i+1);next.grids[i]=storage_->inputs.grid(i);if(!next.grids[i]||!next.grids[i]->loaded()||!raw.contains(0,8)||raw.be32(4)!=raw.size){if(error)*error="battle stage bindings: current decoded grid/source extent required";return false;}next.byte_extents[i]=raw.be32(4);}
        storage_->bindings=next;if(error)error->clear();return true;
    }
    const BattleStageStaticBindings* static_bindings()const{return current()&&storage_->bindings?&*storage_->bindings:nullptr;}
    bool model_callbacks_and_scene_frames_pending()const{return true;}
    void retire(){storage_.reset();attempted_=true;} //descriptor cleanup belongs to its scene owner, not an implicit SDK bulk free
 private:friend class BattleStageConstruction;struct Storage {std::shared_ptr<const ChallengeNativeStageData> data;ChallengeStageInputs inputs;HsdRecordAnimationBank bank;SceneArchiveLease archive;SceneLighting lights;HsdInitialLightConstructor initial_lights;LinearFog fog;std::optional<BattleStageStaticBindings> bindings;};std::unique_ptr<Storage> storage_;bool attempted_=false;
};
} // namespace gf
