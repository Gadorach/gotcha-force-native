#pragma once
#include <optional>
#include "engine/gameplay/force_popup_scene.hpp"
namespace gf {
// Source E6768/E6B9C lifetimes: open requests1..25, close begins at8 then
// decreases to0. Values advance on explicit UI callbacks, never draw calls.
class ForcePopupTimeline {
public:
    bool step(const std::optional<ForcePopupPresentation>& desired,u32 context){
        if(desired){static constexpr unsigned counts[]={6,7,4,2,1,21,2};const auto kind=unsigned(desired->kind);if(kind>6)return false;const auto count=kind==5&&!desired->labels_ascii.empty()?desired->labels_ascii.size():counts[kind];if(!count||count>32||desired->selected>=count||desired->first>desired->selected||desired->selected>=desired->first+8||(count<=8&&desired->first))return false;if(!desired->labels_ascii.empty()){if(kind==6||(kind!=5&&desired->labels_ascii.size()!=count))return false;for(const auto& label:desired->labels_ascii)if(label.empty()||label.size()>32)return false;}}
        auto next=entries_;
        for(auto it=next.begin();it!=next.end();){if(it->context!=context){it=next.erase(it);continue;}if(it->closing){if(it->value.frame)--it->value.frame;else{it=next.erase(it);continue;}++it->value.age;}++it;}
        auto active=std::find_if(next.begin(),next.end(),[](const Entry& e){return !e.closing;});
        const bool same=active!=next.end()&&desired&&active->context==context&&active->value.kind==desired->kind&&active->value.components==desired->components;
        if(!same&&active!=next.end()){active->closing=true;active->value.frame=8;++active->value.age;}
        if(desired){
            if(same){const auto frame=std::min(active->value.frame+1,u32(25)),age=active->value.age+1;active->value=*desired;active->value.frame=frame;active->value.age=age;}
            else{if(next.size()>=4)return false;Entry e;e.value=*desired;e.value.frame=1;e.value.age=1;e.context=context;next.push_back(e);}
        }
        entries_=std::move(next);return true;
    }
    std::vector<ForcePopupPresentation> visible()const{std::vector<ForcePopupPresentation> out;for(const auto& e:entries_)out.push_back(e.value);return out;}
    void clear(){entries_.clear();}
private:
    struct Entry {ForcePopupPresentation value;u32 context=0;bool closing=false;};
    std::vector<Entry> entries_;
};
} // namespace gf
