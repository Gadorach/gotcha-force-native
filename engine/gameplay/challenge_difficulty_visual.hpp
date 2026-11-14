#pragma once
#include <bit>
#include <cmath>
#include <vector>
#include "engine/gameplay/challenge_setup_chrome.hpp"
namespace gf {
struct ChallengeDifficultyVisualInput {u8 visible=8,interactive=0,closing=0,selection=0;};
using ChallengeDifficultyDraw=ChallengeSetupDraw;
//Source191C78/191EB0/192258/192988/192DE0. Difficulty context only;
//later player/load-box/selection/exit widgets remain outside this domain.
//This is state per UI callback; draw/repeated model passes never advance it.
class ChallengeDifficultyVisual {
public:
    bool open(const ChallengeDifficultyArt& art){
        *this={};if(!art.valid()||!chrome_.open(art))return false;art_=art;
        for(unsigned i=0;i<3;++i){auto& p=panels_[i];p.position=panel_rest(i);p.alpha=1;}
        focus_x_=art_.focus_rest_x;
        ready_=true;
        //Source constructors0/1/4 explicitly invoke their update once. Type2's
        //constructor does not: its first position/phase update remains separate.
        update_panels();update_focus();return publish();
    }
    bool step(const ChallengeDifficultyVisualInput& input){
        if(!ready_||updates_>=8192||input.selection>=3||((input.visible|input.interactive|input.closing)&~8u)){ready_=false;draws_.clear();return false;}
        auto next=*this;next.input_=input;if(!next.chrome_.step(input.visible,input.interactive)){ready_=false;draws_.clear();return false;}next.update_panels();next.update_focus();
        ++next.updates_;
        if(!next.publish()){ready_=false;draws_.clear();return false;}
        *this=std::move(next);return true;
    }
    //Explicit widgets-only owner for the joined setup. The caller supplies ONE
    //shared chrome; no second backdrop/footer clock is constructed or advanced.
    bool open_widgets(const ChallengeDifficultyArt& art){
        *this={};if(!art.valid())return false;art_=art;widgets_only_=true;
        for(unsigned i=0;i<3;++i){auto& p=panels_[i];p.position=panel_rest(i);p.alpha=1;}
        focus_x_=art_.focus_rest_x;ready_=true;update_panels();update_focus();return publish();
    }
    bool step_widgets(const ChallengeDifficultyVisualInput& input,u8 context){
        if(!widgets_only_||!ready_||updates_>=8192||input.selection>=3||context>2||((input.visible|input.interactive|input.closing)&~8u)){ready_=false;draws_.clear();return false;}
        auto next=*this;next.input_=input;next.setup_context_=context;next.update_panels();next.update_focus();++next.updates_;
        if(!next.publish()){ready_=false;draws_.clear();return false;}*this=std::move(next);return true;
    }
    const std::vector<ChallengeDifficultyDraw>& draws()const{return draws_;}
    const ChallengeDifficultyArt& art()const{return art_;}
    bool ready()const{return ready_;}u32 updates()const{return updates_;}
private:
    struct Panel {std::array<f32,3> position{};f32 alpha=0,selected=0;s16 spin=0;};
    f32 op(unsigned index)const{return art_.operands[index];}
    static f32 toward(f32 current,f32 target,f32 factor){const f32 delta=f32(target-current);const f32 step=f32(delta*factor);return f32(current+step);}
    std::array<f32,3> panel_position(unsigned i)const{return {f32(op(20)*f32(i)-op(20)),op(21),op(22)};}
    std::array<f32,3> panel_rest(unsigned i)const{auto point=panel_position(i);point[1]=f32(point[1]-op(23));return point;}
    static s16 spin_add(s16 prior,f32 amount){const f32 sum=f32(f32(prior)+amount);return std::bit_cast<s16>(u16(s32(sum)));}
    void update_panels(){
        for(unsigned i=0;i<3;++i){auto& panel=panels_[i];std::array<f32,3> target;
            if(setup_context_){target=panel_rest(i);panel.position=target;panel.alpha=0;}
            else if(!((input_.visible|input_.closing)&8)){target=panel_rest(i);panel.alpha=1;}
            else if(!(input_.closing&8)||(i!=input_.selection&&(input_.visible&8))){target=panel_position(i);panel.alpha=1;}
            else if(i!=input_.selection){target=panel_rest(i);panel.alpha=1;}
            else if(panel.alpha>0){target=panel_position(i);panel.alpha=std::max(0.f,f32(panel.alpha-op(8)));}
            else{target=panel_rest(i);panel.position=target;}
            for(unsigned k=0;k<3;++k)panel.position[k]=toward(panel.position[k],target[k],op(8));
            panel.selected=i==input_.selection?1.f:0.f;
            panel.spin=spin_add(panel.spin,i==input_.selection?op(24):op(25));
        }
    }
    void update_focus(){
        ++focus_tick_;u16 blink=0;f32 target=art_.focus_base_x;
        if((input_.interactive|(input_.visible&input_.closing))&8){focus_alpha_=1;blink=input_.interactive&8?32:2;}
        else if(focus_alpha_>0)focus_alpha_=std::max(0.f,f32(focus_alpha_-op(8)));
        else{focus_previous_=-1;target=art_.focus_rest_x;focus_x_=target;}
        //Source's fused multiply/add here is distinct from the VecSub/Scale/Add
        //used by the other widget positions.
        const f32 delta=f32(target-focus_x_);focus_x_=std::fma(op(8),delta,focus_x_);
        if(focus_previous_!=input_.selection){focus_previous_=input_.selection;focus_tick_=32;}
        focus_draw_mask_=u8(art_.focus_masks[input_.selection]&((focus_tick_&blink)?31u:1u));
        focus_angle_=std::bit_cast<s16>(u16(u16(focus_angle_)-512u));
    }
    bool publish(){
        draws_.clear();auto add=[&](ChallengeDifficultyDraw pose){
            if(draws_.size()>=80||pose.record>=4096||!std::isfinite(pose.alpha)||pose.alpha<0||pose.alpha>1)return false;
            for(float value:pose.world.m)if(!std::isfinite(value)||std::abs(value)>1e7f)return false;
            draws_.push_back(std::move(pose));return true;};
        if(!widgets_only_)for(const auto& pose:chrome_.draws())if(!add(pose))return false;
        for(unsigned choice=0;choice<3;++choice){const auto& panel=panels_[choice];if(panel.alpha<=0)continue;
            const Mat4 parent=mat4_translation(panel.position.data());
            for(unsigned component=0;component<3;++component){const auto& source=art_.energy[choice][component];ChallengeDifficultyDraw pose;pose.record=source.record;pose.alpha=panel.alpha;pose.mask=panel.alpha==1?source.draw_mask:0x24;
                auto world=mat4_multiply(parent,mat4_translation(source.position.data()));world=mat4_multiply(world,mat4_rotation_z(f32(source.rotation_z)*bits_to_f32(0x38c90fdb)));world=mat4_multiply(world,mat4_rotation_x(f32(source.rotation_x)*bits_to_f32(0x38c90fdb)));
                if(component==1)world=mat4_multiply(world,mat4_rotation_y(f32(panel.spin)*bits_to_f32(0x38c90fdb)));
                pose.world=world;pose.material_time=component==2?panel.selected:0;pose.translucent=component==1&&panel.alpha<1;if(!add(pose))return false;
            }
        }
        if(focus_alpha_>0)for(unsigned component=0;component<5;++component){
            if(!(focus_draw_mask_&(1u<<component)))continue;
            const auto& source=art_.focus[component];auto position=source.position;
            if(component==0)position[0]=std::fma(op(20),f32(input_.selection),focus_x_);
            const f32 size[3]={source.scale,source.scale,1};ChallengeDifficultyDraw pose;pose.record=source.record;pose.alpha=focus_alpha_;
            const s16 angle=component==0?focus_angle_:source.rotation_z;
            pose.world=mat4_multiply(mat4_translation(position.data()),mat4_multiply(mat4_rotation_z(f32(angle)*bits_to_f32(0x38c90fdb)),mat4_scale(size)));
            if(!add(pose))return false;
        }
        for(unsigned choice=0;choice<3;++choice){const auto& panel=panels_[choice];if(panel.alpha<=0)continue;
            const Mat4 parent=mat4_translation(panel.position.data());
            //Source child type5: one label + decimal digits, all follow parent's
            //current matrix and alpha. Original digit geometry/UVs, not a font.
            const auto text=std::to_string(art_.limits[choice]);if(text.size()>6)return false;
            const Mat4 label_root=mat4_multiply(parent,mat4_translation(art_.label_origins[choice].data()));
            for(unsigned component=0;component<=text.size();++component){const auto& source=art_.labels[component?1:0];auto point=source.position;ChallengeDifficultyDraw pose;pose.record=source.record;pose.alpha=panel.alpha;
                if(component){const s32 ordinal=s32(component-1);point[0]=f32(f32(op(27)*f32(ordinal*2+1-s32(text.size())))*op(15));pose.material_time=f32(text[component-1]-'0');}
                const f32 size[3]={source.scale,source.scale,1};pose.world=mat4_multiply(label_root,mat4_multiply(mat4_translation(point.data()),mat4_scale(size)));if(!add(pose))return false;
            }
        }
        return true;
    }
    ChallengeDifficultyArt art_;ChallengeDifficultyVisualInput input_;
    ChallengeSetupChrome chrome_;std::array<Panel,3> panels_;std::vector<ChallengeDifficultyDraw> draws_;
    f32 focus_alpha_=0,focus_x_=0;u16 focus_tick_=0;
    s16 focus_angle_=0;s32 focus_previous_=-1;u8 focus_draw_mask_=0;u32 updates_=0;bool ready_=false;
    bool widgets_only_=false;u8 setup_context_=0;
};
} // namespace gf
