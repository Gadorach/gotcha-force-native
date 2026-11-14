#pragma once
#include "engine/assets/story_force_widgets.hpp"
#include "engine/gameplay/force_inventory.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct StoryForceWidgetDraw {
    u16 record=0;u8 type=0,part=0,mask=0x44;Mat4 world;f32 joint_time=0,material_time=0;bool authored_world=false,text=false;
};
//The source widget callbacks own these clocks; draw never advances them.
//A prepared visual owner is not an input, resource, fade or campaign receipt.
class StoryForceWidgets {
public:
    bool open(const StoryForceWidgetsArt& art,const ForceInventory& owner,unsigned force,u8 interactive=0,u8 closing=0){
        *this={};if(!art.ready()||!owner.ready())return false;art_=art;owner_=owner.instance_binding();revision_=owner.revision();ready_=true;
        //F8C28/F8D4C initialize then invoke their update once. Footer constructor
        //does NOT increment its blink counter. Arrows invoke their own callback.
        background_=1;title_counter_=1;arrow_counter_=31;arrow_step(owner,interactive,closing);
        if(!publish(owner,force,interactive,closing))return fail();
        return true;
    }
    bool step(const ForceInventory& owner,unsigned force,u8 interactive,u8 closing){
        if(!current(owner)||updates_>=8192)return fail();
        auto next=*this;
        next.background_=f32(next.background_+1);if(next.background_>=art_.value(StoryForceWidgetsArt::BackgroundPeriod))next.background_=f32(next.background_-art_.value(StoryForceWidgetsArt::BackgroundPeriod));
        next.title_counter_=u16(next.title_counter_+1);while(next.title_counter_>=128)next.title_counter_=u16(next.title_counter_-64);
        next.footer_counter_=u16(next.footer_counter_+1);next.arrow_step(owner,interactive,closing);
        if(!next.publish(owner,force,interactive,closing))return fail();
        ++next.updates_;*this=std::move(next);return true;
    }
    //Explicit graphics-only editor-return adoption, not an editor success flag.
    //The genuine controller/editor bridge remains the authority for its return.
    bool rebind_inventory(const ForceInventory& owner){const auto binding=owner_.lock();if(!ready_||!binding||!owner.ready()||binding!=owner.instance_binding().lock())return fail();revision_=owner.revision();draws_.clear();name_.clear();return true;}
    bool current(const ForceInventory& owner)const{const auto binding=owner_.lock();return ready_&&binding&&owner.ready()&&binding==owner.instance_binding().lock()&&revision_==owner.revision();}
    bool ready()const{return ready_;}unsigned force()const{return force_;}u32 updates()const{return updates_;}u32 art_tag()const{return art_.tag();}
    f32 background_time()const{return background_;}u16 title_counter()const{return title_counter_;}u16 footer_counter()const{return footer_counter_;}u16 arrow_counter()const{return arrow_counter_;}u16 arrow_mask()const{return arrow_mask_;}
    std::span<const StoryForceWidgetDraw> draws()const{return draws_;}const std::string& name()const{return name_;}
private:
    void arrow_step(const ForceInventory& owner,u8 interactive,u8 closing){if(((interactive|closing)&4)&&owner.state()->force_count>1){arrow_mask_=interactive&4?32:2;arrow_counter_=u16(arrow_counter_+1);}else{arrow_counter_=31;arrow_mask_=0;}}
    void part(u16 record,u8 type,u8 index,const Mat4& world,f32 selector=0,u8 mask=0x44,bool authored=false){StoryForceWidgetDraw d;d.record=record;d.type=type;d.part=index;d.world=world;d.material_time=selector;d.mask=mask;d.authored_world=authored;draws_.push_back(d);}
    bool publish(const ForceInventory& owner,unsigned force,u8 interactive,u8 closing){using A=StoryForceWidgetsArt;
        if(force>=kForceCount||!owner.state()->forces[force].count||((interactive|closing)&~4u))return false;
        force_=force;draws_.clear();name_.clear();auto value=[&](A::Scalar s){return art_.value(s);};auto translation=[&](A::Scalar x){const auto at=unsigned(x);const f32 p[3]={value(A::Scalar(at)),value(A::Scalar(at+1)),value(A::Scalar(at+2))};return mat4_translation(p);};
        part(art_.background(),0,0,{},value(A::BackgroundMaterial),0x45,true);draws_.back().joint_time=background_;
        const s16 angle=std::bit_cast<s16>(u16(u32(title_counter_)<<10));const f32 c=std::cos(f32(angle)*value(A::AngleUnit));
        const f32 scale[3]={std::fma(c,value(title_counter_<16?A::TitleXEarly:A::TitleXLate),1.f),std::fma(c,value(title_counter_<16?A::TitleYEarly:A::TitleYLate),1.f),1};
        if(!title_counter_||scale[0]<=0||scale[1]<=0)return false;
        part(art_.title(),1,0,mat4_multiply(translation(A::TitleX),mat4_scale(scale)));
        for(unsigned group=0;group<3;++group){const auto& row=art_.footers()[group];const f32 p[3]={row.x,value(A::FooterY),value(A::FooterZ)};const auto root=mat4_translation(p);
            for(unsigned i=0;i<4;++i){s32 record=row.records[i];if(record<0)continue;if(i==2&&(footer_counter_&0x10))++record;if(record>=4096)return false;const f32 local[3]={i==1?value(A::FooterIconX):0,0,0},s[3]={value(i==1?A::FooterIconScale:A::FooterScale),value(i==1?A::FooterIconScale:A::FooterScale),1};part(u16(record),2,u8(group*4+i),mat4_multiply(root,mat4_multiply(mat4_translation(local),mat4_scale(s))));}
        }
        const auto name_root=translation(A::NameX);const auto number=std::to_string(force+1);const auto& names=art_.name_parts();
        for(unsigned i=0;i<2+number.size();++i)part(names[i].record,4,u8(i),mat4_multiply(name_root,mat4_translation(names[i].position.data())),i<2?0.f:f32(number[i-2]-'0'));
        const auto& bytes=owner.state()->forces[force].name;const auto end=std::find(bytes.begin(),bytes.end(),u8(0));if(end==bytes.end())return false;name_.assign(bytes.begin(),end);
        StoryForceWidgetDraw text;text.type=4;text.part=4;text.text=true;text.world=mat4_multiply(name_root,translation(A::TextX));draws_.push_back(text);
        s32 total=0;if(!owner.total_cost(force,ForceCostMode::Full,&total)||total<0)return false;const auto digits=std::to_string(std::min(total,999999));const auto& cost=art_.cost_parts();const auto cost_root=mat4_multiply(translation(A::CostX),mat4_rotation_z(f32(art_.cost_angle())*value(A::AngleUnit)));
        //F9D84 queues digitN..digit1, then label0 (source producer order).
        for(unsigned i=unsigned(digits.size());i>0;--i){auto local=cost[1].position;local[0]=f32(f32(f32(s32(i*2-1)-s32(digits.size()))*value(A::DigitAdvance))*.5f);part(cost[1].record,5,u8(i),mat4_multiply(cost_root,mat4_translation(local.data())),f32(digits[i-1]-'0'));}
        part(cost[0].record,5,0,mat4_multiply(cost_root,mat4_translation(cost[0].position.data())));
        if(arrow_counter_&arrow_mask_)part(art_.arrows(),6,0,translation(A::ArrowX));
        if(draws_.size()>32)return false;
        for(const auto& draw:draws_)for(f32 f:draw.world.m)if(!std::isfinite(f)||std::abs(f)>1e7f)return false;
        return true;
    }
    bool fail(){ready_=false;draws_.clear();name_.clear();return false;}
    StoryForceWidgetsArt art_;std::weak_ptr<const ForceInventoryScope> owner_;u64 revision_=0;u32 updates_=0;unsigned force_=0;f32 background_=0;u16 title_counter_=0,footer_counter_=0,arrow_counter_=31,arrow_mask_=0;bool ready_=false;std::vector<StoryForceWidgetDraw> draws_;std::string name_;
};
} //namespace gf
