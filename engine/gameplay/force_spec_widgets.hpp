#pragma once
#include "engine/assets/force_spec_art.hpp"
#include "engine/assets/force_specification.hpp"
#include "engine/gameplay/force_spec_navigation.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct ForceSpecWidgetPose {u16 record=0;bool visible=false;u8 layer=3;f32 frame=0;Mat4 world;};
struct ForceSpecWidgetInput {ForceSpecAuthoredView borg;u32 experience=0,callbacks=1;ForceSpecPage page=ForceSpecPage::Statistics;u8 blink=0;bool component=false,edited=false;};
// Source98 widget instances,21 type callbacks. callbacks counts actual widget
// updates (including the constructor callback); page blink is a separate clock.
inline bool force_spec_widget_pose(const ForceSpecArt& art,const ForceSpecWidgetInput& in,unsigned row,ForceSpecWidgetPose* out){
    if(!out)return false;
    *out={};if(row>=art.widgets.size()||unsigned(in.page)>3||in.blink>70||in.borg.variant>=6||in.borg.move_count>5||!in.callbacks||in.borg.cost<0||in.borg.hp<=0||in.borg.level<1||in.borg.level>10||in.experience>0x7fffffffu||!std::isfinite(art.angle_unit)||art.angle_unit<=0||art.angle_unit>.001f)return false;
    for(auto value:{art.number_step,art.value_step,art.part_shift,art.part_variant_x})if(!std::isfinite(value)||std::abs(value)>1000)return false;
    const auto& w=art.widgets[row];if(w.record>=4096||w.type>20||w.index<-1||w.index>5)return false;
    for(auto value:w.position)if(!std::isfinite(value)||std::abs(value)>1e6f)return false;
    auto p=w.position;ForceSpecWidgetPose next;next.record=w.record;next.visible=true;f32 rx=0,rz=0;
    auto digit=[&](u32 value,unsigned maximum,bool leading){if(w.index<0||unsigned(w.index)>=maximum||maximum>6)return false;unsigned count=1;u32 divisor=10;while(count<maximum&&value>=divisor){++count;divisor*=10;}if(!leading&&unsigned(w.index)>=count){next.visible=false;return true;}divisor=1;for(unsigned i=0;i<unsigned(w.index);++i)divisor*=10;next.frame=f32((value/divisor)%10);return true;};
    auto page=[&](ForceSpecPage required){next.visible=next.visible&&in.page==required;};
    auto move=[&]()->const ForceSpecMove*{return w.index>=0&&unsigned(w.index)<in.borg.move_count?&in.borg.moves[unsigned(w.index)]:nullptr;};
    const auto& b=in.borg;
    switch(w.type){
    case 0:next.layer=2;rx=0x1555*art.angle_unit;break;
    case 1:{next.layer=2;rx=0x1555*art.angle_unit;const int initial=int(w.position[0]);if(float(initial)!=w.position[0]||initial<-6000||initial>2000||initial%10)return false;const u32 until_reset=u32((initial+6000)/10+1);p[0]=in.callbacks<until_reset?f32(initial-int(in.callbacks)*10):f32(2000-int((in.callbacks-until_reset)%801)*10);break;}
    case 2:break;
    case 3:rz=std::bit_cast<s16>(u16(0-u16(in.callbacks*256)))*art.angle_unit;break;
    case 4:if(in.component)next.visible=false;else if(w.index<3){if(!digit(b.number+1,3,true))return false;p[0]+=art.number_step*w.index;}else{next.visible=b.variant!=0;next.frame=f32(int(b.variant)-1);}break;
    case 5:case 6:if(in.component)next.visible=false;else if(w.index>=0){const u32 value=w.type==5?u32(b.cost):b.level;if(!digit(value,w.type==5?6:2,false))return false;p[0]+=art.value_step*w.index;}break;
    case 7:next.visible=!in.component&&in.blink<50;rz=w.index?3.14159265358979323846f:0;break;
    case 8:if(in.component&&(w.index==1||w.index==2))next.visible=false;else{if(in.component)p[0]+=art.part_shift;if(w.index>1)next.record=u16(w.record+(in.callbacks%61>29));}break;
    case 9:if(w.index==0){next.visible=b.variant!=0;next.frame=f32(int(b.variant)-1);if(in.component)p[0]=art.part_variant_x;}else next.frame=f32(b.rarity);break;
    case 10:next.visible=!(w.index==2&&b.feature<0);page(ForceSpecPage::Statistics);break;
    case 11:next.visible=b.jump_count!=0;if(w.index&&next.visible)next.frame=f32(b.jump_count-1);page(ForceSpecPage::Statistics);break;
    case 12:case 13:{const auto value=w.type==12?u32(b.hp):in.experience;const unsigned max=w.type==12?4:6;if(!digit(value,max,false))return false;unsigned count=1;u32 power=10;while(count<max&&value>=power){++count;power*=10;}p[0]+=f32(int(count-1)*25-int(w.index)*50);page(ForceSpecPage::Statistics);break;}
    case 14:if(w.index<0||w.index>=4)return false;next.frame=b.ratings[unsigned(w.index)];page(ForceSpecPage::Statistics);break;
    case 15:next.visible=in.page==ForceSpecPage::Description||in.page==ForceSpecPage::Component;break;
    case 16:next.visible=in.page==ForceSpecPage::Description||in.page==ForceSpecPage::Component;rz=std::bit_cast<s16>(u16(int(w.index)*4096-int(u16(in.callbacks*256))))*art.angle_unit;break;
    case 17:case 18:case 19:case 20:{const auto* m=move();next.visible=m!=nullptr;if(m){p[1]-=f32(w.index)*240;if(w.type==18)next.record=u16(w.record+m->button);else if(w.type==19){next.visible=m->activation!=0;next.frame=f32(int(m->activation)-1);}else if(w.type==20)next.frame=m->icon;}page(ForceSpecPage::Moves);break;}
    default:return false;
    }
    if(next.record>=4096)return false;
    next.world=mat4_multiply(mat4_translation(p.data()),mat4_multiply(mat4_rotation_z(rz),mat4_rotation_x(rx)));for(auto value:next.world.m)if(!std::isfinite(value)||std::abs(value)>1e7f)return false;*out=next;return true;
}
} // namespace gf
