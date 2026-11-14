#pragma once
#include "engine/assets/story_force_cards.hpp"
namespace gf {
struct StoryForceWidgetPart {u16 record=0;std::array<f32,3> position{};};
struct StoryForceFooter {std::array<s16,4> records{};f32 x=0;};
class StoryForceWidgetsArt {
public:
    enum Scalar {BackgroundMaterial,BackgroundPeriod,TitleX,TitleY,TitleZ,TitleXEarly,TitleXLate,TitleYEarly,TitleYLate,FooterY,FooterZ,FooterIconX,FooterIconScale,FooterScale,NameX,NameY,NameZ,TextX,TextY,TextZ,TextExtent,TextAdvance,CostX,CostY,CostZ,DigitAdvance,ArrowX,ArrowY,ArrowZ,AngleUnit,ScalarCount};
    bool load(ByteView b,const StoryForceCardArt& card,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="Story force widgets: profile/bank/domain";return false;};
        if(!card.ready()||!b.data||b.size!=280||b.be32(0)!=0x47465357||b.be32(4)!=1||b.be32(8)!=card.bank_hash())return fail();
        std::size_t at=12;for(auto& row:footers_){for(auto& record:row.records){record=std::bit_cast<s16>(b.be16(at));at+=2;if(record<-1||record>=4096)return fail();}row.x=b.bef32(at);at+=4;if(!std::isfinite(row.x)||std::abs(row.x)>1e6f)return fail();}
        auto part=[&](StoryForceWidgetPart& p){p.record=b.be16(at);if(p.record>=4096||b.be16(at+2))return false;at+=4;for(auto& v:p.position){v=b.bef32(at);at+=4;if(!std::isfinite(v)||std::abs(v)>1e6f)return false;}return true;};
        for(auto& p:name_)if(!part(p))return fail();
        for(auto& p:cost_)if(!part(p))return fail();
        for(auto& v:scalars_){v=b.bef32(at);at+=4;if(!std::isfinite(v)||std::abs(v)>1e6f)return fail();}
        high_=b.be32(at);low_=b.be32(at+4);cost_angle_=std::bit_cast<s16>(b.be16(at+8));background_=b.be16(at+10);title_=b.be16(at+12);arrows_=b.be16(at+14);
        if(background_>=4096||title_>=4096||arrows_>=4096||scalars_[BackgroundPeriod]!=120||scalars_[BackgroundMaterial]!=3||scalars_[AngleUnit]!=card.angle_unit()||scalars_[TextAdvance]<=0||scalars_[TextExtent]<=0||scalars_[DigitAdvance]<=0||scalars_[FooterIconScale]<=0||scalars_[FooterScale]<=0)return fail();
        tag_=2166136261;for(std::size_t i=0;i<b.size;++i)tag_=(tag_^b.data[i])*16777619u;bank_hash_=card.bank_hash();card_tag_=card.tag();ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u32 bank_hash()const{return bank_hash_;}u32 tag()const{return tag_;}u32 card_tag()const{return card_tag_;}
    f32 value(Scalar s)const{return scalars_[unsigned(s)];}u16 background()const{return background_;}u16 title()const{return title_;}u16 arrows()const{return arrows_;}s16 cost_angle()const{return cost_angle_;}
    const auto& footers()const{return footers_;}const auto& name_parts()const{return name_;}const auto& cost_parts()const{return cost_;}u32 low()const{return low_;}u32 high()const{return high_;}
private:
    std::array<StoryForceFooter,3> footers_;std::array<StoryForceWidgetPart,4> name_;std::array<StoryForceWidgetPart,2> cost_;std::array<f32,ScalarCount> scalars_{};
    u32 bank_hash_=0,tag_=0,card_tag_=0,low_=0,high_=0;u16 background_=0,title_=0,arrows_=0;s16 cost_angle_=0;bool ready_=false;
};
} //namespace gf
