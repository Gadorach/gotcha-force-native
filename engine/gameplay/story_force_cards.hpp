#pragma once
#include "engine/assets/story_force_cards.hpp"
#include "engine/gameplay/source_force_card_layout.hpp"
#include "engine/gameplay/force_inventory.hpp"
#include "engine/gameplay/static_record_scene.hpp"
namespace gf {
struct StoryForceCardDraw {
    u16 record=0,box_slot=0;u8 source_slot=30,mask=1;bool member=false;s16 model=-1;
    Mat4 world,icon_world;StaticRecordAppearance icon_appearance;
};
class StoryForceCardPlan {
public:
    bool current(const ForceInventory& owner)const{const auto binding=owner_.lock();return ready_&&binding&&owner.ready()&&owner.instance_binding().lock()==binding&&owner.revision()==revision_;}
    std::span<const StoryForceCardDraw> draws()const{return draws_;}
    unsigned force()const{return force_;}s32 prefix()const{return prefix_;}u32 art_tag()const{return art_tag_;}
private:
    friend bool story_force_card_plan(const StoryForceCardArt&,const ForceInventory&,unsigned,s32,StoryForceCardPlan*);
    std::vector<StoryForceCardDraw> draws_;std::weak_ptr<const ForceInventoryScope> owner_;u64 revision_=0;unsigned force_=0;s32 prefix_=0;u32 art_tag_=0;bool ready_=false;
};
//F91E8/F965C type3 physical slots, not compact member indices. Render candidate
//only; no eligibility, confirmation, RNG, event/campaign or inventory mutation.
inline bool story_force_card_plan(const StoryForceCardArt& art,const ForceInventory& owner,unsigned force,s32 energy,StoryForceCardPlan* out){
    if(!out||!art.ready()||!owner.ready()||force>=kForceCount)return false;
    const auto& stored=owner.state()->forces[force];SourceForceCardsArt::Format format;
    if(!source_force_card_format(art.layout(),stored.count,false,&format))return false;
    StoryForceCardPlan next;next.owner_=owner.instance_binding();next.revision_=owner.revision();next.force_=force;next.art_tag_=art.tag();
    if(!owner.budget_prefix(force,energy,ForceCostMode::Full,&next.prefix_)||next.prefix_<-1||next.prefix_>30)return false;
    const auto root=mat4_multiply(mat4_translation(art.origin().data()),mat4_rotation_x(f32(format.angle)*art.angle_unit()));
    //Source F9578 uses SIGNED cmpw. Prefix -1 is unlimited for confirmation,
    //but every nonnegative card slot fails this distinct colour comparison.
    const unsigned visual_prefix=next.prefix_<0?0u:unsigned(next.prefix_);
    for(unsigned slot=0;slot<kForceSlots;++slot){const s16 box=stored.slots[slot];if(box<0)continue;
        SourceForceCardMember member;if(!source_force_card_slot(art.layout(),stored.count,slot,visual_prefix,&member))return false;
        StoryForceCardDraw draw;draw.member=true;draw.record=art.shell();draw.box_slot=u16(box);draw.source_slot=u8(slot);draw.mask=0x45;draw.model=owner.state()->box[unsigned(box)].value.model;
        draw.world=mat4_multiply(root,mat4_multiply(mat4_translation(member.position.data()),mat4_scale(member.scale.data())));
        const f32 child[3]={0,art.layout().y,art.layout().child_z};draw.icon_world=mat4_multiply(draw.world,mat4_multiply(mat4_translation(child),mat4_rotation_x(-f32(format.angle)*art.angle_unit())));
        draw.icon_appearance={member.alpha,{member.source_rgb,member.source_rgb,member.source_rgb}};next.draws_.push_back(draw);
    }
    StoryForceCardDraw plate;plate.record=format.record;plate.world=root;next.draws_.push_back(plate);
    if(next.draws_.size()!=std::size_t(stored.count)+1||next.draws_.size()>31)return false;
    for(const auto& draw:next.draws_)for(const auto* m:{&draw.world,&draw.icon_world})for(f32 f:m->m)if(!std::isfinite(f)||std::abs(f)>1e7f)return false;
    next.ready_=true;*out=std::move(next);return true;
}
} //namespace gf
