#pragma once
#include "engine/assets/challenge_setup_widgets.hpp"
#include "engine/gameplay/challenge_setup_chrome.hpp"
namespace gf {
struct ChallengeSetupWidgetsInput {u8 visible=8,interactive=0,closing=0,players_choice=0,context=0;};
//193508/3670/3A14 players, 193AFC/3BD4/3DDC focus, 193E9C/3FF4/1941B8 box.
//No input/notice/readiness controller lives here. One step is one UI callback;
//draw access never ticks. Context0 includes difficulty AND players; 1/2 are boxes.
class ChallengeSetupWidgets {
public:
    bool open(const ChallengeDifficultyArt& common,const ChallengeSetupWidgetsArt& art){
        *this={};if(!common.valid()||!art.valid()||common.bank_hash!=art.bank_hash)return false;
        common_=common;art_=art;for(auto& panel:panels_)panel.position=player_rest();
        focus_x_=op(8);box_position_=box_rest();ready_=true;
        update_players();update_focus();update_box();if(!publish())return fail();return true;
    }
    bool step(const ChallengeSetupWidgetsInput& input){
        if(!ready_||updates_>=8192||input.players_choice>1||input.context>2||((input.visible|input.interactive|input.closing)&~15u))return fail();
        auto next=*this;next.input_=input;next.update_players();next.update_focus();next.update_box();++next.updates_;
        if(!next.publish())return fail();
        *this=std::move(next);return true;
    }
    const std::vector<ChallengeSetupDraw>& draws()const{return draws_;}bool ready()const{return ready_;}u32 updates()const{return updates_;}
private:
    struct Panel {std::array<f32,3> position{};f32 alpha=1;std::array<s16,2> spin{};};
    f32 op(unsigned i)const{return art_.operands[i];}f32 blend()const{return common_.operands[8];}
    static s16 angle_add(s16 angle,s32 delta){return std::bit_cast<s16>(u16(u32(u16(angle))+u32(delta)));}
    static f32 toward(f32 value,f32 target,f32 factor){return f32(value+f32(f32(target-value)*factor));}
    std::array<f32,3> player_live()const{return {0,0,op(0)};}std::array<f32,3> player_rest()const{return {op(1),0,op(0)};}std::array<f32,3> player_hidden()const{return {0,-op(2),op(0)};}
    std::array<f32,3> box_live()const{return {0,op(10),op(11)};}std::array<f32,3> box_rest()const{return {op(12),op(10),op(11)};}std::array<f32,3> box_close()const{return {op(13),op(10),op(11)};}
    void update_players(){
        for(unsigned i=0;i<2;++i){auto& p=panels_[i];std::array<f32,3> target;
            if(input_.context){target=player_hidden();p.position=target;p.alpha=0;}
            else if(!((input_.visible|input_.closing)&2)){target=player_rest();p.alpha=1;}
            else if(!(input_.closing&2)||(i!=input_.players_choice&&(input_.visible&2))){target=player_live();p.alpha=1;}
            else if(i!=input_.players_choice){target=player_hidden();p.alpha=1;}
            else if(p.alpha>0){p.alpha=std::max(0.f,f32(p.alpha-blend()));target=player_live();}
            else{target=player_rest();p.position=target;}
            for(unsigned k=0;k<3;++k)p.position[k]=toward(p.position[k],target[k],blend());
            for(unsigned child=0;child<2;++child)if(art_.players[i][child+2].record>=0)p.spin[child]=i==input_.players_choice?angle_add(p.spin[child],1024):0;
        }
    }
    void update_focus(){
        ++focus_tick_;f32 target=op(7);u16 blink=0;
        if((input_.interactive|(input_.visible&input_.closing))&2){focus_alpha_=1;blink=input_.interactive&2?32:2;}
        else if(focus_alpha_>0)focus_alpha_=std::max(0.f,f32(focus_alpha_-blend()));
        else{focus_previous_=-1;focus_x_=op(8);target=op(8);}
        focus_x_=std::fma(blend(),f32(target-focus_x_),focus_x_);
        if(focus_previous_!=input_.players_choice){focus_previous_=input_.players_choice;focus_tick_=32;}
        focus_mask_=(focus_tick_&blink)?3:1;focus_angle_=angle_add(focus_angle_,-512);
    }
    void update_box(){
        const auto target=(input_.visible&1)?box_live():(input_.closing&1)?box_close():box_rest();
        if(!((input_.visible|input_.closing)&1))box_position_=target;
        for(unsigned k=0;k<3;++k)box_position_[k]=toward(box_position_[k],target[k],blend());
        box_motion_=f32(box_motion_+common_.operands[15]);if(box_motion_>=op(14))box_motion_=f32(box_motion_-op(14));
        if(input_.context)box_material_=f32(input_.context-1);
        box_angle_=angle_add(box_angle_,-256);
    }
    bool publish(){
        draws_.clear();const f32 unit=bits_to_f32(0x38c90fdb);
        for(unsigned i=0;i<2;++i){const auto& panel=panels_[i];if(panel.alpha<=0)continue;const auto parent=mat4_translation(panel.position.data());
            for(unsigned j=0;j<4;++j){const auto& part=art_.players[i][j];if(part.record<0)continue;
                ChallengeSetupDraw pose;pose.record=u16(part.record);pose.alpha=j==0&&i!=input_.players_choice?f32(panel.alpha*op(3)):panel.alpha;
                pose.mask=pose.alpha==1?part.mask:0x24;pose.material_time=j==1&&i==input_.players_choice?1:0;pose.translucent=j>=2&&pose.alpha<1;pose.after_callbacks_alpha=true;
                pose.world=mat4_multiply(parent,mat4_translation(part.position.data()));
                if(j>=2){const f32 wave=f32(op(4)*std::sin(f32(panel.spin[j-2])*unit));const f32 limited=std::min(op(4),std::max(op(6),wave));const f32 angle=f32(op(5)*limited);
                    pose.world=mat4_multiply(pose.world,mat4_rotation_z(f32(s16(s32(angle)))*unit));}
                draws_.push_back(pose);
            }
        }
        if(focus_alpha_>0)for(unsigned i=0;i<2;++i){if(!(focus_mask_&(1u<<i)))continue;auto point=art_.focus[i].position;
            if(!i)point[0]=std::fma(op(9),f32(input_.players_choice),focus_x_);
            ChallengeSetupDraw pose;pose.record=art_.focus[i].record;pose.alpha=focus_alpha_;const s16 angle=i?(input_.players_choice?std::bit_cast<s16>(u16(0x8000)):0):focus_angle_;
            pose.world=mat4_multiply(mat4_translation(point.data()),mat4_rotation_z(f32(angle)*unit));draws_.push_back(pose);
        }
        const auto parent=mat4_translation(box_position_.data());for(unsigned i=0;i<2;++i){const auto& part=art_.box[i];ChallengeSetupDraw pose;pose.record=part.record;pose.mask=part.mask;
            pose.authored_alpha=true;
            pose.joint_time=i?f32(u32(box_motion_)):0;pose.material_time=i?box_material_:1;const f32 scale[3]={part.scale,part.scale,part.scale};
            auto world=mat4_multiply(parent,mat4_translation(part.position.data()));if(i)world=mat4_multiply(world,mat4_rotation_y(f32(box_angle_)*unit));
            if(!i)world=mat4_multiply(world,mat4_rotation_x(-16384.f*unit));
            pose.world=mat4_multiply(world,mat4_scale(scale));draws_.push_back(pose);
        }
        for(const auto& pose:draws_){if(!std::isfinite(pose.alpha)||pose.alpha<0||pose.alpha>1)return false;for(f32 v:pose.world.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;}
        return draws_.size()<=11;
    }
    bool fail(){ready_=false;draws_.clear();return false;}
    ChallengeDifficultyArt common_;ChallengeSetupWidgetsArt art_;ChallengeSetupWidgetsInput input_;std::array<Panel,2> panels_;std::vector<ChallengeSetupDraw> draws_;
    std::array<f32,3> box_position_{};f32 focus_x_=0,focus_alpha_=0,box_motion_=0,box_material_=1;u16 focus_tick_=0,focus_mask_=0;s16 focus_angle_=0,box_angle_=0;s32 focus_previous_=-1;u32 updates_=0;bool ready_=false;
};
//Common and page-specific clocks stay alive across player/box page changes.
//The initial difficulty widgets and setup lifecycle/controller are separate.
class ChallengeSetupPagesVisual {
public:
    bool open(const ChallengeDifficultyArt& common,const ChallengeSetupWidgetsArt& art){*this={};if(!chrome_.open(common,true)||!widgets_.open(common,art))return false;ready_=true;return publish();}
    bool step(const ChallengeSetupWidgetsInput& input){
        if(!ready_)return false;
        auto next=*this;
        if(!next.widgets_.step(input)||!next.chrome_.step(input.visible,input.interactive)){ready_=false;draws_.clear();return false;}
        if(!next.publish()){ready_=false;draws_.clear();return false;}*this=std::move(next);return true;
    }
    bool ready()const{return ready_;}const std::vector<ChallengeSetupDraw>& draws()const{return draws_;}const ChallengeSetupChrome& chrome()const{return chrome_;}const ChallengeSetupWidgets& widgets()const{return widgets_;}
private:
    bool publish(){draws_=chrome_.draws();draws_.insert(draws_.end(),widgets_.draws().begin(),widgets_.draws().end());return draws_.size()<=80;}
    ChallengeSetupChrome chrome_;ChallengeSetupWidgets widgets_;std::vector<ChallengeSetupDraw> draws_;bool ready_=false;
};
} // namespace gf
