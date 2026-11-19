#pragma once
#include "engine/input/input_map.hpp"
#include "engine/save/borg_package_recovery.hpp"

namespace gf {
enum class BorgRecoveryMenuRequest {None,Confirm,Exit};

// Edge-driven presentation input. This controller cannot write a save: it
// emits Confirm only from the confirmation page after moving off Cancel.
class BorgRecoveryMenu {
public:
    std::size_t notice_index()const{return notice_;}
    unsigned action_index()const{return action_;}
    static unsigned notice_actions(const MissingPackageBorg* notice){
        return notice&&(notice->reason==PackageBorgAvailability::Missing||
            notice->reason==PackageBorgAvailability::Incomplete||notice->reason==PackageBorgAvailability::MissingVariant)?3:1;
    }
    BorgRecoveryMenuRequest step(const GamePadState& pad,BorgPackageRecoverySession& session){
        const auto phase=session.phase();
        if(phase!=previous_phase_){action_=0;previous_phase_=phase;}
        const bool up=pad.dpad_up&&!previous_.dpad_up,down=pad.dpad_down&&!previous_.dpad_down;
        const bool accept=pad.a&&!previous_.a,back=pad.b&&!previous_.b;
        previous_=pad;
        if(back){
            if(session.cancel()){action_=0;return BorgRecoveryMenuRequest::None;}
            return BorgRecoveryMenuRequest::Exit;
        }
        if(phase==BorgRecoveryPhase::List){
            const auto size=session.notices().size();
            if(!size){notice_=0;return BorgRecoveryMenuRequest::None;}
            notice_=std::min(notice_,size-1);
            if(up!=down){
                notice_=up?(notice_+size-1)%size:(notice_+1)%size;
                return BorgRecoveryMenuRequest::None;
            }
            if(accept)session.inspect(session.notices()[notice_].saved.instance);
        }else if(phase==BorgRecoveryPhase::Notice){
            const auto count=notice_actions(session.selected());
            if(up!=down){
                action_=up?(action_+count-1)%count:(action_+1)%count;
                return BorgRecoveryMenuRequest::None;
            }
            if(accept){
                if(!action_)session.cancel();
                else if(action_==1)session.choose(MissingPackageChoice::DisableInForces);
                else session.choose(MissingPackageChoice::DeleteFromSave);
                action_=0;
            }
        }else if(phase==BorgRecoveryPhase::ConfirmDisable||phase==BorgRecoveryPhase::ConfirmDelete){
            if(up!=down){action_=1-action_;return BorgRecoveryMenuRequest::None;}
            if(accept){
                if(action_)return BorgRecoveryMenuRequest::Confirm;
                session.cancel();
            }
        }
        return BorgRecoveryMenuRequest::None;
    }
private:
    std::size_t notice_=0;
    unsigned action_=0;
    BorgRecoveryPhase previous_phase_=BorgRecoveryPhase::Closed;
    GamePadState previous_{};
};
}
