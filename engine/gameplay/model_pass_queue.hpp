#pragma once
// Source097B4/09838 six model-wrapper queues. IDs are native owned identities;
// material-pass selection and deferred sorting use the shared model renderer.
#include <vector>
#include "engine/assets/startup_scene_pass.hpp"
#include "engine/assets/model_pass_queue.hpp"
namespace gf {
struct ModelPassVisit {u32 model=0;u8 queue=0,pass=0;};
class ModelPassQueue {
public:
    bool load(const StartupScenePassProfile& p){if(source_mode_)return false;attempted_=true;ready_=false;frame_=0;items_.clear();if(!startup_scene_pass_valid(p))return false;profile_={p.translucent_groups,p.passes,true};ready_=true;return true;}
    //Separate fresh global-operand entry. It cannot import or reset a populated
    //queue, and does not assert an active camera or a caller's native frame.
    bool load_global(const ModelPassQueueProfile& p){if(attempted_||!model_pass_queue_profile_valid(p))return false;attempted_=source_mode_=ready_=true;profile_=p;return true;}
    bool begin(u32 frame){if(!ready_||frame!=frame_+1||frame>8192){ready_=false;return false;}frame_=frame;items_.clear();for(auto& h:heads_)h.clear();return true;}
    bool register_model(u32 model,u8 mask,bool active){
        if(!ready_||!frame_||!model||items_.size()>=4096||(mask&0x80)){ready_=false;return false;}
        for(const auto& item:items_)if(item.model==model){ready_=false;return false;}
        s32 group=0;if(mask&0x7c){group=profile_.translucent_groups[mask>>4];if(group<0){ready_=false;return false;}}
        std::array<u32,3> next{};auto head=[&](unsigned q){return heads_[q].empty()?0u:items_[heads_[q].back()].model;};
        if(mask&1){next[0]=head(0);}if(mask&2){next[1]=head(1);}if(mask&0x7c){next[2]=head(unsigned(group)+2);}
        const auto index=items_.size();items_.push_back({model,active,next});if(mask&1)heads_[0].push_back(index);if(mask&2)heads_[1].push_back(index);if(mask&0x7c)heads_[unsigned(group)+2].push_back(index);return true;
    }
    bool plan(std::vector<ModelPassVisit>* out)const{
        if(out)out->clear();
        if(!out||!ready_||!frame_)return false;
        for(unsigned q=0;q<6;++q)for(auto i=heads_[q].rbegin();i!=heads_[q].rend();++i)if(items_[*i].active)out->push_back({items_[*i].model,u8(q),profile_.passes[q]});
        return true;
    }
    bool ready()const{return ready_;}
    bool source_links(u32 model,std::array<u32,3>* out)const{if(out)*out={};if(!out||!ready_||!frame_)return false;for(const auto& item:items_)if(item.model==model){*out=item.next;return true;}return false;}
private:
    struct Item {u32 model;bool active;std::array<u32,3> next{};};ModelPassQueueProfile profile_;std::vector<Item> items_;std::array<std::vector<std::size_t>,6> heads_;u32 frame_=0;bool ready_=false,attempted_=false,source_mode_=false;
};
} // namespace gf
