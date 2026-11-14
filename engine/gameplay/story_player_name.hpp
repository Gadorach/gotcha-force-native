#pragma once
#include <array>
#include "engine/assets/story_player_name.hpp"
#include "engine/gameplay/story_start.hpp"
namespace gf {
struct StoryPlayerName {std::array<u8,kStoryPlayerNamePrefixBytes> bytes{};bool operator==(const StoryPlayerName&)const=default;};
inline bool story_player_name_text(const StoryPlayerName& name,const SourceFont& font,std::string_view* out){
    if(out)*out={};
    if(!out)return false;
    std::size_t count=0;while(count<name.bytes.size()&&name.bytes[count])++count;if(count==name.bytes.size())return false;
    const std::string_view text(reinterpret_cast<const char*>(name.bytes.data()),count);if(!story_player_name_text_valid(text,font))return false;
    *out=text;return true;
}
inline bool story_player_name_decode_prefix_declared(ByteView b,const SourceFont& font,StoryPlayerName* out){
    if(!out||!b.data||b.size!=kStoryPlayerNamePrefixBytes)return false;
    StoryPlayerName next;std::copy_n(b.data,next.bytes.size(),next.bytes.begin());std::string_view text;if(!story_player_name_text(next,font,&text))return false;
    *out=next;return true;
}
inline bool story_player_name_encode_prefix(const StoryPlayerName& name,const SourceFont& font,std::span<u8> out){
    std::string_view text;if(out.size()!=kStoryPlayerNamePrefixBytes||!story_player_name_text(name,font,&text))return false;
    std::copy(name.bytes.begin(),name.bytes.end(),out.begin());return true;
}
//Source strcpy semantics: overwrite through the newNUL, keep other prefix bytes.
//Invalid reads are not repaired/defaulted and cannot authorize a storage write.
inline bool story_player_name_copy_text_plan(const StoryPlayerName& before,std::string_view encoded,
                                            const SourceFont& font,StoryPlayerName* out){
    std::string_view old;if(!out||!story_player_name_text(before,font,&old)||!story_player_name_text_valid(encoded,font))return false;
    auto next=before;std::copy(encoded.begin(),encoded.end(),next.bytes.begin());next.bytes[encoded.size()]=0;*out=next;return true;
}
inline bool story_player_name_default_copy_plan(const StoryPlayerName& before,const StoryPlayerNameProfile& profile,
                                               const SourceFont& font,StoryPlayerName* out){
    return profile.valid_for(font)&&story_player_name_copy_text_plan(before,profile.default_name,font,out);
}
struct StoryPlayerNameBindingPlan {bool request_context0_binding=false;std::string encoded;};
inline bool story_player_name_resume_binding_plan(const StoryStartDispatchPlan& dispatch,const StoryPlayerName& name,
                                                 const SourceFont& font,StoryPlayerNameBindingPlan* out){
    if(!out||!story_start_facts_valid(dispatch.facts)||dispatch.selected_play>=8||unsigned(dispatch.route)>unsigned(StoryStartRoute::SpecialMap)||dispatch.bind_saved_player_name!=(dispatch.route==StoryStartRoute::ResumeMap))return false;
    if(dispatch.bind_saved_player_name&&(!dispatch.facts.resume_valid||dispatch.facts.clear_pending||dispatch.selected_play!=dispatch.facts.saved_play))return false;
    StoryPlayerNameBindingPlan next;
    if(dispatch.bind_saved_player_name){std::string_view text;if(!story_player_name_text(name,font,&text))return false;next.request_context0_binding=true;next.encoded=text;}
    *out=std::move(next);return true;
}
} //namespace gf
