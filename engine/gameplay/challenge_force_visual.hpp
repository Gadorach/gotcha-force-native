#pragma once
#include "engine/assets/challenge_force_art.hpp"
#include "engine/gameplay/challenge_force_card_layout.hpp"
#include "engine/gameplay/challenge_setup_chrome.hpp"
#include "engine/gameplay/force_inventory.hpp"
#include "engine/gameplay/static_record_scene.hpp"
namespace gf {
struct ChallengeForceVisualDraw {
    ChallengeSetupDraw pose;
    s16 model=-1;Mat4 icon_world;StaticRecordAppearance icon_appearance;
};
struct ChallengeForceVisualInput {bool visible=false;s16 force=-1;s32 energy=0;};
//Type10/11/12 callback owner only. Read-only inventory snapshots, no controller,
//Random assembly, editor acknowledgement or battle publication is performed.
class ChallengeForceVisual {
public:
    bool open(const ChallengeForceArt& art,const ChallengeForceCardsArt& cards,const ForceInventory& owner){
        *this={};if(!art.valid()||!cards.valid()||art.bank_hash!=cards.bank_hash||!owner.ready())return false;
        art_=art;cards_=cards;owner_=owner.instance_binding();revision_=owner.revision();card_=art.card_hidden;focus_=art.focus_hidden;cost_=art.cost_hidden;ready_=true;return true;
    }
    bool step(const ForceInventory& owner,const ChallengeForceVisualInput& input){
        if(!current(owner)||updates_>=8192||input.force<-1||input.force>=s16(kForceCount)||input.energy<1||(input.force>=0&&!owner.state()->forces[unsigned(input.force)].count))return fail();
        auto next=*this;next.draws_.clear();next.name_.clear();
        auto target=next.art_.card_hidden;
        if(input.visible)target=next.art_.card_active;else if(next.card_[0]!=next.art_.card_hidden[0])target=next.art_.card_exit;
        next.move(next.card_,target);
        f32 distance=0;for(unsigned i=0;i<3;++i){const auto d=f32(next.card_[i]-next.art_.card_exit[i]);distance=f32(distance+f32(d*d));}
        if(std::sqrt(distance)<next.art_.snap)next.card_=next.art_.card_hidden;
        next.fade(next.focus_,next.focus_alpha_,next.art_.focus_hidden,next.art_.focus_active,input.visible);
        next.fade(next.cost_,next.cost_alpha_,next.art_.cost_hidden,next.art_.cost_active,input.visible);
        if(!next.publish(owner,input))return fail();
        ++next.updates_;*this=std::move(next);return true;
    }
    bool current(const ForceInventory& owner)const{return ready_&&owner.ready()&&owner_.lock()&&owner_.lock()==owner.instance_binding().lock()&&revision_==owner.revision();}
    //An explicit editor-return consumer may refresh the SAME inventory binding
    //without restarting retained position/alpha clocks or replaying old packets.
    bool rebind_inventory(const ForceInventory& owner){
        if(!ready_||!owner.ready()||!owner_.lock()||owner_.lock()!=owner.instance_binding().lock())return fail();
        revision_=owner.revision();draws_.clear();name_.clear();return true;
    }
    //Explicit second-box publication changes inventory owner, not widget clocks.
    bool switch_inventory(const ForceInventory& previous,const ForceInventory& next){
        if(!current(previous)||!next.ready()||previous.instance_binding().lock()==next.instance_binding().lock()||previous.catalog()->fingerprint()!=next.catalog()->fingerprint())return fail();
        owner_=next.instance_binding();revision_=next.revision();draws_.clear();name_.clear();return true;
    }
    bool ready()const{return ready_;}u32 updates()const{return updates_;}
    const std::vector<ChallengeForceVisualDraw>& draws()const{return draws_;}
    const std::string& name()const{return name_;}const Mat4& name_world()const{return name_world_;}
    f32 name_alpha()const{return focus_alpha_;}const ChallengeForceArt& art()const{return art_;}
    const std::array<f32,3>& card_position()const{return card_;}const std::array<f32,3>& focus_position()const{return focus_;}const std::array<f32,3>& cost_position()const{return cost_;}
private:
    void move(std::array<f32,3>& p,const std::array<f32,3>& target)const{for(unsigned i=0;i<3;++i)p[i]=f32(p[i]+f32(f32(target[i]-p[i])*art_.approach));}
    void fade(std::array<f32,3>& p,f32& alpha,const std::array<f32,3>& hidden,const std::array<f32,3>& active,bool visible)const{
        if(visible){alpha=1;move(p,active);}else if(alpha>0){alpha=std::max(0.f,f32(alpha-art_.alpha_step));move(p,active);}else p=hidden;
    }
    void widget(const ChallengeForceWidgetPart& part,const Mat4& root,f32 alpha,f32 selector=0){
        ChallengeForceVisualDraw d;d.pose.record=part.record;d.pose.world=mat4_multiply(root,mat4_translation(part.position.data()));d.pose.alpha=alpha;d.pose.material_time=selector;draws_.push_back(d);
    }
    bool publish(const ForceInventory& owner,const ChallengeForceVisualInput& input){
        const bool random=input.force<0;const ForceDefinition* force=nullptr;
        if(!random)force=&owner.state()->forces[unsigned(input.force)];
        const unsigned count=force?force->count:0;ChallengeForceCardsArt::Format format;
        if(!challenge_force_card_format(cards_,count,random,&format))return false;
        const auto root=mat4_multiply(mat4_translation(card_.data()),mat4_rotation_x(f32(format.angle)*art_.angle_unit));
        s32 prefix=0;if(force&&!owner.budget_prefix(unsigned(input.force),input.energy,ForceCostMode::Full,&prefix))return false;
        if(force)for(unsigned slot=0;slot<kForceSlots;++slot){const s16 box=force->slots[slot];if(box<0)continue;
            ChallengeForceCardMember member;if(!challenge_force_card_slot(cards_,count,slot,unsigned(prefix),&member))return false;
            ChallengeForceVisualDraw d;d.pose.record=art_.shell_record;d.pose.mask=0x25;d.pose.always_depth=true;d.pose.alpha=member.alpha;
            d.pose.world=mat4_multiply(root,mat4_multiply(mat4_translation(member.position.data()),mat4_scale(member.scale.data())));
            const f32 child[3]={0,cards_.y,cards_.child_z};d.icon_world=mat4_multiply(d.pose.world,mat4_multiply(mat4_translation(child),mat4_rotation_x(-f32(format.angle)*art_.angle_unit)));
            d.model=owner.state()->box[unsigned(box)].value.model;d.icon_appearance={member.alpha,{member.source_rgb,member.source_rgb,member.source_rgb}};draws_.push_back(d);
        }
        ChallengeForceVisualDraw plate;plate.pose.record=format.record;plate.pose.mask=random?0x24:1;plate.pose.world=root;draws_.push_back(plate);
        if(focus_alpha_>0){const auto focus_root=mat4_translation(focus_.data());widget(art_.focus[0],focus_root,f32(focus_alpha_*art_.focus_alpha));
            if(force){widget(art_.focus[1],focus_root,focus_alpha_);const auto number=std::to_string(unsigned(input.force)+1);for(unsigned i=0;i<number.size();++i)widget(art_.focus[i+2],focus_root,focus_alpha_,f32(number[i]-'0'));}
            if(force){const auto end=std::find(force->name.begin(),force->name.end(),u8(0));name_.assign(force->name.begin(),end);}else name_=art_.random_name;
            name_world_=mat4_multiply(focus_root,mat4_translation(art_.name_origin.data()));
        }
        if(cost_alpha_>0){s32 total=0;if(force&&!owner.total_cost(unsigned(input.force),ForceCostMode::Full,&total))return false;
            const auto digits=random?std::string("????"):std::to_string(std::min(total,999999));const auto cost_root=mat4_multiply(mat4_translation(cost_.data()),mat4_rotation_z(f32(art_.cost_angle)*art_.angle_unit));
            for(unsigned i=digits.size();i>0;--i){auto part=art_.cost[1];part.position[0]=f32(f32(f32(s32(2*(i-1)+1)-s32(digits.size()))*art_.digit_advance)*.5f);widget(part,cost_root,cost_alpha_,digits[i-1]=='?'?10.f:f32(digits[i-1]-'0'));}
            widget(art_.cost[0],cost_root,cost_alpha_);
        }
        if(draws_.size()>48)return false;
        for(const auto& d:draws_)for(f32 f:d.pose.world.m)if(!std::isfinite(f)||std::abs(f)>1e7f)return false;
        return true;
    }
    bool fail(){ready_=false;draws_.clear();name_.clear();return false;}
    ChallengeForceArt art_;ChallengeForceCardsArt cards_;std::weak_ptr<const ForceInventoryScope> owner_;u64 revision_=0;u32 updates_=0;bool ready_=false;
    std::array<f32,3> card_{},focus_{},cost_{};f32 focus_alpha_=0,cost_alpha_=0;std::vector<ChallengeForceVisualDraw> draws_;std::string name_;Mat4 name_world_;
};
} // namespace gf
