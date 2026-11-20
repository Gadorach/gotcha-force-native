#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_pop_honey_voice.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Pop Honey voice: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    constexpr NormalPopHoneySource honey{0x0904,0,3};
    PopHoneyVoiceSetupPlan setup;
    check(normal_pop_honey_voice_setup(honey,true,0x80,{1,9,300,0},&setup)&&
        setup.stock==WeaponStockModeZeroState{0,1,300,300}&&setup.paid544==1&&setup.refill_hold597==0x84,
        "paid setup spends X stock and pauses own refill without changing other bits");
    PopHoneyPaidVoiceProgram voice;
    check(normal_pop_honey_paid_voice_program(honey,setup.paid544,false,&voice)&&!voice.requested&&
        normal_pop_honey_paid_voice_program(honey,setup.paid544,true,&voice)&&voice.requested&&voice.mask==15&&
        voice.group==4&&voice.program==1,"only paid and completed startup requests voice program");
    check(normal_pop_honey_voice_setup(honey,true,0x80,{0,9,300,0},&setup)&&
        setup.stock==WeaponStockModeZeroState{0,9,300,0}&&!setup.paid544&&setup.refill_hold597==0x80&&
        normal_pop_honey_paid_voice_program(honey,setup.paid544,true,&voice)&&!voice.requested,
        "failed debit does not pause stock or queue the voice program");
    PopHoneyNoteShapeRequest shape;
    for(u8 record=0;record<14;++record)
        check(normal_pop_honey_note_shape(honey,62,record,true,&shape)&&shape.credited_caster_slot==3&&
            shape.shape_group==8&&shape.hit_row==5,"all reviewed presentation records use the same note shape");
    check(!normal_pop_honey_note_shape(honey,29,1,true,&shape)&&!normal_pop_honey_note_shape(honey,62,14,true,&shape)&&
        shape.hit_row==5,"presentation object or unknown record cannot supply a hit row");

    PopHoneyNoteContact contact{62,3,5,0,0,0,0,true,{4,0,true}};
    PopHoneyStatusApplication status;
    check(normal_pop_honey_note_status(honey,contact,&status)&&status.writes_timer&&status.request_recipient_presentation&&
        status.updated.slot==4&&status.updated.timer6b8==641,"write only the actual recipient; initial FX is separate");
    contact.recipient.timer6b8=200;
    check(normal_pop_honey_note_status(honey,contact,&status)&&status.writes_timer&&!status.request_recipient_presentation&&
        status.updated.timer6b8==641,"accepted refresh is neither stacking nor repeated initial FX");
    contact.recipient_mask59e=0x20;
    check(normal_pop_honey_note_status(honey,contact,&status)&&!status.writes_timer&&!status.request_recipient_presentation&&
        status.updated.timer6b8==200,"rejected contact does not refresh");
    contact.recipient_mask59e=0;contact.recipient_fused4a0=1;
    check(normal_pop_honey_note_status(honey,contact,&status)&&!status.writes_timer,"fused guard");
    contact.recipient_fused4a0=0;contact.hit_row=0;
    check(!normal_pop_honey_note_status(honey,contact,&status)&&!status.writes_timer,"startup hit is not the note");
    contact.hit_row=5;contact.credited_slot=2;
    check(!normal_pop_honey_note_status(honey,contact,&status)&&!status.writes_timer,"wrong source credit refuses");
    contact.credited_slot=3;contact.resolved_reviewed_contact=false;
    check(!normal_pop_honey_note_status(honey,contact,&status)&&!status.writes_timer,"no status from an aim hint");

    PopHoneyStatusSnapshot timer{4,2,true};
    check(pop_honey_status_bookkeeping(timer,0,1,&timer)&&timer.timer6b8==2,"hold blocks tick independently");
    check(pop_honey_status_bookkeeping(timer,0,0,&timer)&&timer.timer6b8==1,"eligible update decrements once");
    PopHoneyTargetingPlan target;
    check(pop_honey_status_targeting(timer,2,&target)&&target.recipient_slot==4&&target.sector==8&&target.clear_active_aim_snapshot,
        "status swaps lateral category and clears active snapshot only");
    check(pop_honey_status_targeting(timer,4,&target)&&target.sector==4,"other categories are not inverted");
    check(pop_honey_status_bookkeeping(timer,1,1,&timer)&&!timer.timer6b8&&
        pop_honey_status_targeting(timer,2,&target)&&target.sector==2&&!target.clear_active_aim_snapshot,
        "entry clears even held timer; expired readout no longer changes aim");
    std::printf("normal Pop Honey bounded voice, recipient timer and targeting: %u checks PASS\n",checks);
}
