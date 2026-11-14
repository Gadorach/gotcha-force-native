#pragma once
#include "engine/gameplay/option_settings.hpp"
#include "engine/gameplay/menu_navigation/repeat.hpp"
#include "engine/gameplay/menu_navigation/input_bits.hpp"
#include "engine/gameplay/ui_sound_operations.hpp"
#include "engine/gameplay/option_mouse.hpp"
namespace gf {
enum class OptionMenuPhase { Editing,CreditsUnported,Saved,SaveError,CreditsDelay,Credits };
struct OptionMenuEvent {bool changed=false,save_attempted=false,leave=false,bgm_immediate=false,open_cheats=false,open_more=false;};
// Value/navigation slice of original 800FF698. Modal text/persistence are native,
// not emulated card activity. Output policy/credits rendering are separate;
// original audio and memory-card/resource gates aren't simulated here.
class OptionMenu {
public:
    bool configure(u8 limit,u8 restart){
        if(limit==255||restart>limit)return false;
        repeat_.repeat_limit=limit;repeat_.repeat_restart=restart;return true;
    }
    bool configure_credits(u32 wait){if(wait>600)return false;credits_wait_=wait;credits_available_=true;return true;}
    void configure_cheats(){cheats_available_=true;}
    void configure_more(){more_available_=true;cheats_available_=false;}
    bool finish_credits(){if(phase_!=OptionMenuPhase::Credits)return false;phase_=OptionMenuPhase::Editing;return true;}
    u32 credits_delay()const{return delay_left_;}
    void begin(OptionSettingsStore& store,const std::array<GamePadState,4>& pads){
        settings_=OptionSettings{};load_=store.load(&settings_);row_=0;phase_=OptionMenuPhase::Editing;
        history_={};observe(pads);
    }
    void observe(const std::array<GamePadState,4>& pads){(void)events(pads);}
    OptionMenuEvent update(OptionSettingsStore& store,const std::array<GamePadState,4>& pads){
        return update_impl(store,pads,nullptr);
    }
    OptionMenuEvent update_with_sound(OptionSettingsStore& store,const std::array<GamePadState,4>& pads,UiSoundBatch* sounds){
        if(sounds)*sounds={};
        return update_impl(store,pads,sounds);
    }
    OptionMenuEvent update_pointed(OptionSettingsStore& store,const std::array<GamePadState,4>& pads,const OptionMouseCommand& mouse){
        if(!option_mouse_valid(mouse))return {};
        for(const auto& pad:pads)if(menu_pad_bits(pad,48))return update_impl(store,pads,nullptr);
        const auto area=phase_==OptionMenuPhase::Editing?OptionMouseArea::Settings:phase_==OptionMenuPhase::Saved?OptionMouseArea::Saved:phase_==OptionMenuPhase::SaveError?OptionMouseArea::SaveError:OptionMouseArea::None;if(mouse.area!=OptionMouseArea::None&&mouse.area!=area)return {};
        return update_impl(store,pads,nullptr,mouse);
    }
private:
    OptionMenuEvent update_impl(OptionSettingsStore& store,const std::array<GamePadState,4>& pads,UiSoundBatch* sounds,const OptionMouseCommand& mouse={}){
        auto input=events(pads);OptionMenuEvent result;input.pressed|=mouse.pressed;
        const bool a=(input.pressed&0x100u)!=0,b=(input.pressed&0x200u)!=0;
        if(phase_==OptionMenuPhase::Saved){if(a)result.leave=true;return result;}
        if(phase_==OptionMenuPhase::CreditsUnported){if(b||a)phase_=OptionMenuPhase::Editing;return result;}
        if(phase_==OptionMenuPhase::CreditsDelay){
            if(delay_left_)--delay_left_;
            if(!delay_left_)phase_=OptionMenuPhase::Credits;
            return result; // motion/delay input is consumed, not replayed
        }
        if(phase_==OptionMenuPhase::Credits){if(b)phase_=OptionMenuPhase::Editing;return result;}
        if(phase_==OptionMenuPhase::SaveError){
            if(b){phase_=OptionMenuPhase::Editing;return result;}
            if(a)save(store,result);
            return result;
        }
        u32 direction=menu_direction_events(input.pressed|input.repeated);
        // Helpers 8008BA58/8008BA00 compare the complete direction nibble:
        // diagonal/opposite inputs don't acquire an invented priority.
        const auto previous_row=row_;
        const u8 last=cheats_available_?10:9;
        if(mouse.row>=0&&mouse.row<=last)row_=u8(mouse.row);
        if(direction==8)row_=row_?u8(row_-1):last;
        else if(direction==4)row_=row_==last?0:u8(row_+1);
        if(sounds&&row_!=previous_row)sounds->emit(UiSoundOperationKind::OptionNavigate,row_,s8(settings_.volume[1]));
        unsigned steps=1;if(mouse.value>=0&&row_<8){const int current=row_==0?settings_.mono:row_<4?settings_.volume[row_-1]:(settings_.rumble_mask&(1u<<(row_-4)))?0:1;direction=mouse.value>current?2u:mouse.value<current?1u:0u;steps=unsigned(mouse.value>current?mouse.value-current:current-mouse.value);}
        if(row_<8&&(direction==1||direction==2)){
            const bool right=direction==2;const OptionSettings before=settings_;
            for(unsigned n=0;n<steps;++n){
            if(row_==0)settings_.mono=clamp_step(settings_.mono,1,right);
            else if(row_<4)settings_.volume[row_-1]=clamp_step(settings_.volume[row_-1],4,right);
            else{
                const u8 bit=u8(1u<<(row_-4));u8 choice=(settings_.rumble_mask&bit)?0:1;
                choice=clamp_step(choice,1,right);
                if(choice)settings_.rumble_mask=u8(settings_.rumble_mask&~bit);else settings_.rumble_mask|=bit;
            }
            }
            result.changed=settings_!=before;
            if(sounds&&result.changed)sounds->emit(UiSoundOperationKind::OptionValue,row_,s8(settings_.volume[1]));
            if(row_==1&&result.changed)result.bgm_immediate=true;
        }
        if(b){if(sounds)sounds->emit(UiSoundOperationKind::OptionBack,row_,s8(settings_.volume[1]));save(store,result);return result;}
        if(a&&row_==8){
            if(sounds)sounds->emit(UiSoundOperationKind::OptionActivate,row_,s8(settings_.volume[1]));
            if(more_available_)result.open_more=true;
            else if(credits_available_){delay_left_=credits_wait_;phase_=delay_left_?OptionMenuPhase::CreditsDelay:OptionMenuPhase::Credits;}
            else phase_=OptionMenuPhase::CreditsUnported;
        }
        else if(a&&row_==9){if(sounds)sounds->emit(UiSoundOperationKind::OptionActivate,row_,s8(settings_.volume[1]));const auto before=settings_;option_restore_values(settings_);result.changed=settings_!=before;result.bgm_immediate=true;}
        else if(a&&row_==10&&cheats_available_)result.open_cheats=true;
        return result;
    }
public:
    const OptionSettings& settings()const{return settings_;}
    unsigned row()const{return row_;}
    OptionMenuPhase phase()const{return phase_;}
    OptionSettingsLoad load_status()const{return load_;}
private:
    static u8 clamp_step(u8 v,u8 max,bool right){return right?(v<max?u8(v+1):v):(v?u8(v-1):0);}
    MenuPadEvents events(const std::array<GamePadState,4>& pads){
        MenuPadEvents result;
        for(unsigned i=0;i<4;++i){const auto event=menu_repeat_step(history_[i],menu_pad_bits(pads[i],48),repeat_);
            result.pressed|=event.pressed;result.repeated|=event.repeated;}
        return result;
    }
    void save(OptionSettingsStore& store,OptionMenuEvent& result){
        // Unknown/invalid existing records are not silently replaced. Recheck
        // status on explicit retry; an owner can repair/remove them externally.
        OptionSettings ignored;const auto current=store.load(&ignored);
        if(current==OptionSettingsLoad::Invalid||current==OptionSettingsLoad::Unavailable){load_=current;phase_=OptionMenuPhase::SaveError;return;}
        result.save_attempted=true;
        if(store.write(settings_)){phase_=OptionMenuPhase::Saved;load_=OptionSettingsLoad::Loaded;}
        else phase_=OptionMenuPhase::SaveError;
    }
    OptionSettings settings_;OptionSettingsLoad load_=OptionSettingsLoad::Missing;
    OptionMenuPhase phase_=OptionMenuPhase::Editing;u8 row_=0;
    bool credits_available_=false,cheats_available_=false,more_available_=false;u32 credits_wait_=0,delay_left_=0;
    std::array<MenuPadHistory,4> history_{};MenuNavigationConfig repeat_{};
};
} // namespace gf
