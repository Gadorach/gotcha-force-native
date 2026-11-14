#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/story_player_name.hpp"
#include "tests/unit/story_start_visual_fixture.hpp"
namespace {unsigned checks=0;void check(bool x){++checks;if(!x)std::abort();}}
int main(){using namespace gf;const auto font=story_start_visual_fixture::font();const std::string a("\x82\x60",2),b("\x82\x61",2);StoryPlayerNameProfile profile;std::vector<u8> p(20);store_be32(p.data(),0x4746504e);store_be16(p.data()+4,1);store_be16(p.data()+6,12);store_be16(p.data()+8,25);store_be16(p.data()+10,4);p[16]=0x82;p[17]=0x60;p[18]=0x82;p[19]=0x61;std::string error;
    check(profile.load({p.data(),p.size()},font,&error)&&profile.default_name==a+b);
    for(unsigned length=0;length<=12;++length){StoryPlayerName name;name.bytes.fill(0xf3);std::string text;
        for(unsigned i=0;i<length;++i)text+=i%2?b:a;
        std::copy(text.begin(),text.end(),name.bytes.begin());name.bytes[text.size()]=0;StoryPlayerName decoded;std::array<u8,25> encoded{};std::string_view view;
        check(story_player_name_decode_prefix_declared({name.bytes.data(),name.bytes.size()},font,&decoded)&&decoded==name);
        check(story_player_name_encode_prefix(decoded,font,encoded)&&encoded==name.bytes);
        check(story_player_name_text(decoded,font,&view)&&view==text);
        for(unsigned next_length=0;next_length<=12;++next_length){std::string next_text;for(unsigned i=0;i<next_length;++i)next_text+=b;auto expected=name;std::copy(next_text.begin(),next_text.end(),expected.bytes.begin());expected.bytes[next_text.size()]=0;auto copy=name;
            check(story_player_name_copy_text_plan(copy,next_text,font,&copy)&&copy==expected);
        }
        StoryPlayerName next;check(story_player_name_default_copy_plan(name,profile,font,&next));check(next.bytes[0]==0x82&&next.bytes[1]==0x60&&next.bytes[2]==0x82&&next.bytes[3]==0x61&&next.bytes[4]==0);
        for(unsigned i=5;i<25;++i)check(next.bytes[i]==name.bytes[i]);
    }
    StoryPlayerName malformed;malformed.bytes.fill(255);StoryPlayerName unchanged;unchanged.bytes[0]=0;unchanged.bytes[20]=77;const auto old=unchanged;
    check(!story_player_name_decode_prefix_declared({malformed.bytes.data(),25},font,&unchanged)&&unchanged==old);
    check(!story_player_name_default_copy_plan(malformed,profile,font,&unchanged)&&unchanged==old);
    malformed.bytes[0]=0x82;malformed.bytes[1]=0;check(!story_player_name_decode_prefix_declared({malformed.bytes.data(),25},font,&unchanged)&&unchanged==old);
    malformed.bytes[0]=0x82;malformed.bytes[1]=0x62;malformed.bytes[2]=0;check(!story_player_name_decode_prefix_declared({malformed.bytes.data(),25},font,&unchanged)&&unchanged==old);
    check(!story_player_name_copy_text_plan(old,std::string(26,'x'),font,&unchanged)&&unchanged==old);
    check(!story_player_name_decode_prefix_declared({old.bytes.data(),24},font,&unchanged)&&unchanged==old);
    check(!story_player_name_decode_prefix_declared({old.bytes.data(),32},font,&unchanged)&&unchanged==old);
    check(!story_player_name_copy_text_plan(old,std::string("\x82",1),font,&unchanged)&&unchanged==old);
    for(unsigned action=0;action<3;++action)for(unsigned resume=0;resume<2;++resume)for(unsigned clear=0;clear<2;++clear){
        StoryStartState state;state.page=StoryStartPage::DispatchRequested;state.action=StoryStartAction(action);state.facts={3,2,bool(resume),bool(clear)};state.selected_play=1;StoryStartDispatchPlan dispatch;
        check(story_start_dispatch_plan(state,&dispatch));const bool needs=action==0&&resume&&!clear;check(dispatch.bind_saved_player_name==needs);
        StoryPlayerNameBindingPlan plan;check(story_player_name_resume_binding_plan(dispatch,old,font,&plan)&&plan.request_context0_binding==needs&&plan.encoded.empty());
    }
    auto invalid_profile=p;store_be16(invalid_profile.data()+6,20);check(!profile.load({invalid_profile.data(),invalid_profile.size()},font,&error)&&!profile.loaded); //force maximum cannot silently stand in for Story
    StoryStartDispatchPlan broken;broken.bind_saved_player_name=true;StoryPlayerNameBindingPlan binding;check(!story_player_name_resume_binding_plan(broken,old,font,&binding));
    check(!story_player_name_decode_prefix_declared({nullptr,25},font,&unchanged));check(!story_player_name_copy_text_plan(old,a,font,nullptr));
    std::printf("Story saved player-name value/binding: %u owned checks PASS; not a map transform/keyboard/full resume ACK\n",checks);
}
