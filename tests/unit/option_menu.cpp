#include <cstdio>
#include <cstdlib>
#include <map>
#include "engine/gameplay/option_menu.hpp"
#include "engine/gameplay/option_transition.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"option menu: %s\n",why);std::abort();}}
struct Store final:gf::ISaveBackend {
    std::map<gf::u32,std::vector<gf::u8>> slots;unsigned writes=0;bool bad_read=false,bad_write=false;
    bool load(gf::u32 slot,std::vector<gf::u8>& out)override{if(bad_read||!slots.count(slot))return false;out=slots[slot];return true;}
    gf::SaveReadStatus read_slot(gf::u32 slot,std::vector<gf::u8>& out)override{
        if(bad_read)return gf::SaveReadStatus::Error;
        if(!slots.count(slot))return gf::SaveReadStatus::Missing;
        return load(slot,out)?gf::SaveReadStatus::Loaded:gf::SaveReadStatus::Error;
    }
    bool store(gf::u32 slot,const gf::u8* data,std::size_t size)override{++writes;if(bad_write)return false;slots[slot]={data,data+size};return true;}
    bool erase(gf::u32 slot)override{return slots.erase(slot)!=0;}
};
}
int main(){
    Store backend;backend.slots[0]={1,2,3};gf::OptionSettingsStore store(backend);gf::OptionSettings s;
    s.mono=1;check(store.load(&s)==gf::OptionSettingsLoad::Missing&&s.mono==1,"missing read publishes nothing");
    s.volume={0,2,4};s.rumble_mask=9;check(store.write(s),"native record writes");
    gf::OptionSettings loaded;check(store.load(&loaded)==gf::OptionSettingsLoad::Loaded&&s==loaded,"round-trip explicit settings fields");
    auto restored=s;gf::option_restore_values(restored);
    check(restored.mono==1&&restored.volume==std::array<gf::u8,3>{4,4,4}&&restored.rumble_mask==0,"restore changes non-default values while preserving SOUND");
    check(backend.slots[0]==std::vector<gf::u8>({1,2,3})&&backend.slots.size()==2,"gameplay slot0 isolated");
    const auto record=backend.slots[1];const auto before=loaded;
    for(std::size_t i=0;i<record.size();++i){auto bad=record;bad[i]^=1;backend.slots[1]=bad;
        check(store.load(&loaded)==gf::OptionSettingsLoad::Invalid&&loaded==before,"corrupt field/header/checksum has no partial output");}
    backend.slots[1]=record;
    auto invalid=s;invalid.volume[2]=5;const auto writes=backend.writes;
    check(!store.write(invalid)&&backend.writes==writes,"invalid state never reaches backend");
    backend.bad_read=true;check(store.load(&loaded)==gf::OptionSettingsLoad::Unavailable&&loaded==before,"I/O error not called missing");backend.bad_read=false;
    // Valid framing must not make an unknown schema/range/trailing payload valid.
    for(unsigned bad=0;bad<4;++bad){gf::SaveWriter writer;writer.u32_field(bad==0?0:0x47465354);writer.u32_field(bad==1?2:1);
        writer.u32_field(bad==2?256:0);for(unsigned i=0;i<3;++i)writer.u32_field(4);writer.u32_field(0);if(bad==3)writer.u32_field(0);
        gf::SaveEngine framed(backend);check(framed.write(1,writer),"owned malformed schema framed");
        check(store.load(&loaded)==gf::OptionSettingsLoad::Invalid&&loaded==before,"schema/range/length validation independent of checksum");}
    backend.slots.erase(1);gf::OptionMenu menu;check(menu.configure(3,1)&&!menu.configure(255,0),"repeat config validates");
    std::array<gf::GamePadState,4> pads{};pads[0].a=true;menu.begin(store,pads);
    check(menu.phase()==gf::OptionMenuPhase::Editing&&menu.row()==0&&backend.writes>=1,"entry does not save");
    check(!menu.update(store,pads).leave&&menu.row()==0,"held entry confirm consumed");pads={};menu.update(store,pads);
    auto tap=[&](gf::GamePadState pad,unsigned port=0){pads={};pads[port]=pad;const auto event=menu.update(store,pads);pads={};menu.update(store,pads);return event;};
    gf::GamePadState up,down,left,right,a,b;up.dpad_up=true;down.dpad_down=true;left.dpad_left=true;right.dpad_right=true;a.a=true;b.b=true;
    tap(up);check(menu.row()==9,"up wraps first to last");tap(down);check(menu.row()==0,"down wraps last to first");
    tap(right);check(menu.settings().mono==1,"right selects mono");tap(right);check(menu.settings().mono==1,"boolean clamps instead of wraps");
    auto diagonal=down;diagonal.dpad_left=true;tap(diagonal);check(menu.row()==0&&menu.settings().mono==1,"diagonals have no invented direction priority");
    auto opposite=left;opposite.dpad_right=true;tap(opposite);check(menu.settings().mono==1,"opposite directions cancel");
    tap(down);check(menu.row()==1,"next volume row");
    for(unsigned i=0;i<8;++i)tap(left);
    check(menu.settings().volume[0]==0,"volume clamps at zero");
    for(unsigned i=0;i<8;++i)tap(right);
    check(menu.settings().volume[0]==4,"volume clamps at four");
    tap(down);tap(down);tap(down);check(menu.row()==4,"rumble first row");tap(left,3);check(menu.settings().rumble_mask==1,"fourth input port selects first rumble ON");
    tap(right);check(menu.settings().rumble_mask==0,"right selects rumble OFF");
    pads[0]=down;menu.update(store,pads);const auto row=menu.row();
    menu.update(store,pads);menu.update(store,pads);menu.update(store,pads);check(menu.row()==row,"held repeat delay respected");
    menu.update(store,pads);check(menu.row()==row+1,"held repeat pulse");pads={};menu.update(store,pads);
    while(menu.row()!=9)tap(down);
    tap(left);tap(a);check(menu.settings().mono==1&&menu.settings().rumble_mask==0&&menu.settings().volume[0]==4,"restore resets represented values but preserves SOUND");
    tap(up);tap(a);check(menu.phase()==gf::OptionMenuPhase::CreditsUnported&&menu.row()==8,"credits explicitly unported, row retained");tap(b);check(menu.phase()==gf::OptionMenuPhase::Editing&&menu.row()==8,"credits notice B returns without save");
    const auto prior=backend.writes;auto both=a;both.b=true;tap(both);
    check(backend.writes==prior+1&&menu.phase()==gf::OptionMenuPhase::Saved,"B before A; one actual write even without edits");
    pads[2]=a;check(menu.update(store,pads).leave,"fresh confirm acknowledges real save");
    gf::OptionTransition transition;check(transition.configure({1,2,3,1})&&transition.begin(0),"camera flow owns motion");transition.update(0);
    check(transition.request_leave()&&transition.phase()==gf::OptionTransitionPhase::Leaving,"saved acknowledgement requests camera return");
    check(transition.update(0)&&!transition.active(),"camera returns after native acknowledgement");
    // Store failure stays in the UI; no success/leave until an explicit retry.
    pads={};menu.begin(store,pads);backend.bad_write=true;const auto failed=tap(b);
    check(failed.save_attempted&&menu.phase()==gf::OptionMenuPhase::SaveError&&!failed.leave,"failed write cannot produce success or leave");
    tap(b);check(menu.phase()==gf::OptionMenuPhase::Editing,"failed save can return to editing");tap(b);backend.bad_write=false;tap(a);
    check(menu.phase()==gf::OptionMenuPhase::Saved,"explicit retry may succeed");
    backend.slots[1]={9,9,9};menu.begin(store,{});const auto blocked=backend.writes;tap(b);tap(a);
    check(menu.phase()==gf::OptionMenuPhase::SaveError&&backend.writes==blocked&&backend.slots[1]==std::vector<gf::u8>({9,9,9}),"invalid existing native record is never silently overwritten");
    backend.bad_read=true;menu.begin(store,{});tap(b);check(menu.phase()==gf::OptionMenuPhase::SaveError&&backend.writes==blocked,"unreadable native record is not mistaken for missing");
    std::printf("{\"option_menu_checks\":%u,\"all_passed\":true}\n",checks);
}
